#ifndef SID8579_H
#define SID8579_H

#include <stdint.h>
#include "cpu6502.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Chip6581 0
#define Chip8580 1

// Called by bus when SID regs written
void sid_write(uint16_t reg, uint8_t v);

// Called by player to (re)init synth
void synth_init(uint32_t mixfrq);
void synth_prep_per_step();

// Render ONE sample (stereo) into signed 16-bit
void sid_render_sample(int16_t *outL, int16_t *outR);

// Optional: chip config controls (kept from your style)
void SetSidChipTypes(unsigned char chip, unsigned char type);
unsigned char GetSidChipType(unsigned char chip);
void SetSidChipVoices(unsigned char chip, unsigned char voices);
unsigned char GetSidChipVoices(unsigned char chip);
void restartSidChipModes(void);



void sid_render_sample_noadvance(int16_t *outL, int16_t *outR);
void sid_clock_cycles(uint32_t cycles);



#ifdef __cplusplus
}
#endif
#endif
