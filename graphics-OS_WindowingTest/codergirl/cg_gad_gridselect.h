#ifndef CG_GAD_GRIDSELECT_H
#define CG_GAD_GRIDSELECT_H

#include <stdint.h>
#include "cg_wintype.h"
#include "cg_gadgets.h"

typedef void (*fnGridSelectCallBack)(void *button);

typedef struct GAD_GRIDSELECT_T {
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T           h;
    uint8_t             used;
    //-----------------------------------------------------------

    uint8_t cells_x;
    uint8_t cells_y;
    uint8_t cellText[256][4]; // 3 characters + '\0'
    uint8_t cellColour[256];  // Should ONLY limit the creator tool to 256 entries!!

    int16_t down_idx;         // cell where mouse went down (optional)
    int16_t preview_idx;      // draging the selection about
    int16_t selected_idx;     // single-select, -1 allowed unless MUST_ONE

    uint32_t flags;

    //fnGridSelectCallBack onGridSelectClickCallBack;
} GAD_GRIDSELECT_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_GRIDSELECT_T g_gsPool  [MAX_GRIDSELECTS];



// MOUSE EVENTS ---------------------------------------------------------------------------------------------------
uint32_t onMouseDownCaptureGridSelect(sbx_window_t *w, GADGET_BASE_T *gadget, int16_t *mx, int16_t *my);
uint32_t onMouseMoveGridSelect(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my);
uint32_t onMouseReleaseGridSelect(GADGET_BASE_T *g, int16_t *mx, int16_t *my);


// API INTERFACES -------------------------------------------------------------------------------------------------

// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setCellText(CGGadgetHandle h, const char *text, int16_t cellindex);

uint32_t SBOS_setCellColour(CGGadgetHandle h, const uint8_t colourIndex, int16_t cellindex);





#endif // CG_GAD_GRIDSELECT_H
