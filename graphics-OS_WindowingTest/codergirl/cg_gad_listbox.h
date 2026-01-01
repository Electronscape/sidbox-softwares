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

    int8_t          selecting;
    int16_t         sel;         // selected index (-1 if none)
    int16_t         top;         // first visible row (scroll)
    int16_t         visablerows; // a count of how many are shown on box
} GAD_LISTBOX_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_LISTBOX_T    g_lbPool  [MAX_LISTBOXES];


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------
uint32_t onMouseDownCaptureListBox(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my);
uint32_t onMouseMoveListBox(sbx_window_t *w, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my);
uint32_t onMouseReleaseListBox(GADGET_BASE_T *g, int16_t *mx, int16_t *my);


// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setListbox_top(GADGET_BASE_T *g, int top);
int16_t SBOS_ListBoxGetSelectedIndex(CGGadgetHandle h);
ItemLists_t* SBOS_ListBoxGetItems(CGGadgetHandle h);

#endif // CG_GAD_LISTBOX_H
