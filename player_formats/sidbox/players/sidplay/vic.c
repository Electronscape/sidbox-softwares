#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "vic.h"

// --- Constants ---
#define VIC_PAL_CYCLES   64
#define VIC_PAL_LINES    308

#define VIC_NTSC_CYCLES  65
#define VIC_NTSC_LINES   263

static int vic_stall = 0;


uint32_t VIC_MACHINE_CYCLES = VIC_PAL_CYCLES;
uint32_t VIC_MACHINE_LINES  = VIC_PAL_LINES;

// --- Globals ---
uint8_t  VICREG[0x40];
uint16_t VICRASTER;
uint32_t VIC_CYC_ACC;
uint8_t  VIC_IRQ_LINE;

// --- Internal State ---
static uint8_t  VIC_RASTER_CMP_LO;
static uint8_t  VIC_RASTER_CMP_HI;
// Hardware Detail: Raster IRQ stays triggered as long as the match is true
// and the latch isn't cleared.
static uint8_t  raster_irq_triggered;

#define VIC_R(a) ((uint8_t)((a) & 0x3F))

static inline uint16_t vic_get_raster_cmp(void) {
    return (uint16_t)((VIC_RASTER_CMP_HI ? 0x100 : 0x000) | VIC_RASTER_CMP_LO);
}

static inline void vic_update_irq(void) {
    // Bits 0-3 are status, bit 7 is the master "Any VIC IRQ" bit
    uint8_t pending = VICREG[0x19] & 0x0F;
    uint8_t mask    = VICREG[0x1A] & 0x0F;

    if (pending & mask) {
        VICREG[0x19] |= 0x80;
        VIC_IRQ_LINE = 1;
    } else {
        VICREG[0x19] &= 0x7F;
        VIC_IRQ_LINE = 0;
    }
}

void vic_reset(void) {
    // On a real VIC, unused bits usually read as 1.
    memset(VICREG, 0xFF, 0x40);

    VICRASTER         = 0;
    VIC_CYC_ACC       = 0;
    VIC_IRQ_LINE      = 0;
    VIC_RASTER_CMP_LO = 0;
    VIC_RASTER_CMP_HI = 0;
    raster_irq_triggered = 0;

    // Standard Power-up
    VICREG[0x11] = 0x1B;
    VICREG[0x12] = 0x00;
    VICREG[0x19] = 0x70; // High bits 4-6 are always 1
    VICREG[0x1A] = 0xF0; // High bits 4-7 are always 1
}

uint8_t vic_read(uint16_t addr) {
    uint8_t r = VIC_R(addr);

    switch (r) {
    case 0x11:
        // Bit 7 is the 9th bit of the REAL raster counter
        return (VICREG[0x11] & 0x7F) | ((VICRASTER & 0x100) >> 1);

    case 0x12:
        // Low 8 bits of REAL raster counter
        return (uint8_t)(VICRASTER & 0xFF);

    case 0x19:
        return VICREG[0x19] | 0x70; // Bits 4-6 are hardwired to 1

    case 0x1A:
        return VICREG[0x1A] | 0xF0; // Bits 4-7 are hardwired to 1

    default:
        return VICREG[r];
    }
}

void vic_write(uint16_t addr, uint8_t val) {
    uint8_t r = VIC_R(addr);

    switch (r) {
    case 0x11:
        VIC_RASTER_CMP_HI = (val & 0x80) >> 7;
        VICREG[0x11] = val;
        // Re-check for match immediately (crucial for stable rasters)
        if (VICRASTER == vic_get_raster_cmp()) {
            if (!raster_irq_triggered) {
                VICREG[0x19] |= 0x01;
                raster_irq_triggered = 1;
                vic_update_irq();
            }
        } else {
            raster_irq_triggered = 0;
        }
        break;

    case 0x12:
        VIC_RASTER_CMP_LO = val;
        if (VICRASTER == vic_get_raster_cmp()) {
            if (!raster_irq_triggered) {
                VICREG[0x19] |= 0x01;
                raster_irq_triggered = 1;
                vic_update_irq();
            }
        } else {
            raster_irq_triggered = 0;
        }
        break;

    case 0x19:
        // ACK BUG FIX: Writing 1 clears the bit, but ONLY if the match condition
        // is no longer true. We just clear the status bits here.
        VICREG[0x19] &= ~(val & 0x0F);
        vic_update_irq();
        break;

    case 0x1A:
        VICREG[0x1A] = val | 0xF0;
        vic_update_irq();
        break;

    default:
        VICREG[r] = val;
        break;
    }
}




uint8_t vic_cpu_stall(void) {
    return (vic_stall > 0);
}

int vic_cpu_can_run_this_cycle(void) {
    if (vic_stall > 0) {
        vic_stall--;          // steal THIS CPU cycle
        return 0;             // CPU does NOT run
    }
    return 1;                 // CPU may run
}

static inline int vic_badline(void) {
    uint16_t r = VICRASTER;
    uint8_t d011 = VICREG[0x11];

    if (!(d011 & 0x10)) return 0;          // display enabled?
    if (r < 0x30 || r > 0xF7) return 0;    // visible badline region (PAL)
    if ((r & 7) != (d011 & 7)) return 0;   // yscroll match
    return 1;
}








void vic_step(int cycles) {
    VIC_CYC_ACC += (uint32_t)cycles;

    while (VIC_CYC_ACC >= VIC_MACHINE_CYCLES) {
        VIC_CYC_ACC -= VIC_MACHINE_CYCLES;

        VICRASTER++;
        if (VICRASTER >= VIC_MACHINE_LINES) VICRASTER = 0;

        // Raster IRQ compare logic (yours)
        uint16_t cmp = vic_get_raster_cmp();
        if (VICRASTER == cmp) {
            if (!raster_irq_triggered) {
                VICREG[0x19] |= 0x01;
                raster_irq_triggered = 1;
                vic_update_irq();
            }
        } else {
            raster_irq_triggered = 0;
        }

        // Add stalls at start of badline
        if (vic_badline()) {
            vic_stall += 40;   // cheap-but-effective model
        }
    }
}
