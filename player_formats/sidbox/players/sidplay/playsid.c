#include <stdint.h>
#include <string.h>

#include "playsid.h"

static uint16_t be16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] << 8) | (uint16_t)p[1];
}
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

/*
int psid_load_from_bytes(const uint8_t *song, uint32_t song_len, uint8_t ram[65536], psid_info_t *out){
    psid_info_t info;
    uint16_t version, data_off, load_be, init_be, play_be, songs_be, start_be;
    uint32_t speed_be;

    if (!song || song_len < 0x76 || !ram) return -1;

    // Magic: "PSID" or "RSID"
    if (song[0] == 'P' && song[1] == 'S' && song[2] == 'I' && song[3] == 'D') {
        info.is_rsid = 0;
    } else if (song[0] == 'R' && song[1] == 'S' && song[2] == 'I' && song[3] == 'D') {
        info.is_rsid = 1;
        // RSID needs real C64 environment; we bail for now.
        return -2;
    } else {
        return -3;
    }

    version  = be16(&song[0x04]);
    data_off = be16(&song[0x06]);
    load_be  = be16(&song[0x08]);
    init_be  = be16(&song[0x0A]);
    play_be  = be16(&song[0x0C]);
    songs_be = be16(&song[0x0E]);
    start_be = be16(&song[0x10]);
    speed_be = be32(&song[0x12]);

    if (data_off >= song_len) return -4;

    info.data_offset = data_off;
    info.init_addr   = init_be;
    info.play_addr   = play_be;
    info.songs       = songs_be;
    info.start_song  = start_be;
    info.speed       = speed_be;

    // Determine load address + data pointer
    uint32_t data_pos = data_off;
    uint16_t load_addr = load_be;

    if (load_addr == 0) {
        // load address is first 2 bytes of data (little endian)
        if (data_pos + 2 > song_len) return -5;
        load_addr = (uint16_t)song[data_pos] | ((uint16_t)song[data_pos + 1] << 8);
        data_pos += 2;
    }

    info.load_addr = load_addr;
    if (info.init_addr == 0) info.init_addr = info.load_addr;
    if (info.play_addr == 0) info.play_addr = info.load_addr;


    // Copy data into RAM
    uint32_t data_len = song_len - data_pos;
    if ((uint32_t)load_addr + data_len > 65536u) {
        // clamp to RAM end (or treat as error)
        data_len = 65536u - (uint32_t)load_addr;
    }

    memcpy(&ram[load_addr], &song[data_pos], data_len);

    if (out) *out = info;
    return 0;
}
*/

int sid_load_from_bytes(const uint8_t *buf, uint32_t len,
                        uint8_t ram[65536], sid_program_t *out)
{
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





















