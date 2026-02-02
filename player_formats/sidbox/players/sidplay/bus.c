#include <stdio.h>
#include <stdint.h>
#include "bus.h"
#include "cia.h"
#include "sid8579.h"
#include "vic.h"
#include <string.h>

uint8_t C64RAM[65536];

// --- 6510 I/O port ($0000/$0001) for banking ---
static uint8_t cpu_ddr  = 0x2F; // $0000
static uint8_t cpu_port = 0x37; // $0001

uint8_t bus_get_cpu_ddr(void)  { return cpu_ddr; }
uint8_t bus_get_cpu_port(void) { return cpu_port; }

// Effective output value (inputs read as 1 due to pull-ups)
static inline uint8_t cpu_port_eff(void){
    // Logic: bits that are outputs (DDR=1) use the port value.
    // Bits that are inputs (DDR=0) are pulled HIGH.
    return (uint8_t)((cpu_port & cpu_ddr) | (uint8_t)(~cpu_ddr));
}

// Banking bits
static inline int loram_on(uint8_t p)  { return (p & 0x01) != 0; }
static inline int hiram_on(uint8_t p)  { return (p & 0x02) != 0; }
static inline int charen_on(uint8_t p) { return (p & 0x04) != 0; }

static int seen_basic_read   = 0;
static int seen_kernal_read  = 0;
static int seen_chargen_read = 0;

// ROM images
static uint8_t rom_basic  [0x2000];
static uint8_t rom_kernal [0x2000];
static uint8_t rom_chargen[0x1000];
static int have_basic   = 0;
static int have_kernal  = 0;
static int have_chargen = 0;

static int load_file_exact(const char *path, uint8_t *dst, size_t want){
    if(!path || !*path) return 0;
    FILE *f = fopen(path, "rb");
    if(!f) return 0;
    size_t n = fread(dst, 1, want, f);
    fclose(f);
    return n == want;
}

