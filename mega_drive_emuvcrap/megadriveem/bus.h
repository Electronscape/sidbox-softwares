#ifndef BUS_H
#define BUS_H

#include "common.h"
#include "rom.h"
#include "vdp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Bus {
    Rom rom;

    // 64KB work RAM (0xFF0000-0xFFFFFF)
    uint8_t ram[64 * 1024];

    Vdp vdp;
} Bus;

void bus_init(Bus* bus, int fb_w, int fb_h);
void bus_reset(Bus* bus);
void bus_free(Bus* bus);

uint8_t  bus_read8 (Bus* bus, uint32_t addr);
uint16_t bus_read16(Bus* bus, uint32_t addr);

void bus_write8 (Bus* bus, uint32_t addr, uint8_t  val);
void bus_write16(Bus* bus, uint32_t addr, uint16_t val);


#ifdef __cplusplus
}
#endif
#endif
