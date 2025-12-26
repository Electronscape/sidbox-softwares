#ifndef WINDOWEX_H
#define WINDOWEX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "sbui_controls.h"

#define     WIN_TITLE_HEIGHT    16      // title bar height

// goes on the top left of the window
#define     WIN_CLOSE_WIDTH     22      // the close box height is the same as title height

// this go on the far right
#define     WIN_MINMAX_WIDTH    22      // the minmax box height is the same as title height
#define     WIN_ZORDER_WIDTH    22      // same height as title bar, and is before the MINMAX icon


#define     WIN_BORDER          5       // border around the window frame

#define     WIN_BORDER_PEN      3
#define     WIN_BG_PEN          1
#define     WIN_TITLE_PEN       16

#define     WIN_BEVEL_H         2
#define     WIN_BEVEL_L         16      // actual black


#define     MAX_WINDOWS         16      // anymore then the user should probably use a real PC

typedef uint8_t SBWindowId;

#define SBW_INVALID_ID ((SBWindowId)0xFF)

// keep these all 32bits

typedef int16_t SBControlHandle;         // index into global pool, -1 = none
#define SBCTL_NONE ((SBControlHandle)-1)

typedef struct __attribute__((aligned(8))) {
int16_t x, y, w, h;
    int16_t cx, cy, cw, ch;
    char title[64];
    uint32_t flags;

    SBCtrl  ctrls[MAX_CONTROLS];
    uint8_t ctrl_count;

} SBWindow_t;


enum {
    SBW_VISIBLE         = 1 << 0,   // is visable
    SBW_RESIZABLE       = 1 << 1,   // can resize
    SBW_TITLE_BAR       = 1 << 2,   // title bar
    SBW_CLOSE           = 1 << 3,   // has close button
    SBW_ZORDER          = 1 << 4,   // can be pushed to the back
    SBW_MAXRESTORE      = 1 << 5,   // can be maximized or return to original size
    SBW_NOBORDER        = 1 << 6,   // hides the border
    SBW_ALWAYS_TO_BACK  = 1 << 7,   // make window ALWAYS remain at the back, cannot be zorder to front
    SBW_ALWAYS_TO_FRONT = 1 << 8,   // make window ALWAYS on top
    SBW_NOFOCUS         = 1 << 9,   // no focus
    SBW_SCREENBOUND     = 1 << 10,  // only allows windows inside the screen area
};

typedef enum {
    WH_NONE = 0,
    WH_CLIENT,
    WH_TITLE,
    WH_CLOSE,
    WH_ZORDER,
    WH_MAXRESTORE
} WHitRegion;

typedef struct {
    SBWindowId  id;
    WHitRegion  region;
} WHitResult;

typedef struct {
    int16_t x0, y0, x1, y1; // [x0,x1), [y0,y1)
    uint8_t enabled;
} UIClip;


typedef enum { MOUSE_DOWN, MOUSE_UP, MOUSE_MOVE } MouseEvt;








#define     WIN_DEFAULT_FLAGS   (SBW_VISIBLE | SBW_RESIZABLE | SBW_TITLE_BAR | SBW_CLOSE | SBW_ZORDER | SBW_MAXRESTORE)



SBWindowId  SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags);
SBWindow_t* SBOS_getWindow(SBWindowId id);
void        SBOS_destroyWindow(SBWindowId id);
void        SBOS_paintWindow(SBWindowId id);
void        SBOS_paintAllWindows(void);
void        SBOS_bringToFront(SBWindowId id);
void        SBOS_setFocus(SBWindowId id);


void        SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my);









////////// controls ///////////////
int SBOS_addButton(SBWindow_t *w, uint16_t id, int16_t x, int16_t y, int16_t bw, int16_t bh, const char *text);
int SBOS_addLabel(SBWindow_t *w, uint16_t id, int16_t x, int16_t y, const char *text);

#endif // WINDOWEX_H
