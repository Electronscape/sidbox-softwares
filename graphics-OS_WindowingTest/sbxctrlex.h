#ifndef UICONTROLS_H
#define UICONTROLS_H

#include <stdint.h>
#include "sbxguisbw.h"

#define     def_gadget_text_size    32

typedef enum {
    CTL_LABEL,
    CTL_BUTTON,
    CTL_SCROLLBAR,
    CTL_RADIO,
    CTL_CHECKBOX
} CtlType;

typedef enum {
    SBX_SB_VERT = 0,
    SBX_SB_HORZ = 1
} sbx_scrollbar_orient_t;

typedef enum {
    SBX_DOCK_NONE   = 0,
    SBX_DOCK_LEFT   = 1,
    SBX_DOCK_RIGHT  = 2,
    SBX_DOCK_TOP    = 3,
    SBX_DOCK_BOTTOM = 4
} sbx_dock_t;

typedef struct {
    int16_t min;
    int16_t max;
    int16_t value;
    int16_t step;
} sbx_scroll_props_t;

// --------- HANDLE TYPE (index + generation) ----------
typedef uint32_t SBControlHandle;
#define SBCTL_INVALID ((SBControlHandle)0xFFFFFFFF)
#define SBCTL_MAKE(gen, idx)   ((SBControlHandle)((((uint16_t)(gen)) << 16) | ((uint16_t)(idx) & 0xFFFF)))
#define SBCTL_IDX(h)           ((uint8_t)((h) & 0xFFFF))
#define SBCTL_GEN(h)           ((uint8_t)((h) >> 16))

// -----------------------------------------------------



// control types
typedef enum GADGET_CLASS_T {
    GAD_BUTTON,
    GAD_SCROLLBAR,
    GAD_RADIO,
    GAD_CHECKBOX,
    GAD_LABEL
} GADGET_CLASS_T;

typedef struct GAD_BUTTON_T{
    uint8_t         nothing;    // just here to stop empty type // will remove when something useful can be stored here.
} GAD_BUTTON_T;

typedef struct GAD_RADIO_T {
    uint8_t         group;     // 0..255
    uint8_t         checked;   // 0/1
} GAD_RADIO_T;

typedef struct GAD_CHECKBOX_T {
    uint8_t         checked;   // 0/1
} GAD_CHECKBOX_T;

typedef struct GAD_SCROLLBAR_T {
    int16_t         min;
    int16_t         max;
    int16_t         value;
    int16_t         step;
} GAD_SCROLLBAR_T;



typedef struct {
    /// GADGET HOST ///
    /// ** GLOBALE CONTROL HOST STUFF, SYSTEM NEEDS TO KNOW THE GADGET DIMENTIONS AND TYPE, EVERYTHING WELL WILL POINT TO THE ACTUAL GADGET LATER.
    // slot state

    // info about the host, hosting this control
    SBXWindowId     winhnd;     // the window ID, the handler number for the window - useful to "peek at a control cross programs and see who it belongs too"

    // lifetime handles
    // i doubt anyone would ever need more than 1000 gadgets, BUT this allows for scalability later, also keeps with in the 32bit realms of the M7-CPU
    uint16_t         used;      // 0 free, 1 used
    uint16_t         gen;       // generation for stale-handle detection

    // control data
    GADGET_CLASS_T  gadget;     // what type of gadget are wegoing with;
    gad_rect_t      rect;       // the actionable area (container hit area, basic rectangle info)



    char            text[def_gadget_text_size];   // common gadget text

    uint32_t        flags;      // flags for this gadget
    uint8_t         enabled;    // enabled/disabled gadget, sort of like if NOT clickable ;)
    uint8_t         visible;
    uint8_t         down;




    //------------------------------------------------------------------------------------////
    //// *** COMPATIBILITY FIELDS (REMOVE AS WE MOVE AWAY FROM CASE SWITCH NIGHTMARE) *** ////

    CtlType type;

    int16_t x, y, w, h; // client-relative  -- replace this with // gad_rect_t

    uint8_t dock;       // sbx_dock_t
    uint8_t thumb_down; // dragging thumb flag  -- this can be used for other controls too

    /// ** KEEPING THE BELOW WHILE WE MIGRATE STUFF ** ///

    // scroll bars
    sbx_scroll_props_t sb;
    uint8_t orient;     // sbx_sb_orient_t



    // radio
    uint8_t radio_group;    // group id (0..255)
    uint8_t radio_checked;  // 0/1

    // check box
    uint8_t cb_checked;   // 0/1
} GADGET_BASE_T;

#define sbx_control_t   GADGET_BASE_T

#define MAX_CONTROLS 16

#endif
