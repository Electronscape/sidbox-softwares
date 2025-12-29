#ifndef CG_GAD_BITMAPVIEW_H
#define CG_GAD_BITMAPVIEW_H

//#include "cg_type.h"
#include "cg_gadgets.h"

//#include "cg_windowex.h"
#include "cg_wintype.h"

typedef struct GAD_BITMAPVIEW_T {
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------
    // Bitmap source
    const uint8_t  *pixels;      // 8bpp indexed, x-major
    int16_t         bmp_w;
    int16_t         bmp_h;
    int16_t         bmp_stride;  // usually == bmp_h

    // View state (top-left of bitmap in view coords)
    int16_t         scroll_x;
    int16_t         scroll_y;

    // Interaction state (for panning)
    int16_t         pan_start_mx;
    int16_t         pan_start_my;
    int16_t         pan_start_x;
    int16_t         pan_start_y;
    uint8_t         panning;

    // Behaviour flags
    uint32_t        bv_flags;

} GAD_BITMAPVIEW_T;

extern GAD_BITMAPVIEW_T g_bvPool  [MAX_BITMAPVIEWS];




uint32_t onMouseDownCaptureBitmapview(GADGET_BASE_T *gadget, int16_t *mx, int16_t *my);
uint32_t onMouseMoveBitmapView(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my);
uint32_t onMouseUpBitmapView(GADGET_BASE_T *g, int16_t *mx, int16_t *my);


#endif // CG_GAD_BITMAPVIEW_H
