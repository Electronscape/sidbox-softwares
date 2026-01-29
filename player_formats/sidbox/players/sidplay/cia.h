#ifndef CIA_H
#define CIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CIA_CHIP_1 = 0,   // $DC00
    CIA_CHIP_2 = 1    // $DD00
} cia_chip_t;

// --- CIA1 globals (DC00) ---
extern uint8_t  CIA1REG[0x10];
extern uint16_t CIA1_TA;
extern uint16_t CIA1_TB;
extern uint8_t  CIA1_IRQ_LINE;

// --- CIA2 globals (DD00) ---
extern uint8_t  CIA2REG[0x10];
extern uint16_t CIA2_TA;
extern uint16_t CIA2_TB;
extern uint8_t  CIA2_IRQ_LINE;

// Reset both or individually
void    cia_reset_all(void);
void    cia_reset(cia_chip_t chip);

// Read/write per-chip (addr is full CPU addr; we only use low 0..0x0F)
uint8_t cia_read (cia_chip_t chip, uint16_t addr);
void    cia_write(cia_chip_t chip, uint16_t addr, uint8_t val);

// Step per-chip or both
void    cia_step(cia_chip_t chip, int cpu_cycles);
void    cia_step_all(int cpu_cycles);

// Helpers
static inline uint8_t cia1_irq_asserted(void){ return CIA1_IRQ_LINE; }
static inline uint8_t cia2_irq_asserted(void){ return CIA2_IRQ_LINE; }

#ifdef __cplusplus
}
#endif

#endif // CIA_H
