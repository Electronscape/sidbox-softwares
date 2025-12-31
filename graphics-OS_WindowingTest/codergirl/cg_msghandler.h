#ifndef CG_MSGHANDLER_H
#define CG_MSGHANDLER_H

// the crude but hopefully functional message sentinal
#include <stdint.h>
#include "cg_type.h"

typedef enum {
    CGMSG_NONE = 0,
    CGMSG_GADGET,
    CGMSG_WINDOW,   // where windows are minimized, maxed, closed, or zordered, Maybe a resized?
    CGMSG_TIMER,    // not yet implemented these yet tho
    CGMSG_MOUSE,    // mouse event, click, move
    CGMSG_KEY,      // KEYBOARD interface is NOT a thing yet! THIS is going to be interesting :/
} CGMsgType;

typedef enum {
    CGEVT_NONE          = 0,
    CGEVT_BUTTON_CLICK
} CGEventType;

typedef struct CGMessage_t {
    CGMsgType       mtype;
    SBXWindowId     winhnd;
    CGGadgetHandle  gadget;         // this should contain the callback function too

    uint16_t        eventClass;     // BTN_CLICK, GRID_CHANGE, MS_CLICK, KB_HIT
    int32_t         a, b;
} CGMessage_t;


#define     CGMSG_QUEUE_CAP     64u


// INTERNALS ------------------------------------------------------------------------------------------------------
void cg_os_messagehandler(uint8_t msgticks);
uint8_t SBOS_PostMessage(const CGMessage_t *m);
uint8_t SBOS_PopMessage(CGMessage_t *out);



#endif // CG_MSGHANDLER_H
