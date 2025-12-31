#ifndef CG_GAD_BUTTON_H
#define CG_GAD_BUTTON_H


#include "cg_wintype.h"
#include "cg_gadgets.h"

typedef void (*fnButtonCallBack)(void *button);

// create types for each gadget
typedef struct GAD_BUTTON_T{
    //-------------- common parts to the GADGET -----------------
    GAD_HDR_T       h;
    uint8_t         used;
    //-----------------------------------------------------------

    char            text[DEF_GADGET_TEXT_SIZE];   // common gadget text
    // cycle button stuff
    char            *options[32];                 // pointer to the text location its smaller and faster
    int             current_option;               // index of the currently displayed option
    int             max_options;                  // maximum options found

    fnButtonCallBack callbackRouteA;              // basic callback route
} GAD_BUTTON_T;

// INTERNALS ------------------------------------------------------------------------------------------------------
extern GAD_BUTTON_T g_btnPool [MAX_BUTTONS];


// MOUSE EVENTS ---------------------------------------------------------------------------------------------------
uint32_t onMouseReleaseButton(GADGET_BASE_T *g, int16_t *mx, int16_t *my);

// API INTERFACES -------------------------------------------------------------------------------------------------

uint32_t SBOS_setButtonCallBack(CGGadgetHandle h, fnButtonCallBack func);




#endif // CG_GAD_BUTTON_H
