#ifndef SBX_GADGETS_H
#define SBX_GADGETS_H

#include <stdint.h>
#include "sbos_itemlist.h"


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
#define     SB_TRACK_PEN        (16)   // <-- change later to your chosen track colour
#define     SB_TRACK_INSET      2





#define     MAX_GADGETS_PER_WINDOW  16   // this is low, but its for testing. WILL increase this for a normal size
#define     DEF_GADGET_TEXT_SIZE    128



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

typedef enum {
    GAD_TOOL_DEFAULT        = (1 << 0),
    GAD_TOOL_DOCKED_RIGHT   = (1 << 1),     // right dock used
    GAD_TOOL_DOCKED_BOTTOM  = (1 << 2),     // bottom dock used
    GAD_TOOL_CYCLEBUTTON    = (1 << 3),     // button cycle flag
    GAD_TOOL_SCROLLARROWS   = (1 << 4),     // enable the arrows on the scrollbars
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


//__attribute__((section(".bsram")))
typedef struct GAD_TEXT_T{
    char            text[DEF_GADGET_TEXT_SIZE];   // common gadget text
} GAD_TEXT_T;

typedef struct GAD_BITMAPVIEW_T {
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------
    // Bitmap source
    const uint8_t  *pixels;      // 8bpp indexed, x-major
    int16_t         bmp_w;
    int16_t         bmp_h;
    int16_t         bmp_stride;  // usually == bmp_h

    // View state (top-left of bitmap in view coords)
    int16_t         scroll_x;
    int16_t         scroll_y;

    // Interaction state (for panning)
    int16_t         pan_start_mx;
    int16_t         pan_start_my;
    int16_t         pan_start_x;
    int16_t         pan_start_y;
    uint8_t         panning;

    // Behaviour flags
    uint32_t        bv_flags;

} GAD_BITMAPVIEW_T;

// create types for each gadget
typedef struct GAD_BUTTON_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    char            text[DEF_GADGET_TEXT_SIZE];   // common gadget text
    // cycle button stuff
    char            *options[32];                 // pointer to the text location its smaller and faster
    int             current_option;               // index of the currently displayed option
    int             max_options;                  // maximum options found
} GAD_BUTTON_T;

typedef struct GAD_CHECKBOX_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    uint8_t         checked;    // 0/1
    char            text[DEF_GADGET_TEXT_SIZE]; // optional label
} GAD_CHECKBOX_T;

typedef struct GAD_RADIO_T{
    //-------------- common parts -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //------------------------------------------------

    uint8_t         group;      // group id: 0..255 (per-window grouping)
    uint8_t         checked;    // 0/1
    char            text[DEF_GADGET_TEXT_SIZE];
} GAD_RADIO_T;


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


// scroll bar needs extra bits -------------------------------------------
typedef enum {
    SB_ORIENT_VERT = 0,
    SB_ORIENT_HORZ = 1
} SB_ORIENT;

typedef enum {
    SB_PART_NONE = 0,
    SB_PART_THUMB,
    SB_PART_TRACK,
    SB_PART_ARROW_UP,
    SB_PART_ARROW_DOWN,
    SB_PART_ARROW_LEFT,
    SB_PART_ARROW_RIGHT
} SBPart;

typedef struct GAD_SCROLLBAR_T{
    // common
    GAD_HDR_T   h;
    uint8_t     used;

    // behaviour
    uint8_t     orient;      // SB_ORIENT_*
    int16_t     min;         // used for thumb sizing + optional conversion
    int16_t     max;
    int16_t     step;        // affects thumb size + step in percent
    //int16_t     pct;         // 0..100 ALWAYS (this is the scrollbar value)
    int16_t     value;      // the actual value between min and max

    // interaction
    uint8_t     dragging;
    int16_t     drag_off;    // mouse offset inside thumb (in track axis)

    // arrows
    uint8_t     show_arrows;    // needed for if we're using arrows
} GAD_SCROLLBAR_T;
//------------------------------------------------------------------------



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










#endif // SBX_GADGETS_H
