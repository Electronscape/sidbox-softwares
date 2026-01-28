
#include <stdio.h>
#include "ciairq.h"

#define ICR_TA   (1u << 0)   // Timer A
#define ICR_TB   (1u << 1)   // (unused)
#define ICR_ALRM (1u << 2)   // (unused)
#define ICR_SP   (1u << 3)   // (unused)
#define ICR_FLG  (1u << 4)   // (unused)
#define ICR_IRQ  (1u << 7)   // set on read if any enabled source is pending

#define CRA_START    (1u << 0)  // start timer A
#define CRA_PBON     (1u << 1)  // (unused)
#define CRA_OUTMODE  (1u << 2)  // (unused)
#define CRA_RUNMODE  (1u << 3)  // one-shot if 1, continuous if 0
#define CRA_LOAD     (1u << 4)  // force load latch -> counter






cia_t cia1;
cia_t cia2;

static inline void cia_update_irq(cia_t *c) {
    // IRQ asserts if any (flags & mask) is nonzero
    c->irq_level = ((c->icr_flags & c->icr_mask) != 0);
}

void cia_init(cia_t *c) {
    c->ta = 0;
    c->ta_latch = 0;

    c->tb = 0;          // ✅
    c->tb_latch = 0;    // ✅

    c->cra = 0;
    c->crb = 0;         // ✅

    c->icr_mask = 0;
    c->icr_flags = 0;
    c->irq_level = 0;
}

void cia_set_flag(cia_t *c, uint8_t flag) {
    c->icr_flags |= flag;
    cia_update_irq(c);

}

int io_visible(void);;
uint8_t cia_read(cia_t *c, uint8_t reg) {
    reg &= 0x0F;

    if(!io_visible())
        printf("RAW_CIA%c R reg=$%02X IO:%u! => ", (c==&cia1)?'1':'2', reg, io_visible());

    switch (reg) {
        case 0x04: return (uint8_t)(c->ta & 0xFF);        // TALO
        case 0x05: return (uint8_t)(c->ta >> 8);          // TAHI
        case 0x06: return (uint8_t)(c->tb & 0xFF);  // TBLO
        case 0x07: return (uint8_t)(c->tb >> 8);    // TBHI



        case 0x0D: {
            uint8_t pending = (uint8_t)(c->icr_flags & 0x1F);
            uint8_t v = pending;
            if (pending & c->icr_mask) v |= ICR_IRQ;
            c->icr_flags = 0;
            cia_update_irq(c);
            return v;
        }


        case 0x0E: return c->cra;
        case 0x0F: return c->crb;                   // CRB


        default:
            // For RSID-minimum, return 0 for unimplemented regs
            return 0;
    }
    if(!io_visible()){
        printf(" [read]\n");
        fflush(stdout);
    }
}


void cia_write(cia_t *c, uint8_t reg, uint8_t v) {
    reg &= 0x0F;

    printf("RAW_CIA%c W reg=$%02X v=$%02X IO:%u => ", (c==&cia1)?'1':'2', reg, v, io_visible());

    switch (reg) {
        case 0x04: // TALO latch low
            c->ta_latch = (uint16_t)((c->ta_latch & 0xFF00) | (uint16_t)v);
            break;

        case 0x05: // TAHI latch high
            c->ta_latch = (uint16_t)(((uint16_t)v << 8) | (c->ta_latch & 0x00FF));
            if (!(c->cra & CRA_START)) c->ta = c->ta_latch;
            break;

        case 0x06: // TBLO latch low
            c->tb_latch = (uint16_t)((c->tb_latch & 0xFF00) | (uint16_t)v);
            break;

        case 0x07: // TBHI latch high
            c->tb_latch = (uint16_t)(((uint16_t)v << 8) | (c->tb_latch & 0x00FF));
            if (!(c->crb & CRA_START)) {
                c->tb = c->tb_latch;
            }
            break;


        case 0x0D: {
            // ICR write: bit7 = set/clear mask bits, lower 5 bits select which
            uint8_t bits = (uint8_t)(v & 0x1F);
            if (v & 0x80)
                c->icr_mask |= bits;   // set enables
            else {
                c->icr_mask  &= (uint8_t)~bits; // clear enables
                c->icr_flags &= (uint8_t)~bits; // <-- THIS was the only thing added
            }

            // real CIA also clears matching flags when writing with bit7=0 sometimes depending on docs;
            // RSID-minimum: don't clear flags here. Flags cleared on read.
            cia_update_irq(c);
            break;
        }


        case 0x0E: {
            // CRA
            c->cra = v;

            // LOAD bit forces counter = latch (and LOAD bit itself doesn't stay set in real HW)
            if (v & CRA_LOAD) {
                c->ta = c->ta_latch;
                c->cra &= (uint8_t)~CRA_LOAD;
            }
            break;
        }

        case 0x0F: { // CRB
            c->crb = v;
            if (v & CRA_LOAD) {
                c->tb = c->tb_latch;
                c->crb &= (uint8_t)~CRA_LOAD;
            }
            break;
        }


        default: break;
    }
    printf("latch=%04X ta=%04X cra=%02X mask=%02X\n", c->ta_latch, c->ta, c->cra, c->icr_mask);
    fflush(stdout);

}


static inline void cia_tick_timer(
    cia_t *c,
    uint16_t *t,
    uint16_t latch,
    uint8_t *cr,
    uint8_t flag,
    uint32_t cycles
    ){
    if (!(*cr & CRA_START)) return;

    while (cycles) {
        uint32_t to_underflow = (*t == 0) ? 1u : (uint32_t)(*t);

        if (cycles < to_underflow) {
            *t = (uint16_t)(*t - (uint16_t)cycles);
            return;
        }

        cycles -= to_underflow;

        cia_set_flag(c, flag);

        *t = latch;

        if (*cr & CRA_RUNMODE) {
            *cr &= (uint8_t)~CRA_START;
            return;
        }
    }
}

void cia_tick(cia_t *c, uint32_t cycles) {
    cia_tick_timer(c, &c->ta, c->ta_latch, &c->cra, ICR_TA, cycles);
    cia_tick_timer(c, &c->tb, c->tb_latch, &c->crb, ICR_TB, cycles);
}
