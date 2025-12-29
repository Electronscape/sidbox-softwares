#include "stdint.h"
#include "cg_renderer.h"
#include "cg_windowex.h"
#include "cg_gadgets.h"

#include "cg_gad_bitmapview.h"

static inline int16_t clamp_scroll(int16_t v, int16_t maxv){
    if (v < 0) return 0;
    if (v > maxv) return maxv;
    return v;
}

void draw_bitmapview(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    //// for practice really - any global changes to the clip must be stored and restored after you're done with it ////
    //UIClipRect old = g_uiclip;  // <<--- make a copy of the current clip! BELIVE ME you'll need this!!
    //ui_clip_set(0,0,0,0);       // <<--- demo set clip, wont do anything fancy but REMEMBER HOW HERE
    //g_uiclip = old;             // <<--- RESTORE old clip


    GAD_BITMAPVIEW_T *bv = (GAD_BITMAPVIEW_T*)g->gadget;
    if (!bv->h.visible) return;

    // Absolute rect in screen coords
    int16_t ax = (int16_t)(w->clientrect.x + bv->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + bv->h.rect.y);
    int16_t aw = bv->h.rect.w;
    int16_t ah = bv->h.rect.h;
    if (aw <= 0 || ah <= 0) return;

    // Frame + inner area
    int16_t ix = ax, iy = ay, iw = aw, ih = ah;
    if (bv->bv_flags & BVF_SHOW_FRAME) {
        fill_rect_pen(ax, ay, aw, ah, WIN_BORDER_INACTIVE_PEN);
        draw_bevel(ax, ay, aw, ah, WIN_BEVEL_H, WIN_BEVEL_L, 0);

        // inset client area
        ix = (int16_t)(ax + 2);
        iy = (int16_t)(ay + 2);
        iw = (int16_t)(aw - 4);
        ih = (int16_t)(ah - 4);
        if (iw <= 0 || ih <= 0) return;
    }

    // Clear background of the view area
    fill_rect_pen(ix, iy, iw, ih, WIN_CLIENT_PEN);

    // No source? done
    if (!bv->pixels || bv->bmp_w <= 0 || bv->bmp_h <= 0) return;

    // Clamp scroll so we never read past bitmap edges
    int16_t maxx = (int16_t)(bv->bmp_w - iw);
    int16_t maxy = (int16_t)(bv->bmp_h - ih);
    if (maxx < 0) maxx = 0;
    if (maxy < 0) maxy = 0;
    bv->scroll_x = clamp_scroll(bv->scroll_x, maxx);
    bv->scroll_y = clamp_scroll(bv->scroll_y, maxy);

    // Compute unclipped visible blit size (within bitmap)
    int16_t blit_w = iw;
    int16_t blit_h = ih;
    if (blit_w > (int16_t)(bv->bmp_w - bv->scroll_x)) blit_w = (int16_t)(bv->bmp_w - bv->scroll_x);
    if (blit_h > (int16_t)(bv->bmp_h - bv->scroll_y)) blit_h = (int16_t)(bv->bmp_h - bv->scroll_y);
    if (blit_w <= 0 || blit_h <= 0) return;

    // ---- HARD CLIP (screen + ui clip), then adjust source offsets ----
    int16_t dx0 = ix;
    int16_t dy0 = iy;
    int16_t dx1 = (int16_t)(ix + blit_w);
    int16_t dy1 = (int16_t)(iy + blit_h);

    // Screen clip
    if (dx0 < 0) dx0 = 0;
    if (dy0 < 0) dy0 = 0;
    if (dx1 > SCR_WIDTH)  dx1 = SCR_WIDTH;
    if (dy1 > SCR_HEIGHT) dy1 = SCR_HEIGHT;

    // UI clip (same semantics as ui_ppixel: x in [x0,x1), y in [y0,y1))
    if (g_uiclip.enabled) {
        if (dx0 < g_uiclip.x0) dx0 = g_uiclip.x0;
        if (dy0 < g_uiclip.y0) dy0 = g_uiclip.y0;
        if (dx1 > g_uiclip.x1) dx1 = g_uiclip.x1;
        if (dy1 > g_uiclip.y1) dy1 = g_uiclip.y1;
    }

    if (dx1 <= dx0 || dy1 <= dy0) return;

    // Source start offset based on how much we clipped off the left/top
    int16_t src_x0 = (int16_t)(bv->scroll_x + (dx0 - ix));
    int16_t src_y0 = (int16_t)(bv->scroll_y + (dy0 - iy));

    int16_t out_w = (int16_t)(dx1 - dx0);
    int16_t out_h = (int16_t)(dy1 - dy0);

    // ---- BLIT (safe) ----
    if (bv->bv_flags & BVF_SRC_ROWMAJOR) {
        // src[y*stride + x]
        for (int16_t dx = 0; dx < out_w; dx++) {
            int16_t sx = (int16_t)(src_x0 + dx);
            int16_t dst_x = (int16_t)(dx0 + dx);

            int32_t dst = (int32_t)dst_x * SCR_STRIDE + dy0;
            const uint8_t *src = bv->pixels + (int32_t)src_y0 * bv->bmp_stride + sx;

            for (int16_t dy = 0; dy < out_h; dy++) {
                PROJ_VRAM[dst + dy] = src[(int32_t)dy * bv->bmp_stride];
            }
        }
    } else {
        // src[x*stride + y] (x-major)
        for (int16_t dx = 0; dx < out_w; dx++) {
            int16_t sx = (int16_t)(src_x0 + dx);
            int16_t dst_x = (int16_t)(dx0 + dx);

            const uint8_t *src_col = bv->pixels + (int32_t)sx * bv->bmp_stride + src_y0;
            int32_t dst = (int32_t)dst_x * SCR_STRIDE + dy0;

            for (int16_t dy = 0; dy < out_h; dy++) {
                PROJ_VRAM[dst + dy] = src_col[dy];
            }
        }
    }
}

uint32_t onMouseDownCaptureBitmapview(GADGET_BASE_T *gadget, int16_t *mx, int16_t *my){
    GAD_BITMAPVIEW_T *bv = (GAD_BITMAPVIEW_T*) gadget->gadget;
    if (bv->bv_flags & BVF_PAN) {
        bv->panning = 1;        // dragging mode
        bv->pan_start_mx = *mx;
        bv->pan_start_my = *my;
        bv->pan_start_x = bv->scroll_x;
        bv->pan_start_y = bv->scroll_y;
    }
    return 0x00;
}

uint32_t onMouseUpBitmapView(GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    (void)mx; (void)my;
    GAD_BITMAPVIEW_T *bv = (GAD_BITMAPVIEW_T*) g->gadget;
    if (!bv) return 1;  // panic status

    bv->panning = 0;
    return 0;   // all good 0 as in 0k :)
}
