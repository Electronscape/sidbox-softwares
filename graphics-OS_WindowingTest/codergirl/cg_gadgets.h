#ifndef CG_GADGETS_H
#define CG_GADGETS_H

// gadget root file


#include "cg_type.h"
#include <stdint.h>
#include "cg_itemlist.h"

//                                  v----- these numbers are HUGE, but for funsies keeping them this high
#define     MAX_EVERYTHING          512

#define     MAX_BITMAPVIEWS         (MAX_EVERYTHING)
#define     MAX_BUTTONS             (MAX_EVERYTHING)
#define     MAX_CHECKBOXES          (MAX_EVERYTHING)
#define     MAX_GRIDSELECTS         (MAX_EVERYTHING)
#define     MAX_LABELS              (MAX_EVERYTHING)
#define     MAX_LISTBOXES           (MAX_EVERYTHING)
#define     MAX_RADIOS              (MAX_EVERYTHING)
#define     MAX_SCROLLBARS          (MAX_EVERYTHING)


// MASTER COLLECTION change this higher for when the OS is tested with programs, 16 is enough for BASE TESTING nothing serious
#define     MAX_GADGETS         (   MAX_BITMAPVIEWS +   \
                                    MAX_BUTTONS +       \
                                    MAX_CHECKBOXES +    \
                                    MAX_GRIDSELECTS +   \
                                    MAX_LABELS +        \
                                    MAX_LISTBOXES +     \
                                    MAX_RADIOS +        \
                                    MAX_SCROLLBARS      )




#define     SB_SCROLL_THICK         16      // default track thickness
#define     SB_ARROW_SHRINK         16      // arrow sizes

// colour defines





#define     MAX_GADGETS_PER_WINDOW  16   // this is low, but its for testing. WILL increase this for a normal size
#define     DEF_GADGET_TEXT_SIZE    128



// --------- HANDLE TYPE (index + generation) ----------
#define SBCTL_INVALID           ((CGGadgetHandle)0xFFFFFFFF)
#define SBCTL_MAKE(gen, idx)    ((CGGadgetHandle)((((uint32_t)(gen) & 0xFFFFu) << 16) | ((uint32_t)(idx) & 0xFFFFu)))
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

// BITMAP VIEW FLAGS
typedef enum BMV_FLAGS_T {
    BVF_SHOW_FRAME          = (1 << 0),
    BVF_PAN                 = (1 << 1),
    BVF_SRC_ROWMAJOR        = (1 << 2),     // src = pixels[y*stride + x]
    BVF_SRC_XMAJOR          = (1 << 3),     // src = pixels[x*stride + y] (optional)
    BVF_WRAP                = (1 << 4)
} BMV_FLAGS_T;

typedef enum {
    GAD_GRIDSEL_JUST_ONE     = (1 << 0),   // in single mode: never allow -1
    GAD_GRIDSEL_TEXT_INVERT  = (1 << 1),   // allows teh text to be inverted colours
} GSEL_FLAG_T;



// control types
typedef enum GADGET_CLASS_T {
    GAD_NULL        = 0,
    GAD_BITMAPVIEW,
    GAD_BUTTON,
    GAD_CHECKBOX,
    GAD_GRIDSELECT,
    GAD_LABEL,
    GAD_LISTBOX,
    GAD_RADIO,
    GAD_SCROLLBAR
} GADGET_CLASS_T;





typedef struct GAD_HDR_T {
    GADGET_RECT_T   rect;       // the actionable area (container hit area, basic rectangle info)
    uint32_t        flags;      // flags for this gadget
    uint8_t         enabled;    // enabled/disabled gadget, sort of like if NOT clickable ;)
    uint8_t         visible;    //
    uint8_t         down;       // might need to remove this soon
    SBXWindowId     winhnd;     // the window ID, the handler number
    CGGadgetHandle  self;       // self handle id

} GAD_HDR_T;

typedef struct {
    /// GADGET HOST ///
    /// ** GLOBALE CONTROL HOST STUFF, SYSTEM NEEDS TO KNOW THE GADGET DIMENTIONS AND TYPE, EVERYTHING WELL WILL POINT TO THE ACTUAL GADGET LATER.

    //SBXWindowId     winhnd;     // the window ID, the handler number for the window - useful to "peek at a control cross programs and see who it belongs too"

    // lifetime handles
    uint16_t        gadgetSlotUsed;    // 0 free, 1 used
    uint16_t        handleGen;         // generation for stale-handle detection

    // control data
    GADGET_CLASS_T  gadgetType; // what type of gadget are we going with;
    void            *gadget;    // the gadget host (what ever the gadget is assigned here)

} GADGET_BASE_T;






//// API ACCESS ///////////
void SBOS_gadgetsInit(void);

CGGadgetHandle SBOS_CreateBitmapView (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, int16_t bmp_w, int16_t bmp_h, int16_t bmp_stride, uint32_t bv_flags, uint32_t flags);
CGGadgetHandle SBOS_CreateButton     (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags);
CGGadgetHandle SBOS_CreateCheckbox   (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t initial_checked, GAD_TOOL_FLAGS flags);
CGGadgetHandle SBOS_CreateGridSelect (SBXWindowId win, int16_t x, int16_t y, int16_t cell_size_x, int16_t cell_size_y, uint8_t cells_x, uint8_t cells_y, uint32_t gridflags, GAD_TOOL_FLAGS flags);
CGGadgetHandle SBOS_CreateLabel      (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags);
CGGadgetHandle SBOS_CreateListBox    (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, ItemLists_t *items, uint32_t flags);
CGGadgetHandle SBOS_CreateRadioButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t group, uint8_t checked, GAD_TOOL_FLAGS flags);
CGGadgetHandle SBOS_CreateScrollbar  (SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t orient,  int16_t min, int16_t max, int16_t step,    int16_t initial_pct,  uint32_t flags);



CGGadgetHandle base_to_handle(GADGET_BASE_T *g);

GADGET_BASE_T*  SBOS_gadgetFromHandle(CGGadgetHandle h);
GAD_HDR_T*      SBOS_gadgetHdr(GADGET_BASE_T *g);


void            SBOS_enableGadget(CGGadgetHandle h, uint8_t enable);
void            SBOS_destroyGadget(CGGadgetHandle h);




#endif // CG_GADGETS_H
