#ifndef CG_GAD_LISTBOX_H
#define CG_GAD_LISTBOX_H

//#include "cg_type.h"
#include "cg_itemlist.h"
#include "cg_gadgets.h"

#include "cg_wintype.h"

typedef struct GAD_LISTBOX_T {
    //-------------- common parts -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //------------------------------------------------

    // model pointer (does NOT own it)
    ItemLists_t     *items;

    // view
    int16_t         row_h;       // 16 by default (glyph height)
    int16_t         padding_x;   // 2..4 is nice
    int16_t         padding_y;   // 2
} GAD_LISTBOX_T;

extern GAD_LISTBOX_T    g_lbPool  [MAX_LISTBOXES];

uint32_t onMouseDownCaptureListBox(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my);
uint32_t onMouseMoveListBox(sbx_window_t *w, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my);


#endif // CG_GAD_LISTBOX_H
