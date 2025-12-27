#ifndef SBX_WINDOWEX_H
#define SBX_WINDOWEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sbx_render.h"
#include "sbx_gadgets.h"

#define     MAX_WINDOWS             16      // anymore then the user should probably use a real PC


#define     WINDOW_TITLE_MAX_LEN    64

// Window FRAME definition
#define     WIN_TITLE_HEIGHT        16      // title bar height

// left side
#define     WIN_CLOSE_WIDTH         20      // the close box height is the same as title height

// this go on the far right
#define     WIN_MAXRESTORE_WIDTH    20      // the minmax box height is the same as title height
#define     WIN_ZORDER_WIDTH        20      // same height as title bar, and is before the MINMAX icon
#define     WIN_MINIMISE_WIDTH      20
#define     WIN_RESIZE_GLYPH_SIZE   20

#define     WIN_BORDER              4       // border around the window frame



#define     SBW_INVALID_ID          ((SBXWindowId)0xFF)


// keep these all 32bits
typedef struct {
    SBXWindowId     self;               // self id  // we'll keep this so its much quicker to find the window (findWindowEx(winhandle *hnd) for example
    uint32_t        flags;
    char            title[WINDOW_TITLE_MAX_LEN];

    GADGET_RECT_T   winrect;            // geometry
    GADGET_RECT_T   winviewrect;        // the area inside border + below titlebar
    GADGET_RECT_T   contentviewrect;    // the area inside the border, title, (the window frame basically)

    GADGET_BASE_T   *GADGETS[MAX_GADGETS_PER_WINDOW];   // pointer to the gadget in the pool
} sbx_window_t;

typedef enum {
    // system cosmetic
    SBX_WF_VISIBLE          = 1u << 0,
    SBX_WF_NOBORDER         = 1u << 1,

    // gadgets
    SBX_WF_CLOSE            = 1u << 2,
    SBX_WF_TITLE_BAR        = 1u << 3,
    SBX_WF_ZORDER           = 1u << 4,
    SBX_WF_MINIMISE         = 1u << 5,
    SBX_WF_MAXRESTORE       = 1u << 6,

    // window behaviours
    SBX_WF_SCREENBOUND      = 1u << 7,
    SBX_WF_MOVEABLE         = 1u << 8,
    SBX_WF_RESIZABLE        = 1u << 9,
    SBX_WF_NOFOCUS          = 1u << 10,
    SBX_WF_NOAUTOZORDER     = 1u << 11,
    SBX_WF_ALWAYS_TO_BACK   = 1u << 12,
    SBX_WF_ALWAYS_TO_FRONT  = 1u << 13,
} sbx_window_flags_t;


typedef enum {
    WH_NONE = 0,
    WH_CLIENT,
    WH_TITLE,
    WH_CLOSE,
    WH_ZORDER,
    WH_MAXRESTORE,
    WH_MINIMISE,
    WH_RESIZE,
} WHitRegion;

typedef struct {
    SBXWindowId     id;
    WHitRegion      region;
    SBControlHandle ctrl;   // optional, SBCTL_INVALID if none
} WHitResult;

typedef enum {
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE }
MouseEvt;

#define         WIN_DEFAULT_FLAGS   (SBX_WF_VISIBLE | SBX_WF_MOVEABLE | (SBX_WF_CLOSE | SBX_WF_TITLE_BAR | SBX_WF_MINIMISE | SBX_WF_MAXRESTORE | SBX_WF_ZORDER) )

void            initWb(void);

SBXWindowId     SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags);
void            SBOS_destroyWindow(SBXWindowId id);

void            SBOS_paintWindow(SBXWindowId id);
void            SBOS_paintAllWindows(void);

void            SBOS_bringToFront(SBXWindowId id);
void            SBOS_setFocus(SBXWindowId id);









#endif // SBX_WINDOWEX_H
