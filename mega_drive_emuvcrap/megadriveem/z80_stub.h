#ifndef Z80_STUB_H
#define Z80_STUB_H

#include "common.h"

typedef struct Z80 {
    int dummy;
} Z80;

static inline void z80_init(Z80* z) { memset(z, 0, sizeof(*z)); }
static inline void z80_reset(Z80* z) { (void)z; }

#endif
