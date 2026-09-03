#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#define SBA_GETPID _getpid
#else
#include <unistd.h>
#define SBA_GETPID getpid
#endif

#define SBA_MAGIC "SBA1"
#define SBA_VERSION 1u
#define SBA_HEADER_SIZE 64u
#define SBA_SAMPLE_RATE 44100u
#define SBA_CHANNELS 2u
#define SBA_BITS_PER_SAMPLE 16u
#define SBA_BYTES_PER_FRAME 4u
#define SBA_BLOCK_FRAMES 2048u

#define SBA_METHOD_RAW 0u
#define SBA_METHOD_DELTA_VARINT 1u

typedef struct {
    uint32_t payload_offset;
    uint32_t payload_size;
    uint16_t frames;
    uint8_t method;
    uint32_t decoded_crc32;
} BlockEntry;

typedef struct {
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t block_align;
    uint32_t data_offset;
    uint32_t data_size;
} WavInfo;

typedef struct {
    int auto_gain;
    const char *ffmpeg_path;
} EncodeOptions;

static uint32_t crc32_table[256];

static void crc32_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (uint32_t j = 0; j < 8; j++) {
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
}

static uint32_t crc32_bytes(const uint8_t *data, size_t len) {
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        c = crc32_table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

static uint16_t rd16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint64_t rd64le(const uint8_t *p) {
    uint64_t v = 0;
    for (uint32_t i = 0; i < 8; i++) {
        v |= (uint64_t)p[i] << (i * 8u);
    }
    return v;
}

static int16_t rd_i16le(const uint8_t *p) {
    return (int16_t)rd16le(p);
}

static void wr16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)(v >> 8);
}

static void wr32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static void wr64le(uint8_t *p, uint64_t v) {
    for (uint32_t i = 0; i < 8; i++) {
        p[i] = (uint8_t)((v >> (i * 8u)) & 0xFFu);
    }
}

static uint32_t zigzag_encode(int32_t v) {
    return ((uint32_t)v << 1) ^ (uint32_t)(v >> 31);
}

static size_t put_uvarint(uint32_t value, uint8_t *out) {
    size_t n = 0;
    while (value >= 0x80u) {
        out[n++] = (uint8_t)((value & 0x7Fu) | 0x80u);
        value >>= 7;
    }
    out[n++] = (uint8_t)value;
    return n;
}

static int read_exact(FILE *f, void *buf, size_t len) {
    return fread(buf, 1, len, f) == len;
}

