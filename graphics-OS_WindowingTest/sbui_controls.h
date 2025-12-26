#ifndef UICONTROLS_H
#define UICONTROLS_H


#include <stdlib.h>
#include <stdint.h>


typedef enum {
    CTL_LABEL,
    CTL_BUTTON
} CtlType;

typedef struct {
    CtlType type;
    uint16_t id;
    int16_t x, y, w, h;     // client-relative
    char text[32];          // for now: label/button only
    uint8_t visible;
    uint8_t down;           // button pressed state
} SBCtrl;



#define MAX_CONTROLS 16



// Prototypes












#endif
