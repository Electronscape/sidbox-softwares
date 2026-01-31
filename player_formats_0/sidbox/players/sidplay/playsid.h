#ifndef PLAYSID_H
#define PLAYSID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

typedef enum {
    SIDFMT_UNKNOWN = 0,
    SIDFMT_PSID    = 1,
    SIDFMT_RSID    = 2
} sid_fmt_t;


typedef struct {
    sid_fmt_t fmt;

    uint16_t load_addr;   // final load address used
    uint16_t init_addr;   // header init
    uint16_t play_addr;   // header play (PSID usually uses this; RSID may ignore it)
    uint16_t songs;
    uint16_t start_song;
    uint32_t speed_bits;  // PSID speed field

    // convenience: where the data started in the file
    uint16_t data_offset;

    // flags/info (optional)
    uint16_t flags;
} sid_program_t;

// Parse + copy SID data into C64 RAM. Returns 0 on success.
int sid_load_from_bytes(const uint8_t *buf, uint32_t len, uint8_t ram[65536], sid_program_t *out);




#ifdef __cplusplus
}
#endif


#endif // PLAYSID_H
