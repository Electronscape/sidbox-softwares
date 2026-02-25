#include "vdp.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../sbapi_graphics.h"

static uint32_t g_vdp_vram_w = 0;
static uint32_t g_vdp_cram_w = 0;
static uint32_t g_vdp_vsram_w = 0;

static uint32_t md_color_to_rgb888(uint16_t cram_word) {
    // Mega Drive CRAM: 0b0000 BBB0 GGG0 RRR0 (commonly described; 3 bits per channel)
    // Bits: R in bits 1-3, G in 5-7, B in 9-11 (depending on docs). Common mapping:
    //   R = (word >> 1) & 0x7
    //   G = (word >> 5) & 0x7
    //   B = (word >> 9) & 0x7
    // Expand 3-bit to 8-bit: v * 255 / 7
    uint8_t r3 = (uint8_t)((cram_word >> 1) & 0x7);
    uint8_t g3 = (uint8_t)((cram_word >> 5) & 0x7);
    uint8_t b3 = (uint8_t)((cram_word >> 9) & 0x7);

    uint8_t r = (uint8_t)(r3 * 255 / 7);
    uint8_t g = (uint8_t)(g3 * 255 / 7);
    uint8_t b = (uint8_t)(b3 * 255 / 7);

    return (0xFFu << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}



static inline uint8_t vdp_autoinc(const Vdp* vdp) {
    return vdp->regs[0x0F] ? vdp->regs[0x0F] : 2; // many games set it; default sane=2
}

static inline void vdp_set_target_from_code(Vdp* vdp, uint8_t code) {
    // MD VDP: "code" selects access type
    // Commonly:
    //   0: VRAM read
    //   1: VRAM write
    //   2: CRAM read
    //   3: CRAM write
    //   4: VSRAM read
    //   5: VSRAM write
    vdp->code = code & 0x3F;
    if (code == 2 || code == 3) vdp->target = 1;      // CRAM
    else if (code == 4 || code == 5) vdp->target = 2; // VSRAM
    else vdp->target = 0;                             // VRAM
}

static inline void vdp_commit_command(Vdp* vdp, uint16_t w0, uint16_t w1)
{
    // Standard MD VDP command decode (common emulator approach)
    // Command words format (simplified):
    //  w0: .. .. a13..a0  (low 14 addr bits)
    //  w1: .... .... a15 a14 .... cd1 cd0  (upper 2 addr bits + code bits)
    //
    // addr = (w0 & 0x3FFF) | ((w1 & 0x0003) << 14)
    // code = ((w1 >> 4) & 0x0F) | ((w0 >> 14) & 0x03)  -> 6 bits in practice, but we only need low 3 for target
    //
    // Many emus treat "code" low bits:
    //   0: VRAM read
    //   1: VRAM write
    //   2: CRAM write (some docs list as CRAM read/write; but games mostly use write)
    //   3: VSRAM write
    //
    // We'll decode a "type" from the low 2 bits primarily.
    uint32_t addr = (uint32_t)(w0 & 0x3FFFu) | (uint32_t)((w1 & 0x0003u) << 14);

    // This is the *practical* code extraction used widely:
    uint8_t code = (uint8_t)(((w1 >> 4) & 0x3Fu) | ((w0 >> 14) & 0x03u));

    vdp->addr = (uint16_t)addr;
    vdp->code = code & 0x3F;

    // Map target from code (keep it simple & robust)
    // Use low 4-ish bits to decide memory target. Most games: VRAM writes and CRAM writes.
    // Commonly: code&0x0F == 0x01 => VRAM write
    //          code&0x0F == 0x03 => CRAM write
    //          code&0x0F == 0x05 => VSRAM write
    uint8_t low = (uint8_t)(code & 0x0F);
    if (low == 0x03) vdp->target = 1;      // CRAM
    else if (low == 0x05) vdp->target = 2; // VSRAM
    else vdp->target = 0;                  // VRAM

    vdp->autoinc = vdp_autoinc(vdp);
}

void vdp_reset(Vdp* vdp) {
    memset(vdp->vram, 0, sizeof(vdp->vram));
    memset(vdp->cram, 0, sizeof(vdp->cram));
    memset(vdp->vsram, 0, sizeof(vdp->vsram));
    memset(vdp->regs, 0, sizeof(vdp->regs));

    vdp->pending_ctrl = 0;
    vdp->ctrl_latch = 0;

    vdp->addr = 0;
    vdp->code = 0;
    vdp->target = 0;
    vdp->autoinc = 2;

    // Some sensible defaults used by many BIOS-less init sequences
    vdp->regs[0x0F] = 2; // autoinc
}

void vdp_init(Vdp* vdp, int fb_w, int fb_h) {
    memset(vdp, 0, sizeof(*vdp));
    vdp->fb_w = fb_w;
    vdp->fb_h = fb_h;
    vdp->framebuffer = (uint32_t*)malloc((size_t)fb_w * (size_t)fb_h * sizeof(uint32_t));
    vdp_reset(vdp);
}

void vdp_free(Vdp* vdp) {
    if (vdp->framebuffer) free(vdp->framebuffer);
    vdp->framebuffer = NULL;
}

/*
  Control port write accepts:
    1) Register write: 1 rrrr vvvvvvvv  (0x8000..0x9FFF)
    2) Two-word command: first word then second word (latch)
*/
void vdp_write_ctrl(Vdp* vdp, uint16_t value) {
    // Register write shortcut: 10rrrrvv vvvvvvvv (0x8000)
    if ((value & 0xC000u) == 0x8000u) {
        uint8_t reg = (uint8_t)((value >> 8) & 0x1Fu);
        uint8_t val = (uint8_t)(value & 0xFFu);
        vdp->regs[reg] = val;
        if (reg == 0x0F) vdp->autoinc = vdp_autoinc(vdp);
        return;
    }

    // Otherwise command word stream (two-word latch)
    if (vdp->ctrl_latch == 0) {
        vdp->pending_ctrl = value;
        vdp->ctrl_latch = 1;
        return;
    }

    // second word completes command
    uint16_t w0 = vdp->pending_ctrl;
    uint16_t w1 = value;
    vdp->ctrl_latch = 0;

    // If second word is also register write (rare sequencing), handle it:
    if ((w1 & 0xC000u) == 0x8000u) {
        uint8_t reg = (uint8_t)((w1 >> 8) & 0x1Fu);
        uint8_t val = (uint8_t)(w1 & 0xFFu);
        vdp->regs[reg] = val;
        if (reg == 0x0F) vdp->autoinc = vdp_autoinc(vdp);
        return;
    }

    vdp_commit_command(vdp, w0, w1);
}

uint16_t vdp_read_ctrl(Vdp* vdp) {
    // Minimal status. Real bits include FIFO, VBlank, HBlank, sprite overflow/collision, etc.
    // Many games poll VBlank (bit 3 in MD VDP status word is commonly VBlank flag in emus).
    // We'll set "VBlank" always true for now so wait loops don't hang.
    //
    // Common emulator status bits:
    // bit 3: VBlank
    // bit 2: HBlank
    // bit 1: DMA busy (0)
    // We'll return 0x0008 as "in vblank".
    (void)vdp;
    return 0x0008u;
}

uint16_t vdp_read_data(Vdp* vdp) {
    // Basic reads from current target using current address, then autoincrement.
    uint16_t out = 0;

    uint32_t a = vdp->addr & 0xFFFFu;

    if (vdp->target == 0) { // VRAM
        // VRAM is byte array, but VDP data port is 16-bit
        uint8_t hi = vdp->vram[(a + 0) & 0xFFFFu];
        uint8_t lo = vdp->vram[(a + 1) & 0xFFFFu];
        out = (uint16_t)((hi << 8) | lo);
    } else if (vdp->target == 1) { // CRAM
        // CRAM is word addressed; MD uses 7-bit word index (0..63)
        uint32_t wi = (a >> 1) & 0x3Fu;
        out = vdp->cram[wi];
    } else { // VSRAM
        uint32_t wi = (a >> 1) % VDP_VSRAM_WORDS;
        out = vdp->vsram[wi];
    }

    vdp->addr = (vdp->addr + vdp->autoinc) & 0xFFFFu;
    return out;
}

static int seen_nonzero_vram = 0;
void vdp_write_data(Vdp* vdp, uint16_t value) {
    uint32_t a = vdp->addr & 0xFFFFu;

    // Only implement "write" codes properly. Reads ignore writes anyway.
    // Expected write codes: 1=VRAM write, 3=CRAM write, 5=VSRAM write
    if (vdp->target == 0) { // VRAM

        // Write big-endian word into byte-addressed VRAM
        vdp->vram[(a + 0) & 0xFFFFu] = (uint8_t)(value >> 8);
        vdp->vram[(a + 1) & 0xFFFFu] = (uint8_t)(value & 0xFFu);
    } else if (vdp->target == 1) { // CRAM
        uint32_t wi = (a >> 1) & 0x3Fu;
        vdp->cram[wi] = value;
    } else { // VSRAM
        uint32_t wi = (a >> 1) % VDP_VSRAM_WORDS;
        vdp->vsram[wi] = value;
    }

    if (vdp->target == 0) g_vdp_vram_w++;
    else if (vdp->target == 1) g_vdp_cram_w++;
    else g_vdp_vsram_w++;

    if (vdp->target == 0) {
        if (!seen_nonzero_vram && value != 0) {
            seen_nonzero_vram = 1;
            fprintf(stderr, "First nonzero VRAM write: addr=%04X val=%04X\n",
                    (unsigned)(a & 0xFFFF), (unsigned)value);
        }
    }

    vdp->addr = (vdp->addr + vdp->autoinc) & 0xFFFFu;
}



static inline uint8_t md_tile_pixel_4bpp(const uint8_t* tile32, int x, int y)
{
    // Mega Drive tile format is planar 4bpp.
    // 32 bytes per tile. Each row is 4 bytes: plane0, plane1, plane2, plane3.
    // Each plane byte holds 8 bits for the 8 pixels in the row.
    // Pixel bit position: bit 7 is x=0, bit 0 is x=7.

    const uint8_t* row = tile32 + (y * 4);
    uint8_t p0 = row[0];
    uint8_t p1 = row[1];
    uint8_t p2 = row[2];
    uint8_t p3 = row[3];

    uint8_t bit = (uint8_t)(7 - (x & 7));
    uint8_t c =
        ((p0 >> bit) & 1u) |
        (((p1 >> bit) & 1u) << 1) |
        (((p2 >> bit) & 1u) << 2) |
        (((p3 >> bit) & 1u) << 3);

    return c; // 0..15
}

static void vdp_debug_draw_tiles_to_proj_vram(const Vdp* vdp)
{
    // Draw as many 8x8 tiles as fit on screen
    const int tiles_x = SCR_WIDTH / 8;
    const int tiles_y = SCR_HEIGHT / 8;
    const int tiles_per_screen = tiles_x * tiles_y;

    // VRAM is 64KB, 32 bytes per tile => 2048 tiles max
    const int max_tiles = 2048;
    int tile_count = tiles_per_screen;
    if (tile_count > max_tiles) tile_count = max_tiles;

    for (int t = 0; t < tile_count; t++) {
        int tx = (t % tiles_x);
        int ty = (t / tiles_x);
        int px0 = tx * 8;
        int py0 = ty * 8;

        const uint32_t tile_addr = (uint32_t)t * 32u;
        const uint8_t* tile32 = (const uint8_t*)&vdp->vram[tile_addr & 0xFFFFu];

        for (int y = 0; y < 8; y++) {
            uint8_t* dst = PROJ_VRAM + (py0 + y) * SCR_WIDTH + px0;
            for (int x = 0; x < 8; x++) {
                uint8_t pen = md_tile_pixel_4bpp(tile32, x, y);

                // Put it in palette range so it's visible:
                // use palette indices 0..15 directly (pen), but bump by 16 so pen0 isn't black
                //dst[x] = (uint8_t)(pen + 16);
                //dst[x] = (uint8_t)(pen ? (32 + pen * 10) : 0);  // pen 0 stays bg, others bright band
                dst[x] = (uint8_t)pen;
            }
        }
    }
}

void vdp_render_frame(Vdp* vdp)
{
    // Clear using your library
    // Force a visible debug palette in entries 0..15 so the tile viewer ALWAYS shows.


    //sbgfx_fill(0);
    for (int i = 0; i < 16; i++) {
        uint8_t v = (uint8_t)(i * 17); // 0,17,34,...255
        PROJ_CRAM[i] = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
    }


    // 1) Sync emulator CRAM -> your PROJ_CRAM (optional)
    // If you already use PROJ_CRAM as your palette store, you can remove this.
    // Here we just convert MD 9-bit/12-bit-ish CRAM to RGB888 if you have md_color_to_rgb888.
    // If vdp->cram is already RGB888, also remove this loop.


    // 2) DEBUG TILE VIEWER: shows if VRAM is being written correctly
    vdp_debug_draw_tiles_to_proj_vram(vdp);

    // 3) Optional: little HUD text so you know it’s alive
    gfx_setcolour(255);
    draw_text816(4, 4, (const unsigned char*)"VDP VRAM TILE VIEW");


    static int once = 0;
    //if (!once)
    {
        once = 1;
        unsigned nz = 0;
        for (unsigned i = 0; i < 0x1000; i++) if (vdp->vram[i]) { nz = 1; break; }

        char buf[80];
        snprintf(buf, sizeof(buf), "WD:VRAM=%u CRAM=%u VSR=%u", g_vdp_vram_w, g_vdp_cram_w, g_vdp_vsram_w);
        gfx_setcolour(255);
        draw_text816(4, 20, (const unsigned char*)buf);
    }

    unsigned first = 0xFFFFFFFFu;
    unsigned last  = 0;
    unsigned count = 0;

    for (unsigned i = 0; i < 0x10000; i++) {
        if (vdp->vram[i]) {
            if (first == 0xFFFFFFFFu) first = i;
            last = i;
            count++;
        }
    }

    char buf2[96];
    snprintf(buf2, sizeof(buf2), "VRAM nz=%u first=%04X last=%04X", count,
             (first==0xFFFFFFFFu)?0:first, last);
    draw_text816(4, 36, (const unsigned char*)buf2);
}
