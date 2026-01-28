// cpu6502_test.c
#include <stdio.h>
#include <string.h>
#include <stdint.h>


#include "cpu6502.h"
#include "sid8579.h"
#include "sidplaybus.h"
#include "ciairq.h"


// will be loaded later
uint8_t basic_rom[] = {
#include "basic_rom.h"
};

uint8_t kernal_rom[] = {
#include "kernal.h"
};

uint8_t char_rom[] = {
#include "char_rom.h"
};



vic_t vic;




int roms_loaded = 0;

static uint8_t cpu_ddr  = 0x2F;   // common power-up-ish value
static uint8_t cpu_port = 0x37;   // common power-up-ish value

static inline uint8_t cpu_port_effective(void){
    // inputs read as 1 (pullups), outputs come from cpu_port
    return (uint8_t)(cpu_port | (uint8_t)~cpu_ddr);
}

static inline int loram(void){ return (cpu_port_effective() & 0x01) != 0; }
static inline int hiram(void){ return (cpu_port_effective() & 0x02) != 0; }
static inline int charen(void){ return (cpu_port_effective() & 0x04) != 0; }

// I/O visible only if CHAREN=1 and not (LORAM=0 && HIRAM=0)
static inline int io_visible(void){
    if (!charen()) return 0;
    return !( !loram() && !hiram() );
}

static inline int basic_visible(void){
    return roms_loaded && loram() && hiram();
}
static inline int kernal_visible(void){
    return roms_loaded && hiram();
}
static inline int chargen_visible(void){
    // CHAREN=0 selects chargen instead of I/O (when I/O would otherwise be visible)
    return roms_loaded && !charen() && !( !loram() && !hiram() );
}






// ---- simple 64K RAM bus ----
uint8_t ram[65536];

extern sid_t sid;

void romloadeds(){
    roms_loaded = 1;
}

void kernal_write_u8(uint16_t cpu_addr, uint8_t v){
    if (cpu_addr < 0xE000) return;                 // outside kernal
    kernal_rom[(uint16_t)(cpu_addr - 0xE000)] = v; // map to 0..0x1FFF
}


void write_to_rom(uint32_t addr, uint8_t val){
    kernal_rom[addr] = val;

}

void clear_ram(void) {    
    memset(ram, 0, sizeof(ram));
    cpu_ddr  = 0x2F;
    cpu_port = 0x37; // clear HIRAM bit;

    cia_init(&cia1);
    cia_init(&cia2);
    //roms_loaded = 1;


}

// Write a program at addr, and set reset vector to it.
void load_prog(uint16_t addr, const uint8_t *prog, size_t n) {
    memcpy(&ram[addr], prog, n);
    ram[0xFFFC] = (uint8_t)(addr & 0xFF);
    ram[0xFFFD] = (uint8_t)(addr >> 8);
}

uint8_t bus_read8(void *user, uint16_t addr) {
    (void)user;

    // 6510 port regs
    if (addr == 0x0000) return cpu_ddr;
    if (addr == 0x0001) return cpu_port_effective();

    // KERNAL ROM
    if (addr >= 0xE000 && kernal_visible())
        return kernal_rom[addr - 0xE000];

    // BASIC ROM
    if (addr >= 0xA000 && addr <= 0xBFFF && basic_visible())
        return basic_rom[addr - 0xA000];

    // $D000-$DFFF: either I/O or CHARGEN or RAM depending on banking
    if (addr >= 0xD000 && addr <= 0xD03F) {
        uint8_t r = vic.reg[addr - 0xD000];

        // $D012 read = raster low
        if (addr == 0xD012) return (uint8_t)(vic.raster & 0xFF);

        // $D011 bit7 reflects raster high bit (bit8 of raster)
        if (addr == 0xD011) {
            r = (uint8_t)((r & 0x7F) | ((vic.raster & 0x100) ? 0x80 : 0x00));
            return r;
        }

        // $D019 IRQ status
        if (addr == 0xD019) return vic.reg[0x19];

        return r;
    }

    if (addr >= 0xD000 && addr <= 0xDFFF) {
        // SID ($D400-$D41F)

        if (io_visible()) {

            if (addr >= 0xD400 && addr <= 0xD41F)
                return sid.reg[addr - 0xD400];

            // CIA1 ($DC00-$DCFF mirrors)
            if (addr >= 0xDC00 && addr <= 0xDCFF)
                return cia_read(&cia1, (uint8_t)(addr & 0x0F));

            // CIA2 ($DD00-$DDFF mirrors)
            if (addr >= 0xDD00 && addr <= 0xDDFF)
                return cia_read(&cia2, (uint8_t)(addr & 0x0F));

            // TODO: VIC-II, etc.
            return 0;
        } else if (chargen_visible()) {
            return char_rom[addr - 0xD000];
        }
        // else: RAM visible
    }


    return ram[addr];
}

