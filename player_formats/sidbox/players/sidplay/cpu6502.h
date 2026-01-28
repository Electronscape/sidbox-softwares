#ifndef CPU65_2_H
#define CPU65_2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint8_t     byte;

// Flags (same bits you used)
#define sFLAG_N 0x80
#define sFLAG_V 0x40
#define sFLAG_B 0x10
#define sFLAG_D 0x08
#define sFLAG_I 0x04
#define sFLAG_Z 0x02
#define sFLAG_C 0x01

typedef u8 (*cpu6502_read8_fn)(void *user, u16 addr);
typedef void (*cpu6502_write8_fn)(void *user, u16 addr, u8 val);

typedef struct cpu6502_bus_t {
    void *user;
    cpu6502_read8_fn  read8;
    cpu6502_write8_fn write8;
} cpu6502_bus_t;

typedef struct cpu6502_t {
    // registers
    u8  a, x, y;
    u8  sp;     // stack ptr
    u8  p;     // status
    u16 pc;

    // external
    cpu6502_bus_t bus;

    // optional IRQ/NMI lines (level triggered)
    u8 irq_line;   // 1 = asserted
    u8 nmi_line;   // 1 = asserted
    u8 nmi_latch;  // internal edge latch (optional)
} cpu6502_t;

// Init CPU with bus callbacks
void cpu6502_init(cpu6502_t *c, cpu6502_bus_t bus);

// Reset using RESET vector at $FFFC/$FFFD
void cpu6502_reset(cpu6502_t *c);

// Reset PC to a specific address (useful for PSID init/play)
void cpu6502_reset_to(cpu6502_t *c, u16 pc);

// Execute ONE instruction, return cycles consumed
int cpu6502_step(cpu6502_t *c);

// Execute until cycle budget is consumed (or until PC == 0 if you use that sentinel)
int cpu6502_run(cpu6502_t *c, int cycle_budget);

// Convenience: run subroutine and return when it RTS’s back.
// This mimics your old cpuJSR “push 0,0 then run until pc==0” style.
int cpu6502_jsr(cpu6502_t *c, u16 addr, u8 a_reg);

// Assert/clear IRQ/NMI lines (for later RSID)
void cpu6502_irq(cpu6502_t *c, int level);
void cpu6502_nmi(cpu6502_t *c, int level);





int do_cpuTest(void);















#ifdef __cplusplus
}
#endif

#endif // CPU65_2_H
