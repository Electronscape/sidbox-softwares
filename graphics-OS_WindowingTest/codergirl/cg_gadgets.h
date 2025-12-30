#ifndef CG_GADGETS_H
#define CG_GADGETS_H

// gadget root file


#include "cg_type.h"
#include <stdint.h>
#include "cg_itemlist.h"


#define     MAX_BUTTONS             32
#define     MAX_CHECKBOXES          32
#define     MAX_RADIOS              32
#define     MAX_SCROLLBARS          32
#define     MAX_BITMAPVIEWS         8
#define     MAX_LISTBOXES           16


// MASTER COLLECTION change this higher for when the OS is tested with programs, 16 is enough for BASE TESTING nothing serious
#define     MAX_GADGETS         (   MAX_BUTTONS +       \
                                    MAX_CHECKBOXES +    \
                                    MAX_RADIOS +        \
                                    MAX_SCROLLBARS +    \
                                    MAX_BITMAPVIEWS +   \
                                    MAX_LISTBOXES       )




#define     SB_SCROLL_THICK         16      // default track thickness
#define     SB_ARROW_SHRINK         16      // arrow sizes

// colour defines





#define     MAX_GADGETS_PER_WINDOW  16   // this is low, but its for testing. WILL increase this for a normal size
#define     DEF_GADGET_TEXT_SIZE    128



// --------- HANDLE TYPE (index + generation) ----------
typedef uint32_t SBControlHandle;
#define SBCTL_INVALID           ((SBControlHandle)0xFFFFFFFF)
#define SBCTL_MAKE(gen, idx)    ((SBControlHandle)((((uint32_t)(gen) & 0xFFFFu) << 16) | ((uint32_t)(idx) & 0xFFFFu)))
#define SBCTL_IDX(h)            ((uint16_t)((h) & 0xFFFFu))
#define SBCTL_GEN(h)            ((uint16_t)(((h) >> 16) & 0xFFFFu))


// -----------------------------------------------------



typedef enum {
    GAD_TOOL_DEFAULT        = (1 << 0),
    GAD_TOOL_DOCKED_RIGHT   = (1 << 1),     // right dock used
    GAD_TOOL_DOCKED_BOTTOM  = (1 << 2),     // bottom dock used
    GAD_TOOL_CYCLEBUTTON    = (1 << 3),     // button cycle flag
    GAD_TOOL_SCROLLARROWS   = (1 << 4),     // enable the arrows on the scrollbars
    GAD_TOOL_NOBORDER       = (1 << 5),     // no border around gadgets
    GAD_TOOL_INSET          = (1 << 6),     // invert the bevel on gadgets
} GAD_TOOL_FLAGS;

typedef enum BMV_FLAGS_T {
    BVF_SHOW_FRAME          = (1 << 0),
    BVF_PAN                 = (1 << 1),
    BVF_SRC_ROWMAJOR        = (1 << 2),     // src = pixels[y*stride + x]
    BVF_SRC_XMAJOR          = (1 << 3)      // src = pixels[x*stride + y] (optional)

} BMV_FLAGS_T;

// control types
typedef enum GADGET_CLASS_T {
    GAD_NULL        = 0,
    GAD_BUTTON,
    GAD_CHECKBOX,
    GAD_RADIO,
    GAD_SCROLLBAR,
    GAD_BITMAPVIEW,
    GAD_LISTBOX,
} GADGET_CLASS_T;





typedef struct GAD_HDR_T {
    GADGET_RECT_T   rect;       // the actionable area (container hit area, basic rectangle info)
    uint32_t        flags;      // flags for this gadget
    uint8_t         enabled;    // enabled/disabled gadget, sort of like if NOT clickable ;)
    uint8_t         visible;    //
    uint8_t         down;       // might need to remove this soon
} GAD_HDR_T;

typedef struct {
    /// GADGET HOST ///
    /// ** GLOBALE CONTROL HOST STUFF, SYSTEM NEEDS TO KNOW THE GADGET DIMENTIONS AND TYPE, EVERYTHING WELL WILL POINT TO THE ACTUAL GADGET LATER.

    SBXWindowId     winhnd;     // the window ID, the handler number for the window - useful to "peek at a control cross programs and see who it belongs too"

    // lifetime handles
    uint16_t        gadgetSlotUsed;    // 0 free, 1 used
    uint16_t        handleGen;         // generation for stale-handle detection

    // control data
    GADGET_CLASS_T  gadgetType; // what type of gadget are we going with;
    void            *gadget;    // the gadget host (what ever the gadget is assigned here)

} GADGET_BASE_T;







void SBOS_gadgetsInit(void);

SBControlHandle SBOS_addScrollbar(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h,
                                  uint8_t orient,  int16_t min, int16_t max,
                                  int16_t step,    int16_t initial_pct,  uint32_t flags);


SBControlHandle SBOS_addRadioButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t group, uint8_t checked, GAD_TOOL_FLAGS flags);
SBControlHandle SBOS_addButton     (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags);
SBControlHandle SBOS_addCheckbox   (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t initial_checked, GAD_TOOL_FLAGS flags);
SBControlHandle SBOS_addBitmapView (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, int16_t bmp_w, int16_t bmp_h, int16_t bmp_stride, uint32_t bv_flags, uint32_t flags);
SBControlHandle SBOS_addListBox    (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, ItemLists_t *items, uint32_t flags);

SBControlHandle base_to_handle(GADGET_BASE_T *g);

GADGET_BASE_T*  SBOS_gadgetFromHandle(SBControlHandle h);
GAD_HDR_T*      SBOS_gadgetHdr(GADGET_BASE_T *g);
void            SBOS_destroyGadget(SBControlHandle h);




#endif // CG_GADGETS_H
