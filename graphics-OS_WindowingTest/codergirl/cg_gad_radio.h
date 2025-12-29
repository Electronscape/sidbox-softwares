#ifndef CG_GAD_RADIO_H
#define CG_GAD_RADIO_H

#include "cg_gadgets.h"

typedef struct GAD_RADIO_T{
    //-------------- common parts -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //------------------------------------------------

    uint8_t         group;      // group id: 0..255 (per-window grouping)
    uint8_t         checked;    // 0/1
    char            text[DEF_GADGET_TEXT_SIZE];
} GAD_RADIO_T;

extern GAD_RADIO_T      g_radPool [MAX_RADIOS];



#endif // CG_GAD_RADIO_H
