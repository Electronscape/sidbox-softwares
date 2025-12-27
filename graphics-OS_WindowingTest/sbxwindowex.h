#ifndef SBXWINDOWEX_H
#define SBXWINDOWEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "sbxguisbw.h"
#include "sbxctrlex.h"


#define     WIN_TITLE_HEIGHT        16      // title bar height

// goes on the top left of the window
#define     WIN_CLOSE_WIDTH         20      // the close box height is the same as title height

// this go on the far right
#define     WIN_MAXRESTORE_WIDTH    20      // the minmax box height is the same as title height
#define     WIN_ZORDER_WIDTH        20      // same height as title bar, and is before the MINMAX icon
#define     WIN_MINIMISE_WIDTH      20
#define     WIN_RESIZE_GLYPH_SIZE   20

#define     WIN_BORDER              4       // border around the window frame

#define     WIN_BORDER_PEN          3
#define     WIN_BG_PEN              1
#define     WIN_TITLE_PEN           16
#define     WIN_SCROLLER_PROP_PEN   4

#define     WIN_BEVEL_H             2
#define     WIN_BEVEL_L             16      // actual black


#define     MAX_WINDOWS             16      // anymore then the user should probably use a real PC


// radio button things
#define RADIO_DIAM      12
#define RADIO_GAP       6
#define RADIO_H         16


// check boxes
#define CB_BOX          12
#define CB_GAP          6
#define CB_H            16



#define SBX_MIN_WIN_W  120
#define SBX_MIN_WIN_H  80





#define SBW_INVALID_ID ((SBXWindowId)0xFF)

// keep these all 32bits

typedef struct __attribute__((aligned(8))) {
    // geometry
    int16_t x, y, w, h;

    // full client rect (inside border + below titlebar)
    int16_t cx, cy, cw, ch;

    // app/content rect (client minus docked controls)
    int16_t ax, ay, aw, ah;

    // metadata
    char title[64];
    uint32_t flags;

    // controls
    sbx_control_t  ctrls[MAX_CONTROLS];
    uint8_t ctrl_count;
    SBXWindowId id;     // self id
} sbx_window_t;




typedef enum {
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
    SBX_WF_ALWAYS_TO_BACK   = 1u << 11,
    SBX_WF_ALWAYS_TO_FRONT  = 1u << 12,
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
    SBXWindowId  id;
    WHitRegion   region;
} WHitResult;


typedef struct {
    int16_t x0, y0, x1, y1; // [x0,x1), [y0,y1)
    uint8_t enabled;
} UIClip;


typedef enum { MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE } MouseEvt;

#define     WIN_DEFAULT_FLAGS   (SBX_WF_VISIBLE | SBX_WF_MOVEABLE | (SBX_WF_CLOSE | SBX_WF_TITLE_BAR | SBX_WF_MINIMISE | SBX_WF_MAXRESTORE | SBX_WF_ZORDER) )


int16_t       SBOS_getScrollX(sbx_window_t *w, uint16_t ctrl_id);
int16_t       SBOS_getScrollY(sbx_window_t *w, uint16_t ctrl_id);
sbx_window_t* SBOS_getWindow(SBXWindowId id);
void          SBOS_destroyWindow(SBXWindowId id);
void          SBOS_paintWindow(SBXWindowId id);
void          SBOS_paintAllWindows(void);
void          SBOS_bringToFront(SBXWindowId id);
void          SBOS_setFocus(SBXWindowId id);


void          SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my);


SBXWindowId   SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags);


// API access
void SBOS_DestroyControl(sbx_window_t *w, SBControlHandle h);
void SBOS_CtrlSetText(sbx_window_t *w, SBControlHandle h, const char *text);
void SBOS_CtrlSetVisible(sbx_window_t *w, SBControlHandle h, uint8_t visible);
void SBOS_MoveScrollbar(sbx_window_t *w, SBControlHandle h, int16_t x, int16_t y, int16_t ww, int16_t hh, uint8_t orient);





////////// controls ///////////////
// call back

SBControlHandle SBOS_CreateScrollbar(sbx_window_t *w, uint8_t orient, uint8_t dock, int16_t thickness, int16_t min, int16_t max, int16_t value, int16_t step);
SBControlHandle SBOS_CreateButton(sbx_window_t *w, int16_t x, int16_t y, int16_t bw, int16_t bh, const char *text);
SBControlHandle SBOS_CreateLabel(sbx_window_t *w, int16_t x, int16_t y, const char *text);

// radio buttons
SBControlHandle SBOS_CreateRadioButton(sbx_window_t *w, int16_t x, int16_t y, uint8_t groupid, const char *text, uint8_t checked);
void SBOS_RadioSetChecked(sbx_window_t *w, SBControlHandle h, uint8_t checked);
uint8_t SBOS_RadioGetChecked(sbx_window_t *w, SBControlHandle h);

// check boxs
SBControlHandle SBOS_CreateCheckbox(sbx_window_t *w, int16_t x, int16_t y, const char *text, uint8_t checked);






#endif // SBXWINDOWEX_H
