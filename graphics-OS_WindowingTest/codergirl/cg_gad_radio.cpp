//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_radio.h"

// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_radio(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_RADIO_T *r = (GAD_RADIO_T*)g->gadget;
    if (!r->h.visible) return;

    const int16_t ax = (int16_t)(w->clientrect.x + r->h.rect.x);
    const int16_t ay = (int16_t)(w->clientrect.y + r->h.rect.y);

    // circle-ish box (16x16 like checkbox)
    const int16_t box = 16;
    int16_t box_x = ax;
    int16_t box_y = (int16_t)(ay + (r->h.rect.h - box) / 2);

    fill_rect_pen(box_x, box_y, r->h.rect.w, box, r->h.BPen);
    fill_rect_pen(box_x + box, box_y, r->h.rect.w, box, w->backColour);



    draw_bevel(box_x, box_y, box, box, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, r->h.down);

    // “radio dot” when checked
    if (r->checked) {
        //gfx_setcolour(PEN_SELECTED);

        // cheap filled center blob (looks fine at 8x16 font scale)
        int16_t cx = (int16_t)(box_x + 4);
        int16_t cy = (int16_t)(box_y + 4);

        fill_rect_pen(cx, cy, box-8, box-8, r->h.HPen);
        //ui_ppixel(cx, cy);
        //ui_ppixel((int16_t)(cx+1), cy);
        //ui_ppixel(cx, (int16_t)(cy+1));
        //ui_ppixel((int16_t)(cx+1), (int16_t)(cy+1));

        //ui_ppixel((int16_t)(cx-1), cy);
        //ui_ppixel((int16_t)(cx+2), cy);
        //ui_ppixel(cx, (int16_t)(cy-1));
        //ui_ppixel(cx, (int16_t)(cy+2));
    }

    // label
    if (r->text[0]) {
        gfx_setcolour(PEN_TEXT);
        int16_t tx = (int16_t)(box_x + box + 6);
        int16_t ty = (int16_t)(ay + (r->h.rect.h - 16) / 2);
        if (r->h.down) { tx++; ty++; }
        ui_draw_text816(tx, ty, (const unsigned char*)r->text);
    }

    if (!r->h.enabled) {
        draw_disabled_dots(box_x + 1, box_y + 1, r->h.rect.w-2, box);
    }
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------

// API INTERFACES -------------------------------------------------------------------------------------------------