static uint32_t crc32_simple(const uint8_t *p, size_t n){
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++){
        crc ^= p[i];
        for (int k = 0; k < 8; k++){
            uint32_t mask = (uint32_t)-(int)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

int bus_load_roms(const char *basic_rom_path, const char *kernal_rom_path, const char *chargen_rom_path){
    have_basic   = load_file_exact(basic_rom_path,   rom_basic,   sizeof(rom_basic));
    have_kernal  = load_file_exact(kernal_rom_path,  rom_kernal,  sizeof(rom_kernal));
    have_chargen = load_file_exact(chargen_rom_path, rom_chargen, sizeof(rom_chargen));

    printf("[ROM] BASIC    %s  (%s)\n", have_basic   ? "OK" : "MISSING", basic_rom_path   ? basic_rom_path   : "(null)");
    printf("[ROM] KERNAL   %s  (%s)\n", have_kernal  ? "OK" : "MISSING", kernal_rom_path  ? kernal_rom_path  : "(null)");
    printf("[ROM] CHARGEN  %s  (%s)\n", have_chargen ? "OK" : "MISSING", chargen_rom_path ? chargen_rom_path : "(null)");

    if (have_basic)  printf("[ROM] BASIC   crc32=%08X first=%02X last=%02X\n", crc32_simple(rom_basic, sizeof(rom_basic)), rom_basic[0], rom_basic[sizeof(rom_basic)-1]);
    if (have_kernal) printf("[ROM] KERNAL  crc32=%08X first=%02X last=%02X\n", crc32_simple(rom_kernal, sizeof(rom_kernal)), rom_kernal[0], rom_kernal[sizeof(rom_kernal)-1]);
    if (have_chargen)printf("[ROM] CHARGEN crc32=%08X first=%02X last=%02X\n", crc32_simple(rom_chargen, sizeof(rom_chargen)), rom_chargen[0], rom_chargen[sizeof(rom_chargen)-1]);

    return have_basic && have_kernal && have_chargen;
}

#define OUT_CHAR_ADDR   0xD7F0

void clear64KRam(void){
    memset(C64RAM, 0, sizeof(C64RAM));
    cpu_ddr  = 0x2F;
    cpu_port = 0x37;
}

uint8_t bus_read8(uint16_t addr) {
    if (playmode_sidtype == SIDPLAY_PLAYMODE_PSID) {
        if (addr >= 0xD400 && addr <= 0xD41F) return 0;
        return C64RAM[addr];
    }

    const uint8_t p = cpu_port_eff();

    // 1. Processor Port
    if (addr == 0x0000) return cpu_ddr;
    if (addr == 0x0001) return p;

    // 2. BASIC ROM ($A000-$BFFF)
    if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (have_basic && loram_on(p) && hiram_on(p))
            return rom_basic[addr - 0xA000];
    }

    // 3. KERNAL ROM ($E000-$FFFF)
    if (addr >= 0xE000) {
        if (have_kernal && hiram_on(p))
            return rom_kernal[addr - 0xE000];
    }

    // 4. I/O or Character ROM ($D000-$DFFF)
    if (addr >= 0xD000 && addr <= 0xDFFF) {
        // I/O visibility check
        if (charen_on(p) && (loram_on(p) || hiram_on(p))) {
            // VIC-II ($D000-$D3FF) - mirrored every 64 bytes
            if (addr <= 0xD3FF) return vic_read(0xD000 | (addr & 0x3F));

            // SID ($D400-$D7FF) - mirrored every 32 bytes
            if (addr >= 0xD400 && addr <= 0xD7FF) {
                // Determine if read is supported (usually SID is write-only)
                // Mirrors exist every 0x20 bytes
                printf("SID Read!\n");
                return 0xff;
            }

            // CIA 1 ($DC00-$DCFF) - mirrored every 16 bytes
            if (addr >= 0xDC00 && addr <= 0xDCFF) return cia_read(CIA_CHIP_1, addr & 0x0F);

            // CIA 2 ($DD00-$DDFF) - mirrored every 16 bytes
            if (addr >= 0xDD00 && addr <= 0xDDFF) return cia_read(CIA_CHIP_2, addr & 0x0F);

            return 0xFF;
        }
        // Character ROM visibility
        else if (!charen_on(p) && (loram_on(p) || hiram_on(p))) {
            if (have_chargen) return rom_chargen[addr & 0x0FFF];
        }
    }

    return C64RAM[addr];
}


void bus_write8(uint16_t addr, uint8_t v) {
    C64RAM[addr] = v;


    if (playmode_sidtype == SIDPLAY_PLAYMODE_PSID) {
        if (addr >= 0xD400 && addr <= 0xD7FF) sid_write( addr, v);
        return;
    }

    const uint8_t p = cpu_port_eff();

    if (addr == 0x0000) { cpu_ddr  = v; return; }
    if (addr == 0x0001) { cpu_port = v; return; }

    if (charen_on(p) && (loram_on(p) || hiram_on(p))) {
        // VIC-II Mirroring
        if (addr >= 0xD000 && addr <= 0xD3FF) {
            vic_write(0xD000 | (addr & 0x3F), v);
            return;
        }

        // SID 1 & 2 Mirroring (ranges from $D400 to $D7FF)
        if (addr >= 0xD400 && addr <= 0xD7FF) {
            sid_write( addr, v);
            return;
        }

        // CIA 1 Mirroring
        if (addr >= 0xDC00 && addr <= 0xDCFF) {
            cia_write(CIA_CHIP_1, addr & 0x0F, v);
            return;
        }

        // CIA 2 Mirroring
        if (addr >= 0xDD00 && addr <= 0xDDFF) {
            cia_write(CIA_CHIP_2, addr & 0x0F, v);
            return;
        }
    }
}

void bus_write16(uint16_t addr, uint16_t v) {
    bus_write8(addr, (uint8_t)(v & 0xFF));
    bus_write8(addr + 1, (uint8_t)(v >> 8));
}

uint16_t bus_read16(uint16_t addr) {
    return (uint16_t)bus_read8(addr) | ((uint16_t)bus_read8(addr + 1) << 8);
}

uint16_t bus_read16_wrap(uint16_t addr) {
    uint8_t lo = bus_read8(addr);
    uint16_t addr2 = (addr & 0xFF00) | ((addr + 1) & 0x00FF);
    uint8_t hi = bus_read8(addr2);
    return (uint16_t)(lo | ((uint16_t)hi << 8));
}