static int parse_wav(FILE *f, WavInfo *wi) {
    uint8_t hdr[12];
    memset(wi, 0, sizeof(*wi));

    if (!read_exact(f, hdr, sizeof(hdr))) {
        fprintf(stderr, "error: WAV header too short\n");
        return 0;
    }
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        fprintf(stderr, "error: expected RIFF/WAVE input\n");
        return 0;
    }

    int got_fmt = 0;
    int got_data = 0;
    for (;;) {
        uint8_t chdr[8];
        long data_pos;
        uint32_t size;

        if (fread(chdr, 1, sizeof(chdr), f) != sizeof(chdr)) break;
        size = rd32le(chdr + 4);
        data_pos = ftell(f);
        if (data_pos < 0) {
            fprintf(stderr, "error: ftell failed\n");
            return 0;
        }

        if (memcmp(chdr, "fmt ", 4) == 0) {
            uint8_t *fmt = (uint8_t *)malloc(size);
            if (size < 16u || !fmt || !read_exact(f, fmt, size)) {
                fprintf(stderr, "error: invalid fmt chunk\n");
                free(fmt);
                return 0;
            }
            wi->audio_format = rd16le(fmt + 0);
            wi->channels = rd16le(fmt + 2);
            wi->sample_rate = rd32le(fmt + 4);
            wi->block_align = rd16le(fmt + 12);
            wi->bits_per_sample = rd16le(fmt + 14);

            if (wi->audio_format == 0xFFFEu && size >= 40u) {
                uint16_t valid_bits = rd16le(fmt + 18);
                uint8_t *guid = fmt + 24;
                static const uint8_t pcm_guid[16] = {
                    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
                };
                static const uint8_t float_guid[16] = {
                    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00,
                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71
                };
                if (memcmp(guid, pcm_guid, sizeof(pcm_guid)) == 0) {
                    wi->audio_format = 1u;
                } else if (memcmp(guid, float_guid, sizeof(float_guid)) == 0) {
                    wi->audio_format = 3u;
                }
                if (valid_bits && valid_bits < wi->bits_per_sample) {
                    wi->bits_per_sample = valid_bits;
                }
            }

            free(fmt);
            got_fmt = 1;
        } else if (memcmp(chdr, "data", 4) == 0) {
            wi->data_offset = (uint32_t)data_pos;
            wi->data_size = size;
            got_data = 1;
        }

        if (fseek(f, data_pos + (long)size + (long)(size & 1u), SEEK_SET) != 0) {
            fprintf(stderr, "error: failed to skip WAV chunk\n");
            return 0;
        }
    }

    if (!got_fmt || !got_data) {
        fprintf(stderr, "error: WAV missing fmt or data chunk\n");
        return 0;
    }
    if (wi->channels == 0 || wi->sample_rate == 0 || wi->block_align == 0) {
        fprintf(stderr, "error: WAV has invalid channel/rate/block alignment\n");
        return 0;
    }
    if (wi->audio_format != 1u && wi->audio_format != 3u) {
        fprintf(stderr, "error: only PCM and IEEE float WAV are supported\n");
        return 0;
    }
    if (wi->audio_format == 1u &&
        wi->bits_per_sample != 8u &&
        wi->bits_per_sample != 16u &&
        wi->bits_per_sample != 24u &&
        wi->bits_per_sample != 32u) {
        fprintf(stderr, "error: unsupported PCM bit depth %u\n", wi->bits_per_sample);
        return 0;
    }
    if (wi->audio_format == 3u &&
        wi->bits_per_sample != 32u &&
        wi->bits_per_sample != 64u) {
        fprintf(stderr, "error: unsupported float bit depth %u\n", wi->bits_per_sample);
        return 0;
    }
    if ((wi->data_size % wi->block_align) != 0) {
        fprintf(stderr, "error: WAV data chunk is not whole source frames\n");
        return 0;
    }

    return 1;
}

static int32_t sign_extend(uint32_t v, uint32_t bits) {
    uint32_t sign = 1u << (bits - 1u);
    return (int32_t)((v ^ sign) - sign);
}

static double read_source_sample(const WavInfo *wi, const uint8_t *frame, uint32_t channel) {
    uint32_t bytes_per_sample = wi->bits_per_sample / 8u;
    const uint8_t *p = frame + channel * bytes_per_sample;

    if (wi->audio_format == 3u) {
        if (wi->bits_per_sample == 32u) {
            float v;
            uint32_t raw = rd32le(p);
            memcpy(&v, &raw, sizeof(v));
            if (v < -1.0f) v = -1.0f;
            if (v > 1.0f) v = 1.0f;
            return (double)v;
        } else {
            double v;
            uint64_t raw = rd64le(p);
            memcpy(&v, &raw, sizeof(v));
            if (v < -1.0) v = -1.0;
            if (v > 1.0) v = 1.0;
            return v;
        }
    }

    if (wi->bits_per_sample == 8u) {
        return ((double)p[0] - 128.0) / 128.0;
    }
    if (wi->bits_per_sample == 16u) {
        return (double)rd_i16le(p) / 32768.0;
    }
    if (wi->bits_per_sample == 24u) {
        uint32_t raw = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
        return (double)sign_extend(raw, 24u) / 8388608.0;
    }

    return (double)(int32_t)rd32le(p) / 2147483648.0;
}

