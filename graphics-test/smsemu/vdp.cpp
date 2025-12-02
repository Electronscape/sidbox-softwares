//#pragma GCC optimize ("Ofast")
#include "../dialog.h"
#include "sms.h"
#include "internal.h"
#include <assert.h>
#include <string.h>

#define VDP sms->vdp
#define VDPi sms.vdp

float stretch_wide = 1.0f;

int16_t dpv_offset_x, dpv_offset_y;

enum {
    NTSC_HCOUNT_MAX = 342, NTSC_VCOUNT_MAX = 262,
    NTSC_FPS = 60,
    NTSC_CYCLES_PER_LINE = (int)(CPU_CLOCK / NTSC_VCOUNT_MAX / NTSC_FPS), // ~228
    NTSC_HBLANK_LEFT = 29, NTSC_HBLANK_RIGHT = 29,
    NTSC_BORDER_TOP = 27, NTSC_BORDER_BOTTOM = 24,
    NTSC_BORDER_LEFT = 13, NTSC_BORDER_RIGHT = 15,
    NTSC_ACTIVE_DISPLAY_HORIZONTAL = 256, NTSC_ACTIVE_DISPLAY_VERTICAL = 192,
    NTSC_DISPLAY_VERTICAL_START = NTSC_BORDER_TOP, NTSC_DISPLAY_VERTICAL_END = NTSC_DISPLAY_VERTICAL_START + NTSC_ACTIVE_DISPLAY_VERTICAL,
    NTSC_DISPLAY_HORIZONTAL_START = NTSC_BORDER_LEFT, NTSC_DISPLAY_HORIZONTAL_END = NTSC_DISPLAY_HORIZONTAL_START + NTSC_ACTIVE_DISPLAY_HORIZONTAL,
    SPRITE_EOF = 208,
};

static bool vdp_is_line_irq_wanted(const struct SMS_Core *sms) {
    return BIT_TST4(VDP.registers[0x0]);
}

static bool vdp_is_vblank_irq_wanted(const struct SMS_Core *sms) {
    return BIT_TST5(VDP.registers[0x1]);
}

static inline uint16_t vdp_get_nametable_base_addr(const struct SMS_Core *sms) {
    return ((VDP.registers[0x2] >> 1) & 0x7) << 11;
}

static inline uint16_t vdp_get_sprite_attribute_base_addr(const struct SMS_Core *sms) {
    return ((VDP.registers[0x5] >> 1) & 0x3F) << 8;
}

static bool vdp_get_sprite_pattern_select(const struct SMS_Core *sms) {
    return BIT_TST2(VDP.registers[0x6]);
}

static bool vdp_is_display_enabled(const struct SMS_Core *sms) {
    return BIT_TST6(VDP.registers[0x1]);
}

static uint8_t vdp_get_sprite_height(const struct SMS_Core *sms) {
    const bool doubled_sprites = BIT_TST0(VDP.registers[0x1]);
    const uint8_t sprite_size = BIT_TST1(VDP.registers[0x1]) ? 16 : 8;

    return sprite_size << doubled_sprites;
}


static inline bool vdp_is_display_active(const struct SMS_Core *sms) {
    if (SMS_is_system_type_gg(sms)) {
        return VDP.vcount >= 24 && VDP.vcount < 144 + 24;
    } else {
        return VDP.vcount < 192;
    }
}


uint8_t vdp_status_flag_read(struct SMS_Core *sms) {
    uint8_t v = 31; // unused bits 1,2,3,4

    v |= VDP.frame_interrupt_pending << 7;
    v |= VDP.sprite_overflow << 6;
    v |= VDP.sprite_collision << 5;

    // these are reset on read
    VDP.frame_interrupt_pending = false;
    VDP.line_interrupt_pending = false;
    VDP.sprite_overflow = false;
    VDP.sprite_collision = false;

    return v;
}


