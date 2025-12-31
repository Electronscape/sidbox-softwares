#include <stdio.h>
//#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
//#include "cg_gadgets.h"


//#include "cg_input.h"

#include "cg_gad_gridselect.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
// insider code, tight loops!! (tight being the key feature here)
static inline int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int16_t gridselect_cell_at_abs(const sbx_window_t *w, const GAD_GRIDSELECT_T *gs, int16_t mx, int16_t my)
{
    // Absolute gadget bounds
    int16_t ax = (int16_t)(w->clientrect.x + gs->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + gs->h.rect.y);
    int16_t bw = gs->h.rect.w;
    int16_t bh = gs->h.rect.h;

    // Inner grid area (inside outer frame)
    int16_t inset = 1;     // match your bevel thickness

    if(gs->h.flags & GAD_TOOL_NOBORDER) inset = 0;
    int16_t gx = (int16_t)(ax + inset);
    int16_t gy = (int16_t)(ay + inset);
    int16_t gw = (int16_t)(bw - inset * 2);
    int16_t gh = (int16_t)(bh - inset * 2);

    if (gw <= 0 || gh <= 0) return -1;
    if (gs->cells_x == 0 || gs->cells_y == 0) return -1;

    // Outside grid?
    if (mx < gx || my < gy || mx >= gx + gw || my >= gy + gh) return -1;

    int16_t cw = (int16_t)(gw / gs->cells_x);
    int16_t ch = (int16_t)(gh / gs->cells_y);
    if (cw <= 0 || ch <= 0) return -1;

    int16_t lx = (int16_t)(mx - gx);
    int16_t ly = (int16_t)(my - gy);

    int16_t col = (int16_t)(lx / cw);
    int16_t row = (int16_t)(ly / ch);

    if (col < 0 || row < 0) return -1;
    if (col >= gs->cells_x || row >= gs->cells_y) return -1;

    return (int16_t)(row * gs->cells_x + col);
}

static inline uint8_t text_len3(const char t[4]) {
    uint8_t n = 0;
    while (n < 3 && t[n]) n++;
    return n;
}