static int16_t double_to_i16(double v) {
    int32_t s;
    if (v < -1.0) v = -1.0;
    if (v > 1.0) v = 1.0;
    s = (int32_t)(v * 32767.0 + (v >= 0.0 ? 0.5 : -0.5));
    if (s < -32768) s = -32768;
    if (s > 32767) s = 32767;
    return (int16_t)s;
}

static double source_channel_at(const WavInfo *wi,
                                const uint8_t *data,
                                uint64_t src_frames,
                                uint64_t frame_index,
                                uint32_t out_channel) {
    const uint8_t *frame;

    if (frame_index >= src_frames) frame_index = src_frames - 1u;
    frame = data + frame_index * wi->block_align;

    if (wi->channels == 1u) {
        return read_source_sample(wi, frame, 0);
    }

    if (out_channel == 0u) {
        return read_source_sample(wi, frame, 0);
    }
    return read_source_sample(wi, frame, 1);
}

static void convert_output_block(const WavInfo *wi,
                                 const uint8_t *src_data,
                                 uint64_t src_frames,
                                 uint64_t out_start_frame,
                                 uint32_t out_frames,
                                 uint8_t *out) {
    const uint64_t den = SBA_SAMPLE_RATE;

    for (uint32_t i = 0; i < out_frames; i++) {
        uint64_t out_frame = out_start_frame + i;
        uint64_t num = out_frame * (uint64_t)wi->sample_rate;
        uint64_t i0 = num / den;
        uint64_t rem = num % den;
        uint64_t i1 = (i0 + 1u < src_frames) ? (i0 + 1u) : i0;
        double frac = (double)rem / (double)den;

        for (uint32_t ch = 0; ch < SBA_CHANNELS; ch++) {
            double a = source_channel_at(wi, src_data, src_frames, i0, ch);
            double b = source_channel_at(wi, src_data, src_frames, i1, ch);
            int16_t sample = double_to_i16(a + (b - a) * frac);
            wr16le(out + ((size_t)i * SBA_BYTES_PER_FRAME) + ch * 2u, (uint16_t)sample);
        }
    }
}

static uint32_t pcm_block_peak(const uint8_t *raw, uint32_t frames) {
    uint32_t peak = 0;
    uint32_t samples = frames * SBA_CHANNELS;

    for (uint32_t i = 0; i < samples; i++) {
        int32_t v = rd_i16le(raw + (size_t)i * 2u);
        uint32_t mag = (v < 0) ? (uint32_t)(-v) : (uint32_t)v;
        if (mag > peak) peak = mag;
    }

    return peak;
}

static uint32_t measure_converted_peak(const WavInfo *wi,
                                       const uint8_t *src_data,
                                       uint64_t src_frames,
                                       uint64_t total_frames,
                                       uint8_t *raw) {
    uint32_t peak = 0;

    for (uint64_t frame = 0; frame < total_frames; frame += SBA_BLOCK_FRAMES) {
        uint32_t frames = (uint32_t)((total_frames - frame) > SBA_BLOCK_FRAMES
                                         ? SBA_BLOCK_FRAMES
                                         : (total_frames - frame));
        uint32_t block_peak;

        convert_output_block(wi, src_data, src_frames, frame, frames, raw);
        block_peak = pcm_block_peak(raw, frames);
        if (block_peak > peak) peak = block_peak;
    }

    return peak;
}

static void apply_gain_to_block(uint8_t *raw, uint32_t frames, double gain) {
    uint32_t samples = frames * SBA_CHANNELS;

    if (gain <= 1.000001) return;

    for (uint32_t i = 0; i < samples; i++) {
        int32_t v = rd_i16le(raw + (size_t)i * 2u);
        double scaled = (double)v * gain;
        int32_t out = (int32_t)(scaled + (scaled >= 0.0 ? 0.5 : -0.5));

        if (out < -32768) out = -32768;
        if (out > 32767) out = 32767;
        wr16le(raw + (size_t)i * 2u, (uint16_t)(int16_t)out);
    }
}

