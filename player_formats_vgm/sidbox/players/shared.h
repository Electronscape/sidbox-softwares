#ifndef SHARED_H
#define SHARED_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ym2612.h"



#define UINT64 uint64_t

#define UINT32  uint32_t
#define UINT16  uint16_t
#define UINT8   uint8_t

#define INT32   int32_t
#define INT16   int16_t
#define INT8    int8_t


#ifndef YM2612_CLOCK
#define YM2612_CLOCK   7670454/2u   // Mega Drive / Genesis NTSC YM2612 clock
#endif

#ifndef YM2612_RATE
#define YM2612_RATE    44100u     // your mixer/output rate
#endif

static UINT32 ym_freqbase_q16;    // Q16 fixed-point: clock/(72*rate)








#endif // SHARED_H
