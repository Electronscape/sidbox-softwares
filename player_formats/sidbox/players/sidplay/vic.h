#ifndef VIC_H
#define VIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Single global VIC state
extern uint8_t  VICREG[0x40];     // $D000-$D03F
extern uint16_t VICRASTER;        // 0..311
extern uint32_t VIC_CYC_ACC;      // cycle accumulator
extern uint8_t  VIC_IRQ_LINE;     // 1 = asserted

void    vic_reset(void);

uint8_t vic_read(uint16_t addr);
void    vic_write(uint16_t addr, uint8_t val);

void    vic_step(int cpu_cycles);

// helpers
static inline uint8_t vic_irq_asserted(void){ return VIC_IRQ_LINE; }
void    vic_clear_irq(void);

#ifdef __cplusplus
}
#endif

#endif
