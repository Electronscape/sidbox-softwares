#!/usr/bin/env python3
"""
SidBox Audio tool.

Creates and inspects .sba files from 44.1 kHz, 16-bit, stereo WAV files.
The codec is intentionally small so the firmware decoder can be simple later.
"""

from __future__ import annotations

import argparse
import binascii
import shutil
import pathlib
import struct
import subprocess
import sys
import tempfile
import wave


MAGIC = b"SBA1"
VERSION = 1
HEADER_SIZE = 64
SAMPLE_RATE = 44100
CHANNELS = 2
BITS_PER_SAMPLE = 16
BYTES_PER_FRAME = 4
METHOD_RAW = 0
METHOD_DELTA_VARINT = 1
DEFAULT_BLOCK_FRAMES = 2048

HEADER_STRUCT = struct.Struct("<4sHHIHHIIQIII4s16s")
BLOCK_STRUCT = struct.Struct("<IIHBBI")


class SBAError(Exception):
    pass


def zigzag_encode(value: int) -> int:
    return (value << 1) ^ (value >> 31)


def zigzag_decode(value: int) -> int:
    return (value >> 1) ^ -(value & 1)


def write_uvarint(value: int, out: bytearray) -> None:
    while value >= 0x80:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)


def read_uvarint(data: bytes, pos: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while True:
        if pos >= len(data):
            raise SBAError("truncated varint")
        b = data[pos]
        pos += 1
        value |= (b & 0x7F) << shift
        if not (b & 0x80):
            return value, pos
        shift += 7
        if shift > 35:
            raise SBAError("varint too large")


def read_wav_pcm(path: pathlib.Path) -> bytes:
    with wave.open(str(path), "rb") as wav:
        if wav.getframerate() != SAMPLE_RATE:
            raise SBAError(f"expected {SAMPLE_RATE} Hz WAV")
        if wav.getnchannels() != CHANNELS:
            raise SBAError("expected stereo WAV")
        if wav.getsampwidth() != 2:
            raise SBAError("expected 16-bit WAV")
        return wav.readframes(wav.getnframes())


def read_wav_pcm_if_standard(path: pathlib.Path) -> bytes | None:
    try:
        return read_wav_pcm(path)
    except (wave.Error, SBAError):
        return None


def ffmpeg_to_standard_wav(input_path: pathlib.Path, ffmpeg_path: str) -> pathlib.Path:
    tmp = tempfile.NamedTemporaryFile(prefix="sba_", suffix=".wav", delete=False)
    tmp_path = pathlib.Path(tmp.name)
    tmp.close()

    cmd = [
        ffmpeg_path,
        "-v", "error",
        "-y",
        "-i", str(input_path),
        "-ac", str(CHANNELS),
        "-ar", str(SAMPLE_RATE),
        "-sample_fmt", "s16",
        "-f", "wav",
        str(tmp_path),
    ]

    try:
        subprocess.run(cmd, check=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        try:
            tmp_path.unlink()
        except OSError:
            pass
        raise SBAError(f"ffmpeg failed decoding {input_path}: {exc}") from exc

    return tmp_path


def read_input_pcm(path: pathlib.Path, ffmpeg_path: str) -> bytes:
    pcm = read_wav_pcm_if_standard(path)
    if pcm is not None:
        return pcm

    if not shutil.which(ffmpeg_path) and not pathlib.Path(ffmpeg_path).exists():
        raise SBAError("input is not a standard SBA WAV and ffmpeg was not found")

    print(f"decoding {path} with ffmpeg...")
    tmp_path = ffmpeg_to_standard_wav(path, ffmpeg_path)
    try:
        return read_wav_pcm(tmp_path)
    finally:
        try:
            tmp_path.unlink()
        except OSError:
            pass


def write_wav_pcm(path: pathlib.Path, pcm: bytes) -> None:
    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(CHANNELS)
        wav.setsampwidth(2)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(pcm)


def split_channels(block: bytes) -> tuple[list[int], list[int]]:
    frames = len(block) // BYTES_PER_FRAME
    samples = struct.unpack("<" + "h" * frames * CHANNELS, block)
    left = list(samples[0::2])
    right = list(samples[1::2])
    return left, right


def encode_channel(samples: list[int], out: bytearray) -> None:
    if not samples:
        return
    out.extend(struct.pack("<h", samples[0]))
    prev = samples[0]
    for sample in samples[1:]:
        write_uvarint(zigzag_encode(sample - prev), out)
        prev = sample


def decode_channel(data: bytes, pos: int, frames: int, out: list[int]) -> int:
    if frames == 0:
        return pos
    if pos + 2 > len(data):
        raise SBAError("truncated channel seed")
    prev = struct.unpack_from("<h", data, pos)[0]
    pos += 2
    out.append(prev)
    for _ in range(1, frames):
        zz, pos = read_uvarint(data, pos)
        prev += zigzag_decode(zz)
        if prev < -32768 or prev > 32767:
            raise SBAError("decoded sample out of int16 range")
        out.append(prev)
    return pos


def encode_block(block: bytes) -> tuple[int, bytes]:
    left, right = split_channels(block)
    encoded = bytearray()
    encode_channel(left, encoded)
    encode_channel(right, encoded)
    payload = bytes(encoded)
    if len(payload) < len(block):
        return METHOD_DELTA_VARINT, payload
    return METHOD_RAW, block


def pcm_peak(pcm: bytes) -> int:
    peak = 0
    for (sample,) in struct.iter_unpack("<h", pcm):
        mag = -sample if sample < 0 else sample
        if mag > peak:
            peak = mag
    return peak


def apply_auto_gain(pcm: bytes) -> tuple[bytes, int, float]:
    peak = pcm_peak(pcm)
    if peak <= 0 or peak >= 32767:
        return pcm, peak, 1.0

    gain = 32767.0 / float(peak)
    out = bytearray(len(pcm))
    for i, (sample,) in enumerate(struct.iter_unpack("<h", pcm)):
        scaled = int(sample * gain + (0.5 if sample >= 0 else -0.5))
        if scaled < -32768:
            scaled = -32768
        if scaled > 32767:
            scaled = 32767
        struct.pack_into("<h", out, i * 2, scaled)
    return bytes(out), peak, gain


def decode_block(method: int, payload: bytes, frames: int) -> bytes:
    if method == METHOD_RAW:
        expected = frames * BYTES_PER_FRAME
        if len(payload) != expected:
            raise SBAError("raw block size mismatch")
        return payload
    if method != METHOD_DELTA_VARINT:
        raise SBAError(f"unknown block method {method}")

    left: list[int] = []
    right: list[int] = []
    pos = decode_channel(payload, 0, frames, left)
    pos = decode_channel(payload, pos, frames, right)
    if pos != len(payload):
        raise SBAError("extra bytes after compressed block")

    out = bytearray(frames * BYTES_PER_FRAME)
    for i, (l, r) in enumerate(zip(left, right)):
        struct.pack_into("<hh", out, i * BYTES_PER_FRAME, l, r)
    return bytes(out)


def make_header(block_count: int, total_frames: int, block_frames: int, data_offset: int) -> bytes:
    zero_crc = b"\0\0\0\0"
    header = HEADER_STRUCT.pack(
        MAGIC,
        HEADER_SIZE,
        VERSION,
        SAMPLE_RATE,
        CHANNELS,
        BITS_PER_SAMPLE,
        block_frames,
        block_count,
        total_frames,
        HEADER_SIZE,
        data_offset,
        0,
        zero_crc,
        b"\0" * 16,
    )
    crc = binascii.crc32(header[:44]) & 0xFFFFFFFF
    return header[:44] + struct.pack("<I", crc) + header[48:]


def parse_header(data: bytes) -> dict[str, int]:
    if len(data) < HEADER_SIZE:
        raise SBAError("file too small")
    fields = HEADER_STRUCT.unpack_from(data, 0)
    magic, header_size, version, rate, channels, bits, block_frames, block_count, total_frames, table_offset, data_offset, flags, crc_bytes, _reserved = fields
    if magic != MAGIC:
        raise SBAError("not an SBA1 file")
    if header_size != HEADER_SIZE or version != VERSION:
        raise SBAError("unsupported SBA header")
    if rate != SAMPLE_RATE or channels != CHANNELS or bits != BITS_PER_SAMPLE:
        raise SBAError("unsupported audio format in SBA")
    stored_crc = struct.unpack("<I", crc_bytes)[0]
    check = data[:44] + b"\0\0\0\0" + data[48:HEADER_SIZE]
    actual_crc = binascii.crc32(check[:44]) & 0xFFFFFFFF
    if stored_crc != actual_crc:
        raise SBAError("header CRC mismatch")
    return {
        "block_frames": block_frames,
        "block_count": block_count,
        "total_frames": total_frames,
        "table_offset": table_offset,
        "data_offset": data_offset,
        "flags": flags,
    }


def encode_file(input_path: pathlib.Path,
                sba_path: pathlib.Path,
                block_frames: int,
                auto_gain: bool,
                ffmpeg_path: str) -> None:
    pcm = read_input_pcm(input_path, ffmpeg_path)
    peak = 0
    gain = 1.0

    if auto_gain:
        pcm, peak, gain = apply_auto_gain(pcm)

    total_frames = len(pcm) // BYTES_PER_FRAME
    blocks = []
    payloads = []
    payload_offset = 0

    for start_frame in range(0, total_frames, block_frames):
        frames = min(block_frames, total_frames - start_frame)
        raw = pcm[start_frame * BYTES_PER_FRAME : (start_frame + frames) * BYTES_PER_FRAME]
        method, payload = encode_block(raw)
        crc = binascii.crc32(raw) & 0xFFFFFFFF
        blocks.append((payload_offset, len(payload), frames, method, 0, crc))
        payloads.append(payload)
        payload_offset += len(payload)

    data_offset = HEADER_SIZE + len(blocks) * BLOCK_STRUCT.size
    header = make_header(len(blocks), total_frames, block_frames, data_offset)

    with sba_path.open("wb") as f:
        f.write(header)
        for entry in blocks:
            f.write(BLOCK_STRUCT.pack(*entry))
        for payload in payloads:
            f.write(payload)

    raw_size = len(pcm)
    out_size = sba_path.stat().st_size
    pct = (out_size * 100.0 / raw_size) if raw_size else 0.0
    print(f"wrote {sba_path} ({out_size} bytes, {pct:.1f}% of raw PCM)")
    print(f"input -> output: {SAMPLE_RATE} Hz, 16-bit, stereo")
    if auto_gain:
        if peak == 0:
            print("auto gain: silent input, gain left at 1.000x")
        else:
            print(f"auto gain: peak {peak} -> gain {gain:.3f}x")


def read_sba(path: pathlib.Path) -> tuple[dict[str, int], list[tuple[int, int, int, int, int]], bytes]:
    data = path.read_bytes()
    header = parse_header(data)
    entries = []
    pos = header["table_offset"]
    for _ in range(header["block_count"]):
        if pos + BLOCK_STRUCT.size > len(data):
            raise SBAError("truncated block table")
        payload_offset, payload_size, frames, method, _reserved, crc = BLOCK_STRUCT.unpack_from(data, pos)
        entries.append((payload_offset, payload_size, frames, method, crc))
        pos += BLOCK_STRUCT.size
    return header, entries, data


def decode_file(sba_path: pathlib.Path, wav_path: pathlib.Path) -> None:
    header, entries, data = read_sba(sba_path)
    out = bytearray()
    for payload_offset, payload_size, frames, method, expected_crc in entries:
        start = header["data_offset"] + payload_offset
        end = start + payload_size
        if end > len(data):
            raise SBAError("payload extends past EOF")
        block = decode_block(method, data[start:end], frames)
        actual_crc = binascii.crc32(block) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            raise SBAError("decoded block CRC mismatch")
        out.extend(block)
    if len(out) // BYTES_PER_FRAME != header["total_frames"]:
        raise SBAError("decoded frame count mismatch")
    write_wav_pcm(wav_path, bytes(out))
    print(f"wrote {wav_path}")


def info_file(sba_path: pathlib.Path) -> None:
    header, entries, data = read_sba(sba_path)
    raw_bytes = header["total_frames"] * BYTES_PER_FRAME
    compressed_blocks = sum(1 for e in entries if e[3] == METHOD_DELTA_VARINT)
    payload_bytes = sum(e[1] for e in entries)
    pct = (len(data) * 100.0 / raw_bytes) if raw_bytes else 0.0
    print(f"file: {sba_path}")
    print(f"frames: {header['total_frames']}")
    print(f"blocks: {header['block_count']} ({compressed_blocks} compressed)")
    print(f"block frames: {header['block_frames']}")
    print(f"payload bytes: {payload_bytes}")
    print(f"file bytes: {len(data)} ({pct:.1f}% of raw PCM)")


def verify_file(input_path: pathlib.Path, sba_path: pathlib.Path, ffmpeg_path: str) -> None:
    original = read_input_pcm(input_path, ffmpeg_path)
    header, entries, data = read_sba(sba_path)
    decoded = bytearray()
    for payload_offset, payload_size, frames, method, _expected_crc in entries:
        start = header["data_offset"] + payload_offset
        decoded.extend(decode_block(method, data[start:start + payload_size], frames))
    if bytes(decoded) != original:
        raise SBAError("verify failed: decoded PCM differs")
    print("verify ok")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="SidBox Audio .sba tool")
    sub = parser.add_subparsers(dest="cmd", required=True)

    enc = sub.add_parser("encode", help="encode audio to SBA")
    enc.add_argument("input_audio", type=pathlib.Path)
    enc.add_argument("output_sba", type=pathlib.Path)
    enc.add_argument("--block-frames", type=int, default=DEFAULT_BLOCK_FRAMES)
    enc.add_argument("-ag", "--auto-gain", action="store_true",
                     help="peak normalize converted audio before encoding")
    enc.add_argument("-ffmpeg", "--ffmpeg", default="ffmpeg",
                     help="ffmpeg executable for non-standard WAV/MP3/FLAC/OGG input")

    dec = sub.add_parser("decode", help="decode SBA to WAV")
    dec.add_argument("input_sba", type=pathlib.Path)
    dec.add_argument("output_wav", type=pathlib.Path)

    inf = sub.add_parser("info", help="inspect SBA")
    inf.add_argument("input_sba", type=pathlib.Path)

    ver = sub.add_parser("verify", help="verify converted input audio equals decoded SBA")
    ver.add_argument("input_audio", type=pathlib.Path)
    ver.add_argument("input_sba", type=pathlib.Path)
    ver.add_argument("-ffmpeg", "--ffmpeg", default="ffmpeg",
                     help="ffmpeg executable for non-standard WAV/MP3/FLAC/OGG input")

    args = parser.parse_args(argv)

    try:
        if args.cmd == "encode":
            if args.block_frames <= 0 or args.block_frames > 65535:
                raise SBAError("block frames must be 1..65535")
            encode_file(args.input_audio, args.output_sba, args.block_frames,
                        args.auto_gain, args.ffmpeg)
        elif args.cmd == "decode":
            decode_file(args.input_sba, args.output_wav)
        elif args.cmd == "info":
            info_file(args.input_sba)
        elif args.cmd == "verify":
            verify_file(args.input_audio, args.input_sba, args.ffmpeg)
    except (OSError, wave.Error, SBAError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