void vdp_io_write(struct SMS_Core *sms, uint8_t addr, uint8_t value) {
    const uint8_t reg = addr & 0xF;
    if (reg >= 0xB) {
        return;  // unused registers B-F
    }

    switch (reg) {
    case 0x1:
        assert(IS_BIT_SET(value, 3) == 0 && "240 height mode set");
        assert(IS_BIT_SET(value, 4) == 0 && "224 height mode set");
        break;

    case 0x3:
        assert(value == 0xFF && "colour table bits not all set");
        break;

    case 0x4:
        assert((value & 0xF) == 0xF && "Background Pattern Generator Base Address");
        break;

    case 0x09:
        if (VDP.vcount >= 192) {
            VDP.vertical_scroll = value;
        }
        break;

    case 0xA:
        if (VDP.vcount >= 192) {
            VDP.line_counter = value;
        }
        break;
    }

    VDP.registers[addr & 0xF] = value;
}


// same as i used in dmg / gbc rendering for gb

struct PriorityBuf {
    bool array[NTSC_ACTIVE_DISPLAY_HORIZONTAL];
};

struct PriorityBuf prio = { 0 };

static void vdp_render_background() {
    const uint8_t line = VDPi.vcount;
    const uint16_t vdpy = line + dpv_offset_y;
    const uint8_t fine_line = line & 0x7;
    const uint8_t row = line >> 3;

    const uint8_t reg8 = VDPi.registers[0x8];
    const uint8_t starting_col = (32 - (reg8 >> 3)) & 31;
    const uint8_t starting_row = VDPi.vertical_scroll >> 3;

    uint8_t fine_scrollx_base = reg8 & 0x7;
    const uint8_t fine_scrolly = VDPi.vertical_scroll & 0x7;

    const uint16_t nametable_base_addr = vdp_get_nametable_base_addr(&sms);

    const uint8_t reg0 = VDPi.registers[0x0];
    const uint8_t reg1 = VDPi.registers[0x1];

    for (uint8_t col = 0; col < 32; ++col) {
        uint16_t horizontal_offset;
        uint8_t fine_scrollx;

        // Check horizontal scrolling disable condition (avoid recalculating fine_scrollx inside)
        if ((reg0 & (1 << 6)) && (line < 16)) {
            horizontal_offset = col * 2;
            fine_scrollx = 0;
        } else {
            horizontal_offset = ((starting_col + col) & 31) * 2;
            fine_scrollx = fine_scrollx_base;
        }

        uint16_t vertical_offset;
        uint8_t palette_index_offset;
        uint8_t check_col;

        if (reg1 & (1 << 7)) {
            check_col = col;
        } else {
            check_col = (col + starting_col) & 31;
        }

        // Vertical scrolling disable condition
        if ((reg0 & (1 << 7)) && (check_col >= 24)) {
            vertical_offset = (row % 28) * 64;
            palette_index_offset = fine_line;
        } else {
            // Next row check
            uint8_t next_row = ((fine_line + fine_scrolly) > 7);
            vertical_offset = ((row + starting_row + next_row) % 28) * 64;
            palette_index_offset = (fine_line + fine_scrolly) & 0x7;
        }

        uint16_t nametable_addr = nametable_base_addr + vertical_offset + horizontal_offset;

        // Access tile as 16-bit directly
        uint16_t tile = *(uint16_t*)(&vram[nametable_addr]);

        const uint8_t priority = (tile >> 12) & 1;
        const uint8_t palette_select = (tile >> 11) & 1;
        const uint8_t vertical_flip = (tile >> 10) & 1;
        const uint8_t horizontal_flip = (tile >> 9) & 1;
        uint16_t pattern_index = (tile & 0x1FF) * 32;

        if (vertical_flip)
            pattern_index += (7 - palette_index_offset) * 4;
        else
            pattern_index += palette_index_offset * 4;

        uint32_t *cref = (uint32_t*) &vram[pattern_index];
        uint32_t p = *cref;

        int16_t x_index = (col * 8) + fine_scrollx + dpv_offset_x;

        for (uint8_t x = 0; x < 8; ++x) {
            x_index++;
            if (x_index >= 256)
                break;

            uint8_t palette_index = palette_select ? 16 : 0;

            if (horizontal_flip) {
                // Pull bits efficiently for horizontal flip
                palette_index += ((p >> 0) & 1) | ((p >> 7) & 2) | ((p >> 14) & 4) | ((p >> 21) & 8);
                p >>= 1;
            } else {
                palette_index += ((p >> 7) & 1) | ((p >> 14) & 2) | ((p >> 21) & 4) | ((p >> 28) & 8);
                p <<= 1;
            }

            if (x_index < 0)
                continue;

            prio.array[x_index] = (priority && palette_index != 0 && palette_index != 16);

            SMSPlot(x_index-1, vdpy, palette_index);
        }
    }
}

