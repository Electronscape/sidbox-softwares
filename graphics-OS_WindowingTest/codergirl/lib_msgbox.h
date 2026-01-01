#ifndef LIB_MSGBOX_H
#define LIB_MSGBOX_H

#include "cg_type.h"
//#include "cg_windowex.h"     // SBXWindowId
//#include "cg_msghandler.h"   // MSG_PTR if needed

// Result values (in message a)
#define MSGBOX_CANCEL   0
#define MSGBOX_OK       1
#define MSGBOX_YES      2
#define MSGBOX_NO       3

typedef enum {
    MSGBOXF_OK          = 0x00,   // OK only
    MSGBOXF_OKCANCEL    = 0x01,   // OK + Cancel
    MSGBOXF_YESNO       = 0x02,   // Yes + No
    MSGBOXF_YESNOCANCEL = 0x03    // Yes + No + Cancel (optional)
} MSGBOX_FLAGS;

typedef struct CGFMsgBoxParams {
    const char *title;
    const char *txtMsg;
    void       *user;
    uint8_t     flags;   // MSGBOX_FLAGS
} CGFMsgBoxParams;

// Posts CGEVT_SYS_MSGBOX_DONE to owner window.
// a = choice (MSGBOX_*)
// b = MSG_PTR(user)
// c = 0 (reserved)
// d = msgbox window id
SBXWindowId SBOS_MessageBox(SBXWindowId owner_winhnd, const CGFMsgBoxParams *p);

// Convenience wrapper for your app code style:
static inline SBXWindowId SBOS_MessageBoxSimple(SBXWindowId owner_winhnd,
                                                const char *title,
                                                const char *message,
                                                uint8_t flags)
{
    CGFMsgBoxParams p = {0};
    p.title = title;
    p.txtMsg = message;
    p.user = (void*)0xC0FFEE;
    p.flags = flags;
    return SBOS_MessageBox(owner_winhnd, &p);
}

void SBOS_CloseMessageBox(SBXWindowId msgbox_winhnd);

uint16_t SBOS_msgbox_used_count(void);
uint16_t SBOS_msgbox_capacity(void);
uint16_t SBOS_msgbox_poolsize(void);
uint16_t SBOS_msgbox_poolsize1(void);

#endif // LIB_MSGBOX_H
