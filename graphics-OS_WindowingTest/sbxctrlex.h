#ifndef UICONTROLS_H
#define UICONTROLS_H


#include <stdlib.h>
#include <stdint.h>


typedef enum {
    CTL_LABEL,
    CTL_BUTTON,
    CTL_SCROLLBAR
} CtlType;



//////////// SCROLL BAR ////////////////////
typedef enum {
    SBX_SB_VERT = 0,
    SBX_SB_HORZ = 1
} sbx_sb_orient_t;

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
    //int16_t page;     // visible amount (aka "page size")
    int16_t value;    // current scroll position
    int16_t step;     // arrow/button step (small increment)
} sbx_scroll_props_t;






typedef struct {
    CtlType type;           // button, labels, sliders... blah blah
    uint16_t id;
    int16_t x, y, w, h;     // client-relative
    char text[32];          // for now: label/button only
    uint8_t visible;
    uint8_t down;           // button pressed state

    sbx_scroll_props_t sb;
    uint8_t orient;   // sbx_sb_orient_t
    uint8_t dock;     // sbx_dock_t
    uint8_t thumb_down; // later for dragging

} sbx_control_t;


#define MAX_CONTROLS 16     // per window



// Prototypes






#endif
