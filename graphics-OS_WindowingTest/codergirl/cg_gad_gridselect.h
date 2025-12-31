#ifndef CG_GAD_GRIDSELECT_H
#define CG_GAD_GRIDSELECT_H


#include "cg_wintype.h"
#include "cg_gadgets.h"

typedef void (*fnGridSelectCallBack)(void *button);



typedef void (*GridSelHandler)(
    CGGadgetHandle gad,
    uint16_t idx,
    uint8_t  row,
    uint8_t  col,
    uint8_t  is_selected,
    uint32_t user);

typedef struct GAD_GRIDSELECT_T {
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T           h;
    uint8_t             used;
    //-----------------------------------------------------------

    uint8_t cells_x;
    uint8_t cells_y;

    int16_t hover_idx;        // -1 if none
    int16_t down_idx;         // cell where mouse went down (optional)

    int16_t selected_idx;     // single-select, -1 allowed unless MUST_ONE

    uint32_t flags;

    // Multi-select state:
    // If you expect <= 64 cells often, start with u64 and evolve later.
    uint64_t sel_mask64;

    // Optional: user payload per cell (color, id, pointer index)
    // Keep it optional; you can add later without breaking layout if you reserve.
    // void *cell_data;

    GridSelHandler handler;

    fnGridSelectCallBack onGridSelectClickCallBack;
    uint32_t user;
} GAD_GRIDSELECT_T;





#endif // CG_GAD_GRIDSELECT_H
