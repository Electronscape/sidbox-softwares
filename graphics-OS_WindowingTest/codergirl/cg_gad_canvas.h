#ifndef CG_GAD_CANVAS_H
#define CG_GAD_CANVAS_H

#include "cg_wintype.h"
#include "cg_gadgets.h"


// create types for each gadget
typedef struct GAD_CANVAS_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    uint8_t         BPen, FPen;     // set the colours for this canvas
    uint32_t        drawtype;       // draw type

    //fnButtonCallBack callbackRouteA;              // basic callback route
} GAD_CANVAS_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_CANVAS_T g_cnPool [MAX_CANVASES];














#endif // CG_GAD_CANVAS_H
