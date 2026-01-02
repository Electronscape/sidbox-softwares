#ifndef CG_WINDOWEX_H
#define CG_WINDOWEX_H

// NAME OF SYSTEM:
// CODERGIRL -- named as such cos this is the windowing system for any coder :) and have to be gentle with the os :)

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cg_theme.h"

#include "cg_wintype.h"
//#include "cg_renderer.h"
#include "cg_gadgets.h"

#define     MAX_WINDOWS             16      // anymore then the user should probably use a real PC




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


#define     SB_BDOCK_OFFSET_X   (-2 )
#define     SB_BDOCK_OFFSET_Y   ( 2 )

#define     SB_RDOCK_OFFSET_X   ( 2 )
#define     SB_RDOCK_OFFSET_Y   ( 1 )

#define     SBW_INVALID_ID          ((SBXWindowId)0xFF)


#define     DEF_DIALOG_BUTTON_WIDTH      80
#define     DEF_DIALOG_BUTTON_HEIGHT     22
#define     DEF_DIALOG_SCROLL_WIDTH      22
#define     DEF_DIALOG_SCROLL_HEIGHT     22


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
    SBX_WF_DOCKRIGHT        = 1u << 14,
    SBX_WF_DOCKBOTTOM       = 1u << 15,
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
    CGGadgetHandle  ctrl;   // optional, SBCTL_INVALID if none
} WHitResult;


extern sbx_window_t     gui_windows[MAX_WINDOWS];
extern uint8_t          gui_used[MAX_WINDOWS];
extern SBXWindowId      g_winZorder[MAX_WINDOWS];
extern uint8_t          g_winZcount;

typedef struct {
    uint16_t win_used;  // windows!

    uint16_t base_used; // BASE gadgets

    uint16_t btn_used;  // buttons
    uint16_t chk_used;  // checkbox
    uint16_t rad_used;  // radio buttons
    uint16_t sb_used;   // scrollbars
    uint16_t bv_used;   // bitmap views
    uint16_t lb_used;   // list box
    uint16_t lbl_used;  // label
    uint16_t gs_used;   // gridselect
    uint16_t cn_used;   // canvas

    // NEW: library dialogs
    uint16_t filerq_used;
    uint16_t msgbox_used;

    // Optional: capacity too (handy for printing)
    //uint16_t filerq_cap;
    //uint16_t msgbox_cap;
} SBOS_UiUsageCounts;

SBOS_UiUsageCounts SBOS_get_ui_usage_counts(void);
//SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void);

uint8_t gadget_mouse_inside(const sbx_window_t *w, const GADGET_BASE_T *g, int16_t mx, int16_t my);

GADGET_BASE_T* hittest_gadget(sbx_window_t *w, int16_t mx, int16_t my);





#define         WIN_DEFAULT_FLAGS   (SBX_WF_VISIBLE | SBX_WF_MOVEABLE | (SBX_WF_CLOSE | SBX_WF_TITLE_BAR | SBX_WF_MINIMISE | SBX_WF_MAXRESTORE | SBX_WF_ZORDER) )

void            initWb(void);

SBXWindowId     SBOS_createWindow(SBXWindowId *selfPTR, int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags);
void            SBOS_destroyWindow(SBXWindowId id);
sbx_window_t*   SBOS_getWindow(SBXWindowId id);
void            SBOS_setWinBackColour(SBXWindowId winId, uint8_t newcolor);
void            SBOS_setWindowProc(SBXWindowId win, MSGWndProc proc);
void            SBOS_setWindowResizeLimits(SBXWindowId win, int16_t minw, int16_t minh, int16_t maxw, int16_t maxh);

void            SBOS_paintWindow(SBXWindowId id);
void            SBOS_paintAllWindows(void);

void            SBOS_bringToFront(SBXWindowId id);
void            SBOS_setFocus(SBXWindowId id);

SBXWindowId     SBOS_getWindowByGadget(const GADGET_BASE_T *b);
CGWindowProcRes SBOS_DefaultWindowProc(SBXWindowId win, const CGMessage_t *m);
uint8_t         SBOS_isWindowValid(SBXWindowId id);


// gadgets interactions from window host
GADGET_RECT_T   win_inner_rect(const sbx_window_t *w);
int16_t         win_gutter_right(const sbx_window_t *w);
int16_t         win_gutter_bottom(const sbx_window_t *w);
int16_t         win_inner_reserve_right(const sbx_window_t *w);
int16_t         win_inner_reserve_bottom(const sbx_window_t *w);
int16_t         sb_thumb_len_from_step(int16_t track_len, int16_t min, int16_t max, int16_t step);
int16_t         sb_thumb_pos_from_value(int16_t value, int16_t min, int16_t max, int16_t travel);

uint8_t         mousept_in_rect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h);
uint8_t         pt_in_r16(int16_t px, int16_t py, const GADGET_RECT_T *r);
GADGET_RECT_T   r16(int16_t x, int16_t y, int16_t w, int16_t h);


// EVENT HANDLING ///////////////////////////////////////////////////////////////////
uint32_t        commitGadgetRelease(sbx_window_t *gw, GADGET_BASE_T *g);

#endif // CG_WINDOWEX_H
