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
    CGEVT_NONE             = 0,
//- gadget events -------------
    CGEVT_GAD_BUTTON_HIT      ,
    CGEVT_GAD_CHECK_CHANGED   ,
    CGEVT_GAD_RADIO_CHANGED   ,
    CGEVT_GAD_LABEL_HIT       ,

    CGEVT_GAD_SCROLL_CHANGED  ,
    CGEVT_GAD_LISTBOX_CHANGED ,
    CGEVT_GAD_LISTBOX_DBLHIT  ,

    CGEVT_GAD_GRIDSEL_CHANGED ,

//- window events -------------
    CGEVT_WIN_CLOSE_REQUEST   ,
    CGEVT_WIN_MOVE            ,
    CGEVT_WIN_MOVED           ,

    CGEVT_WIN_RESIZE          ,
    CGEVT_WIN_RESIZED         ,

    CGEVT_WIN_ZORDER          ,
    CGEVT_WIN_MAXRESTORED     ,
    CGEVT_WIN_MINIMISE        ,

//- system reserved events
    CGEVT_SYS_FILERQ_DONE     = 0x8000u, // a feed back for when a file requester is done :)
    CGEVT_SYS_FILERQ_CHANGED  = 0x8001u, // if list changed (might be useful)
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
#define MSG_PTR(p)      ((int32_t)(uintptr_t)(p))
#define MSG_AS_PTR(t,p) ((t*)(uintptr_t)(p))

int16_t cg_os_messagehandler(uint8_t msgticks);
uint8_t SBOS_PostMessage(const CGMessage_t *m);
uint8_t SBOS_PopMessage(CGMessage_t *out);

void CG_PostGadgetMsg(SBXWindowId win, CGGadgetHandle gad, uint16_t evt, int32_t a, int32_t b, int32_t c, int32_t d);
void CG_PostWindowMsg(SBXWindowId win, uint16_t evt, int32_t a, int32_t b, int32_t c, int32_t d);



// API ------------------------------------------------------------------------------------------------------------
void SBOS_CG_PostWindowMsg(SBXWindowId win, CGMsgType messagetype, uint16_t evt, int32_t a, int32_t b, int32_t c, int32_t d);

#endif // CG_MSGHANDLER_H
