//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_checkbox.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_checkbox(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_CHECKBOX_T *c = (GAD_CHECKBOX_T*)g->gadget;
    if (!c->h.visible) return;

    const int16_t ax = (int16_t)(w->clientrect.x + c->h.rect.x);
    const int16_t ay = (int16_t)(w->clientrect.y + c->h.rect.y);

    // background for the whole control rect (optional, but consistent look)
    // If you want it transparent, remove this.
    //fill_rect_pen(ax, ay, c->h.rect.w, c->h.rect.h, WIN_BG_PEN);

    // checkbox square size: 16x16 centered vertically in control height
    const int16_t box = 16;
    int16_t box_x = ax;
    int16_t box_y = (int16_t)(ay + (c->h.rect.h - box) / 2);

    // face + bevel
    fill_rect_pen(box_x, box_y, box, box, PEN_CHECKBOX_FACE);
    draw_bevel(box_x, box_y, box, box, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, c->h.down);

    // check mark
    if (c->checked) {
        gfx_setcolour(PEN_WIN_BEVEL_L);

        // simple “tick” using pixels/lines (cheap + readable)
        // adjust offsets for your font/pixel vibe
        int16_t cx = (int16_t)(box_x + 4);
        int16_t cy = (int16_t)(box_y + 8);

        // down-left to center
        ui_ppixel(cx,     cy);
        ui_ppixel((int16_t)(cx+1), (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+2), (int16_t)(cy+2));

        // center to up-right
        ui_ppixel((int16_t)(cx+3), (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+4), cy);
        ui_ppixel((int16_t)(cx+5), (int16_t)(cy-1));
        ui_ppixel((int16_t)(cx+6), (int16_t)(cy-2));
    }

    // label text (optional)
    if (c->text[0]) {
        gfx_setcolour(PEN_WIN_TITLE);

        // text baseline centered-ish
        int16_t tx = (int16_t)(box_x + box + 6);
        int16_t ty = (int16_t)(ay + (c->h.rect.h - 16) / 2);

        if (c->h.down) { tx++; ty++; }

        ui_draw_text816(tx, ty, (const unsigned char*)c->text);
    }
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------

// API INTERFACES -------------------------------------------------------------------------------------------------
