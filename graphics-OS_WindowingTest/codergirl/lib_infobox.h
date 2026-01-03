#ifndef LIB_INFOBOX_H
#define LIB_INFOBOX_H

#include "cg_type.h"

#define INFOBOX_OK     0


typedef enum {
    INFOBOX_OKONLE      = 0x00,   // OK only
} INFBOX_FLAGS;

typedef struct CGFInfoBoxParams {
    const char *title;
    const char *txtMsg;
    void       *user;
} CGFInfoBoxParams;


SBXWindowId SBOS_InfoBox(SBXWindowId owner_winhnd, const CGFInfoBoxParams *p);

// Convenience wrapper for your app code style:
static inline SBXWindowId SBOS_InfoBoxSimple(SBXWindowId owner_winhnd,
                                                const char *title,
                                                const char *message)
{
    CGFInfoBoxParams p = {0};
    p.title = title;
    p.txtMsg = message;
    p.user = (void*)0xC0FFEE;
    return SBOS_InfoBox(owner_winhnd, &p);
}


void SBOS_CloseInfoBox(SBXWindowId msgbox_winhnd);

uint16_t SBOS_infbox_used_count(void);
uint16_t SBOS_infbox_capacity(void);
uint16_t SBOS_infbox_poolsize(void);
uint16_t SBOS_infbox_poolsize1(void);


#endif // LIB_INFOBOX_H
