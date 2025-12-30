#ifndef CG_GAD_SCROLLBAR_H
#define CG_GAD_SCROLLBAR_H

//#include "cg_type.h"
#include "cg_gadgets.h"

#include "cg_wintype.h"

typedef void (*fnSBCallBack)(void *sb);


typedef struct GAD_SCROLLBAR_T{
    // common
    GAD_HDR_T   h;
    uint8_t     used;

    // behaviour
    uint8_t     orient;      // SB_ORIENT_*
    int16_t     min;         // used for thumb sizing + optional conversion
    int16_t     max;
    int16_t     step;        // affects thumb size + step in percent
    int16_t     value;      // the actual value between min and max

    // interaction
    uint8_t     dragging;
    int16_t     drag_off;    // mouse offset inside thumb (in track axis)

    // arrows
    uint8_t     show_arrows;    // needed for if we're using arrows

    // function callback systems
    //void (*onScrollCallBack)(void *scrollerPtr);    // <-- since we should be able to peek at the scrollers internal anyway?
    fnSBCallBack onScrollCallBack;

    uint8_t suppress_cb; // prevents recursion

} GAD_SCROLLBAR_T;


// scroll bar needs extra bits -------------------------------------------
typedef enum {
    SB_ORIENT_VERT = 0,
    SB_ORIENT_HORZ = 1
} SB_ORIENT;

typedef enum {
    SB_PART_NONE = 0,
    SB_PART_THUMB,
    SB_PART_TRACK,
    SB_PART_ARROW_UP,
    SB_PART_ARROW_DOWN,
    SB_PART_ARROW_LEFT,
    SB_PART_ARROW_RIGHT
} SBPart;


extern GAD_SCROLLBAR_T  g_sbPool  [MAX_SCROLLBARS];

SBPart hittest_scrollbar_part(const sbx_window_t *w, const GAD_SCROLLBAR_T *s,
                              int16_t mx, int16_t my,
                              int16_t *out_thumb_axis_start,
                              int16_t *out_thumb_len,
                              int16_t *out_track_axis_start);

uint32_t onMouseDownCaptureScrollBar(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my);

uint32_t onMouseMoveScrollbar(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my);


uint32_t onMouseReleaseScrollbar(GADGET_BASE_T *g, int16_t *mx, int16_t *my);


uint32_t SBOS_setScrollBarCallBack(SBControlHandle h, fnSBCallBack func);

#endif // CG_GAD_SCROLLBAR_H
