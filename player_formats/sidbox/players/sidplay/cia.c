#include <stdio.h>

#include <stdint.h>
#include "cia.h"
#include <string.h>

// ===== Internal Constants =====
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

typedef struct {
    uint8_t  reg[0x10];

    // Internal 16-bit counters and latches
    uint16_t ta, tb;
    uint16_t ta_latch, tb_latch;

    // ICR: separate "status" (sources that occurred) from "mask"
    uint8_t  icr_status;     // bits 0..4 are sources that happened (TA/TB/TOD/SP/FLAG)
    uint8_t  icr_mask;       // written mask (as you already do)
    uint8_t  irq_line;

    // two little “CIA weirdness” flags
    uint8_t  ta_hold;        // “stolen clock” after reload/force-load (skip next decrement)
    uint8_t  tb_hold;
    uint8_t  ta_irq_delay;   // old 6526: interrupt asserted 1 cycle after underflow if masked
    uint8_t  tb_irq_delay;



    uint8_t ta_reload_pending;
    uint8_t tb_reload_pending;


    // --- TOD Clock Internal State ---
    uint8_t  tod_10ths, tod_sec, tod_min, tod_hr;
    uint8_t  latch_10ths, latch_sec, latch_min, latch_hr;
    uint8_t  alm_10ths, alm_sec, alm_min, alm_hr;

    int32_t  tod_tick_cnt;
    uint8_t  tod_latched; // Clock is frozen for reading
    uint8_t  tod_stopped; // Clock is stopped (waiting for 10ths write)
} CIAState;

volatile CIAState cia[2];

// Globals for external access
uint8_t  CIA1REG[0x10];
uint16_t CIA1_TA, CIA1_TB;
uint8_t  CIA1_IRQ_LINE;

uint8_t  CIA2REG[0x10];
uint16_t CIA2_TA, CIA2_TB;
uint8_t  CIA2_IRQ_LINE;


// --- Internal Helper Functions ---

static inline uint8_t to_bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }
static inline uint8_t from_bcd(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0F); }

static inline void cia_sync_out(cia_chip_t chip) {
    CIAState *c = &cia[chip];
    if (chip == CIA_CHIP_1) {
        memcpy(CIA1REG, c->reg, 0x10);
        CIA1_TA = (uint16_t)c->ta;
        CIA1_TB = (uint16_t)c->tb;
        CIA1_IRQ_LINE = c->irq_line;
    } else {
        memcpy(CIA2REG, c->reg, 0x10);
        CIA2_TA = (uint16_t)c->ta;
        CIA2_TB = (uint16_t)c->tb;
        CIA2_IRQ_LINE = c->irq_line;
    }
}

static inline void cia_update_irq(CIAState *c) {
    // IRQ/NMI line is asserted if the latched IRQ flag is set (bit7 in *read* ICR semantics)
    // We'll compute it from status+mask and the delay countdowns.
    uint8_t match = (uint8_t)(c->icr_status & c->icr_mask & 0x1F);

    // If a delayed interrupt is scheduled and delay reached 0, assert IRQ.
    // (We’ll set ta_irq_delay/tb_irq_delay to 1 on underflow when match is true.)
    if ((c->ta_irq_delay == 0 && c->tb_irq_delay == 0) && match) {
        // NOTE: this is the "6526A / no delay" behavior.
        // We are NOT using it; we will assert via delay counters below.
    }

    // IRQ line: active if ANY interrupt latched (bit7 behavior).
    // We'll set bit7 when the delay expires.
    if (c->icr_status & ICR_IRQLATCH) c->irq_line = 1;
    else c->irq_line = 0;
}

