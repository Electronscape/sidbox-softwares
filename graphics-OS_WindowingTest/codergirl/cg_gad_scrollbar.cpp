//#include <stdio.h>
#include <stdint.h>
#include "cg_renderer.h"
#include "cg_windowex.h"
#include "cg_glyphs.h"
#include "cg_gadgets.h"

#include "cg_input.h"

#include "cg_gad_scrollbar.h"

static inline int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


// Thumb length derived from step relative to (max-min)
int16_t sb_thumb_len_from_step(int16_t track_len, int16_t min, int16_t max, int16_t step){
    const int16_t MIN_THUMB = 8;
    int32_t range = (int32_t)max - (int32_t)min;

    if (track_len <= 0) return 0;
    if (range <= 0) return track_len; // no scroll range => thumb fills track

    if (step <= 0) step = 1;
    if ((int32_t)step > range) step = (int16_t)range;

    int32_t len = ((int32_t)step * (int32_t)track_len + range/2) / range;
    if (len < MIN_THUMB) len = MIN_THUMB;
    if (len > track_len) len = track_len;
    return (int16_t)len;
}

// Map VALUE -> thumb position (0..travel)
int16_t sb_thumb_pos_from_value(int16_t value, int16_t min, int16_t max, int16_t travel){
    if (travel <= 0) return 0;
    if (max <= min) return 0;

    value = clamp_i16(value, min, max);

    int32_t range = (int32_t)max - (int32_t)min;
    int32_t v     = (int32_t)value - (int32_t)min;

    // rounded
    return (int16_t)((v * (int32_t)travel + range/2) / range);
}



void draw_scrollbar(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g->gadget;
    if (!s->h.visible) return;

    // Determine absolute rect (outer frame rect)
    int16_t ax, ay, aw, ah;

    Rect16 inner = win_inner_rect(w);
    int16_t reserveR = win_inner_reserve_right(w);
    int16_t reserveB = win_inner_reserve_bottom(w);

    if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT) && (s->orient == SB_ORIENT_VERT)) {
        ax = (int16_t)(inner.x + inner.w - SB_SCROLL_THICK + SB_RDOCK_OFFSET_X);
        ay = inner.y  + SB_RDOCK_OFFSET_Y;
        aw = SB_SCROLL_THICK;
        ah = (int16_t)(inner.h - reserveB);   // <-- NEW (avoid resizer overlap / bottom stuff)
    } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM) && (s->orient == SB_ORIENT_HORZ)) {
        ax = inner.x + SB_BDOCK_OFFSET_X;
        ay = (int16_t)(inner.y + inner.h - SB_SCROLL_THICK + SB_BDOCK_OFFSET_Y);
        aw = (int16_t)(inner.w - reserveR);   // <-- NEW (avoid resizer overlap on right)
        ah = SB_SCROLL_THICK;
    } else {
        ax = (int16_t)(w->clientrect.x + s->h.rect.x);
        ay = (int16_t)(w->clientrect.y + s->h.rect.y);
        aw = s->h.rect.w;
        ah = s->h.rect.h;
    }

    if (aw <= 0 || ah <= 0) return;

    // ------------------------------------------------------------------
    // 1) Outer frame (border + bevel)
    // ------------------------------------------------------------------
    if (!(s->h.flags & (GAD_TOOL_DOCKED_BOTTOM | GAD_TOOL_DOCKED_RIGHT)))
    {
        fill_rect_pen(ax, ay, aw, ah, WIN_BORDER_INACTIVE_PEN);
        draw_bevel(ax, ay, aw, ah, WIN_BEVEL_H, WIN_BEVEL_L, 0);
    }

    // ------------------------------------------------------------------
    // 2) Adjust inner "track well" inset based on arrows
    // ------------------------------------------------------------------
    int16_t ix = (int16_t)(ax + SB_TRACK_INSET);
    int16_t iy = (int16_t)(ay + SB_TRACK_INSET);
    int16_t iw = (int16_t)(aw - SB_TRACK_INSET * 2);
    int16_t ih = (int16_t)(ah - SB_TRACK_INSET * 2);

    // Shrink the inner well area if arrows are enabled
    if (s->show_arrows) {
        if (s->orient == SB_ORIENT_VERT) {
            ih -= (SB_ARROW_SHRINK * 2);  // Reduce height to accommodate top and bottom arrows (8 each)
            iy += SB_ARROW_SHRINK;   // Move the track down a little
        } else {
            iw -= (SB_ARROW_SHRINK * 2);  // Reduce width to accommodate left and right arrows (8 each)
            ix += SB_ARROW_SHRINK;   // Move the track right a little
        }
    }

    // Check for invalid track area
    if (iw <= 0 || ih <= 0) return;

    // Fill the well with a different pen (change SB_TRACK_PEN later)
    gfx_setcolour(SB_TRACK_PEN);
    ui_fill_dots(ix, iy, iw, ih, 2);

    // Draw the well bevel "inset" look:
    draw_bevel(ix, iy, iw, ih, WIN_BEVEL_H, WIN_BEVEL_L, 1);

