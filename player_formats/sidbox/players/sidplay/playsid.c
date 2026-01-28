#include <stdint.h>
#include <string.h>

#include "playsid.h"

static uint16_t be16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] << 8) | (uint16_t)p[1];
}
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

//int psid_song_uses_cia(const sid_program_t *p, uint16_t song1_based);
//c64_video_t sid_pick_video(const sid_program_t *p);

int sid_load_from_bytes(const uint8_t *buf, uint32_t len, uint8_t *ram, sid_program_t *out){
    if (!buf || !ram || !out) return -1;
    if (len < 0x76) return -2; // header is at least 0x76 for PSID v2+

    // magic
    sid_fmt_t fmt = SIDFMT_UNKNOWN;
    if (buf[0]=='P' && buf[1]=='S' && buf[2]=='I' && buf[3]=='D') fmt = SIDFMT_PSID;
    if (buf[0]=='R' && buf[1]=='S' && buf[2]=='I' && buf[3]=='D') fmt = SIDFMT_RSID;
    if (fmt == SIDFMT_UNKNOWN) return -3;

    uint16_t version     = be16(buf + 0x04);
    uint16_t data_offset = be16(buf + 0x06);
    uint16_t load_addr_h = be16(buf + 0x08);
    uint16_t init_addr   = be16(buf + 0x0A);
    uint16_t play_addr   = be16(buf + 0x0C);
    uint16_t songs       = be16(buf + 0x0E);
    uint16_t start_song  = be16(buf + 0x10);
    uint32_t speed       = be32(buf + 0x12);

    uint16_t flags = 0;
    if (version >= 2 && len >= 0x78) { // flags exist in v2NG-ish layouts
        flags = be16(buf + 0x76);
    }

    if (data_offset >= len) return -4;

    // Determine final load address:
    // If header load addr == 0, first two bytes of data are little-endian load address.
    uint32_t pos = data_offset;
    uint16_t load_addr = load_addr_h;

    if (load_addr == 0) {
        if (pos + 2 > len) return -5;
        load_addr = (uint16_t)((uint16_t)buf[pos] | ((uint16_t)buf[pos + 1] << 8));
        pos += 2;
    }

    // Copy payload into RAM
    uint32_t payload_len = len - pos;
    if ((uint32_t)load_addr + payload_len > 65536u) {
        // clamp to fit rather than explode
        payload_len = 65536u - (uint32_t)load_addr;
    }
    memcpy(&ram[load_addr], &buf[pos], payload_len);

    // Fill result
    memset(out, 0, sizeof(*out));
    out->fmt         = fmt;
    out->load_addr   = load_addr;
    out->init_addr   = init_addr;
    out->play_addr   = play_addr;
    out->songs       = songs;
    out->start_song  = start_song;
    out->speed_bits  = speed;
    out->data_offset = data_offset;
    out->flags       = flags;

    return 0;
}





