// --- TOD Advance Logic ---
static void cia_advance_tod(CIAState *c) {
    if (c->tod_stopped) return;

    c->tod_10ths++;
    if (c->tod_10ths > 9) {
        c->tod_10ths = 0;
        c->tod_sec++;
        if (c->tod_sec > 59) {
            c->tod_sec = 0;
            c->tod_min++;
            if (c->tod_min > 59) {
                c->tod_min = 0;
                uint8_t h = (c->tod_hr & 0x1F);
                uint8_t pm = (c->tod_hr & 0x80);
                h++;
                if (h > 12) h = 1;
                if (h == 12) pm ^= 0x80;
                c->tod_hr = h | pm;
            }
        }
    }

    // Check Alarm match
    if (c->tod_10ths == c->alm_10ths && c->tod_sec == c->alm_sec &&
        c->tod_min == c->alm_min && c->tod_hr == c->alm_hr) {
        //c->icr_pending |= ICR_TOD;
        cia_update_irq(c);
    }
}

// --- Core API ---

void cia_reset(cia_chip_t chip) {
    CIAState *c = &cia[chip];

    // 1. Clear everything (Registers, pending IRQs, masks)
    memset(c, 0, sizeof(*c));

    // 2. Timers default to 0xFFFF on power-up/reset
    c->ta_latch = 0xFFFF;
    c->tb_latch = 0xFFFF;
    c->ta = 0xFFFF;
    c->tb = 0xFFFF;

    // 3. TOD State
    // Hardware Detail: TOD is halted until 10ths is written.
    c->tod_stopped = 1;
    c->tod_latched = 0;

    // 4. Input/Output (Crucial for Keyboard/Joystick scanning)
    // On reset, DDR registers are 0 (all pins are inputs)
    // PRA and PRB usually default to 0xFF (pull-ups)
    c->reg[CIA_PRA] = 0xFF;
    c->reg[CIA_PRB] = 0xFF;

    // 5. Ensure the IRQ line is physically pulled high (inactive)
    c->irq_line = 0;
    c->icr_mask = 0x00; // Disable all CIA interrupts

    cia_sync_out(chip);


}

void cia_reset_all(void) {
    cia_reset(CIA_CHIP_1);
    cia_reset(CIA_CHIP_2);
}

uint8_t cia_read(cia_chip_t chip, uint16_t addr) {
    CIAState *c = &cia[chip];
    uint8_t r = CIA_R(addr);

    //if(addr != 0xD)
    //printf("genaral read CIA%u $%04X = $%02X\n", chip, addr, c->reg[r]);

    switch (r) {
        case CIA_TALO: {
            uint8_t v = (uint8_t)(c->ta & 0xFF);
            // debug: only for the speech tune region if you can filter
            //printf("CIA%u TA LO read: 0x%04X\n", chip, c->ta);
            return v;
        }

        case CIA_TAHI: {
            uint8_t v = (uint8_t)(c->ta >> 8);
            //printf("CIA%u TA HI read: 0x%04X\n", chip, c->ta);
            return v;
        }

        case CIA_TBLO: {
            uint8_t v = (uint8_t)(c->tb & 0xFF);
            // debug: only for the speech tune region if you can filter
            //printf("CIA%u TB LO read: 0x%04X\n", chip, c->tb);
            return v;
        }

        case CIA_TBHI: {
            uint8_t v = (uint8_t)(c->tb >> 8);
            //printf("CIA%u TB HI read: 0x%04X\n", chip, c->tb);
            return v;
        }

        case CIA_TOD10:
            c->tod_latched = 0; // Unlatches the clock
            return to_bcd(c->tod_latched ? c->latch_10ths : c->tod_10ths);
        case CIA_TODSEC: return to_bcd(c->tod_latched ? c->latch_sec : c->tod_sec);
        case CIA_TODMIN: return to_bcd(c->tod_latched ? c->latch_min : c->tod_min);
        case CIA_TODHR:
            if (!c->tod_latched) { // Latch clock on HR read
                c->latch_10ths = c->tod_10ths; c->latch_sec = c->tod_sec;
                c->latch_min = c->tod_min;     c->latch_hr = c->tod_hr;
                c->tod_latched = 1;
            }
            return to_bcd(c->latch_hr);

        case CIA_ICR: {
            uint8_t v = (uint8_t)(c->icr_status & 0x1F);
            if (c->icr_status & ICR_IRQLATCH) v |= 0x80;

            // reading ICR clears sources and the IRQ latch
            c->icr_status = 0;
            c->irq_line = 0;

            cia_sync_out(chip);
            return v;
            return v;

        }
        default:
            return c->reg[r];
    }
    return c->reg[r];
}