struct SpriteEntry {
    int16_t y;
    uint8_t xn_index;
};

struct SpriteEntries {
    struct SpriteEntry entry[MAX_SPRITES];
    uint8_t count;
};

static struct SpriteEntries vdp_parse_sprites(struct SMS_Core *sms) {
    struct SpriteEntries sprites = { 0 };

    const uint8_t line = VDP.vcount;

    const uint16_t sprite_attribute_base_addr = vdp_get_sprite_attribute_base_addr(sms);
    const uint8_t sprite_size = vdp_get_sprite_height(sms);

    for (uint8_t i = 0; i < 64; ++i) {
        int16_t y = vram[sprite_attribute_base_addr + i] + 1;

        // special number used to stop sprite parsing!
        if (y == SPRITE_EOF + 1) {
            break;
        }

        // docs (TMS9918.pdf) state that y is partially signed (-31, 0) which
        // is line 224 im not sure if this is correct, as it likely isn't as
        // theres a 240 hieght mode, meaning no sprites could be displayed past 224...
        // so maybe this should reset on 240? or maybe it depends of the
        // the height mode selected, ie, 192, 224 and 240
#if 0
        if (y > 224)
#else
        if (y > 192)
#endif
        {
            y -= 256;
        }

        if (line >= y && line < (y + sprite_size)) {
            if (sprites.count < MAX_SPRITES) {
                sprites.entry[sprites.count].y = y;
                // xn values start at + 0x80 in the SAT
                sprites.entry[sprites.count].xn_index = 0x80 + (i * 2);
                sprites.count++;
            }

            // if we have filled the sprite array, we need to keep checking further
            // entries just in case another sprite falls on the same line, in which
            // case, the sprite overflow flag is set for stat.
            else {
                VDP.sprite_overflow = true;
                break;
            }
        }
    }

    return sprites;
}

