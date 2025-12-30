#include <stdio.h>
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

    GADGET_RECT_T inner = win_inner_rect(w);
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
        fill_rect_pen(ax, ay, aw, ah, PEN_WIN_BORDER_INACTIVE);
        draw_bevel(ax, ay, aw, ah, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 0);
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
    gfx_setcolour(PEN_SCROLLBAR_TRACK);
    ui_fill_dots(ix, iy, iw, ih, 2);

    // Draw the well bevel "inset" look:
    draw_bevel(ix, iy, iw, ih, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);

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
    fill_rect_pen(tx, ty, tw, th, PEN_SCROLLBAR_GADGET_PROP);
    draw_bevel(tx, ty, tw, th, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, pressed);
}


// Map thumb position (0..travel) -> VALUE (min..max)
static inline int16_t sb_value_from_thumb_pos(int16_t pos, int16_t min, int16_t max, int16_t travel){
    if (travel <= 0) return min;
    if (max <= min) return min;

    if (pos < 0) pos = 0;
    if (pos > travel) pos = travel;

    int32_t range = (int32_t)max - (int32_t)min;
    int32_t v = ((int32_t)pos * range + travel/2) / travel;

    return (int16_t)(min + (int16_t)v);
}


SBPart hittest_scrollbar_part(const sbx_window_t *w, const GAD_SCROLLBAR_T *s,
                              int16_t mx, int16_t my,
                              int16_t *out_thumb_axis_start,
                              int16_t *out_thumb_len,
                              int16_t *out_track_axis_start)
{
    // Compute abs rect same way draw does
    int16_t ax, ay, aw, ah;

    GADGET_RECT_T inner = win_inner_rect(w);
    int16_t reserveR = win_inner_reserve_right(w);
    int16_t reserveB = win_inner_reserve_bottom(w);

    // If docked, adjust the position accordingly
    if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT) && (s->orient == SB_ORIENT_VERT)) {
        ax = (int16_t)(inner.x + inner.w - SB_SCROLL_THICK + SB_RDOCK_OFFSET_X);
        ay = inner.y + SB_RDOCK_OFFSET_Y;
        aw = SB_SCROLL_THICK;
        ah = (int16_t)(inner.h - reserveB);
    } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM) && (s->orient == SB_ORIENT_HORZ)) {
        ax = inner.x + SB_BDOCK_OFFSET_X;
        ay = (int16_t)(inner.y + inner.h - SB_SCROLL_THICK + SB_BDOCK_OFFSET_Y);
        aw = (int16_t)(inner.w - reserveR);
        ah = SB_SCROLL_THICK;
    } else {
        ax = (int16_t)(w->clientrect.x + s->h.rect.x);
        ay = (int16_t)(w->clientrect.y + s->h.rect.y);
        aw = s->h.rect.w;
        ah = s->h.rect.h;
    }

    // Check if the mouse is within the scrollbar area
    if (!mousept_in_rect(mx, my, ax, ay, aw, ah)) return SB_PART_NONE;

    // Shrink the track area if arrows are enabled

    if (s->show_arrows) {
        if (s->orient == SB_ORIENT_VERT) {
            ah -= SB_ARROW_SHRINK * 2;  // Shrink the height for the top and bottom arrows
            ay += SB_ARROW_SHRINK;      // Move the track down to fit arrows
        } else {
            aw -= SB_ARROW_SHRINK * 2;  // Shrink the width for the left and right arrows
            ax += SB_ARROW_SHRINK;      // Move the track right to fit arrows
        }
    }


    // Calculate track length (reduced by arrows)
    int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;

    // Ensure the track length doesn't go negative
    if (track_len < 0) track_len = 0;

    // Calculate the thumb length
    int16_t thumb_len = sb_thumb_len_from_step(track_len, s->min, s->max, s->step);

    // Calculate the available space (travel distance) for the thumb
    int16_t travel = (int16_t)(track_len - thumb_len);
    if (travel < 0) travel = 0;

    // Calculate the thumb position based on the value
    int16_t tpos = sb_thumb_pos_from_value(s->value, s->min, s->max, travel);

    // Start position of the track
    int16_t track_axis_start = (s->orient == SB_ORIENT_VERT) ? ay : ax;
    int16_t thumb_axis_start = (int16_t)(track_axis_start + tpos);

    // Output the thumb's start position and length
    if (out_thumb_axis_start) *out_thumb_axis_start = thumb_axis_start;
    if (out_thumb_len) *out_thumb_len = thumb_len;
    if (out_track_axis_start) *out_track_axis_start = track_axis_start;

    // Handle arrow clicks
    if (s->show_arrows) {
        if (s->orient == SB_ORIENT_VERT) {
            // Vertical scrollbar arrows

            // FIX: Subtract SB_ARROW_SHRINK because 'ay' is currently the track start
            if (mousept_in_rect(mx, my, ax, ay - SB_ARROW_SHRINK, aw, SB_ARROW_SHRINK)) {
                printf("SCROLL-BAR-BUTTON!! -- {UP}\n");
                return SB_PART_ARROW_UP;
            }

            // Bottom arrow (this actually works fine with current ax/ay/ah values)
            if (mousept_in_rect(mx, my, ax, ay + ah, aw, SB_ARROW_SHRINK)) {
                printf("SCROLL-BAR-BUTTON!! -- {DOWN}\n");
                return SB_PART_ARROW_DOWN;
            }
        } else {
            // Horizontal scrollbar arrows

            // FIX: Subtract SB_ARROW_SHRINK because 'ax' is currently the track start
            if (mousept_in_rect(mx, my, ax - SB_ARROW_SHRINK, ay, SB_ARROW_SHRINK, ah)) {
                printf("SCROLL-BAR-BUTTON!! -- {LEFT}\n");
                return SB_PART_ARROW_LEFT;
            }

            // Right arrow
            if (mousept_in_rect(mx, my, ax + aw, ay, SB_ARROW_SHRINK, ah)) {
                printf("SCROLL-BAR-BUTTON!! -- {RIGHT}\n");
                return SB_PART_ARROW_RIGHT;
            }
        }
    }

    // Check if the mouse is over the thumb
    if (s->orient == SB_ORIENT_VERT) {
        if (mousept_in_rect(mx, my, ax, thumb_axis_start, aw, thumb_len)) return SB_PART_THUMB;
    } else {
        if (mousept_in_rect(mx, my, thumb_axis_start, ay, thumb_len, ah)) return SB_PART_THUMB;
    }

    return SB_PART_TRACK;
}


