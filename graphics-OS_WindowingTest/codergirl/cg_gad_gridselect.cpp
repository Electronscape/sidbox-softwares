//#include <stdio.h>
//#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
//#include "cg_gadgets.h"


//#include "cg_input.h"

#include "cg_gad_gridselect.h"


void draw_gridselect(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_GRIDSELECT_T *b = (GAD_GRIDSELECT_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = b->h.rect.w;
    int16_t bh = b->h.rect.h;

    // face
    fill_rect_pen(ax, ay, bw, bh, PEN_BUTTON_FACE);
    //draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, b->h.down);
    draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 0);    // is the frame

}
