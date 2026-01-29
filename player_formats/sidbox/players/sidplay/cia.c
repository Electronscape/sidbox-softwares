#include "cia.h"
#include <string.h>

// ===== Registers (offsets) =====
#define CIA_R(a)   ((uint8_t)((a) & 0x0F))

#define CIA_PRA     0x00
#define CIA_PRB     0x01
#define CIA_DDRA    0x02
#define CIA_DDRB    0x03
#define CIA_TALO    0x04
#define CIA_TAHI    0x05
#define CIA_TBLO    0x06
#define CIA_TBHI    0x07
#define CIA_TOD10   0x08
#define CIA_TODSEC  0x09
#define CIA_TODMIN  0x0A
#define CIA_TODHR   0x0B
#define CIA_SDR     0x0C
#define CIA_ICR     0x0D
#define CIA_CRA     0x0E
#define CIA_CRB     0x0F

// ===== CRA/CRB bits (subset) =====
#define CR_START    0x01
#define CR_RUNMODE  0x08   // 1=one-shot, 0=continuous
#define CR_LOAD     0x10   // force load latch -> counter (self clears)

// ===== ICR bits =====
#define ICR_TA      0x01
#define ICR_TB      0x02
#define ICR_SETCLR  0x80   // write bit7 selects set/clear enable bits
#define ICR_IRQLATCH 0x80  // read bit7 = (pending & mask) != 0

typedef struct {
    uint8_t  reg[0x10];

    uint16_t ta, tb;
    uint16_t ta_latch, tb_latch;

    uint8_t  icr_pending;   // bits 0..4
    uint8_t  icr_mask;      // bits 0..4

    uint8_t  irq_line;      // 0/1
} CIAState;

static CIAState cia[2];

// Public globals (aliases for your “vic.c style” access)
uint8_t  CIA1REG[0x10];
uint16_t CIA1_TA;
uint16_t CIA1_TB;
uint8_t  CIA1_IRQ_LINE;

uint8_t  CIA2REG[0x10];
uint16_t CIA2_TA;
uint16_t CIA2_TB;
uint8_t  CIA2_IRQ_LINE;

static inline void cia_sync_out(cia_chip_t chip){
    CIAState *c = &cia[chip];

    if (chip == CIA_CHIP_1){
        memcpy(CIA1REG, c->reg, 0x10);
        CIA1_TA       = c->ta;
        CIA1_TB       = c->tb;
        CIA1_IRQ_LINE = c->irq_line;
    } else {
        memcpy(CIA2REG, c->reg, 0x10);
        CIA2_TA       = c->ta;
        CIA2_TB       = c->tb;
        CIA2_IRQ_LINE = c->irq_line;
    }
}

static inline void cia_sync_in(cia_chip_t chip){
    CIAState *c = &cia[chip];

    if (chip == CIA_CHIP_1){
        memcpy(c->reg, CIA1REG, 0x10);
        c->ta       = CIA1_TA;
        c->tb       = CIA1_TB;
        c->irq_line = CIA1_IRQ_LINE;
    } else {
        memcpy(c->reg, CIA2REG, 0x10);
        c->ta       = CIA2_TA;
        c->tb       = CIA2_TB;
        c->irq_line = CIA2_IRQ_LINE;
    }
}

static inline void cia_update_irq(CIAState *c){
    c->irq_line = (uint8_t)((c->icr_pending & c->icr_mask) ? 1 : 0);
}

static inline void cia_timer_event(CIAState *c, uint8_t bit){
    c->icr_pending |= (uint8_t)(bit & 0x1F);
    cia_update_irq(c);
}

static inline void cia_force_load_a(CIAState *c){
    c->ta = c->ta_latch;
}
static inline void cia_force_load_b(CIAState *c){
    c->tb = c->tb_latch;
}

static inline uint8_t cia_read_icr(CIAState *c){
    uint8_t v = (uint8_t)(c->icr_pending & 0x1F);
    if (v & c->icr_mask) v |= ICR_IRQLATCH;

    // common behaviour: reading clears pending
    c->icr_pending = 0;
    cia_update_irq(c);
    return v;
}

void cia_reset(cia_chip_t chip){
    CIAState *c = &cia[chip];
    memset(c, 0, sizeof(*c));

    // power-up-ish defaults
    c->reg[CIA_CRA] = 0x00;
    c->reg[CIA_CRB] = 0x00;

    // mirror out to globals
    cia_sync_out(chip);
}

void cia_reset_all(void){
    cia_reset(CIA_CHIP_1);
    cia_reset(CIA_CHIP_2);
}

