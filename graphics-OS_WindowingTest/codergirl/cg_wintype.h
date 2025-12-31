#ifndef CG_WINTYPE_H
#define CG_WINTYPE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "cg_type.h"
#include "cg_gadgets.h"



#define     WINDOW_TITLE_MAX_LEN    64

// keep these all 32bits
typedef struct {
    SBXWindowId     self;               // self id  // we'll keep this so its much quicker to find the window (findWindowEx(winhandle *hnd) for example
    uint32_t        flags;
    char            title[WINDOW_TITLE_MAX_LEN];

    GADGET_RECT_T   winrect;            // window geometry (the actual area of the window)
    GADGET_RECT_T   clientrect;         // this is the inner view port, which will adjust according to what things will be there

    uint8_t         backColour;         // default background colour

    GADGET_BASE_T   *GADGETS[MAX_GADGETS_PER_WINDOW];   // pointer to the gadget in the pool
} sbx_window_t;

typedef enum {
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_MOVE }
MouseEvt;


#endif // CG_WINTYPE_H
