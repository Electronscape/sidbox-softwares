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



uint32_t cpuHz   = 985248;  // PAL or 1022727 : NTSC

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

static inline int basic_visible(void){
    return roms_loaded && loram() && hiram();
}
static inline int kernal_visible(void){
    return roms_loaded && hiram();
}


int io_visible(void){
    if (!charen()) return 0;
    return !( !loram() && !hiram() );
}

static inline int chargen_visible(void){
    // CHAREN=0 selects chargen INSTEAD OF I/O (but only when I/O would have been visible)
    return !charen() && !( !loram() && !hiram() );
}





// ---- simple 64K RAM bus ----
uint8_t ram[65536];

extern sid_t sid;

void romloadeds(){
    roms_loaded = 1;
}

void kernal_write_u8(uint16_t cpu_addr, uint8_t v){
    // this is to install ONLY STUBS as we dont HAVE a real rom, its NOT for programs or us to use at will
    if (cpu_addr < 0xE000) return;                 // outside kernal
    kernal_rom[(uint16_t)(cpu_addr - 0xE000)] = v; // map to 0..0x1FFF
    //ram[cpu_addr] = v;
}


void write_to_rom(uint32_t addr, uint8_t val){
    // this is to install ONLY STUBS as we dont HAVE a real rom, its NOT for programs or us to use at will
    kernal_rom[addr] = val;
}

void clear_ram(void) {    
    memset(ram, 0, sizeof(ram));
    cpu_ddr  = 0x2F;
    cpu_port = 0x37; // clear HIRAM bit;

    cia_init(&cia1);
    cia_init(&cia2);
    roms_loaded = 0;


}

// Write a program at addr, and set reset vector to it.
void load_prog(uint16_t addr, const uint8_t *prog, size_t n) {
    memcpy(&ram[addr], prog, n);
    ram[0xFFFC] = (uint8_t)(addr & 0xFF);
    ram[0xFFFD] = (uint8_t)(addr >> 8);
}


static inline void vic_update_irq(vic_t *v){
    uint8_t pending = (uint8_t)(v->reg[0x19] & 0x0F);
    uint8_t enable  = (uint8_t)(v->reg[0x1A] & 0x0F);
    v->irq_line = ((pending & enable) != 0);
}


uint8_t bus_read8(void *user, uint16_t addr) {
    (void)user;


    if(addr >=0x0000 && addr <=0x0001){
        //printf("CPU PORT READ: %04X  (io_visible=%d)\n", addr, io_visible());
        //fflush(stdout);

    }

    // 6510 port regs
    if (addr == 0x0000) return cpu_ddr;
    if (addr == 0x0001) return cpu_port_effective();

    // KERNAL ROM
    if (addr >= 0xE000 && kernal_visible())
        return kernal_rom[addr - 0xE000];

    // BASIC ROM
    if (addr >= 0xA000 && addr <= 0xBFFF && basic_visible())
        return basic_rom[addr - 0xA000];

    // $D000-$DFFF: I/O or CHARGEN or RAM depending on banking
    if (addr >= 0xD000 && addr <= 0xDFFF) {

        // ---- Always allow I/O devices for SID playback correctness ----
        // VIC ($D000-$D03F) and mirrors up to $D3FF
        if (addr <= 0xD3FF) {
            uint8_t r = vic.reg[addr & 0x3f];
            if (addr == 0xD012) return (uint8_t)(vic.raster & 0xFF);
            if (addr == 0xD011) {
                r = (uint8_t)((r & 0x7F) | ((vic.raster & 0x100) ? 0x80 : 0x00));
                return r;
            }
            if (addr == 0xD019) {
                uint8_t flags = (uint8_t)(vic.reg[0x19] & 0x0F);
                uint8_t b7    = vic.irq_line ? 0x80 : 0x00;
                return (uint8_t)(0x70 | flags | b7);
            }
            return r;
        }

        // SID ($D400-$D41F)
        if (addr >= 0xD400 && addr <= 0xD41F)
            return sid.reg[addr - 0xD400];

        // CIA1 ($DC00-$DCFF mirrors)
        if (addr >= 0xDC00 && addr <= 0xDCFF)
            return cia_read(&cia1, (uint8_t)(addr & 0x0F));

        // CIA2 ($DD00-$DDFF mirrors)
        if (addr >= 0xDD00 && addr <= 0xDDFF)
            return cia_read(&cia2, (uint8_t)(addr & 0x0F));

        // ---- If you want to preserve chargen mapping for non-IO reads, keep this ----
        if (chargen_visible())
            return char_rom[addr - 0xD000];

        return ram[addr];
    }



    // if we get here then NOTHING was handled where it should at least be caught by something

    //printf("RAM FALL THROUGH:\n");
    //fflush(stdout);
    return ram[addr];
}

