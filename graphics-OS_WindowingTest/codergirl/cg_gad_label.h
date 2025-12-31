#ifndef CG_GAD_LABEL_H
#define CG_GAD_LABEL_H

#include "cg_wintype.h"
#include "cg_gadgets.h"


// create types for each gadget
typedef struct GAD_LABEL_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    char            text[DEF_GADGET_TEXT_SIZE];   // common gadget text

    uint8_t         bgColour;       // inherit the bgcolour from window on creation
} GAD_LABEL_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_LABEL_T g_lblPool [MAX_LABELS];


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------


// API INTERFACES -------------------------------------------------------------------------------------------------
uint32_t SBOS_setLabelText(SBControlHandle h, const char *text);




#endif // CG_GAD_LABEL_H
