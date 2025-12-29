//// SBX_RENDER.CPP //////


#include "stdint.h"

#include "font.h"

#include "sbapi_graphics.h"
#include "sbx_render.h"

// variables
UIClipRect g_uiclip = {0,0,0,0,0};


void ui_ppixel(int16_t x, int16_t y){
    if ((unsigned)x < 0 || (unsigned)y < 0) return;
    if ((unsigned)x >= SCR_WIDTH || (unsigned)y >= SCR_HEIGHT) return;

    if (g_uiclip.enabled) {
        if (x < g_uiclip.x0 || x >= g_uiclip.x1 || y < g_uiclip.y0 || y >= g_uiclip.y1)
            return;
    }
    sbgfx_ppixel(x, y);
}

void fill_rect_pen(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen){
    //sbgfx_drawbox(x, y, w, h, pen);
    gfx_setcolour(pen);
    for(int dx = x; dx < x + w; dx++){
        for(int dy = y; dy < y + h; dy++){
            ui_ppixel(dx, dy);
        }
    }
}

void ui_draw_glyph(int16_t x, int16_t y, uint8_t *glyph) {
    int16_t x0 = x;
    int16_t y0 = y;
    int16_t x1 = x + 16;
    int16_t y1 = y + 16;

    // Global clip
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > SCR_WIDTH)  x1 = SCR_WIDTH;
    if (y1 > SCR_HEIGHT) y1 = SCR_HEIGHT;

    // UI clip
    if (g_uiclip.enabled) {
        if (x0 < g_uiclip.x0) x0 = g_uiclip.x0;
        if (y0 < g_uiclip.y0) y0 = g_uiclip.y0;
        if (x1 > g_uiclip.x1) x1 = g_uiclip.x1;
        if (y1 > g_uiclip.y1) y1 = g_uiclip.y1;
    }

    if (x1 <= x0 || y1 <= y0)
        return;

    const int16_t yoff = y0 - y;
    uint8_t *col = glyph + (x0 - x) * 16 + yoff;

    for (int16_t dx = x0; dx < x1; dx++) {
        uint8_t *dat = col;

        for (int16_t dy = y0; dy < y1; dy++) {
            uint8_t c = *dat++;
            if (c)
                sbgfx_pixel(dx, dy, c);
        }

        col += 16; // next glyph column
    }
}



void ui_fill_dots(int16_t x, int16_t y, int16_t w, int16_t h, int16_t step){
    for (int16_t yy = 0; yy < h; yy++){
        for (int16_t xx = ((yy & 1) ? 1 : 0); xx < w; xx += step){
            ui_ppixel((int16_t)(x + xx), (int16_t)(y + yy));
        }
    }
}

void ui_draw_text816(int x, int y, const unsigned char* textptr){
    if (!textptr) return;

    int start_x = x;

    for (int i = 0; textptr[i] != '\0'; ++i) {
        unsigned char ch = textptr[i];

        if (ch == '\n') {
            x = start_x;
            y += 16;
            continue;
        }

        const uint8_t* pixeldata = DEFAULT_SYSFONT[ch];

        // 8 columns per glyph
        for (int j = 0; j < 8; ++j) {
            int xc = x + j;
            uint8_t pixdat = pixeldata[j];

            // Each bit represents a 2-pixel-tall segment
            if (pixdat & 0x01) { ui_ppixel((int16_t)xc, (int16_t)(y + 0));  ui_ppixel((int16_t)xc, (int16_t)(y + 1));  }
            if (pixdat & 0x02) { ui_ppixel((int16_t)xc, (int16_t)(y + 2));  ui_ppixel((int16_t)xc, (int16_t)(y + 3));  }
            if (pixdat & 0x04) { ui_ppixel((int16_t)xc, (int16_t)(y + 4));  ui_ppixel((int16_t)xc, (int16_t)(y + 5));  }
            if (pixdat & 0x08) { ui_ppixel((int16_t)xc, (int16_t)(y + 6));  ui_ppixel((int16_t)xc, (int16_t)(y + 7));  }
            if (pixdat & 0x10) { ui_ppixel((int16_t)xc, (int16_t)(y + 8));  ui_ppixel((int16_t)xc, (int16_t)(y + 9));  }
            if (pixdat & 0x20) { ui_ppixel((int16_t)xc, (int16_t)(y + 10)); ui_ppixel((int16_t)xc, (int16_t)(y + 11)); }
            if (pixdat & 0x40) { ui_ppixel((int16_t)xc, (int16_t)(y + 12)); ui_ppixel((int16_t)xc, (int16_t)(y + 13)); }
            if (pixdat & 0x80) { ui_ppixel((int16_t)xc, (int16_t)(y + 14)); ui_ppixel((int16_t)xc, (int16_t)(y + 15)); }
        }

        // Advance to next character (8px wide)
        x += 8;
    }
}


void ui_hlinedotted(int16_t x, int16_t y, int16_t w){
    for (int16_t i = 0; i < w; i+=2)
        ui_ppixel((int16_t)(x + i), y);
}