/*

static void vdp_render_sprites() {
    //const struct VDP_region region = vdp_get_region(sms);

    const uint8_t line = VDPi.vcount;
    const uint16_t lny = VDPi.vcount + dpv_offset_y;
    const uint16_t attr_addr = vdp_get_sprite_attribute_base_addr(&sms);
    const uint16_t pattern_select = vdp_get_sprite_pattern_select(&sms) ? 256 : 0;
    const int8_t sprite_x_offset = BIT_TST3(VDPi.registers[0x0]) ? -8 : 0;

    const struct SpriteEntries sprites = vdp_parse_sprites(&sms);

    bool drawn_sprites[NTSC_ACTIVE_DISPLAY_HORIZONTAL] = { 0 };

    for (uint8_t i = 0; i < sprites.count; ++i) {
        const struct SpriteEntry *sprite = &sprites.entry[i];

        // signed because the sprite can be negative if -8!
        const int16_t sprite_x = vram[attr_addr + sprite->xn_index + 0] + sprite_x_offset;
        uint16_t pattern_index = (vram[attr_addr + sprite->xn_index + 1] + pattern_select);

        // docs state that is bit1 of reg1 is set (should always), bit0 is ignored
        // i am not sure if this is applied to the final value of the value
        // initial value however...
        if (BIT_TST1(VDPi.registers[0x1])) {
            pattern_index &= ~0x1;
        }

        pattern_index *= 32;

        // this has already taken into account of sprite size when parsing
        // sprites, so for example:
        // - line = 3,
        // - sprite->y = 1,
        // - sprite_size = 8,
        // then y will be accepted for this line, but needs to be offset from
        // the current line we are on, so line-sprite->y = 2
        pattern_index += (line - sprite->y) * 4;

        const uint8_t bit_plane0 = vram[pattern_index + 0];
        const uint8_t bit_plane1 = vram[pattern_index + 1];
        const uint8_t bit_plane2 = vram[pattern_index + 2];
        const uint8_t bit_plane3 = vram[pattern_index + 3];
        uint8_t cbr = 7;

        int16_t x_index = sprite_x + dpv_offset_x;
        for (uint8_t x = 0; x < 8; ++x) {

            x_index++;
            // skip if column0 or offscreen
            if (x_index < 0)
                continue;

            if (x_index >= 256)
                break;

            // skip is bg has priority
            if (prio.array[x_index])
                continue;

            // if set, we load from the tile index instead!
            uint8_t palette_index = 0;

            //palette_index |= IS_BIT_SET(bit_plane0, cbr) << 0;
            //palette_index |= IS_BIT_SET(bit_plane1, cbr) << 1;
            //palette_index |= IS_BIT_SET(bit_plane2, cbr) << 2;
            //palette_index |= IS_BIT_SET(bit_plane3, cbr) << 3;
            palette_index |= ((bit_plane0 >> cbr) & 1)
                          | (((bit_plane1 >> cbr) & 1) << 1)
                          | (((bit_plane2 >> cbr) & 1) << 2)
                          | (((bit_plane3 >> cbr) & 1) << 3);
            cbr--;

            // for sprites, pal0 is transparent
            if (palette_index == 0)
                continue;

            // skip if we already rendered a sprite!
            if (drawn_sprites[x_index]) {
                VDPi.sprite_collision = true;
                continue;
            }

            // keep track of this sprite already being rendered
            drawn_sprites[x_index] = true;

            // sprite cram index is the upper 16-bytes!
            //vdp_write_pixel(sms, VDP.colour[palette_index + 16], region.pixelx + x_index, region.pixely);
            SMSPlot(x_index-1, lny, palette_index + 16);

        }
    }
}
*/


static void vdp_render_sprites() {
    const uint8_t line = VDPi.vcount;
    const uint16_t lny = line + dpv_offset_y;

    const uint16_t attr_addr = vdp_get_sprite_attribute_base_addr(&sms);
    const uint16_t pattern_select = vdp_get_sprite_pattern_select(&sms) ? 256 : 0;

    // bit 3 of reg0: if set, sprites shift left by 8 pixels (x offset -8)
    const int8_t sprite_x_offset = (VDPi.registers[0x0] & (1 << 3)) ? -8 : 0;

    // bit 1 of reg1: affects pattern index masking
    const bool mask_pattern_bit0 = (VDPi.registers[0x1] & (1 << 1)) != 0;

    const struct SpriteEntries sprites = vdp_parse_sprites(&sms);

    // Track pixels already drawn by sprites to detect collisions
    bool drawn_sprites[NTSC_ACTIVE_DISPLAY_HORIZONTAL] = {0};

    for (uint8_t i = 0; i < sprites.count; ++i) {
        const struct SpriteEntry *sprite = &sprites.entry[i];

        // Sprite X position with possible -8 offset
        const int16_t sprite_x = vram[attr_addr + sprite->xn_index + 0] + sprite_x_offset;

        // Sprite pattern index plus pattern select offset
        uint16_t pattern_index = vram[attr_addr + sprite->xn_index + 1] + pattern_select;
        if (mask_pattern_bit0) {
            pattern_index &= ~0x1;
        }
        pattern_index *= 32;

        // Add vertical offset for sprite line relative to current scanline
        pattern_index += (line - sprite->y) * 4;

        // Read the 4 bitplanes for this sprite line
        const uint8_t bit_plane0 = vram[pattern_index + 0];
        const uint8_t bit_plane1 = vram[pattern_index + 1];
        const uint8_t bit_plane2 = vram[pattern_index + 2];
        const uint8_t bit_plane3 = vram[pattern_index + 3];

        uint8_t cbr = 7;
        int16_t x_index = sprite_x + dpv_offset_x;

        for (uint8_t x = 0; x < 8; ++x) {
            x_index++;
            if (x_index < 0) continue;
            if (x_index >= 256) break;

            // Skip pixels where BG has priority
            if (prio.array[x_index]) continue;

            // Compose palette index from bitplanes for pixel bit cbr
            uint8_t palette_index = ((bit_plane0 >> cbr) & 1)
                                    | (((bit_plane1 >> cbr) & 1) << 1)
                                    | (((bit_plane2 >> cbr) & 1) << 2)
                                    | (((bit_plane3 >> cbr) & 1) << 3);

            cbr--;

            // Transparent palette (0) skips pixel
            if (palette_index == 0) continue;

            // Collision detection: skip if already drawn, set collision flag
            if (drawn_sprites[x_index]) {
                VDPi.sprite_collision = true;
                continue;
            }

            drawn_sprites[x_index] = true;

            // Palette offset for sprites is +16
            SMSPlot(x_index - 1, lny, palette_index + 16);
        }
    }
}


