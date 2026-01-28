
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
    c->cra = 0;
    c->icr_mask = 0;
    c->icr_flags = 0;
    c->irq_level = 0;
}

void cia_set_flag(cia_t *c, uint8_t flag) {
    c->icr_flags |= flag;
    cia_update_irq(c);

}

uint8_t cia_read(cia_t *c, uint8_t reg) {
    reg &= 0x0F;

    switch (reg) {
    case 0x04: return (uint8_t)(c->ta & 0xFF);        // TALO
    case 0x05: return (uint8_t)(c->ta >> 8);          // TAHI

    case 0x0D: { // ICR: reading returns flags + bit7 if any enabled pending, then clears flags
        uint8_t pending = (uint8_t)(c->icr_flags & 0x1F);
        uint8_t v = pending;
        if (pending & c->icr_mask) v |= ICR_IRQ;

        // Read-ack: clear pending flags
        c->icr_flags = 0;
        cia_update_irq(c);
        return v;
    }

    case 0x0E: return c->cra;

    default:
        // For RSID-minimum, return 0 for unimplemented regs
        return 0;
    }
}

void cia_write(cia_t *c, uint8_t reg, uint8_t v) {
    reg &= 0x0F;

    printf("RAW_CIA%c W reg=$%02X v=$%02X  \n", (c==&cia1)?'1':'2', reg, v);
    fflush(stdout);

    switch (reg) {
        case 0x04: // TALO latch low
            c->ta_latch = (uint16_t)((c->ta_latch & 0xFF00) | (uint16_t)v);
            break;

        case 0x05: // TAHI latch high
            c->ta_latch = (uint16_t)(((uint16_t)v << 8) | (c->ta_latch & 0x00FF));
            if (!(c->cra & CRA_START)) {
                c->ta = c->ta_latch;
            }
            break;

        case 0x0D: {
            // ICR write: bit7 = set/clear mask bits, lower 5 bits select which
            uint8_t bits = (uint8_t)(v & 0x1F);
            if (v & 0x80) c->icr_mask |= bits;   // set enables
            else          c->icr_mask &= (uint8_t)~bits; // clear enables

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

        default: break;
    }
    printf("latch=%04X ta=%04X cra=%02X mask=%02X\n", c->ta_latch, c->ta, c->cra, c->icr_mask);
    fflush(stdout);

}

void cia_tick(cia_t *c, uint32_t cycles) {
    if (!(c->cra & CRA_START)) return;
    while (cycles) {
        // How many cycles until the next underflow?
        // If ta == 0, underflow happens on the very next tick.
        uint32_t to_underflow = (c->ta == 0) ? 1u : (uint32_t)c->ta + 1u;

        if (cycles < to_underflow) {
            // No underflow this tick window
            c->ta = (uint16_t)(c->ta - (uint16_t)cycles);
            return;
        }


        // Consume up to and including the underflow tick
        cycles -= to_underflow;

        // Underflow event
        cia_set_flag(c, ICR_TA);

        // Reload
        c->ta = c->ta_latch;

        // One-shot stops after underflow
        if (c->cra & CRA_RUNMODE) {
            c->cra &= (uint8_t)~CRA_START;
            return;
        }

        // If latch is 0, you'd underflow every cycle; loop continues safely.
    }

}
