#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_button.h"



void draw_button(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_BUTTON_T *b = (GAD_BUTTON_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = b->h.rect.w;
    int16_t bh = b->h.rect.h;

    // face
    fill_rect_pen(ax, ay, bw, bh, PEN_BUTTON_FACE);
    draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, b->h.down);

    // choose what text to display
    const char *displaytext = b->text;
    if ((b->h.flags & GAD_TOOL_CYCLEBUTTON) && b->max_options > 0 && b->options[b->current_option]) {
        displaytext = b->options[b->current_option];
    }

    const int16_t char_w = 8, char_h = 16;

    // ---- cycle glyph area (left) ----
    int16_t leftPad = 0;
    if (b->h.flags & GAD_TOOL_CYCLEBUTTON) {
        // reserve one character + a little breathing room
        leftPad = (int16_t)(char_w + 16);

        // draw '@' glyph on the left, vertically centered
        int16_t gx = (int16_t)(ax + 4);
        int16_t gy = (int16_t)(ay + (bh - char_h) / 2);

        gfx_setcolour(PEN_WIN_BEVEL_L);
        ui_vline(gx+22, gy-2, bh-6);

        gfx_setcolour(PEN_WIN_BEVEL_H);
        ui_vline(gx+23, gy-2, bh-6);
        sbgfx_glyph(gx+2, gy, glyph_cycle);
    }

    // love this! //
    // ---- centered text, but centered within the "remaining" width ----
    int16_t len = 0;
    while (displaytext[len] && len < (DEF_GADGET_TEXT_SIZE - 1)) len++;

    int16_t text_w = (int16_t)(len * char_w);

    // available region for centering text (excluding glyph space)
    int16_t avail_x = (int16_t)(ax + leftPad);
    int16_t avail_w = (int16_t)(bw - leftPad);
    if (avail_w < 0) avail_w = 0;

    int16_t tx = (int16_t)(avail_x + (avail_w - text_w) / 2);
    int16_t ty = (int16_t)(ay + (bh - char_h) / 2);

    if (b->h.down) { tx++; ty++; }

    gfx_setcolour(PEN_WIN_TITLE);
    ui_draw_text816(tx, ty, (const unsigned char*)displaytext);
}

