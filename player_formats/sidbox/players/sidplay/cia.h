#ifndef CIA_H
#define CIA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#define CIA_R(a)       ((uint8_t)((a) & 0x0F))

// Registers
#define CIA_PRA        0x00
#define CIA_PRB        0x01
#define CIA_DDRA       0x02
#define CIA_DDRB       0x03
#define CIA_TALO       0x04
#define CIA_TAHI       0x05
#define CIA_TBLO       0x06
#define CIA_TBHI       0x07
#define CIA_TOD10      0x08
#define CIA_TODSEC     0x09
#define CIA_TODMIN     0x0A
#define CIA_TODHR      0x0B
#define CIA_SDR        0x0C
#define CIA_ICR        0x0D
#define CIA_CRA        0x0E
#define CIA_CRB        0x0F

// Control Register Bits
#define CR_START       0x01
#define CR_PBON        0x02
#define CR_OUTMODE     0x04
#define CR_RUNMODE     0x08
#define CR_LOAD        0x10
#define CR_INMODE      0x20
#define CR_SPMODE      0x40
#define CR_TODIN       0x80

// Interrupt Control Bits
#define ICR_TA         0x01
#define ICR_TB         0x02
#define ICR_TOD        0x04
#define ICR_SP         0x08
#define ICR_FLAG       0x10
#define ICR_IRQLATCH   0x80

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
