#ifndef M68K_CORE_H
#define M68K_CORE_H

#include "common.h"
#include "bus.h"

#ifdef __cplusplus
extern "C" {
#endif



typedef struct M68K {
    uint32_t d[8];
    uint32_t a[8];     // a7 is SP
    uint32_t pc;
    uint16_t sr;       // status register (we'll implement CCR bits first)
    int      stopped;
} M68K;

void m68k_init(M68K* cpu);
void m68k_reset(M68K* cpu, Bus* bus);
void m68k_step_cycles(M68K* cpu, Bus* bus, int cycles);








#ifdef __cplusplus
}
#endif
#endif