//extern unsigned long CLUT[];

static void vdp_update_sms_colours() {
    const uint8_t overscan_index = 16; // + vdp_get_overscan_colour(sms);

    for (size_t i = 0; i < 32; i++) {
        if (sms.vdp.dirty_cram[i]) {
            uint8_t r = (sms.vdp.cram[i] >> 0) & 0x3;
            uint8_t g = (sms.vdp.cram[i] >> 2) & 0x3;
            uint8_t b = (sms.vdp.cram[i] >> 4) & 0x3;

            //r *=64;
            //g *=64;
            //b *=64;

            //CLUT[i] = ((r << 22) & 0xff0000) | ((g << 14) & 0xff00) | ((b & 0xff) << 6) | 0xff000000;
            clut[i] = (r << 22) | (g << 14) | (b << 6) | 0xff000000;

            //sms->vdp.colour[i] = sms->vdp.cram[i];//(r<<4) | (g<<2) | b;//sms->colour_callback(sms->colour_callback_user, r, g, b);
            sms.vdp.dirty_cram[i] = false;
        }
    }
}

static void vdp_update_gg_colours() {
    const uint8_t overscan_index = 16;            // + vdp_get_overscan_colour(sms);
    for (size_t i = 0; i < 64; i += 2) {
        if (sms.vdp.dirty_cram[i]) {
            // GG colours are in [----BBBBGGGGRRRR] format
            uint8_t r = (sms.vdp.cram[i + 0] >> 0) & 0xF;
            uint8_t g = (sms.vdp.cram[i + 0] >> 4) & 0xF;
            uint8_t b = (sms.vdp.cram[i + 1] >> 0) & 0xF;

            //CLUT[i >> 1] = ((r << 20) & 0xff0000) | ((g << 12) & 0xff00) | ((b << 4) & 0xff) | 0xff000000;
            clut[i >> 1] = (r << 20) | (g << 12) | (b << 4) | 0xff000000;
            sms.vdp.dirty_cram[i] = false;
        }
    }
}

bool vdp_has_interrupt(const struct SMS_Core *sms) {
    if (VDP.frame_interrupt_pending && vdp_is_vblank_irq_wanted(sms)) {
        return true;
    }

    if (VDP.line_interrupt_pending && vdp_is_line_irq_wanted(sms)) {
        return true;
    }
    return false;
}



static char tbitOf1 = 0;

#define NTSC_VBLANK_START 		193
#define NTSC_VBLANK_END   		262
#define NTSC_VCOUNT_PORT_JUMP 	213

volatile bool frameReady = false;

void sms_update_screen() {
    if (g_dialog)
        g_dialog->updateSMSScreen();

}

void sms_clear_screen(){
    if (g_dialog)
        g_dialog->clearSMSScreen();
}

