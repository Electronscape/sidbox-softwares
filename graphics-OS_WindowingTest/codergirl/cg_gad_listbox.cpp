//#include <stdio.h>
#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
//#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_gad_listbox.h"


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
    fill_rect_pen(ax, ay, aw, ah, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(ax, ay, aw, ah, WIN_BEVEL_H, WIN_BEVEL_L, 0);

    // Inner client area (inset)
    int16_t ix = (int16_t)(ax + 2);
    int16_t iy = (int16_t)(ay + 2);
    int16_t iw = (int16_t)(aw - 4);
    int16_t ih = (int16_t)(ah - 4);
    if (iw <= 0 || ih <= 0) return;

    fill_rect_pen(ix, iy, iw, ih, WIN_CLIENT_PEN);


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
    if (list->sel < list->top) list->top = list->sel;
    if (list->sel >= list->top + rows) list->top = (int16_t)(list->sel - rows + 1);

    if (list->top < 0) list->top = 0;
    if (list->top > maxTop) list->top = maxTop;



    // How many chars fit per row (8x16 font)
    const int16_t char_w = 8;
    int16_t max_chars = (int16_t)((iw - pad_x * 2) / char_w);
    if (max_chars < 0) max_chars = 0;
    if (max_chars > (DEF_GADGET_TEXT_SIZE - 1)) max_chars = (DEF_GADGET_TEXT_SIZE - 1);

    // Draw rows

    gfx_setcolour(WIN_TITLE_PEN);

    int16_t tx;
    int16_t ty;

    for (int16_t r = 0; r < rows; r++){
        int16_t idx = (int16_t)(list->top + r);
        if (idx >= count) break;

        int16_t ry = (int16_t)(iy + pad_y + r * row_h);

        uint8_t selected = (idx == list->sel);
        // Selected highlight
        if (selected) {
            fill_rect_pen(ix, ry-2, iw, row_h+2, WIN_SCROLLER_PROP_PEN);

            // Focus rectangle inside the row (clipped by listbox clip already)
            //gfx_setcolour(WIN_BEVEL_L);
            //ui_hline((int16_t)(ix), (int16_t)(ry-2), (int16_t)(iw));
            //ui_hline((int16_t)(ix), (int16_t)(ry+row_h), (int16_t)(iw));
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

        gfx_setcolour(WIN_TITLE_PEN);
        ui_draw_text816(tx, ty, (const unsigned char*)tmp);
    }
}



static int16_t listbox_index_from_mouse(const sbx_window_t *w, const GAD_LISTBOX_T *lb,
                                        int16_t mx, int16_t my,
                                        int16_t *out_rows_visible)
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
    int16_t ih = (int16_t)(ah - 4);
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
    int16_t idx = (int16_t)(list->top + row);
    if (idx < 0) idx = 0;
    if (idx >= count) idx = (int16_t)(count - 1);

    return idx;
}

uint32_t onMouseDownCaptureListBox(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    int16_t rows = 0;
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);
    if (idx < 0) return 0; // clicked inside gadget but not on a row: ignore

    ItemLists_t *list = lb->items;

    if (list->sel != idx) {
        list->sel = idx;

        // keep selection visible (nice UX)
        if (list->sel < list->top) list->top = list->sel;
        if (list->sel >= list->top + rows) list->top = (int16_t)(list->sel - rows + 1);

        SBOS_paintAllWindows();
    }

    return 0;
}


uint32_t onMouseMoveListBox(sbx_window_t *w, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my){
    (void)evt;
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    ItemLists_t *list = lb->items;

    int16_t rows = 0;
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);
    if (idx < 0) return 0;

    // Recompute the same inner band to detect “outside above/below”
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    //int16_t aw = lb->h.rect.w;
    int16_t ah = lb->h.rect.h;

    //int16_t ix = (int16_t)(ax + 2);
    int16_t iy = (int16_t)(ay + 2);
    int16_t ih = (int16_t)(ah - 4);

    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    // Same band height used by listbox_index_from_mouse()
    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return 0;

    int16_t band_h = (int16_t)(rows * row_h);

    // Mouse position relative to first row
    int16_t ly = (int16_t)(*my - iy - pad_y);

    // Auto-scroll when dragging outside above/below
    int16_t count = (int16_t)list->count;
    int16_t maxTop = (int16_t)(count - rows);
    if (maxTop < 0) maxTop = 0;

    uint8_t changed = 0;

    if (ly < 0) {
        if (list->top > 0) { list->top--; changed = 1; }
    } else if (ly >= band_h) {
        if (list->top < maxTop) { list->top++; changed = 1; }
    }

    // Update selection (now keeps working even outside)
    if (list->sel != idx) {
        list->sel = idx;
        changed = 1;
    }

    // Keep selection visible (nice behavior even when not outside)
    if (list->sel < list->top) { list->top = list->sel; changed = 1; }
    if (list->sel >= list->top + rows) { list->top = (int16_t)(list->sel - rows + 1); changed = 1; }

    // Clamp top
    if (list->top < 0) list->top = 0;
    if (list->top > maxTop) list->top = maxTop;

    if (changed) SBOS_paintAllWindows();
    return 0;
}