static size_t encode_channel_delta(const uint8_t *raw, uint32_t frames, uint32_t channel, uint8_t *out) {
    size_t pos = 0;
    int16_t prev = rd_i16le(raw + channel * 2u);
    wr16le(out + pos, (uint16_t)prev);
    pos += 2;

    for (uint32_t i = 1; i < frames; i++) {
        int16_t sample = rd_i16le(raw + i * SBA_BYTES_PER_FRAME + channel * 2u);
        int32_t delta = (int32_t)sample - (int32_t)prev;
        pos += put_uvarint(zigzag_encode(delta), out + pos);
        prev = sample;
    }
    return pos;
}

static int encode_block(FILE *payload_file,
                        const uint8_t *raw,
                        uint32_t frames,
                        BlockEntry *entry,
                        uint32_t *payload_offset) {
    size_t raw_bytes = (size_t)frames * SBA_BYTES_PER_FRAME;
    size_t enc_cap = 4u + ((size_t)frames * SBA_CHANNELS * 5u);
    uint8_t *enc = (uint8_t *)malloc(enc_cap);
    size_t enc_len = 0;

    if (!enc) {
        fprintf(stderr, "error: out of memory\n");
        return 0;
    }

    enc_len += encode_channel_delta(raw, frames, 0, enc + enc_len);
    enc_len += encode_channel_delta(raw, frames, 1, enc + enc_len);

    entry->payload_offset = *payload_offset;
    entry->decoded_crc32 = crc32_bytes(raw, raw_bytes);
    entry->frames = (uint16_t)frames;

    if (enc_len < raw_bytes) {
        entry->method = SBA_METHOD_DELTA_VARINT;
        entry->payload_size = (uint32_t)enc_len;
        if (fwrite(enc, 1, enc_len, payload_file) != enc_len) {
            fprintf(stderr, "error: failed writing temp payload\n");
            free(enc);
            return 0;
        }
    } else {
        entry->method = SBA_METHOD_RAW;
        entry->payload_size = (uint32_t)raw_bytes;
        if (fwrite(raw, 1, raw_bytes, payload_file) != raw_bytes) {
            fprintf(stderr, "error: failed writing temp payload\n");
            free(enc);
            return 0;
        }
    }

    if (UINT32_MAX - *payload_offset < entry->payload_size) {
        fprintf(stderr, "error: SBA payload is too large for v1\n");
        free(enc);
        return 0;
    }
    *payload_offset += entry->payload_size;
    free(enc);
    return 1;
}

static int append_entry(BlockEntry **entries, uint32_t *count, uint32_t *cap, const BlockEntry *entry) {
    if (*count == *cap) {
        uint32_t new_cap = *cap ? (*cap * 2u) : 64u;
        BlockEntry *new_entries = (BlockEntry *)realloc(*entries, (size_t)new_cap * sizeof(**entries));
        if (!new_entries) {
            fprintf(stderr, "error: out of memory\n");
            return 0;
        }
        *entries = new_entries;
        *cap = new_cap;
    }
    (*entries)[(*count)++] = *entry;
    return 1;
}

static void make_header(uint8_t header[SBA_HEADER_SIZE],
                        uint32_t block_count,
                        uint64_t total_frames,
                        uint32_t data_offset) {
    memset(header, 0, SBA_HEADER_SIZE);
    memcpy(header + 0, SBA_MAGIC, 4);
    wr16le(header + 4, SBA_HEADER_SIZE);
    wr16le(header + 6, SBA_VERSION);
    wr32le(header + 8, SBA_SAMPLE_RATE);
    wr16le(header + 12, SBA_CHANNELS);
    wr16le(header + 14, SBA_BITS_PER_SAMPLE);
    wr32le(header + 16, SBA_BLOCK_FRAMES);
    wr32le(header + 20, block_count);
    wr64le(header + 24, total_frames);
    wr32le(header + 32, SBA_HEADER_SIZE);
    wr32le(header + 36, data_offset);
    wr32le(header + 40, 0);
    wr32le(header + 44, crc32_bytes(header, 44));
}

