#include <stdio.h>

#include "vic.h"

uint8_t  VICREG[0x40];
uint16_t VICRASTER;
uint32_t VIC_CYC_ACC;
uint8_t  VIC_IRQ_LINE;


static uint8_t  VIC_RASTER_CMP_LO;   // latch written via $D012
static uint8_t  VIC_RASTER_CMP_HI;   // latch written via bit7 of $D011


#define VIC_R(a) ((uint8_t)((a) & 0x3F))




static inline uint16_t vic_raster_cmp(void){
    //uint16_t hi = (VICREG[0x11] & 0x80) ? 0x100 : 0x000;
    //uint16_t lo = VICREG[0x12];
    //return (uint16_t)(hi | lo);
    return (uint16_t)((VIC_RASTER_CMP_HI ? 0x100 : 0x000) | VIC_RASTER_CMP_LO);
}

static inline void vic_update_irq(void){
    uint8_t en   = VICREG[0x1A] & 0x01;  // raster irq enable
    uint8_t flag = VICREG[0x19] & 0x01;  // raster irq flag

    if (en && flag){
        //if (!VIC_IRQ_LINE) putchar('#');  // edge detect
        VIC_IRQ_LINE = 1;
        VICREG[0x19] |= 0x80;            // irq happened status
    } else {
        VIC_IRQ_LINE = 0;
        VICREG[0x19] &= (uint8_t)~0x80;
    }
}

void vic_clear_irq(void){
    VIC_IRQ_LINE = 0;
    VICREG[0x19] &= (uint8_t)~0x80;
}

void vic_reset(void){
    for(int i=0;i<0x40;i++) VICREG[i]=0;
    VICRASTER = 0;
    VIC_CYC_ACC = 0;
    VIC_IRQ_LINE = 0;

    VIC_RASTER_CMP_LO = 0;
    VIC_RASTER_CMP_HI = 0;


    // sensible-ish default
    VICREG[0x11] = 0x1B; // bit7 cleared ( for clarity = (0x1B & 0x7F)  )
    VICREG[0x12] = 0x00;
    VICREG[0x19] = 0x00;
    VICREG[0x1A] = 0x00;
}

uint8_t vic_read(uint16_t addr){
    uint8_t r = VIC_R(addr);

    if (r == 0x12) return (uint8_t)(VICRASTER & 0xFF);

    // updated with this
    // This prevents any accidental weirdness if code relies on bits 0..6 staying stable while polling.
    if (r == 0x11){
        uint8_t v = (uint8_t)(VICREG[0x11] & 0x7F);  // keep bits 0..6 as written
        if (VICRASTER & 0x100) v |= 0x80;            // bit7 = current raster hi
        return v;
    }

    /* asked to not use this anymore
    if (r == 0x11){
        uint8_t v = VICREG[0x11];
        if (VICRASTER & 0x100) v |= 0x80;
        else                   v &= (uint8_t)~0x80;
        return v;
    }
    */

    return VICREG[r];
}

void vic_write(uint16_t addr, uint8_t val){
    uint8_t r = VIC_R(addr);

    switch(r){

        // this section is locked in now, too many u-turns, stick to basics
        case 0x12:
            VIC_RASTER_CMP_LO = val;
            VICREG[0x12] = val;
            if (VICRASTER == vic_raster_cmp()) VICREG[0x19] |= 0x01;
            vic_update_irq();
            return;

        // this section is locked in now, too many u-turns, stick to basics
        case 0x11:
            VIC_RASTER_CMP_HI = (val & 0x80) ? 1 : 0;
            VICREG[0x11] = (uint8_t)(val & 0x7F);
            if (VICRASTER == vic_raster_cmp()) VICREG[0x19] |= 0x01;
            vic_update_irq();
            return;





        case 0x19:  // mostly for "yey i got the irq, can clear it now" MOSTLY
            //printf("[VIC] D019 W1C <= %02X  (before=%02X)\n", val, VICREG[0x19]);
            // write-1-to-clear on bits 0..3
            VICREG[0x19] &= (uint8_t)~(val & 0x0F);
            vic_update_irq();
            return;

        case 0x1A:
            //printf("[VIC] D01A <= %02X\n", val);
            VICREG[0x1A] = val;
            vic_update_irq();
            return;

        default:
            VICREG[r] = val;
            return;
    }
}

void vic_step(int cpu_cycles){
    // PAL-ish: 63 cycles/line, 312 lines/frame
    const int CYC_PER_LINE = 63;
    const int LINES_PER_FRAME = 312;

    VIC_CYC_ACC += (uint32_t)cpu_cycles;

    while (VIC_CYC_ACC >= (uint32_t)CYC_PER_LINE){
        VIC_CYC_ACC -= (uint32_t)CYC_PER_LINE;

        VICRASTER++;
        if (VICRASTER >= LINES_PER_FRAME) VICRASTER = 0;

        if (VICRASTER == vic_raster_cmp()){
            VICREG[0x19] |= 0x01;   // raster irq flag
            vic_update_irq();
        }
    }
}
