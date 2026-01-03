//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"


//#include "cg_input.h"

#include "cg_gad_label.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_label(const sbx_window_t *w, const GADGET_BASE_T *g)
{
    if (!w || !g || !g->gadget) return;

    GAD_LABEL_T *b = (GAD_LABEL_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = b->h.rect.w;
    int16_t bh = b->h.rect.h;

    // face
    fill_rect_pen(ax, ay, bw, bh, b->h.BPen);
    if(b->h.flags & GAD_TOOL_INSET)
        draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);

    // choose what text to display
    const char *displaytext = b->text;

    const int16_t char_w = 8, char_h = 16;


    gfx_setcolour(b->h.FPen);
    ui_draw_text816(ax, ay, (const unsigned char*)displaytext);
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------



// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setLabelText(CGGadgetHandle h, const char *text)
{
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return 1;
    if (g->gadgetType != GAD_LABEL) return 2;
    if (!g->gadget) return 3;
    GAD_LABEL_T *lbl = (GAD_LABEL_T*)g->gadget;
    int16_t strc = 0;   // max chars in any line
    int16_t lcnt = 0;   // chars in current line


    if (text) {
        int i = 0;
        for (; text[i] && i < (DEF_GADGET_TEXT_SIZE - 1); i++) {
            char c = text[i];
            lbl->text[i] = c;

            if (c == '\n') {
                if (lcnt > strc) strc = lcnt;
                lcnt = 0;
            } else {
                lcnt++;
            }
        }
        lbl->text[i] = '\0';

        // handle last line (if text didn't end with '\n')
        if (lcnt > strc) strc = lcnt;
    } else {
        lbl->text[0] = '\0';
        strc = 0;
    }
    lbl->h.rect.w = (int16_t)(strc * 8);
    return 0;
}

uint32_t SBOS_setLabelColour(CGGadgetHandle h, int16_t FPen, int16_t BPen)
{
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return 1;
    if (g->gadgetType != GAD_LABEL) return 2;
    if (!g->gadget) return 3;
    GAD_LABEL_T *lbl = (GAD_LABEL_T*)g->gadget;

    if(FPen >=0) lbl->h.FPen = (uint8_t)FPen; // set only if NOT -1 (no change)
    if(BPen >=0) lbl->h.BPen = (uint8_t)BPen; // set only if NOT -1 (no change)

    return 0;
}

