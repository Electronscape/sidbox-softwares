#include <stdio.h>

#include "bus.h"
#include "cia.h"
#include "sid8579.h"
#include "vic.h"
#include <string.h>

uint8_t C64RAM[65536];

#define OUT_CHAR_ADDR   0xD7F0

void clear64KRam(void){
    memset(C64RAM, 0, sizeof(C64RAM));
}

uint8_t bus_read8(uint16_t addr){
    // For now: pure RAM reads.

    // VIC registers $D000-$D03F
    // NOTE: VIC registers are ONLY $D000-$D03F here.
    // Do NOT widen to $D3FF: it will wrongly treat non-VIC I/O space as VIC mirrors.
    // (Later we can implement proper I/O decode/mirroring explicitly.)
    if (addr >= 0xD000 && addr <= 0xD03F)   // range 0xD000 - 0xD03F (do not u turn on this now, this is locked in)
        return vic_read((uint16_t)(0xD000 | (addr & 0x3F)));



    // CIA registers!!! 1: $DC00 - $DC0F, 2: $DD00 - $DD0F
    if (addr >= 0xDC00 && addr <= 0xDC0F) return cia_read(CIA_CHIP_1, addr);
    if (addr >= 0xDD00 && addr <= 0xDD0F) return cia_read(CIA_CHIP_2, addr);




    if(addr == OUT_CHAR_ADDR) return 0;

    return C64RAM[addr];
}

void bus_write8(uint16_t addr, uint8_t v){
    // Always write underlying RAM (matches C64 "writes under ROM" philosophy)
    C64RAM[addr] = v;

    // VIC regions: THIS IS EXCITING NOW!!!
    // NOTE: VIC registers are ONLY $D000-$D03F here.
    // Do NOT widen to $D3FF: it will wrongly treat non-VIC I/O space as VIC mirrors.
    // (Later we can implement proper I/O decode/mirroring explicitly.)
    if (addr >= 0xD000 && addr <= 0xD03F) { // range 0xD000 - 0xD03F (do not u turn on this now, this is locked in)
        vic_write((uint16_t)(0xD000 | (addr & 0x3F)), v);
        return;
    }

    // CIA registers!!! 1: $DC00 - $DC0F, 2: $DD00 - $DD0F
    if (addr >= 0xDC00 && addr <= 0xDC0F) { cia_write(CIA_CHIP_1, addr, v); return; }
    if (addr >= 0xDD00 && addr <= 0xDD0F) { cia_write(CIA_CHIP_2, addr, v); return; }


    // SID regions:
    // $D400-$D41F (SID1)
    if ((addr >= 0xD400) && (addr <= 0xD41F)) {
        sid_write(0, (uint8_t)(addr & 0x1F), v);
        return;
    }

    // $D420-$D43F (SID2 / "second SID block")
    if ((addr >= 0xD420) && (addr <= 0xD43F)) {
        // Important: pass reg in the same 0..63 scheme your old code expects
        // (reg>=32 selects chip) — so we give 0x20..0x3F here.
        sid_write(1, (uint8_t)(0x20 + (addr & 0x1F)), v);
        return;
    }

    // Debug console output: emulate C64 KERNAL CHROUT target buffer
    // We'll treat writing to $FFD2 as "print the character".
    if (addr == OUT_CHAR_ADDR) {
        putchar((char)v);
    }

}

void bus_write16(uint16_t addr, uint16_t v){
    if (addr == 0xFFFE) printf("[VEC] IRQ vec <= %04X\n", v);
    bus_write8(addr, (uint8_t)(v & 0xFF));
    bus_write8((uint16_t)(addr + 1), (uint8_t)(v >> 8));
}

uint16_t bus_read16(uint16_t addr){
    uint8_t lo = bus_read8(addr);
    uint8_t hi = bus_read8((uint16_t)(addr + 1));
    return (uint16_t)(lo | ((uint16_t)hi << 8));
}

// 6502 JMP (indirect) bug: wraps high-byte fetch within the same page
uint16_t bus_read16_wrap(uint16_t addr){
    uint8_t lo = bus_read8(addr);
    uint16_t addr2 = (uint16_t)((addr & 0xFF00) | ((addr + 1) & 0x00FF));
    uint8_t hi = bus_read8(addr2);
    return (uint16_t)(lo | ((uint16_t)hi << 8));
}




