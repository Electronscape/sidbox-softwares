#ifndef BUS_H
#define BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t C64RAM[65536];

void     clear64KRam(void);

uint8_t  bus_read8(uint16_t addr);
void     bus_write8(uint16_t addr, uint8_t v);

void bus_write16(uint16_t addr, uint16_t v);
uint16_t bus_read16(uint16_t addr);          // normal little-endian
uint16_t bus_read16_wrap(uint16_t addr);     // 6502 JMP($xxFF) wrap bug


#ifdef __cplusplus
}
#endif

#endif // BUS_H

