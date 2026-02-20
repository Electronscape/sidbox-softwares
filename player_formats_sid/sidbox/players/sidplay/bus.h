#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t C64RAM[65536];

#define SIDPLAY_PLAYMODE_PSID  0x00
#define SIDPLAY_PLAYMODE_RSID  0x01

void     clear64KRam(void);

uint8_t  bus_read8(uint16_t addr);
void     bus_write8(uint16_t addr, uint8_t v);

void bus_write16(uint16_t addr, uint16_t v);
uint16_t bus_read16(uint16_t addr);          // normal little-endian
uint16_t bus_read16_wrap(uint16_t addr);     // 6502 JMP($xxFF) wrap bug

// --- Optional ROM support (RSID needs this for many tunes) ---
// User must provide their own ROM dumps (copyrighted).
// Returns 1 if all requested ROMs loaded successfully, 0 otherwise.
int bus_load_roms(const char *basic_rom_path, const char *kernal_rom_path, const char *chargen_rom_path);

// Expose current 6510 port state (useful for debugging banking)
uint8_t bus_get_cpu_ddr(void);
uint8_t bus_get_cpu_port(void);


#ifdef __cplusplus
}
#endif

#endif // BUS_H

