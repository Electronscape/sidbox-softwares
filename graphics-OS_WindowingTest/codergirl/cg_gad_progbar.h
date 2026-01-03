#ifndef CG_GAD_PROGBAR_H
#define CG_GAD_PROGBAR_H

//#include "cg_wintype.h"
#include "cg_gadgets.h"

#define FPEN_NOCHANGE   -1
#define BPEN_NOCHANGE   -1



// create types for each gadget
typedef struct GAD_PROGBAR_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    int16_t         min;
    int16_t         max;
    int16_t         value;

    uint32_t        flags;
} GAD_PROGBAR_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_PROGBAR_T g_pbPool [MAX_PROGBARS];


// API ------------------------------------------------------------------------------------------------------------
void    SBOS_setProgBarValue(CGGadgetHandle h, int16_t value);
void    SBOS_setProgBarMinMax(CGGadgetHandle h, int16_t newMin, int16_t newMax);

#endif // CG_GAD_PROGBAR_H