void cia_write(cia_chip_t chip, uint16_t addr, uint8_t val) {
    CIAState *c = &cia[chip];
    uint8_t r = CIA_R(addr);

    c->reg[r] = val;

    //if(addr != 0xD)        printf("genaral write CIA%u $%04X = $%02X\n", chip, addr, c->reg[r]);

    switch (r) {
        case CIA_TALO:
            c->ta_latch = (c->ta_latch & 0xFF00) | (uint16_t)val;
            //printf("CIA%u TA LO write: %04X = 0x%02x\n", chip, addr, val);
            //c->ta_latch = 0x6f;

            break;

        case CIA_TAHI:
            c->ta_latch = (c->ta_latch & 0x00FF) | ((uint16_t)val << 8);
            //printf("CIA%u TA HI write: %04X = 0x%02x\n", chip, addr, val);
            if (!(c->reg[CIA_CRA] & CR_START)) {
                c->ta = c->ta_latch;
            }
            break;

        case CIA_TBLO:
            c->tb_latch = (c->tb_latch & 0xFF00) | (uint16_t)val;
            //printf("CIA%u TB LO write: %04X = 0x%02x\n", chip, addr, val);
            break;

        case CIA_TBHI:
            c->tb_latch = (c->tb_latch & 0x00FF) | ((uint16_t)val << 8);
            //printf("CIA%u TB HI write: %04X = 0x%02x\n", chip, addr, val);
            if (!(c->reg[CIA_CRB] & CR_START)) {
                c->tb = c->tb_latch;

            }

            break;

        case CIA_TOD10:
            if (c->reg[CIA_CRB] & 0x80) c->alm_10ths = from_bcd(val);
            else { c->tod_10ths = from_bcd(val); c->tod_stopped = 0; }
            break;
        case CIA_TODSEC:
            if (c->reg[CIA_CRB] & 0x80) c->alm_sec = from_bcd(val);
            else c->tod_sec = from_bcd(val);
            break;
        case CIA_TODMIN:
            if (c->reg[CIA_CRB] & 0x80) c->alm_min = from_bcd(val);
            else c->tod_min = from_bcd(val);
            break;
        case CIA_TODHR:
            if (c->reg[CIA_CRB] & 0x80) c->alm_hr = from_bcd(val);
            else { c->tod_hr = from_bcd(val); c->tod_stopped = 1; }
            break;

        case CIA_SDR:
            c->reg[CIA_SDR] = val;
            //c->icr_pending |= ICR_SP; // Writing SDR triggers Serial IRQ
            cia_update_irq(c);
            break;

        case CIA_ICR:
            if (val & 0x80) c->icr_mask |= (val & 0x7F);
            else c->icr_mask &= ~(val & 0x7F);
            cia_update_irq(c);
            break;

        case CIA_CRA:
            c->reg[CIA_CRA] = val;
            if (val & CR_LOAD) {
                c->ta = c->ta_latch;
                c->ta_hold = 1;
                c->reg[CIA_CRA] &= ~CR_LOAD;
            }
            break;


        case CIA_CRB:
            c->reg[CIA_CRB] = val;
            if (val & CR_LOAD) {
                c->tb = c->tb_latch;
                c->tb_hold = 1;
                c->reg[CIA_CRB] &= ~CR_LOAD;
            }
            break;

        default: c->reg[r] = val; break;
    }
    cia_sync_out(chip);
}

//#define safety_barrier  116