void vdp_run(struct SMS_Core *sms, uint8_t cycles) {
    VDP.cycles += cycles;

    if (VDP.cycles >= NTSC_CYCLES_PER_LINE) {
        if (VDP.vcount < 192) {
            const uint8_t reg0 = VDP.registers[0x0];
            const bool reg0_bit5 = (reg0 & (1 << 5)) != 0;

            short xoff = reg0_bit5 ? 8 : 0;

            // Use stretch_wide for rendering scaling (1.87 or 1.94)
            stretch_wide = (xoff == 0) ? 1.87f : 1.94f;

            const bool is_gg = SMS_is_system_type_gg(sms);

            if (is_gg) {
                dpv_offset_x = 14;
                dpv_offset_y = 27;
            } else {
                if (reg0_bit5) {
                    dpv_offset_x = -8;
                    dpv_offset_y = 0;
                } else {
                    dpv_offset_x = 0;
                    dpv_offset_y = 0;
                }
            }

            if (is_gg) {
                vdp_update_gg_colours();
            } else {
                vdp_update_sms_colours();
            }

            VDP.line_counter--;

            if (VDP.line_counter == 0xFF) {
                VDP.line_interrupt_pending = true;
                VDP.line_counter = VDP.registers[0xA];
            }

            if (vdp_is_display_enabled(sms)) {
                if (vdp_is_display_active(sms)) {
                    vdp_render_background();
                    vdp_render_sprites();
                    tbitOf1 = 50;
                }
            } else {
                // Display off: simulate CRT clunkiness by clearing screen after delay
                if (tbitOf1 == 30) {
#ifndef __linux__
                    LCDClearBuffer(200);
                    SCB_CleanDCache();
                    LCDUpdateDD();
#else
                    // clear screen
                    // update graphics port
                    // TODO: DO NOT FORGET THIS PART!
                    //sms_update_screen();
                    sms_clear_screen();
#endif
                }
                if (tbitOf1 > 0) {
                    tbitOf1--;
                }
            }
        }

        VDP.cycles -= NTSC_CYCLES_PER_LINE;
        VDP.vcount++;
        VDP.vcount_port++;

        switch (VDP.vcount) {
        case NTSC_VBLANK_START:
            VDP.frame_interrupt_pending = true;
            VDP.vertical_scroll = VDP.registers[0x9];
            VDP.line_counter = VDP.registers[0xA];

            core_on_vblank();
            //LCDUpdateClutDD();
            break;

        case NTSC_VBLANK_START + 1: // 194
            if (SMS_is_spiderman_int_hack_enabled(sms) && vdp_is_vblank_irq_wanted(sms)) {
                z80_irq(sms);
            }
            break;

        case 219:
            VDP.vcount_port = NTSC_VCOUNT_PORT_JUMP;
            break;

        case NTSC_VBLANK_END:
            VDP.vcount = 0;
            VDP.vcount_port = 0;
            break;
        }
    }
}


void vdp_mark_palette_dirty(struct SMS_Core* sms){
    memset(sms->vdp.dirty_cram, true, sizeof(sms->vdp.dirty_cram));
    /*
    sms->vdp.dirty_cram_min = 0;
    if (SMS_is_system_type_gg(sms))
    {
        sms->vdp.dirty_cram_max = 64;
    }
    else
    {
        sms->vdp.dirty_cram_max = 32;
    }
    vdp_update_palette(sms);
    */
}

void vdp_init(struct SMS_Core *sms) {
    int c;
    for (c = 0; c < 256; c++)
        clut[c] = 0xff000000;
    clut[0] = 0x00000000;

    memset(&VDP, 0, sizeof(VDP));
    vram = (volatile unsigned char *)smsVRAMLocation;  // 16kb	// reset the moment location as its just been wiped
    // i think unused regs return 0xFF?
    memset(VDP.registers, 0xFF, sizeof(VDP.registers));

    VDP.registers[0x0] = 0x36;
    VDP.registers[0x1] = 0x80;
    VDP.registers[0x6] = 0xFB;

    VDP.line_counter = 0xFF;
}


