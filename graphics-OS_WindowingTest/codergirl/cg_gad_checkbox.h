#ifndef CG_GAD_CHECKBOX_H
#define CG_GAD_CHECKBOX_H

#include "cg_gadgets.h"

typedef struct GAD_CHECKBOX_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    uint8_t         checked;    // 0/1
    char            text[DEF_GADGET_TEXT_SIZE]; // optional label
} GAD_CHECKBOX_T;

extern GAD_CHECKBOX_T   g_chkPool [MAX_CHECKBOXES];










#endif // CG_GAD_CHECKBOX_H
