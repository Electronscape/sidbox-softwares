#ifndef PLAYSID_H
#define PLAYSID_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


#define AUDIO_MIX_FREQ  48000


extern uint8_t playmode_sidtype;


// Init + load PSID
int PlaySID_Init(const char *filename, int subsong /* -1 = default */);
int CheckSIDType(const char *filename);
void playsid_start_tune(int subtune);

// Fill interleaved stereo buffer (S16_LE)
// frames = number of stereo frames requested
// returns frames actually written
uint32_t doPlaySidStep(int16_t *out_interleaved, uint32_t frames);


//////////// RSID Sections //////////////////
int PlaySID_InitRSID(const char *filename, uint8_t subsong);
uint32_t doRSIDStep(int16_t *out_interleaved, uint32_t frames, uint32_t sample_rate);



void playsid_switch_subsong_rsid_soft(uint8_t song0);



#ifdef __cplusplus
}
#endif
#endif
