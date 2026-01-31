#ifndef SIDPLAYBUS_H
#define SIDPLAYBUS_H

#include <stdint.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif


typedef enum { C64_PAL, C64_NTSC } c64_video_t;

typedef struct {
    uint8_t reg[0x40];     // $D000-$D03F
    uint16_t raster;       // current raster line 0..311 (PAL)
    uint16_t raster_cmp;   // compare value (from $D012 + bit from $D011)
    uint8_t irq_line;      // 1 = asserted (however your cpu core expects)
    uint32_t cyc_accum;    // cycle accumulator for raster stepping

    uint16_t cycles_per_line;    // cycles per line
    uint16_t lines_per_frame;   // lines per frame
} vic_t;



#define IO_PUTCHAR 0xD020

void romloadeds();

extern uint32_t cpuHz;

extern uint8_t ram[];

extern vic_t vic;






void clear_ram(void);
void load_prog(uint16_t addr, const uint8_t *prog, size_t n);
uint8_t bus_read8(void *user, uint16_t addr);
void bus_write8(void *user, uint16_t addr, uint8_t v);
void vic_set_video(vic_t *v, c64_video_t mode);


void kernal_write_u8(uint16_t cpu_addr, uint8_t v);

void vic_tick(vic_t *v, uint32_t cpu_cycles);

#ifdef __cplusplus
}
#endif

#endif // SIDPLAYBUS_H