// ------------------------------------------------------------------
// 3) Draw the arrows if enabled
// ------------------------------------------------------------------
#define ARROW_BUTTON_SIZE_TMP   16
    if (s->show_arrows) {
        // Vertical scrollbar arrows (Top and Bottom)
        if (s->orient == SB_ORIENT_VERT) {
            // Top Arrow: Centered horizontally in 'aw', at the very top 'ay'
            // We use (aw - 8) / 2 to perfectly center the 8px box
            draw_bevel_rect(ax, ay, aw, ARROW_BUTTON_SIZE_TMP);

            ui_draw_glyph(ax + (aw - ARROW_BUTTON_SIZE_TMP) / 2, ay, glyph_arrow_up);

            // Bottom Arrow: At the very bottom of the scrollbar (ay + ah - 8)
            draw_bevel_rect(ax, ay + ah - ARROW_BUTTON_SIZE_TMP, aw, ARROW_BUTTON_SIZE_TMP);

            ui_draw_glyph(ax + (aw - ARROW_BUTTON_SIZE_TMP) / 2 , ay + (ah - ARROW_BUTTON_SIZE_TMP) , glyph_arrow_down);

        }
        // Horizontal scrollbar arrows (Left and Right)
        else {
            int ht = ah; // The thickness of the bar

            // --- LEFT ARROW ---
            // Button is at 'ax'
            draw_bevel_rect(ax, ay, ARROW_BUTTON_SIZE_TMP, ht);

            // Glyph is at 'ax', vertically centered in 'ht'
            ui_draw_glyph(ax, ay + (ht - ARROW_BUTTON_SIZE_TMP) / 2, glyph_arrow_left);

            // --- RIGHT ARROW ---
            // Button is at the far right: ax + total_width - button_width
            int16_t right_btn_x = ax + aw - ARROW_BUTTON_SIZE_TMP;
            draw_bevel_rect(right_btn_x, ay, ARROW_BUTTON_SIZE_TMP, ht);

            // Glyph is at the same 'right_btn_x', vertically centered
            ui_draw_glyph(right_btn_x, ay + (ht - ARROW_BUTTON_SIZE_TMP) / 2, glyph_arrow_right);

        }
    }

    // ------------------------------------------------------------------
    // 4) Thumb geometry inside the inner well (after shrinking for arrows)
    // ------------------------------------------------------------------
    int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ih : iw;
    int16_t thumb_len = sb_thumb_len_from_step(track_len, s->min, s->max, s->step);

    int16_t travel = (int16_t)(track_len - thumb_len);
    if (travel < 0) travel = 0;

    int16_t tpos = sb_thumb_pos_from_value(s->value, s->min, s->max, travel);

    // Thumb rect is based on the inner well, not the outer frame
    int16_t tx = ix, ty = iy, tw = iw, th = ih;
    if (s->orient == SB_ORIENT_VERT) { ty = (int16_t)(iy + tpos); th = thumb_len; }
    else                            { tx = (int16_t)(ix + tpos); tw = thumb_len; }

    if (tw > 2) { tx++; tw -= 2; }
    if (th > 2) { ty++; th -= 2; }

    // Thumb face + bevel (your "perfect" thumb look stays)
    uint8_t pressed = (s->dragging || s->h.down) ? 1 : 0;
    fill_rect_pen(tx, ty, tw, th, WIN_SCROLLER_PROP_PEN);
    draw_bevel(tx, ty, tw, th, WIN_BEVEL_H, WIN_BEVEL_L, pressed);
}



uint32_t onMouseReleaseScrollbar(GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    (void)mx; (void)my;

    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*) g->gadget;
    if (!s) return 1;   // panic

    s->dragging = 0;
    s->drag_off = 0;
    g_ui.sb_track_start = 0;
    g_ui.sb_travel = 0;
    g_ui.sb_drag_off = 0;
    return 0;   // all good 0 as in 0k :)
}