void ui_vlinedotted(int16_t x, int16_t y, int16_t h){
    for (int16_t i = 0; i < h; i+=2)
        ui_ppixel(x, (int16_t)(y + i));
}

void ui_hline(int16_t x, int16_t y, int16_t w){
    for (int16_t i = 0; i < w; i++)
        ui_ppixel((int16_t)(x + i), y);
}


void ui_vline(int16_t x, int16_t y, int16_t h){
    for (int16_t i = 0; i < h; i++)
        ui_ppixel(x, (int16_t)(y + i));
}


void ui_fillrect(int16_t x, int16_t y, int16_t w, int16_t h){
    for (int16_t yy = 0; yy < h; yy++)
        ui_hline(x, (int16_t)(y + yy), w);
}

void draw_bevel_rect(int16_t x, int16_t y, int16_t w, int16_t h){
    // light (top + left)
    gfx_setcolour(WIN_BEVEL_H);
    ui_hline(x, y, w);
    ui_vline(x, y, h);

    // dark (bottom + right)
    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(x, y + h - 1, w);
    ui_vline(x + w - 1, y, h);
}

void draw_bevel(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen_hi, uint16_t pen_lo, uint8_t pressed){
    if (w <= 0 || h <= 0) return;

    uint16_t tl = pressed ? pen_lo : pen_hi;
    uint16_t br = pressed ? pen_hi : pen_lo;

    // top + left
    gfx_setcolour(tl);
    ui_hline(x, y, w);
    ui_vline(x, y, h);

    // bottom + right
    gfx_setcolour(br);
    ui_hline(x+1, (int16_t)(y + h - 1), w-1);
    ui_vline((int16_t)(x + w - 1), y, h);
}



void glyph_close_x(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t bx = x + 6;
    int16_t by = y + 6;
    int16_t bw = w - 12;
    int16_t bh = h - 12;
    if (bw <= 0 || bh <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_vline(bx, by, bh);
    ui_hline(bx, by, bw);


    gfx_setcolour(WIN_BEVEL_H);
    ui_hline(bx+1, by + bh - 1, bw);
    ui_vline(bx + bw, by, bh);
}


void glyph_resize_grip(int16_t x, int16_t y, int16_t w, int16_t h){
    // Draw 3 diagonal dotted lines (classic gripper vibe)
    gfx_setcolour(WIN_BEVEL_L);

    // bottom-right diagonal-ish marks
    ui_hline((int16_t)(x + w - 4), (int16_t)(y + h - 2), 3);
    ui_hline((int16_t)(x + w - 7), (int16_t)(y + h - 5), 4);
    ui_hline((int16_t)(x + w - 10), (int16_t)(y + h - 8), 5);

    gfx_setcolour(WIN_BEVEL_H);
    ui_hline((int16_t)(x + w - 3), (int16_t)(y + h - 3), 2);
    ui_hline((int16_t)(x + w - 6), (int16_t)(y + h - 6), 3);
    ui_hline((int16_t)(x + w - 9), (int16_t)(y + h - 9), 4);
}



void glyph_minimise(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t lx = x + 6;
    int16_t ly = y + h - 5;
    int16_t lw = w - 12;
    if (lw <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(lx, ly, lw);
}

void glyph_max_box(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t bx = x + 5;
    int16_t by = y + 5;
    int16_t bw = w - 10;
    int16_t bh = h - 10;
    if (bw <= 0 || bh <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(bx, by, bw);
    ui_hline(bx, by+1, bw);
    ui_hline(bx, by + bh - 1, bw);
    ui_vline(bx, by, bh);
    ui_vline(bx + bw - 1, by, bh);
}

void glyph_zorder(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t ix = x + 5;
    int16_t iy = y + 5;
    int16_t iw = w - 10;
    if (iw <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(ix, iy, iw);
    ui_hline(ix + 2, iy + 3, iw - 2);
    ui_hline(ix + 4, iy + 6, iw - 4);
}

static void gfx_fillrect(int16_t x, int16_t y, int16_t w, int16_t h){
    if (w <= 0 || h <= 0) return;
    for (int16_t yy = 0; yy < h; yy++) {
        ui_hline(x, y + yy, w);
    }
}

void draw_rect_outline_thick(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, uint16_t pen){
    gfx_setcolour(pen);
    for (int16_t i = 0; i < t; i++) {
        int16_t xx = x + i, yy = y + i;
        int16_t ww = w - 2*i, hh = h - 2*i;
        if (ww <= 0 || hh <= 0) break;

        ui_hline(xx, yy, ww);
        ui_hline(xx, yy + hh - 1, ww);
        ui_vline(xx, yy, hh);
        ui_vline(xx + ww - 1, yy, hh);
    }
}


void draw_title_button(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fill_pen, uint8_t pressed){
    fill_rect_pen(x, y, w, h, fill_pen);

    // pressed => invert bevel
    draw_bevel(x, y, w, h, WIN_BEVEL_H, WIN_BEVEL_L, pressed);
}



///////////// the window call to paint though will have to be on the sbx_windowex.c

