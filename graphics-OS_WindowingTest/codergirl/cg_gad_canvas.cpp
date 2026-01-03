#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
#include "cg_glyphs.h"
#include "cg_gadgets.h"


#include "cg_input.h"

#include "cg_gad_canvas.h"


void internal_draw_line(int16_t x, int16_t y, int16_t x2, int16_t y2){

}

// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_canvas(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_CANVAS_T *b = (GAD_CANVAS_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = ax + b->h.rect.w;
    int16_t bh = ay + b->h.rect.h;

    // face
    //fill_rect_pen(ax, ay, bw, bh, 16);
    gfx_setcolour(b->h.FPen);
    switch(b->drawtype){
        case CNV_LINE:
            ui_draw_line(ax, ay, bw, bh);
            break;
    }
}