void bus_write8(void *user, uint16_t addr, uint8_t v) {
    (void)user;


    // 6510 port regs
    if (addr == 0x0000) { cpu_ddr = v; return; }
    if (addr == 0x0001) { cpu_port = v;
        //printf("CPU PORT write: %02X  (io_visible=%d)\n", v, io_visible());
        //fflush(stdout);

        return;
    }

    // Writes to ROM regions go to RAM underneath when ROM is mapped in
    // (so don't block writes just because ROM is visible)

    // I/O writes only when I/O is visible
    // VIC-II minimal writes ($D000-$D03F)
    if (addr >= 0xD000 && addr <= 0xD03F) {
        uint8_t idx = (uint8_t)(addr - 0xD000);

        if (addr == 0xD019) {
            // write-1-to-clear IRQ flags
            vic.reg[0x19] &= (uint8_t)~(v & 0x0F);
            if ((vic.reg[0x19] & 0x0F) == 0) vic.irq_line = 0;
            return;
        }

        vic.reg[idx] = v;

        // update raster compare when D011/D012 change
        if (addr == 0xD012 || addr == 0xD011) {
            uint16_t cmp = (uint16_t)vic.reg[0x12];
            if (vic.reg[0x11] & 0x80) cmp |= 0x100;
            vic.raster_cmp = cmp;
        }
        return;
    }



    if (addr >= 0xD000 && addr <= 0xDFFF && io_visible()) {
        if (addr >= 0xD400 && addr <= 0xD41F) { sid_write(&sid, addr, v); return; }

        if (addr >= 0xDC00 && addr <= 0xDCFF) {
            uint8_t r = (uint8_t)(addr & 0x0F);
            printf("CIA1 W %04X (r=%02X) = %02X   io=%d\n", addr, r, v, io_visible());
            fflush(stdout);
            cia_write(&cia1, r, v);
            return;
        }
        if (addr >= 0xDD00 && addr <= 0xDDFF) {
            uint8_t r = (uint8_t)(addr & 0x0F);
            printf("CIA2 W %04X (r=%02X) = %02X   io=%d\n", addr, r, v, io_visible());
            fflush(stdout);
            cia_write(&cia2, r, v);
            return;
        }
        // TODO: VIC-II writes
        return;
    }

    ram[addr] = v;
}

void vic_tick(vic_t *v, uint32_t cpu_cycles)
{
    // PAL-ish timing model:
    // ~19704 cycles per frame @ 50Hz, 312 lines => ~63 cycles/line.
    // We'll use 63 cycles/line as a crude but workable constant.
    const uint32_t CYC_PER_LINE = 63;
    const uint16_t LINES_PER_FRAME = 312;

    v->cyc_accum += cpu_cycles;
    while (v->cyc_accum >= CYC_PER_LINE) {
        v->cyc_accum -= CYC_PER_LINE;

        v->raster++;
        if (v->raster >= LINES_PER_FRAME) v->raster = 0;

        // If raster IRQ enabled ($D01A bit0) and raster matches compare, fire IRQ
        if ((v->reg[0x1A] & 0x01) && (v->raster == v->raster_cmp)) {
            v->reg[0x19] |= 0x01;  // set raster IRQ flag
            v->irq_line = 1;       // assert
            //printf(".");
            //fflush(stdout);
        }

    }
}
