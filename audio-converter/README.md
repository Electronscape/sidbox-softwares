# SidBox Side Tools

Host-side tools that do not belong in `Core`.

## SidBox Audio (`.sba`)

`.sba` is a small, firmware-friendly audio container. The file format is always:

- 44100 Hz only
- 16-bit signed PCM only
- stereo only
- block based streaming
- lossless payloads

The C encoder accepts common WAV inputs and converts them to that standard SBA layout:

- PCM 8/16/24/32-bit
- IEEE float 32/64-bit
- mono, stereo, or wider WAVs
- arbitrary sample rates, resampled to 44100 Hz

Non-WAV inputs such as MP3, FLAC, and OGG are supported when `ffmpeg` is installed
and available on your `PATH`. The tool decodes them to a temporary WAV first, then
uses the normal SBA encoder path.

Version 1 uses one of two block methods:

- `0`: raw interleaved stereo PCM
- `1`: delta-coded signed samples with unsigned LEB128 varints

Each compressed block stores left and right channels separately. Every channel starts with its first signed 16-bit sample, then stores sample deltas using zig-zag + varint encoding. If compression is not smaller than raw PCM, the encoder stores that block as raw.

## C Encoder

Build:

```sh
cd sidetools
make
```

Encode a WAV:

```sh
./sba input.wav output.sba
```

Encode an MP3, using `ffmpeg`:

```sh
./sba input.mp3 output.sba
```

If `ffmpeg` is not on your `PATH`, pass it explicitly:

```sh
./sba -ffmpeg /path/to/ffmpeg input.mp3 output.sba
```

Encode and peak-normalize first:

```sh
./sba -ag input.wav output.sba
./sba -ag input.mp3 output.sba
```

On Windows with MinGW:

```bat
mingw32-make
sba.exe input.wav output.sba
sba.exe input.mp3 output.sba
sba.exe -ffmpeg C:\tools\ffmpeg\bin\ffmpeg.exe input.mp3 output.sba
```

Windows users can either add `ffmpeg.exe` to `PATH`, put it next to `sba.exe`
and run from that folder, or use `-ffmpeg` with the full path.

## Python Reference Tool

From the repo root:

Encode audio:

```sh
python3 sidetools/sba_tool.py encode input.wav output.sba
python3 sidetools/sba_tool.py encode input.mp3 output.sba
python3 sidetools/sba_tool.py encode --auto-gain input.flac output.sba
```

Use a specific `ffmpeg` executable:

```sh
python3 sidetools/sba_tool.py encode --ffmpeg /path/to/ffmpeg input.ogg output.sba
```

Inspect an SBA:

```sh
python3 sidetools/sba_tool.py info output.sba
```

Decode back to WAV:

```sh
python3 sidetools/sba_tool.py decode output.sba roundtrip.wav
```

Verify lossless round-trip against the converted input PCM:

```sh
python3 sidetools/sba_tool.py verify input.wav output.sba
python3 sidetools/sba_tool.py verify input.mp3 output.sba
```

This script uses only the Python standard library for SBA read/write. Non-standard
WAVs and non-WAV inputs need `ffmpeg`, matching the C encoder behaviour.
