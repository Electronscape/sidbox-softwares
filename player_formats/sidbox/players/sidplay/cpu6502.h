#ifndef CPU6502_H
#define CPU6502_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define C64_CPU_HZ_PAL  985248u
#define C64_CPU_HZ_NTSC 1022727u

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t dword;

extern uint8_t playmode_sidtype;


void cpuReset(void);
void cpuResetTo(word npc);

int  cpuStep(void);              // executes ONE opcode, returns cycles
void cpu_nmi(void);
void cpu_irq(void);
uint8_t getIFlagStatus();

void cpu_set_regs(byte A, byte X, byte Y);

void cpu_irq(void);
void cpu_force_cli(void);

int  cpu_call_jsr(word target);  // call subroutine without resetting CPU
int  cpu_call_jsr_resetting(word target, byte A); // legacy helper (optional)



void cpu_set_sp(byte S);
void cpu_push_byte(byte val);

int cpuGetPC(void);
void cpuSetPC(word npc);

void cpu_set_regs(byte A, byte X, byte Y);
void cpu_set_a(byte A);
void cpu_set_x(byte X);
void cpu_set_y(byte Y);
void cpu_set_p(byte P);
void cpu_set_s(byte S);

byte cpu_get_a(void);
byte cpu_get_x(void);
byte cpu_get_y(void);
byte cpu_get_p(void);
byte cpu_get_s(void);

#ifdef __cplusplus
}
#endif
#endif
