//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_listbox.h"


// INTERNALS ------------------------------------------------------------------------------------------------------
void draw_listbox(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->h.visible) return;

    // Absolute rect in screen coords
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    int16_t aw = lb->h.rect.w;
    int16_t ah = lb->h.rect.h;
    if (aw <= 0 || ah <= 0) return;

    // Defaults if caller left them 0
    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_x = (lb->padding_x > 0) ? lb->padding_x : 4;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    // Outer frame


    fill_rect_pen(ax, ay, aw, ah, PEN_WIN_BG);
    if(!(lb->h.flags & GAD_TOOL_NOBORDER)) {
        //fill_rect_pen(ax, ay, aw, ah, PEN_WIN_BORDER_INACTIVE);
        if(lb->h.flags & GAD_TOOL_INSET)
            draw_bevel(ax, ay, aw, ah, PEN_WIN_BEVEL_L, PEN_WIN_BEVEL_H, 0);
        else
            draw_bevel(ax, ay, aw, ah, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 0);
    }

    // Inner client area (inset)

    int16_t ix = (int16_t)(ax + 1);
    int16_t iy = (int16_t)(ay + 1);
    int16_t iw = (int16_t)(aw - 2);
    int16_t ih = (int16_t)(ah);
    if (iw <= 0 || ih <= 0) return;


    //fill_rect_pen(ix, iy, iw, ih, PEN_WIN_BG);


    // No model? still draw empty box
    ItemLists_t *list = lb->items;
    if (!list) return;

    // Compute visible rows
    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return;

    int16_t rows = (int16_t)(usable_h / row_h);
    if (rows <= 0) return;

    // Clamp top so we don't scroll into the void
    int16_t count = (int16_t)list->count;
    int16_t maxTop = (int16_t)(count - rows);
    if (maxTop < 0) maxTop = 0;

    if(lb->selecting){
        if (lb->sel < lb->top) lb->top = lb->sel;
        if (lb->sel >= lb->top + rows) lb->top = (int16_t)(lb->sel - rows + 1);

    }
    if (lb->top < 0) lb->top = 0;
    if (lb->top > maxTop) lb->top = maxTop;



    // How many chars fit per row (8x16 font)
    const int16_t char_w = 8;
    int16_t max_chars = (int16_t)((iw - pad_x * 2) / char_w);
    if (max_chars < 0) max_chars = 0;
    if (max_chars > (DEF_GADGET_TEXT_SIZE - 1)) max_chars = (DEF_GADGET_TEXT_SIZE - 1);

    // Draw rows

    //gfx_setcolour(WIN_TITLE_PEN);

    int16_t tx;
    int16_t ty;

    for (int16_t r = 0; r < rows; r++){
        int16_t idx = (int16_t)(lb->top + r);
        if (idx >= count) break;

        int16_t ry = (int16_t)(iy + pad_y + r * row_h);

        uint8_t selected = (idx == lb->sel);
        // Selected highlight
        if (selected) {
            fill_rect_pen(ix, ry-2, iw, row_h+2, PEN_SELECTED);

            // Focus rectangle inside the row (clipped by listbox clip already)
            //gfx_setcolour(WIN_BEVEL_L);
            //ui_hline((int16_t)(ix), (int16_t)(ry-2), (int16_t)(iw));
            //ui_hline((int16_t)(ix), (int16_t)(ry+row_h), (int16_t)(iw));

            gfx_setcolour(PEN_SELECTED_INV);
        } else {
            gfx_setcolour(PEN_TEXT);

        }

        const char *s = listitem_get(list, (uint16_t)idx);
        if (!s) s = "";


        // Truncate to visible width (cheap + cheerful)
        char tmp[DEF_GADGET_TEXT_SIZE];
        int16_t n = 0;
        while (s[n] && n < max_chars) { tmp[n] = s[n]; n++; }
        tmp[n] = 0;

        tx = (int16_t)(ix + pad_x);
        ty = ry; // row_h assumed 16; if you later change row_h, you can center text

        // If you want text vertically centered when row_h != 16:
        // ty = (int16_t)(ry + (row_h - 16) / 2);

        //gfx_setcolour(selected ? WIN_CLIENT_PEN : WIN_TITLE_PEN);
        //ui_draw_text816(tx, ty, (const unsigned char*)tmp);


        ui_draw_text816(tx, ty, (const unsigned char*)tmp);
    }
}

static void listbox_tidyup(GAD_LISTBOX_T *lb){
    int count = (lb && lb->items) ? (int)lb->items->count : 0;

    if (lb->sel >= count) lb->sel = (count ? (int16_t)(count - 1) : (int16_t)-1);
    if (lb->sel < -1) lb->sel = -1;

    if (lb->top < 0) lb->top = 0;
}

