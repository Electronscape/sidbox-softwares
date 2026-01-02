#include <stdio.h>
#include <string.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"


//#include "cg_input.h"

#include "cg_gad_progbar.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
static int16_t clamp16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void draw_progbar(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_PROGBAR_T *b = (GAD_PROGBAR_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);
    int16_t bw = b->h.rect.w;
    int16_t bh = b->h.rect.h;

    if (bw <= 2 || bh <= 2) return;

    /* test area
    b->min = 0;
    b->max = 100;
    //b->value = 70;
    b->value ++;
    if(b->value>99) b->value = 0;
    */

    // --- Outer frame (matches your UI look) ---
    fill_rect_pen(ax, ay, bw, bh, w->backColour);
    draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);

    // --- Inner track (recessed channel) ---
    int16_t tx = ax + 2, ty = ay + 2;
    int16_t tw = bw - 4, th = bh - 4;
    if (tw <= 0 || th <= 0) return;

    // Track background (a little darker than face is nice)
    fill_rect_pen(tx, ty, tw, th, w->backColour); // swap to your "dark" pen if you have one
    draw_bevel(tx, ty, tw, th, PEN_WIN_BEVEL_L, PEN_WIN_BEVEL_H, 1); // inverted bevel => recessed

    // --- Compute fill width ---
    // Assumes b has: int16_t min,max,value
    int16_t range = (int16_t)(b->max - b->min);
    int16_t val   = (int16_t)(b->value - b->min);

    int16_t fillw = 0;
    if (range > 0) {
        val = clamp16(val, 0, range);
        fillw = (int16_t)(((int32_t)val * (int32_t)tw) / (int32_t)range);
        fillw = clamp16(fillw, 0, tw);
    }

    // --- Draw filled bar with its own bevel ---
    if (fillw > 0) {
        int16_t fx = tx + 1, fy = ty + 1;
        int16_t fw = fillw - 2;   // keep inside track bevel
        int16_t fh = th - 2;

        if (fw > 0 && fh > 0) {
            fill_rect_pen(fx, fy, fw, fh, PEN_WIN_BORDER_ACTIVE);
            draw_bevel(fx, fy, fw, fh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);
        }
    }

    // --- Optional percent text (if you want) ---
    // If you have a text helper, you can center it in the track.
    // Keeping it off by default to preserve "OS vibe".

    if (range > 0) {
        int16_t pct = 0;
        if (range > 0) pct = (int16_t)(((int32_t)val * 100) / range);

        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", pct);

        int16_t text_w = (int16_t)(strlen(buf) * 8);
        int16_t text_h = 16;

        // Your "stay above x position" anchor (centered home)
        int16_t base_x = (int16_t)(tx + 2);//(tw - text_w) );
        int16_t text_y = (int16_t)(ty + (th - text_h) / 2);

        // Follow the right edge of the filled part (with a tiny padding)
        int16_t fill_right = (int16_t)(tx + fillw);
        int16_t want_x = (int16_t)(fill_right - text_w - 2);

        // Only move right; never go left of base_x
        int16_t text_x = (want_x > base_x) ? want_x : base_x;

        // Clamp to track (1px padding)
        if (text_x < tx + 1) text_x = (int16_t)(tx + 1);
        if (text_y < ty + 1) text_y = (int16_t)(ty + 1);
        if (text_x + text_w > tx + tw - 1) text_x = (int16_t)(tx + tw - 1 - text_w);
        if (text_y + text_h > ty + th - 1) text_y = (int16_t)(ty + th - 1 - text_h);

        gfx_setcolour(PEN_TEXT);
        ui_draw_text816(text_x, text_y+1, (const unsigned char *)buf);

    }
}




// API INTERFACES -------------------------------------------------------------------------------------------------
void SBOS_setProgBarValue(CGGadgetHandle h, int16_t value)
{
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return ;
    if (g->gadgetType != GAD_PROGBAR) return ;
    if (!g->gadget) return ;
    GAD_PROGBAR_T *pb = (GAD_PROGBAR_T*)g->gadget;

    pb->value = value;
}

void SBOS_setProgBarMinMax(CGGadgetHandle h, int16_t newMin, int16_t newMax)
{
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return ;
    if (g->gadgetType != GAD_PROGBAR) return ;
    if (!g->gadget) return ;
    GAD_PROGBAR_T *pb = (GAD_PROGBAR_T*)g->gadget;

    if(newMin < 0) newMin = 0;
    if(newMin > 0x7fff) newMin = 0x7fff;

    if(newMax < newMin) newMax = newMin;
    if(newMax > 0x7fff) newMax = 0x7fff;
    pb->min = newMin;
    pb->max = newMax;
}