uint8_t cia_read(cia_chip_t chip, uint16_t addr){
    CIAState *c = &cia[chip];
    uint8_t r = CIA_R(addr);

    switch (r){
    case CIA_TALO: return (uint8_t)(c->ta & 0xFF);
    case CIA_TAHI: return (uint8_t)(c->ta >> 8);
    case CIA_TBLO: return (uint8_t)(c->tb & 0xFF);
    case CIA_TBHI: return (uint8_t)(c->tb >> 8);

    case CIA_ICR: {
        uint8_t v = cia_read_icr(c);
        cia_sync_out(chip);
        return v;
    }

    default:
        return c->reg[r];
    }
}

void cia_write(cia_chip_t chip, uint16_t addr, uint8_t val){
    CIAState *c = &cia[chip];
    uint8_t r = CIA_R(addr);

    switch (r){
    case CIA_TALO:
        c->ta_latch = (uint16_t)((c->ta_latch & 0xFF00) | val);
        c->reg[CIA_TALO] = val;
        break;

    case CIA_TAHI:
        c->ta_latch = (uint16_t)((c->ta_latch & 0x00FF) | ((uint16_t)val << 8));
        c->reg[CIA_TAHI] = val;
        break;

    case CIA_TBLO:
        c->tb_latch = (uint16_t)((c->tb_latch & 0xFF00) | val);
        c->reg[CIA_TBLO] = val;
        break;

    case CIA_TBHI:
        c->tb_latch = (uint16_t)((c->tb_latch & 0x00FF) | ((uint16_t)val << 8));
        c->reg[CIA_TBHI] = val;
        break;

    case CIA_ICR: {
        uint8_t bits = (uint8_t)(val & 0x1F);
        if (val & ICR_SETCLR) c->icr_mask |= bits;
        else                  c->icr_mask &= (uint8_t)~bits;

        // keep mirror in reg for debugging/visibility
        c->reg[CIA_ICR] = (uint8_t)(c->icr_mask & 0x1F);

        cia_update_irq(c);
        break;
    }

    case CIA_CRA: {
        uint8_t old = c->reg[CIA_CRA];
        c->reg[CIA_CRA] = val;

        if (val & CR_LOAD){
            cia_force_load_a(c);
            c->reg[CIA_CRA] &= (uint8_t)~CR_LOAD;
        }

        // Helpful behaviour: starting with counter=0 loads latch
        if (!(old & CR_START) && (c->reg[CIA_CRA] & CR_START)){
            if (c->ta == 0) cia_force_load_a(c);
        }
        break;
    }

    case CIA_CRB: {
        uint8_t old = c->reg[CIA_CRB];
        c->reg[CIA_CRB] = val;

        if (val & CR_LOAD){
            cia_force_load_b(c);
            c->reg[CIA_CRB] &= (uint8_t)~CR_LOAD;
        }

        if (!(old & CR_START) && (c->reg[CIA_CRB] & CR_START)){
            if (c->tb == 0) cia_force_load_b(c);
        }
        break;
    }

    default:
        c->reg[r] = val;
        break;
    }

    cia_sync_out(chip);
}

void cia_step(cia_chip_t chip, int cpu_cycles){
    if (cpu_cycles <= 0) return;

    CIAState *c = &cia[chip];

    // Timer A counts CPU cycles if running
    if (c->reg[CIA_CRA] & CR_START){
        for (int i = 0; i < cpu_cycles; i++){
            if (c->ta == 0){
                // underflow event
                cia_timer_event(c, ICR_TA);

                if (c->reg[CIA_CRA] & CR_RUNMODE){
                    // one-shot stops
                    c->reg[CIA_CRA] &= (uint8_t)~CR_START;
                } else {
                    // continuous reload
                    cia_force_load_a(c);
                }
            } else {
                c->ta--;
            }
        }
    }

    // Timer B (same simple CPU-clock model)
    if (c->reg[CIA_CRB] & CR_START){
        for (int i = 0; i < cpu_cycles; i++){
            if (c->tb == 0){
                cia_timer_event(c, ICR_TB);

                if (c->reg[CIA_CRB] & CR_RUNMODE){
                    c->reg[CIA_CRB] &= (uint8_t)~CR_START;
                } else {
                    cia_force_load_b(c);
                }
            } else {
                c->tb--;
            }
        }
    }

    cia_sync_out(chip);
}

void cia_step_all(int cpu_cycles){
    cia_step(CIA_CHIP_1, cpu_cycles);
    cia_step(CIA_CHIP_2, cpu_cycles);
}