static int write_block_entry(FILE *out, const BlockEntry *entry) {
    uint8_t b[16];
    wr32le(b + 0, entry->payload_offset);
    wr32le(b + 4, entry->payload_size);
    wr16le(b + 8, entry->frames);
    b[10] = entry->method;
    b[11] = 0;
    wr32le(b + 12, entry->decoded_crc32);
    return fwrite(b, 1, sizeof(b), out) == sizeof(b);
}

static int copy_payload(FILE *dst, FILE *src) {
    uint8_t buf[32768];
    size_t n;

    if (fseek(src, 0, SEEK_SET) != 0) {
        fprintf(stderr, "error: failed to rewind temp payload\n");
        return 0;
    }

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fprintf(stderr, "error: failed writing output payload\n");
            return 0;
        }
    }

    if (ferror(src)) {
        fprintf(stderr, "error: failed reading temp payload\n");
        return 0;
    }
    return 1;
}

static int encode_wav_file(const char *wav_path, const char *sba_path, const EncodeOptions *opts) {
    FILE *wav = fopen(wav_path, "rb");
    FILE *payload = NULL;
    FILE *out = NULL;
    WavInfo wi;
    BlockEntry *entries = NULL;
    uint32_t entry_count = 0;
    uint32_t entry_cap = 0;
    uint32_t payload_offset = 0;
    uint64_t total_frames;
    uint64_t src_frames;
    uint8_t *src_data = NULL;
    uint8_t *raw = NULL;
    uint32_t peak = 0;
    double gain = 1.0;
    int ok = 0;

    if (!wav) {
        fprintf(stderr, "error: cannot open %s: %s\n", wav_path, strerror(errno));
        return 0;
    }

    if (!parse_wav(wav, &wi)) goto done;
    src_frames = wi.data_size / wi.block_align;
    if (src_frames == 0) {
        fprintf(stderr, "error: WAV has no samples\n");
        goto done;
    }

    total_frames = ((src_frames * (uint64_t)SBA_SAMPLE_RATE) + wi.sample_rate - 1u) / wi.sample_rate;
    if (total_frames > UINT32_MAX) {
        fprintf(stderr, "error: converted audio is too long for this SBA tool\n");
        goto done;
    }

    src_data = (uint8_t *)malloc(wi.data_size);
    if (!src_data) {
        fprintf(stderr, "error: out of memory loading WAV data\n");
        goto done;
    }

    if (fseek(wav, (long)wi.data_offset, SEEK_SET) != 0) {
        fprintf(stderr, "error: failed to seek to WAV data\n");
        goto done;
    }
    if (!read_exact(wav, src_data, wi.data_size)) {
        fprintf(stderr, "error: failed reading WAV samples\n");
        goto done;
    }

    payload = tmpfile();
    if (!payload) {
        fprintf(stderr, "error: cannot create temp payload file: %s\n", strerror(errno));
        goto done;
    }

    raw = (uint8_t *)malloc((size_t)SBA_BLOCK_FRAMES * SBA_BYTES_PER_FRAME);
    if (!raw) {
        fprintf(stderr, "error: out of memory\n");
        goto done;
    }

    if (opts && opts->auto_gain) {
        peak = measure_converted_peak(&wi, src_data, src_frames, total_frames, raw);
        if (peak > 0 && peak < 32767u) {
            gain = 32767.0 / (double)peak;
        }
    }

    for (uint64_t frame = 0; frame < total_frames; frame += SBA_BLOCK_FRAMES) {
        uint32_t frames = (uint32_t)((total_frames - frame) > SBA_BLOCK_FRAMES
                                         ? SBA_BLOCK_FRAMES
                                         : (total_frames - frame));
        BlockEntry entry;

        convert_output_block(&wi, src_data, src_frames, frame, frames, raw);
        apply_gain_to_block(raw, frames, gain);
        if (!encode_block(payload, raw, frames, &entry, &payload_offset)) goto done;
        if (!append_entry(&entries, &entry_count, &entry_cap, &entry)) goto done;
    }

    out = fopen(sba_path, "wb");
    if (!out) {
        fprintf(stderr, "error: cannot create %s: %s\n", sba_path, strerror(errno));
        goto done;
    }

    {
        uint8_t header[SBA_HEADER_SIZE];
        uint32_t data_offset = SBA_HEADER_SIZE + entry_count * 16u;
        make_header(header, entry_count, total_frames, data_offset);
        if (fwrite(header, 1, SBA_HEADER_SIZE, out) != SBA_HEADER_SIZE) {
            fprintf(stderr, "error: failed writing SBA header\n");
            goto done;
        }
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        if (!write_block_entry(out, &entries[i])) {
            fprintf(stderr, "error: failed writing SBA block table\n");
            goto done;
        }
    }

    if (!copy_payload(out, payload)) goto done;

    printf("wrote %s (%u blocks, %u payload bytes)\n", sba_path, entry_count, payload_offset);
    printf("input: %u Hz, %u-bit, %u ch -> output: 44100 Hz, 16-bit, stereo\n",
           wi.sample_rate, wi.bits_per_sample, wi.channels);
    if (opts && opts->auto_gain) {
        if (peak == 0) {
            printf("auto gain: silent input, gain left at 1.000x\n");
        } else {
            printf("auto gain: peak %u -> gain %.3fx\n", peak, gain);
        }
    }
    ok = 1;

done:
    free(src_data);
    free(raw);
    free(entries);
    if (out) fclose(out);
    if (payload) fclose(payload);
    fclose(wav);
    return ok;
}

