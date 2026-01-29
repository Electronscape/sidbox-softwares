#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define C64_CPU_HZ_PAL 985248u

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;


void cpuReset(void);
void cpuResetTo(word npc);

int  cpuStep(void);              // executes ONE opcode, returns cycles

void cpu_set_regs(byte A, byte X, byte Y);

void cpu_irq(void);
void cpu_force_cli(void);

int  cpu_call_jsr(word target);  // call subroutine without resetting CPU
int  cpu_call_jsr_resetting(word target, byte A); // legacy helper (optional)



#ifdef __cplusplus
}
#endif
#endif