void bus_write8(void *user, uint16_t addr, uint8_t v) {
    (void)user;

    ram[addr] = v;

    // 6510 port regs

    if (addr >= 0x0000 && addr <= 0x0001){
        //printf("CPU PORT write: $%04X=0x%02X  (io_visible=%d)\n", addr, v, io_visible());
        //fflush(stdout);
    }

    if (addr == 0x0000) { cpu_ddr = v;  return; }
    if (addr == 0x0001) { cpu_port = v; return; }

    // Writes to ROM regions go to RAM underneath when ROM is mapped in
    // (so don't block writes just because ROM is visible)

    if (addr >= 0xD000 && addr <= 0xDFFF){//&& io_visible()) {
        if (addr >= 0xD400 && addr <= 0xD41F) { sid_write(&sid, addr, v); return; }

        if (addr >= 0xDC00 && addr <= 0xDCFF) {
            uint8_t r = (uint8_t)(addr & 0x0F);
            cia_write(&cia1, r, v);
            return;
        }
        if (addr >= 0xDD00 && addr <= 0xDDFF) {
            uint8_t r = (uint8_t)(addr & 0x0F);
            cia_write(&cia2, r, v);
            return;
        }


        // I/O writes only when I/O is visible
        // VIC-II minimal writes ($D000-$D03F)
        if (addr >= 0xD000 && addr <= 0xD03F) {
            uint8_t idx = (uint8_t)(addr - 0xD000);

            if (addr == 0xD019) {
                // ACK: writing 1s clears those pending IRQ flags
                //vic.reg[0x19] &= (uint8_t)~(v & 0x0F); // asked to change this to THIS before
                vic.reg[0x19] &= (uint8_t)~(v & 0x0F); // clear requested flags
                vic.reg[0x19] &= 0x0F;                // keep only real flags

                vic_update_irq(&vic);
                return;
            }

            vic.reg[idx] = v;

            if (addr == 0xD01A) {
                // enabling/disabling sources changes irq_line
                vic_update_irq(&vic);
            }

            if (addr == 0xD012 || addr == 0xD011) {
                uint16_t cmp = (uint16_t)vic.reg[0x12];
                if (vic.reg[0x11] & 0x80) cmp |= 0x100;
                vic.raster_cmp = cmp;
            }
            return;
        }



        return;
    }

    ram[addr] = v;
}

void vic_tick(vic_t *v, uint32_t cpu_cycles){
    const uint32_t CYC_PER_LINE = v->cycles_per_line;
    const uint16_t LINES_PER_FRAME = v->lines_per_frame;

    v->cyc_accum += cpu_cycles;
    while (v->cyc_accum >= CYC_PER_LINE) {
        v->cyc_accum -= CYC_PER_LINE;

        v->raster++;
        if (v->raster >= LINES_PER_FRAME) v->raster = 0;

        // Raster compare hit -> set flag (bit0)
        if ((v->raster == v->raster_cmp)) {
            v->reg[0x19] |= 0x01;
            //if(v->raster == v->raster_cmp) printf("v->raster(%lu) == v->raster_cmp(%lu)\n", v->raster, v->raster_cmp);
            vic_update_irq(v);
        }

    }
}


void vic_set_video(vic_t *v, c64_video_t mode) {
    //cpuHz   = 985248;  // PAL or 1022727 : NTSC
    if (mode == C64_NTSC) {
        v->cycles_per_line = 65;
        v->lines_per_frame = 262;
        cpuHz   = 1022727;
    } else {
        v->cycles_per_line = 63;
        v->lines_per_frame = 312;
        cpuHz   = 985248;
    }
}
