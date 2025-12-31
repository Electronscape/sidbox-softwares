//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"


//#include "cg_input.h"

#include "cg_gad_label.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_label(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_LABEL_T *b = (GAD_LABEL_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = b->h.rect.w;
    int16_t bh = b->h.rect.h;

    // face
    fill_rect_pen(ax, ay, bw, bh, b->bgColour);
    //draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, b->h.down);

    // choose what text to display
    const char *displaytext = b->text;

    const int16_t char_w = 8, char_h = 16;


    gfx_setcolour(PEN_WIN_TITLE);
    ui_draw_text816(ax, ay, (const unsigned char*)displaytext);
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------



// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setLabelText(SBControlHandle h, const char *text){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return 1;
    if (g->gadgetType != GAD_LABEL) return 2;
    if (!g->gadget) return 3;
    GAD_LABEL_T *lbl = (GAD_LABEL_T*)g->gadget;

    if (text){
        int i = 0;
        for (; text[i] && i < (DEF_GADGET_TEXT_SIZE - 1); i++) lbl->text[i] = text[i];
        lbl->text[i] = '\0';
    } else {
        lbl->text[0] = '\0';
    }
    return 0;
}