static int has_wav_ext(const char *path) {
    size_t n;
    const char *ext;

    if (!path) return 0;
    n = strlen(path);
    if (n < 4u) return 0;
    ext = path + n - 4u;
    return (ext[0] == '.') &&
           (ext[1] == 'w' || ext[1] == 'W') &&
           (ext[2] == 'a' || ext[2] == 'A') &&
           (ext[3] == 'v' || ext[3] == 'V');
}

static char *dup_cstr(const char *s) {
    size_t n;
    char *out;

    if (!s) return NULL;
    n = strlen(s) + 1u;
    out = (char *)malloc(n);
    if (!out) return NULL;
    memcpy(out, s, n);
    return out;
}

static char *shell_quote(const char *s) {
    size_t len = 2u;
    char *out;
    char *p;

    if (!s) return NULL;

#ifdef _WIN32
    for (const char *q = s; *q; q++) {
        len += (*q == '"') ? 2u : 1u;
    }
    out = (char *)malloc(len + 1u);
    if (!out) return NULL;
    p = out;
    *p++ = '"';
    for (const char *q = s; *q; q++) {
        if (*q == '"') *p++ = '\\';
        *p++ = *q;
    }
    *p++ = '"';
    *p = '\0';
#else
    for (const char *q = s; *q; q++) {
        len += (*q == '\'') ? 4u : 1u;
    }
    out = (char *)malloc(len + 1u);
    if (!out) return NULL;
    p = out;
    *p++ = '\'';
    for (const char *q = s; *q; q++) {
        if (*q == '\'') {
            memcpy(p, "'\\''", 4u);
            p += 4u;
        } else {
            *p++ = *q;
        }
    }
    *p++ = '\'';
    *p = '\0';
#endif

    return out;
}

static char *make_temp_wav_path(void) {
    const char *tmp = getenv("TMPDIR");
    char buf[512];

#ifdef _WIN32
    if (!tmp || !*tmp) tmp = getenv("TEMP");
    if (!tmp || !*tmp) tmp = ".";
    snprintf(buf, sizeof(buf), "%s\\sba_%ld.wav", tmp, (long)SBA_GETPID());
#else
    if (!tmp || !*tmp) tmp = "/tmp";
    snprintf(buf, sizeof(buf), "%s/sba_%ld.wav", tmp, (long)SBA_GETPID());
#endif

    return dup_cstr(buf);
}

