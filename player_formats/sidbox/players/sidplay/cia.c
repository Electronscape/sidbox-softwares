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
    int32_t  ta, tb;
    int32_t  ta_latch, tb_latch;

    uint8_t  icr_pending;
    uint8_t  icr_mask;
    uint8_t  irq_line;

    // --- TOD Clock Internal State ---
    uint8_t  tod_10ths, tod_sec, tod_min, tod_hr;
    uint8_t  latch_10ths, latch_sec, latch_min, latch_hr;
    uint8_t  alm_10ths, alm_sec, alm_min, alm_hr;

    int32_t  tod_tick_cnt;
    uint8_t  tod_latched; // Clock is frozen for reading
    uint8_t  tod_stopped; // Clock is stopped (waiting for 10ths write)
} CIAState;

static CIAState cia[2];

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
    if (c->icr_pending & c->icr_mask) {
        c->irq_line = 1;
        c->icr_pending |= ICR_IRQLATCH;
    } else {
        c->irq_line = 0;
        c->icr_pending &= ~ICR_IRQLATCH;
    }
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
        c->icr_pending |= ICR_TOD;
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

    switch (r) {
    case CIA_TALO: return (uint8_t)(c->ta & 0xFF);
    case CIA_TAHI: return (uint8_t)(c->ta >> 8);
    case CIA_TBLO: return (uint8_t)(c->tb & 0xFF);
    case CIA_TBHI: return (uint8_t)(c->tb >> 8);

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
        uint8_t v = c->icr_pending;
        c->icr_pending = 0;
        cia_update_irq(c);
        cia_sync_out(chip);
        return v;
    }
    default: return c->reg[r];
    }
}

void cia_write(cia_chip_t chip, uint16_t addr, uint8_t val) {
    CIAState *c = &cia[chip];
    uint8_t r = CIA_R(addr);

    switch (r) {
    case CIA_TALO:
        c->ta_latch = (c->ta_latch & 0xFF00) | val;
        break;
    case CIA_TAHI:
        c->ta_latch = (c->ta_latch & 0x00FF) | (val << 8);
        if (!(c->reg[CIA_CRA] & CR_START)) c->ta = c->ta_latch;
        break;
    case CIA_TBLO:
        c->tb_latch = (c->tb_latch & 0xFF00) | val;
        break;
    case CIA_TBHI:
        c->tb_latch = (c->tb_latch & 0x00FF) | (val << 8);
        if (!(c->reg[CIA_CRB] & CR_START)) c->tb = c->tb_latch;
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
        c->icr_pending |= ICR_SP; // Writing SDR triggers Serial IRQ
        cia_update_irq(c);
        break;

    case CIA_ICR:
        if (val & 0x80) c->icr_mask |= (val & 0x7F);
        else c->icr_mask &= ~(val & 0x7F);
        cia_update_irq(c);
        break;

    case CIA_CRA:
        c->reg[CIA_CRA] = val;
        if (val & CR_LOAD) { c->ta = c->ta_latch; c->reg[CIA_CRA] &= ~CR_LOAD; }
        break;
    case CIA_CRB:
        c->reg[CIA_CRB] = val;
        if (val & CR_LOAD) { c->tb = c->tb_latch; c->reg[CIA_CRB] &= ~CR_LOAD; }
        break;

    default: c->reg[r] = val; break;
    }
    cia_sync_out(chip);
}

void cia_step(cia_chip_t chip, int cpu_cycles) {
    CIAState *c = &cia[chip];

    // --- TOD Clock Step ---
    c->tod_tick_cnt += cpu_cycles;
    int tod_threshold = (c->reg[CIA_CRA] & CR_TODIN) ? 16431 : 19705;
    if (c->tod_tick_cnt >= tod_threshold) {
        c->tod_tick_cnt -= tod_threshold;
        cia_advance_tod(c);
    }

    // --- Timer A ---
    int ta_underflows = 0;
    if (c->reg[CIA_CRA] & CR_START) {
        c->ta -= cpu_cycles;

        if (c->ta <= 0) { // Accuracy Note: Hardware reloads when it hits 0
            // Calculate how many times we underflowed if cycles > counter
            // For most steps, this will just be 1.
            ta_underflows = 1 + (-c->ta / (c->ta_latch + 1));

            c->icr_pending |= ICR_TA;

            // Handle One-Shot Mode
            if (c->reg[CIA_CRA] & CR_RUNMODE) {
                c->reg[CIA_CRA] &= ~CR_START;
                c->ta = c->ta_latch; // Stop and reload
            } else {
                // Reload with remainder for cycle-exact accuracy
                c->ta += ta_underflows * (c->ta_latch + 1);
            }

            // Port B Toggle (PB6) - Bit 1 of CRA enables this
            if (c->reg[CIA_CRA] & CR_PBON) {
                // Technically toggles or pulses PB6 based on OUTMODE
                // If you are emulating Port B, update bit 6 of PRA/PRB here
            }
        }
    }

    // --- Timer B ---
    // Mode 0: Phi2, Mode 1: CNT, Mode 2: TA, Mode 3: TA+CNT
    uint8_t tb_mode = (uint8_t)((c->reg[CIA_CRB] >> 5) & 0x03);

    if (c->reg[CIA_CRB] & CR_START) {
        int decrement = 0;
        if (tb_mode == 0)      decrement = cpu_cycles;
        else if (tb_mode == 2) decrement = ta_underflows;
        // Mode 1 and 3 require CNT pin state, usually ignored in simple emus

        if (decrement > 0) {
            c->tb -= decrement;
            if (c->tb <= 0) {
                int tb_underflows = 1 + (-c->tb / (c->tb_latch + 1));
                c->icr_pending |= ICR_TB;

                if (c->reg[CIA_CRB] & CR_RUNMODE) {
                    c->reg[CIA_CRB] &= ~CR_START;
                    c->tb = c->tb_latch;
                } else {
                    c->tb += tb_underflows * (c->tb_latch + 1);
                }
            }
        }
    }

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
