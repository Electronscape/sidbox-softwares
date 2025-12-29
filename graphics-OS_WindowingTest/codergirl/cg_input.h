#ifndef CG_INPUT_H
#define CG_INPUT_H

#include <stdint.h>
#include "cg_windowex.h"

extern sbx_window_t     gui_windows[MAX_WINDOWS];
extern uint8_t          gui_used[MAX_WINDOWS];

extern SBXWindowId      g_winZorder[MAX_WINDOWS];
extern uint8_t          g_winZcount;


typedef struct UIInputState {
    uint8_t         mouse_down;

    // what the mouse-down started on (window + region)
    SBXWindowId     down_win;
    WHitRegion      down_region;

    // title gadget tracking (close/min/max/zorder)
    SBXWindowId     title_win;
    WHitRegion      title_region;
    uint8_t         title_inside;

    // window drag
    SBXWindowId     drag_win;
    int16_t         drag_off_x;
    int16_t         drag_off_y;

    // resize
    SBXWindowId     resize_win;
    int16_t         r_start_mx;
    int16_t         r_start_my;
    int16_t         r_start_w;
    int16_t         r_start_h;

    // then here we will use, REUSABLE start_x, start_y, drag_x, drag_y's
    // since i HOPE we are going to use the gadget handle as the references later
    // example: dummy stuff below

    GADGET_BASE_T   *capturedGadget;
    GADGET_RECT_T   *capturedGadgetRect;    // this is to help with the checking for "are we still inside the button rect" for exmaple

    int16_t         clicked_x, clicked_y;   // starting gadget mouse hit point
    int16_t         drag_x, drag_y;         // moving mouse point over the gadget (usually for sliders

    // scrollbar drag cache (axis coords)
    int16_t     sb_track_start;   // screen axis start of track (x or y)
    int16_t     sb_travel;        // pixels thumb can travel (track_len - thumb_len)
    int16_t     sb_drag_off;      // cached offset inside thumb (axis)

} UIInputState;


extern UIInputState g_ui;


//  UI INTERFACING //
void            SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my);


#endif // CG_INPUT_H
