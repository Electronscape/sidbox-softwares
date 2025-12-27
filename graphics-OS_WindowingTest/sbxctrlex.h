#ifndef UICONTROLS_H
#define UICONTROLS_H

#include <stdint.h>

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
    int16_t value;
    int16_t step;
} sbx_scroll_props_t;

// --------- HANDLE TYPE (index + generation) ----------
typedef uint16_t SBControlHandle;
#define SBCTL_INVALID ((SBControlHandle)0xFFFF)

#define SBCTL_MAKE(gen, idx)   ((SBControlHandle)((((uint16_t)(gen)) << 8) | ((uint16_t)(idx) & 0xFF)))
#define SBCTL_IDX(h)           ((uint8_t)((h) & 0xFF))
#define SBCTL_GEN(h)           ((uint8_t)((h) >> 8))

// -----------------------------------------------------

typedef struct {
    // slot state
    uint8_t used;      // 0 free, 1 used
    uint8_t gen;       // generation for stale-handle detection

    // control data
    CtlType type;

    // rect (mostly buttons)
    int16_t x, y, w, h; // client-relative
    char text[32];

    uint8_t visible;
    uint8_t down;

    // scroll bars
    sbx_scroll_props_t sb;
    uint8_t orient;     // sbx_sb_orient_t
    uint8_t dock;       // sbx_dock_t
    uint8_t thumb_down; // dragging thumb flag

    // radio
    uint8_t radio_group;    // group id (0..255)
    uint8_t radio_checked;  // 0/1

    // check box
    uint8_t cb_checked;   // 0/1



} sbx_control_t;

#define MAX_CONTROLS 16

#endif
