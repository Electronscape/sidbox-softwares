#ifndef PLAYSID_H
#define PLAYSID_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif













#define SIDPLAY_PLAYMODE_PSID  0x00
#define SIDPLAY_PLAYMODE_RSID  0x01

// Init + load PSID
int PlaySID_Init(const char *filename, int subsong /* -1 = default */);
void playsid_start_tune(int subtune);

// Fill interleaved stereo buffer (S16_LE)
// frames = number of stereo frames requested
// returns frames actually written
uint32_t doPlaySidStep(int16_t *out_interleaved, uint32_t frames);


// Cycle stats
uint32_t PlaySID_GetLastPlayCycles(void);
uint64_t PlaySID_GetTotalCycles(void);
void     PlaySID_ResetCycleCounters(void);

//////////// RSID Sections //////////////////
int PlaySID_InitRSID(const char *filename);
uint32_t doRSIDStep(int16_t *out_interleaved, uint32_t frames, uint32_t sample_rate);





/////////// program testing ////////////////
// Load a 6502 test program into C64 RAM.
// If prg_has_loadaddr=1, bytes[0..1] contain load address (little endian) and code starts at bytes+2.
// If prg_has_loadaddr=0, you must provide load_addr.
//
// Vectors are written into RAM at $FFFA..$FFFF.
// After loading, cpuReset() is called (it fetches RESET vector from $FFFC).
//
// Returns 1 on success, 0 on failure.
int PlaySID_LoadProgram(const uint8_t *bytes, size_t len,
                        int prg_has_loadaddr,
                        uint16_t load_addr,
                        uint16_t reset_vec,
                        uint16_t irq_vec,
                        uint16_t nmi_vec);

// Convenience: PRG bytes with embedded load address
static inline int PlaySID_LoadProgramPRG(const uint8_t *bytes, size_t len, uint16_t reset_vec, uint16_t irq_vec, uint16_t nmi_vec) {
    return PlaySID_LoadProgram(bytes, len, 1, 0, reset_vec, irq_vec, nmi_vec);
}

// Convenience: raw bytes + explicit load address, reset vector defaults to load_addr if reset_vec==0
static inline int PlaySID_LoadProgramRAW(const uint8_t *bytes, size_t len, uint16_t load_addr, uint16_t reset_vec, uint16_t irq_vec, uint16_t nmi_vec){
    if (reset_vec == 0) reset_vec = load_addr;
    return PlaySID_LoadProgram(bytes, len, 0, load_addr, reset_vec, irq_vec, nmi_vec);
}






#ifdef __cplusplus
}
#endif
#endif
