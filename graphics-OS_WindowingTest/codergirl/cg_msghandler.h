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
    CGEVT_BUTTON_CLICK,

    CGEVT_CHECK_CHANGED,      // a = 0/1
    CGEVT_RADIO_CHANGED,      // a = selected index or 1
    CGEVT_LABEL_CLICK,        // if you ever want it

    CGEVT_SCROLL_CHANGED,     // a = value, b = delta or max
    CGEVT_LISTBOX_CHANGED,    // a = selected index
    CGEVT_LISTBOX_DBLCLICK,   // a = selected index

    CGEVT_GRIDSEL_CHANGED     // a = cell index
} CGEventType;

typedef struct CGMessage_t {
    CGMsgType       mtype;
    SBXWindowId     winhnd;
    CGGadgetHandle  gadget;         // this should contain the callback function too

    uint16_t        eventClass;     // BTN_CLICK, GRID_CHANGE, MS_CLICK, KB_HIT
    int32_t         a, b, c, d;     // param payloads
} CGMessage_t;


#define     CGMSG_QUEUE_CAP     64u


// INTERNALS ------------------------------------------------------------------------------------------------------
int16_t cg_os_messagehandler(uint8_t msgticks);
uint8_t SBOS_PostMessage(const CGMessage_t *m);
uint8_t SBOS_PopMessage(CGMessage_t *out);

void CG_PostGadgetMsg(SBXWindowId win, CGGadgetHandle gad, uint16_t evt, int32_t a, int32_t b);



#endif // CG_MSGHANDLER_H
