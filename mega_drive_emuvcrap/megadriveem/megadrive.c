#include "common.h"
#include "rom.h"
#include "bus.h"
#include "m68k_core.h"
#include "renderer.h"

static void vdp_write_reg(Bus* bus, uint8_t reg, uint8_t val) {
    // VDP reg write: 0x8000 | (reg<<8) | val to control port
    uint16_t w = (uint16_t)(0x8000u | ((uint16_t)(reg & 0x1F) << 8) | val);
    bus_write16(bus, 0xC00004, w);
}

static void vdp_set_cram_write_addr(Bus* bus, uint16_t byte_addr)
{
    // Build words to match *your* vdp_commit_command() decode:
    // addr = (w0 & 0x3FFF) | ((w1 & 0x0003) << 14)
    //
    // For CRAM write, we want code low nibble == 0x3.
    // Your vdp_commit_command maps target using (code & 0x0F):
    //   0x03 => CRAM
    //
    // Your code combines ((w0 >> 14) & 3) into the low bits, so force those to 3 by using 0xC000.
    uint16_t addr = byte_addr & 0xFFFFu;

    uint16_t w0 = (uint16_t)((addr & 0x3FFFu) | 0xC000u);      // low 14 addr + code bits forced to 3
    uint16_t w1 = (uint16_t)((addr >> 14) & 0x0003u);          // upper 2 addr bits in bits 0..1

    bus_write16(bus, 0xC00004, w0);
    bus_write16(bus, 0xC00004, w1);
}

static void test_make_visible_output(Bus* bus) {
    // Set auto-increment to 2 bytes (word)
    vdp_write_reg(bus, 0x0F, 2);

    // Write CRAM[0] to a bright color so the frame isn’t black.
    // Example: max red in MD 3-bit is 7 -> RRR = 7 => bits (>>1) field set.
    // cram word approx: 0x000E for red-ish under our masking
    vdp_set_cram_write_addr(bus, 0x0000);
    bus_write16(bus, 0xC00000, 0x000E); // CRAM[0] = bright red (rough)

    // Could write more CRAM entries later.
}

M68K cpu;
Bus bus;
int megadriveGO(char* romfilename) {


    bus_init(&bus, 320, 224);

    if (!rom_load(&bus.rom, romfilename)) {
        bus_free(&bus);
        return 1;
    }


    m68k_init(&cpu);
    bus_reset(&bus);
    m68k_reset(&cpu, &bus);

    fprintf(stderr, "Initial PC (from vector): 0x%06X\n", cpu.pc);

    // Until we have a CPU core, do a direct VDP poke to prove the pipeline works.
    test_make_visible_output(&bus);

    // PAL-only timing: 50 frames


    //bus_free(&bus);
    return 0;
}


void doMDFrames(){
    for (int frame = 0; frame < 1; frame++) {
        // Normally you'd run CPU cycles for a frame here and let it drive VDP.
        m68k_step_cycles(&cpu, &bus, 140000);

        // Render frame


    }

    vdp_render_frame(&bus.vdp);
    //printf(".");
}
