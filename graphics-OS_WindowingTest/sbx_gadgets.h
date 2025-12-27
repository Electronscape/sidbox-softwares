#ifndef SBX_GADGETS_H
#define SBX_GADGETS_H

#include <stdint.h>


#define     MAX_GADGETS             16  // change this higher for when the OS is tested with programs, 16 is enough for BASE TESTING nothing serious
#define     MAX_GADGETS_PER_WINDOW  4   // this is low, but its for testing. WILL increase this for a normal size
#define     DEF_GADGET_TEXT_SIZE    32



// --------- HANDLE TYPE (index + generation) ----------
typedef uint32_t SBControlHandle;
#define SBCTL_INVALID           ((SBControlHandle)0xFFFFFFFF)
#define SBCTL_MAKE(gen, idx)    ((SBControlHandle)((((uint32_t)(gen) & 0xFFFFu) << 16) | ((uint32_t)(idx) & 0xFFFFu)))
#define SBCTL_IDX(h)            ((uint16_t)((h) & 0xFFFFu))
#define SBCTL_GEN(h)            ((uint16_t)(((h) >> 16) & 0xFFFFu))


// -----------------------------------------------------

// GADGET TYPES
typedef struct GADGET_RECT_T {  // this is likely going to be used for things like scrollbars, with more than one hit regions
    int16_t     x, y, w, h;
} GADGET_RECT_T;

typedef uint8_t SBXWindowId;

// add more as we add more gadgets
typedef enum {
    CTL_LABEL,
    CTL_BUTTON,
} CtlType;

typedef enum {
    GAD_TOOL_DEFAULT        = (1 << 0),
    GAD_TOOL_DOCKED_RIGHT   = (1 << 1),     // right dock used
    GAD_TOOL_DOCKED_BOTTOM  = (1 << 2),     // bottom dock used
} GAD_TOOL_FLAGS;



// control types
typedef enum GADGET_CLASS_T {
    GAD_BUTTON,
    GAD_SCROLLBAR,
    GAD_RADIO,
    GAD_CHECKBOX,
    GAD_LABEL
} GADGET_CLASS_T;


// create types for each gadget
typedef struct GAD_BUTTON_T{
    uint8_t         nothing;    // just here to stop empty type // will remove when something useful can be stored here.
} GAD_BUTTON_T;


typedef struct GET_SCROLLBAR_T {

} GET_SCROLLBAR_T;


typedef struct {
    /// GADGET HOST ///
    /// ** GLOBALE CONTROL HOST STUFF, SYSTEM NEEDS TO KNOW THE GADGET DIMENTIONS AND TYPE, EVERYTHING WELL WILL POINT TO THE ACTUAL GADGET LATER.
    // info about the host, hosting this control
    SBXWindowId     winhnd;     // the window ID, the handler number for the window - useful to "peek at a control cross programs and see who it belongs too"

    // lifetime handles
    // i doubt anyone would ever need more than 1000 gadgets, BUT this allows for scalability later, also keeps with in the 32bit realms of the M7-CPU
    // elaborate naming for easy to read later
    uint16_t        gadgetSlotUsed;    // 0 free, 1 used
    uint16_t        handleGen;         // generation for stale-handle detection

    // control data
    GADGET_CLASS_T  gadgetType; // what type of gadget are we going with;
    GADGET_RECT_T   rect;       // the actionable area (container hit area, basic rectangle info)
    GAD_TOOL_FLAGS  flags;      // flags for this gadget
    uint8_t         enabled;    // enabled/disabled gadget, sort of like if NOT clickable ;)
    uint8_t         visible;    //
    uint8_t         down;       // might need to remove this soon

    char            text[DEF_GADGET_TEXT_SIZE];   // common gadget text

} GADGET_BASE_T;


#endif // SBX_GADGETS_H