#define safety_barrier  110
void cia_step(cia_chip_t chip, int cpu_cycles) {
    CIAState *c = &cia[chip];
    if (cpu_cycles <= 0) return;

    // --- TOD Clock Step (unchanged, still coarse) ---
    c->tod_tick_cnt += cpu_cycles;
    int tod_threshold = (c->reg[CIA_CRA] & CR_TODIN) ? 16431 : 19705;
    while (c->tod_tick_cnt >= tod_threshold) {
        c->tod_tick_cnt -= tod_threshold;
        cia_advance_tod(c);
    }

    // --- Decode modes up front ---
    const uint8_t cra = c->reg[CIA_CRA];
    const uint8_t crb = c->reg[CIA_CRB];

    const int ta_running = (cra & CR_START) != 0;
    const int tb_running = (crb & CR_START) != 0;

    // TB mode: bits 6..5 (per your code)
    // 0 = count cpu cycles
    // 2 = count TA underflows
    const uint8_t tb_mode = (uint8_t)((crb >> 5) & 0x03);

    int ta_underflows = 0;

    // handle delayed interrupt assertion (old 6526 behavior)
    if (c->ta_irq_delay) {
        if (--c->ta_irq_delay == 0) {
            if (c->icr_status & ICR_TA) {
                if (c->icr_mask & ICR_TA) c->icr_status |= ICR_IRQLATCH;
            }
        }
    }

    if (c->tb_irq_delay) {
        if (--c->tb_irq_delay == 0) {
            if (c->icr_status & ICR_TB) {
                if (c->icr_mask & ICR_TB) c->icr_status |= ICR_IRQLATCH;
            }
        }
    }


    // --- Timer A: step per CPU cycle ---

    if (ta_running) {
        for (int i = 0; i < cpu_cycles; i++) {

            // stolen clock: skip ONE decrement after a reload/force-load
            if (c->ta_hold) {
                c->ta_hold = 0;
                continue;
            }


            if (c->ta == 0) {
                // Underflow event at 0 (gives stable behavior for low latches)
                ta_underflows++;
                c->icr_status |= ICR_TA;     // status bit set immediately

                // schedule IRQ/NMI assertion with +1 cycle delay if masked
                if (c->icr_mask & ICR_TA) c->ta_irq_delay = 1;

                // reload from latch (0 not “visible” because of stolen clock)
                c->ta = c->ta_latch;

                // steal next clock (pipeline removal)
                c->ta_hold = 1;

                // one-shot stops immediately when 0 reached
                if (c->reg[CIA_CRA] & CR_RUNMODE) {
                    c->reg[CIA_CRA] &= (uint8_t)~CR_START;
                    break;
                }
            } else {
                c->ta --;
            }
        }
    }


    // --- Timer B ---
    int tb_underflows = 0;

    if (tb_running) {
        for (int i = 0; i < cpu_cycles; i++) {

            // stolen clock: skip ONE decrement after a reload/force-load
            if (c->tb_hold) {
                c->tb_hold = 0;
                continue;
            }


            if (c->tb == 0) {
                // Underflow event at 0 (gives stable behavior for low latches)
                tb_underflows++;
                c->icr_status |= ICR_TB;     // status bit set immediately

                // schedule IRQ/NMI assertion with +1 cycle delay if masked
                if (c->icr_mask & ICR_TB) c->tb_irq_delay = 1;

                // reload from latch (0 not “visible” because of stolen clock)
                c->tb = c->tb_latch;

                // steal next clock (pipeline removal)
                c->tb_hold = 1;

                // one-shot stops immediately when 0 reached
                if (c->reg[CIA_CRB] & CR_RUNMODE) {
                    c->reg[CIA_CRB] &= (uint8_t)~CR_START;
                    break;
                }
            } else {
                c->tb--;
            }
        }
    }


    // --- Update IRQ latch/line once ---
    cia_update_irq(c);

    // Refresh Globals
    if (chip == CIA_CHIP_1) {
        CIA1_IRQ_LINE = c->irq_line;
        CIA1_TA = (uint16_t)c->ta;
        CIA1_TB = (uint16_t)c->tb;
    } else {
        CIA2_IRQ_LINE = c->irq_line;
        CIA2_TA = (uint16_t)c->ta;
        CIA2_TB = (uint16_t)c->tb;
    }
}


void cia_step_all(int cpu_cycles) {
    cia_step(CIA_CHIP_1, cpu_cycles);
    cia_step(CIA_CHIP_2, cpu_cycles);
}
