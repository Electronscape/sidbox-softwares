#ifndef CG_WINTYPE_H
#define CG_WINTYPE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cg_type.h"
#include "cg_gadgets.h"
#include "cg_msghandler.h"


#define     WINDOW_TITLE_MAX_LEN    64

// WINDOW HANDLER EVENT RETURN RESULTS // GOD AM TIRED..... BLAH BLAH BLAH
typedef enum {
    CGPROC_DEFAULT      = 0x00,  // not handled, let default / others see it
    CGPROC_NORMAL       = 0x01,  // handled, but propagation allowed
    // anything in between is likely a use return path
    CGPROC_COMPLETE     = 0xFF   // handled AND stop propagation
} CGWindowProcRes;


typedef CGWindowProcRes (*MSGWndProc)(SBXWindowId win, const CGMessage_t *m);

// keep these all 32bits
typedef struct {
    SBXWindowId     self;               // self id  // we'll keep this so its much quicker to find the window (findWindowEx(winhandle *hnd) for example
    SBXWindowId     *lptrRef;           // feed the variable handle address here
    uint32_t        flags;
    char            title[WINDOW_TITLE_MAX_LEN];

    GADGET_RECT_T   winrect;            // window geometry (the actual area of the window)
    GADGET_RECT_T   maxrect;            // min/maximum size geometry can grow to!
    GADGET_RECT_T   clientrect;         // this is the inner view port, which will adjust according to what things will be there

    uint8_t         backColour;         // default background colour

    //GADGET_BASE_T   *GADGETS[MAX_GADGETS_PER_WINDOW];   // pointer to the gadget in the pool
    GADGET_BASE_T   **GADGETS;   // pointer to the gadget in the pool

    MSGWndProc      proc;               // process tree id

} sbx_window_t;

typedef enum {
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE }
MouseEvt;


#endif // CG_WINTYPE_H
