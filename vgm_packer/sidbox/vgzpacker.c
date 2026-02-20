#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t magic;     // 'VGP0'
    uint32_t method;    // 1 = LZSS
    uint32_t raw_size;
    uint32_t pack_size;
} vgp_hdr_t;

#define VGP_MAGIC  0x30504756u
#define VGP_METHOD_LZSS 1u

#define WIN_SIZE   4096u
#define WIN_MASK   (WIN_SIZE - 1u)
#define MIN_MATCH  3u
#define MAX_MATCH  18u

static uint8_t* file_load(const char *path, uint32_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);

    uint8_t *buf = (uint8_t*)malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }

    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) { free(buf); return NULL; }

    *out_size = (uint32_t)sz;
    return buf;
}

static int file_save(const char *path, const void *data, uint32_t size)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    size_t wr = fwrite(data, 1, size, f);
    fclose(f);
    return (wr == size);
}

/* ---------------- LZSS PACK ---------------- */

static uint32_t lzss_pack(const uint8_t *in, uint32_t in_len, uint8_t *out, uint32_t out_cap)
{
    uint8_t win[WIN_SIZE];
    memset(win, 0, sizeof(win));
    uint32_t wpos = 0;

    uint32_t ip = 0;
    uint32_t op = 0;

    while (ip < in_len)
    {
        if (op + 1 >= out_cap) return 0; // need space for flag
        uint32_t flag_pos = op++;
        uint8_t flags = 0;

        for (uint32_t bit = 0; bit < 8 && ip < in_len; bit++)
        {
            uint32_t best_len = 0;
            uint32_t best_ofs = 0;

            /* brute force search last 4096 bytes */
            uint32_t max_back = (ip < WIN_SIZE) ? ip : WIN_SIZE;
            for (uint32_t back = 1; back <= max_back; back++)
            {
                uint32_t ofs = (wpos - back) & WIN_MASK;

                /* quick reject */
                if (win[ofs] != in[ip]) continue;

                uint32_t ml = 1;
                while (ml < MAX_MATCH && (ip + ml) < in_len)
                {
                    if (win[(ofs + ml) & WIN_MASK] != in[ip + ml]) break;
                    ml++;
                }

                if (ml >= MIN_MATCH && ml > best_len)
                {
                    best_len = ml;
                    best_ofs = ofs;
                    if (best_len == MAX_MATCH) break;
                }
            }

            if (best_len >= MIN_MATCH)
            {
                /* emit backref: 2 bytes */
                if (op + 2 > out_cap) return 0;

                uint32_t ofs = best_ofs;          // 0..4095
                uint32_t len = best_len;          // 3..18
                uint8_t b1 = (uint8_t)(ofs & 0xFFu);
                uint8_t b2 = (uint8_t)(((ofs >> 8) & 0x0Fu) | ((uint8_t)(len - MIN_MATCH) << 4));

                out[op++] = b1;
                out[op++] = b2;

                /* update window with the bytes we matched */
                for (uint32_t k = 0; k < len; k++)
                {
                    uint8_t v = in[ip++];
                    win[wpos] = v;
                    wpos = (wpos + 1) & WIN_MASK;
                }
            }
            else
            {
                /* emit literal */
                if (op + 1 > out_cap) return 0;
                flags |= (uint8_t)(1u << bit);
                uint8_t v = in[ip++];
                out[op++] = v;

                win[wpos] = v;
                wpos = (wpos + 1) & WIN_MASK;
            }
        }

        out[flag_pos] = flags;
    }

    return op;
}


int vgp_pack_file(const char *in_path, const char *out_path)
{
    uint32_t raw_sz = 0;
    uint8_t *raw = file_load(in_path, &raw_sz);
    if (!raw) return 0;

    /* worst case: flags + literals (~ +12.5%) */
    uint32_t out_cap = raw_sz + (raw_sz / 8u) + 64u;
    uint8_t *packed = (uint8_t*)malloc(out_cap);
    if (!packed) { free(raw); return 0; }

    uint32_t pack_sz = lzss_pack(raw, raw_sz, packed, out_cap);
    if (pack_sz == 0) { free(raw); free(packed); return 0; }

    vgp_hdr_t hdr;
    hdr.magic = VGP_MAGIC;
    hdr.method = VGP_METHOD_LZSS;
    hdr.raw_size = raw_sz;
    hdr.pack_size = pack_sz;

    uint32_t total = (uint32_t)sizeof(hdr) + pack_sz;
    uint8_t *blob = (uint8_t*)malloc(total);
    if (!blob) { free(raw); free(packed); return 0; }

    memcpy(blob, &hdr, sizeof(hdr));
    memcpy(blob + sizeof(hdr), packed, pack_sz);

    int ok = file_save(out_path, blob, total);

    free(raw);
    free(packed);
    free(blob);

    return ok;
}