static int ffmpeg_to_wav(const char *ffmpeg_path, const char *input_path, const char *wav_path) {
    const char *ffmpeg = (ffmpeg_path && *ffmpeg_path) ? ffmpeg_path : "ffmpeg";
    char *qffmpeg = shell_quote(ffmpeg);
    char *qin = shell_quote(input_path);
    char *qout = shell_quote(wav_path);
    char *cmd = NULL;
    int rc;
    int ok = 0;
    size_t cmd_len;

    if (!qffmpeg || !qin || !qout) {
        fprintf(stderr, "error: out of memory\n");
        goto done;
    }

    cmd_len = strlen(qffmpeg) + strlen(qin) + strlen(qout) + 128u;
    cmd = (char *)malloc(cmd_len);
    if (!cmd) {
        fprintf(stderr, "error: out of memory\n");
        goto done;
    }

    snprintf(cmd, cmd_len,
             "%s -v error -y -i %s -ac 2 -ar 44100 -sample_fmt s16 -f wav %s",
             qffmpeg, qin, qout);

    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "error: ffmpeg failed decoding %s\n", input_path);
        fprintf(stderr, "hint: install ffmpeg or convert to WAV first\n");
        goto done;
    }

    ok = 1;

done:
    free(qffmpeg);
    free(qin);
    free(qout);
    free(cmd);
    return ok;
}

static int encode_file(const char *input_path, const char *sba_path, const EncodeOptions *opts) {
    char *tmp_wav;
    int ok;

    if (!input_path || !sba_path) {
        fprintf(stderr, "error: missing input or output path\n");
        return 0;
    }

    if (has_wav_ext(input_path)) {
        return encode_wav_file(input_path, sba_path, opts);
    }

    tmp_wav = make_temp_wav_path();
    if (!tmp_wav) {
        fprintf(stderr, "error: out of memory\n");
        return 0;
    }

    printf("decoding %s with ffmpeg...\n", input_path);
    ok = ffmpeg_to_wav(opts ? opts->ffmpeg_path : NULL, input_path, tmp_wav);
    if (ok) {
        ok = encode_wav_file(tmp_wav, sba_path, opts);
    }

    remove(tmp_wav);
    free(tmp_wav);
    return ok;
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-ag] [-ffmpeg path] input.wav|input.mp3 output.sba\n", argv0);
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -ag             auto gain/peak normalize converted audio before encoding\n");
    fprintf(stderr, "  -ffmpeg path    ffmpeg executable to use for MP3/FLAC/OGG input\n");
    fprintf(stderr, "input WAV may be PCM 8/16/24/32-bit or float 32/64-bit\n");
    fprintf(stderr, "non-WAV inputs use ffmpeg when available, e.g. MP3/FLAC/OGG\n");
    fprintf(stderr, "output SBA is always 44100 Hz, 16-bit, stereo\n");
}

int main(int argc, char **argv) {
    EncodeOptions opts;
    const char *wav_path;
    const char *sba_path;
    int argi = 1;

    memset(&opts, 0, sizeof(opts));

    while (argi < argc && argv[argi][0] == '-') {
        if (strcmp(argv[argi], "-ag") == 0) {
            opts.auto_gain = 1;
            argi++;
            continue;
        }
        if (strcmp(argv[argi], "-ffmpeg") == 0) {
            if (argi + 1 >= argc) {
                usage(argv[0]);
                return 2;
            }
            opts.ffmpeg_path = argv[argi + 1];
            argi += 2;
            continue;
        }

        usage(argv[0]);
        return 2;
    }

    if (argc - argi != 2) {
        usage(argv[0]);
        return 2;
    }

    wav_path = argv[argi];
    sba_path = argv[argi + 1];

    crc32_init();
    return encode_file(wav_path, sba_path, &opts) ? 0 : 1;
}