static int16_t listbox_index_from_mouse(const sbx_window_t *w, const GAD_LISTBOX_T *lb, int16_t mx, int16_t my, int16_t *out_rows_visible)
{
    if (!w || !lb || !lb->items) return -1;

    ItemLists_t *list = lb->items;

    // Gadget absolute rect (screen coords)
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    int16_t aw = lb->h.rect.w;
    int16_t ah = lb->h.rect.h;
    if (aw <= 0 || ah <= 0) return -1;

    // Inner matches draw_listbox inset
    int16_t ix = (int16_t)(ax + 2);
    int16_t iy = (int16_t)(ay + 2);
    int16_t iw = (int16_t)(aw - 4);
    int16_t ih = (int16_t)(ah);
    if (iw <= 0 || ih <= 0) return -1;

    // Defaults
    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    // Visible rows (match your draw fix: 2px less sensitive)
    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return -1;

    int16_t rows = (int16_t)(usable_h / row_h);
    if (rows <= 0) return -1;

    if (out_rows_visible) *out_rows_visible = rows;

    int16_t count = (int16_t)list->count;
    if (count <= 0) return -1;

    // Mouse -> local y inside the list rows band
    int16_t ly = (int16_t)(my - iy - pad_y);

    // Clamp row when mouse is above/below the band (this is the key change)
    int16_t row;
    if (ly < 0) {
        row = 0;
    } else {
        int16_t band_h = (int16_t)(rows * row_h);
        if (ly >= band_h) row = (int16_t)(rows - 1);
        else              row = (int16_t)(ly / row_h);
    }

    // Convert to model index and clamp
    int16_t idx = (int16_t)(lb->top + row);
    if (idx < 0) idx = 0;
    if (idx >= count) idx = (int16_t)(count - 1);

    return idx;
}


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------
uint32_t onMouseDownCaptureListBox(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    int16_t rows = 0;
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);
    if (idx < 0) return 0; // clicked inside gadget but not on a row: ignore

    ItemLists_t *list = lb->items;

    //listboxSelecting = true;
    lb->selecting = true;
    if (lb->sel != idx) {
        lb->sel = idx;

        // keep selection visible (nice UX)
        if (lb->sel < lb->top) lb->top = lb->sel;
        if (lb->sel >= lb->top + rows) lb->top = (int16_t)(lb->sel - rows + 1);

        SBOS_paintAllWindows();
    }

    return 0;
}

uint32_t onMouseMoveListBox(sbx_window_t *w, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my)
{
    (void)evt;
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    ItemLists_t *list = lb->items;

    // rows = visible rows in the box (from your helper)
    int16_t rows = 0;

    // First pass: idx might be -1 if mouse is outside the row band — that's OK now.
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);

    // Compute the same inner band to detect “outside above/below”
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    int16_t ah = lb->h.rect.h;

    int16_t iy = (int16_t)(ay + 2);
    int16_t ih = (int16_t)(ah - 4);

    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return 0;

    int16_t band_h = (int16_t)(rows * row_h);

    // Mouse position relative to first row
    int16_t ly = (int16_t)(*my - iy - pad_y);

    // Auto-scroll when dragging outside above/below
    int count = (int)list->count;
    int maxTop = count - (int)rows;
    if (maxTop < 0) maxTop = 0;

    uint8_t changed = 0;

    if (ly < 0) {
        if (lb->top > 0) { lb->top--; changed = 1; }
    } else if (ly >= band_h) {
        if (lb->top < maxTop) { lb->top++; changed = 1; }
    }

    // Clamp top (so recomputing idx is stable)
    if (lb->top < 0) lb->top = 0;
    if (lb->top > maxTop) lb->top = (int16_t)maxTop;

    // Recompute idx AFTER top possibly changed (fixes drag jitter)
    idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);

    // If still outside, snap idx to nearest visible row
    if (count <= 0) return 0;

    if (idx < 0) {
        if (ly < 0) {
            idx = lb->top; // above -> first visible
        } else {
            idx = (int16_t)(lb->top + rows - 1); // below -> last visible
        }
        if (idx < 0) idx = 0;
        if (idx >= count) idx = (int16_t)(count - 1);
    }

    // Update selection
    if (lb->sel != idx) {
        lb->sel = idx;
        changed = 1;
    }

    // Keep selection visible (even if mouse not outside)
    if(lb->selecting){
        if (lb->sel < lb->top) { lb->top = lb->sel; changed = 1; }
        if (lb->sel >= lb->top + rows) { lb->top = (int16_t)(lb->sel - rows + 1); changed = 1; }
        // Clamp top again after adjustments
        if (lb->top < 0) lb->top = 0;
        if (lb->top > maxTop) lb->top = (int16_t)maxTop;
        // Sanity clamp selection in case model changed mid-drag
        listbox_tidyup(lb);

    }



    if (changed) SBOS_paintAllWindows();
    return 0;
}

uint32_t onMouseReleaseListBox(GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    (void)mx; (void)my;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*) g->gadget;
    if (!lb) return 1;   // panic

    lb->selecting = false;
    return 0;   // all good 0 as in 0k :)
}


// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setListbox_top(GADGET_BASE_T *g, int top){
    if (!g || !g->gadget) return 0;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    ItemLists_t *items = lb->items;

    int count = items ? (int)items->count : 0;
    int max_top = count - (int)lb->visablerows;
    if (max_top < 0) max_top = 0;

    if (top < 0) top = 0;
    if (top > max_top) top = max_top;

    if (lb->top == top) return 0;
    lb->top = (int16_t)top;

    // optional: keep selection valid if data shrank
    listbox_tidyup(lb);

    //SBOS_paintAllWindows();

    // invalidate listbox rect (recommended)
    // cg_invalidate_gadget(g);
    return(1);
}




