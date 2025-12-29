//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_radio.h"


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

    fill_rect_pen(box_x, box_y, box, box, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(box_x, box_y, box, box, WIN_BEVEL_H, WIN_BEVEL_L, r->h.down);

    // “radio dot” when checked
    if (r->checked) {
        gfx_setcolour(WIN_BEVEL_L);

        // cheap filled center blob (looks fine at 8x16 font scale)
        int16_t cx = (int16_t)(box_x + 6);
        int16_t cy = (int16_t)(box_y + 6);

        ui_ppixel(cx, cy);
        ui_ppixel((int16_t)(cx+1), cy);
        ui_ppixel(cx, (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+1), (int16_t)(cy+1));

        ui_ppixel((int16_t)(cx-1), cy);
        ui_ppixel((int16_t)(cx+2), cy);
        ui_ppixel(cx, (int16_t)(cy-1));
        ui_ppixel(cx, (int16_t)(cy+2));
    }

    // label
    if (r->text[0]) {
        gfx_setcolour(WIN_TITLE_PEN);
        int16_t tx = (int16_t)(box_x + box + 6);
        int16_t ty = (int16_t)(ay + (r->h.rect.h - 16) / 2);
        if (r->h.down) { tx++; ty++; }
        ui_draw_text816(tx, ty, (const unsigned char*)r->text);
    }
}