uint32_t onMouseDownCaptureScrollBar(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*) g->gadget;

    int16_t thumb_start = 0, thumb_len = 0, track_start = 0;
    SBPart part = hittest_scrollbar_part(w, s, *mx, *my, &thumb_start, &thumb_len, &track_start);

    s->dragging = 0;

    if (part == SB_PART_ARROW_UP) {
        s->value = clamp_i16(s->value - s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_DOWN) {
        s->value = clamp_i16(s->value + s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_LEFT) {
        s->value = clamp_i16(s->value - s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_RIGHT) {
        s->value = clamp_i16(s->value + s->step, s->min, s->max);

    } else if (part == SB_PART_THUMB) {
        s->dragging = 1;

        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;

        // Cache drag offset (mouse position relative to thumb start)
        g_ui.sb_drag_off = (int16_t) (m_axis - thumb_start);
        s->drag_off = g_ui.sb_drag_off; // optional

        // Cache track start (axis start of the track in screen coords)
        g_ui.sb_track_start = track_start;

        // Compute track_len using the same geometry logic

        GADGET_RECT_T inner = win_inner_rect(w);
        int16_t reserveR = win_inner_reserve_right(w);
        int16_t reserveB = win_inner_reserve_bottom(w);

        int16_t ax, ay, aw, ah;

        if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT)
            && (s->orient == SB_ORIENT_VERT)) {
            ax = (int16_t) (inner.x + inner.w
                            - SB_SCROLL_THICK);
            ay = inner.y;
            aw = SB_SCROLL_THICK;
            ah = (int16_t) (inner.h - reserveB);
        } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM)
                   && (s->orient == SB_ORIENT_HORZ)) {
            ax = inner.x;
            ay = (int16_t) (inner.y + inner.h
                            - SB_SCROLL_THICK);
            aw = (int16_t) (inner.w - reserveR);
            ah = SB_SCROLL_THICK;
        } else {
            ax = (int16_t) (w->clientrect.x + s->h.rect.x);
            ay = (int16_t) (w->clientrect.y + s->h.rect.y);
            aw = s->h.rect.w;
            ah = s->h.rect.h;
        }

        if (s->show_arrows) {
            if (s->orient == SB_ORIENT_VERT) {
                ah -= SB_ARROW_SHRINK * 2;
                ay += SB_ARROW_SHRINK; // Not strictly needed for length calc, but good for consistency
            } else {
                aw -= SB_ARROW_SHRINK * 2;
                ax += SB_ARROW_SHRINK;
            }
        }

        int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;
        int16_t travel = (int16_t) (track_len - thumb_len);
        if (travel < 0)
            travel = 0;
        g_ui.sb_travel = travel;

    } else if (part == SB_PART_TRACK) {
        // Track step click (VALUE-BASED)
        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;

        // ensure sane step
        int16_t step = (s->step <= 0) ? 1 : s->step;

        if (m_axis < thumb_start)
            s->value = clamp_i16( (int16_t) (s->value - step), s->min, s->max);
        else
            s->value = clamp_i16(
                (int16_t) (s->value + step), s->min,
                s->max);

        // IMPORTANT: not dragging
        s->dragging = 0;
        s->drag_off = 0;
        g_ui.sb_track_start = 0;
        g_ui.sb_travel = 0;
        g_ui.sb_drag_off = 0;
        //SBOS_paintAllWindows();
    }

    //if (!s->dragging) return 1; // not dragging,
    return 0x00;    // always ok FOR NOW
}

uint32_t onMouseMoveScrollbar(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my){
    (void)win; (void)evt;
    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*) g->gadget;
    if (s->dragging) {
        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;
        int16_t new_thumb_pos = (int16_t) (m_axis - g_ui.sb_track_start - g_ui.sb_drag_off);
        int16_t new_val = sb_value_from_thumb_pos(new_thumb_pos, s->min, s->max, g_ui.sb_travel);

        if (new_val != s->value) {
            s->value = new_val;
            printf("SCROLLING: %d\n", s->value);
            SBOS_paintAllWindows();
        }
    }
    if (!s || !s->dragging) return 1;

    return 0;   // this is happening all the time
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