void draw_gridselect(const sbx_window_t *w, const GADGET_BASE_T *g)
{
    if (!w || !g || !g->gadget) return;

    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;
    if (!gs->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + gs->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + gs->h.rect.y);
    int16_t bw = gs->h.rect.w;
    int16_t bh = gs->h.rect.h;

    const int16_t FONT_W = 8;
    const int16_t FONT_H = 16;


    // Face
    fill_rect_pen(ax, ay, bw, bh, w->backColour);

    // Outer frame bevel (your existing frame)
    draw_bevel(ax, ay, bw, bh, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, !!(gs->h.flags & GAD_TOOL_INSET));

    // Inner grid region
    int16_t inset = 1;

    if(gs->h.flags & GAD_TOOL_NOBORDER) inset = 0;

    int16_t gx = (int16_t)(ax + inset);
    int16_t gy = (int16_t)(ay + inset);
    int16_t gw = (int16_t)(bw - inset*2);
    int16_t gh = (int16_t)(bh - inset*2);

    if (gw <= 0 || gh <= 0) return;
    if (gs->cells_x == 0 || gs->cells_y == 0) return;

    int16_t cw = (int16_t)(gw / gs->cells_x);
    int16_t ch = (int16_t)(gh / gs->cells_y);
    if (cw <= 0 || ch <= 0) return;

    // Draw each cell

    for (int16_t row = 0; row < gs->cells_y; row++) {
        for (int16_t col = 0; col < gs->cells_x; col++) {

            int16_t idx = (int16_t)(row * gs->cells_x + col);

            // Let last column/row absorb remainder so no dead strip at edges
            int16_t x = (int16_t)(gx + col * cw);
            int16_t y = (int16_t)(gy + row * ch);
            int16_t wcell = (col == gs->cells_x - 1) ? (int16_t)(gx + gw - x) : cw;
            int16_t hcell = (row == gs->cells_y - 1) ? (int16_t)(gy + gh - y) : ch;

            // Cell background (you can replace with per-cell color later)
            fill_rect_pen(x, y, wcell, hcell, gs->cellColour[idx]);

            int16_t eff = (gs->h.down && gs->preview_idx >= 0) ? gs->preview_idx : gs->selected_idx;
            uint8_t sel = (idx == eff);

            // Cell bevel: selected looks "pressed"
            // Use your bevel pens; tweak if you want stronger hover.

            // Text (up to 3 chars)
            const char *txt = (char *)gs->cellText[idx];          // assuming cellText[256][4] or [5]
            uint8_t n = text_len3((char *)gs->cellText[idx]);
            if (n) {
                int16_t tw = (int16_t)(n * FONT_W);

                // center in cell
                int16_t tx = (int16_t)(x + (wcell - tw) / 2);
                int16_t ty = (int16_t)(y + (hcell - FONT_H) / 2);

                // “pressed” nudge for selected cell
                if (sel) { tx++; ty++; }

                if(gs->flags & GAD_GRIDSEL_TEXT_INVERT){
                    uint8_t col = gs->cellColour[idx];
                    gfx_setcolour(~col);
                } else
                    gfx_setcolour(16);

                ui_draw_text816(tx, ty, (const unsigned char*)txt);
            }


            if (sel) {
                draw_bevel(x, y, wcell, hcell, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);
            }
        }
    }

    if (!gs->h.enabled) {
        draw_disabled_dots(ax + 1, ay + 1, bw-2, bh-2);
    }
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------
uint32_t onMouseDownCaptureGridSelect(sbx_window_t *win, GADGET_BASE_T *g, int16_t *mx, int16_t *my) {
    if (!win || !g || !g->gadget || !mx || !my) return 1;
    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;
    if (!gs->h.visible) return 1;

    int16_t idx = gridselect_cell_at_abs(win, gs, *mx, *my);

    gs->down_idx    = idx;
    gs->preview_idx = idx;

    return 0;
}

uint32_t onMouseMoveGridSelect(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my) {
    (void)evt;
    if (!win || !g || !g->gadget || !mx || !my) return 0;

    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;
    if (!gs->h.visible) return 0;

    int16_t idx = gridselect_cell_at_abs(win, gs, *mx, *my);

    if (gs->h.down) {
        if (idx != gs->preview_idx) {
            gs->preview_idx = idx;   // can become -1 when leaving grid
            return 1;                // repaint
        }
        return 0;
    }
    return 0;
}

uint32_t onMouseReleaseGridSelect(GADGET_BASE_T *g, int16_t *mx, int16_t *my) {
    if (!g || !g->gadget || !mx || !my) return 0;

    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;
    if (!gs->h.visible) return 0;

    SBXWindowId wid = SBOS_getWindowByGadget(g);
    sbx_window_t *win = SBOS_getWindow(wid);
    if (!win) return 0;

    int16_t idx = gridselect_cell_at_abs(win, gs, *mx, *my);
    int16_t commit = gs->preview_idx;
    if (commit < 0) commit = idx;

    gs->down_idx = -1;
    gs->preview_idx = -1;

    if (commit >= 0) {
        int16_t old = gs->selected_idx;
        gs->selected_idx = commit;
        // launch any callbacks attached here
        // TODO: Do the actual feature

        return (old != gs->selected_idx) ? 1u : 0u; // repaint if changed
    }

    return 0;
}


// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setCellText(CGGadgetHandle h, const char *text, int16_t cellindex)
{
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return 1;
    if (g->gadgetType != GAD_GRIDSELECT) return 2;
    if (!g->gadget) return 3;

    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;

    uint16_t cellcount = (uint16_t)gs->cells_x * (uint16_t)gs->cells_y;

    if (cellindex < 0 || cellindex >= cellcount)
        return 4; // out of range

    for (int i = 0; i < 3 && text[i]; i++)
        gs->cellText[cellindex][i] = text[i];
    gs->cellText[cellindex][3] = '\0';


    return 0;
}


uint32_t SBOS_setCellColour(CGGadgetHandle h, const uint8_t colourIndex, int16_t cellindex){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return 1;
    if (g->gadgetType != GAD_GRIDSELECT) return 2;
    if (!g->gadget) return 3;
    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g->gadget;

    uint16_t cellcount = (uint16_t)gs->cells_x * (uint16_t)gs->cells_y;
    if (cellindex < 0 || cellindex >= cellcount)
        return 4;


    gs->cellColour[cellindex] = colourIndex;
    return 0; // success
}
