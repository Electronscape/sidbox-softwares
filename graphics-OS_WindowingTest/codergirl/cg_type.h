#ifndef CG_TYPE_H
#define CG_TYPE_H

#include <stdint.h>


#define BYTES_OF(x) ((size_t)sizeof(x))

//#define SBOS_UI_BUDGET_BYTES (64u * 1024u)


// GADGET TYPES
typedef struct GADGET_RECT_T {  // this is likely going to be used for things like scrollbars, with more than one hit regions
    int16_t     x, y, w, h;
} GADGET_RECT_T;

typedef uint8_t SBXWindowId;





#endif // CG_TYPE_H
