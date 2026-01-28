#ifndef CIAIRQ_H
#define CIAIRQ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Timer A
    uint16_t ta;       // current counter
    uint16_t ta_latch; // reload value

    // Control + IRQ
    uint8_t cra;       // Control Register A ($0E)
    uint8_t icr_mask;  // interrupt enable mask (lower 5 bits)
    uint8_t icr_flags; // pending interrupt flags (lower 5 bits)

    uint8_t irq_level; // 1 if IRQ line asserted
} cia_t;



extern cia_t cia1;
extern cia_t cia2;


// init one CIA
void cia_init(cia_t *c);

// register read/write (reg 0..15)
uint8_t cia_read(cia_t *c, uint8_t reg);
void    cia_write(cia_t *c, uint8_t reg, uint8_t v);

// tick by CPU cycles (phi2 cycles). updates timer + irq_level
void cia_tick(cia_t *c, uint32_t cycles);

void cia_set_flag(cia_t *c, uint8_t flag);

#ifdef __cplusplus
}
#endif

#endif // CIAIRQ_H
