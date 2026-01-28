#ifndef SID8579_H
#define SID8579_H


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SID_AUDIO_BUF  4096

typedef struct {
    uint8_t reg[0x20];     // D400-D41F shadow

    struct {
        uint32_t freq_inc;     // scaled increment per sample tick (old: freq*freqmul)
        uint32_t pulse_fp;     // pulse (12-bit) << 16
        uint8_t  wave;         // ctrl/wave bits
        uint32_t attack;
        uint32_t decay;
        uint32_t sustain;      // sustain nibble << 16
        uint32_t release;

        uint32_t counter;      // phase/counter (28-bit-ish in old code)
        int32_t  envval;       // 0..0xFFFFFF
        uint8_t  envphase;     // 0/1/2/3
        uint32_t noisepos;
        uint32_t noiseval;
        uint8_t  noiseout;
        uint8_t  filter_en;    // voice routed to filter bit (from res_ftv)
    } cv[3];


    struct {
        int32_t freq;          // fixedpoint
        int32_t rez;
        int32_t h, b, l;       // filter state
        uint8_t l_ena, b_ena, h_ena;
        uint8_t v3ena;
        uint8_t vol;
    } filt;

    // tables prepared for this sample rate
    uint32_t attacks[16];
    uint32_t releases[16];
    int32_t  freqmul;         // like old freqmul
    int32_t  filtmul;         // like old filtmul

    // timing
    uint32_t sid_hz;       // PAL/NTSC clock
    uint32_t sample_hz;    // output sample rate
    uint32_t cycles_acc;   // accumulate CPU cycles until sample step

    // output
    int16_t  out_l[SID_AUDIO_BUF];
    int16_t  out_r[SID_AUDIO_BUF];
    uint32_t out_count;   // how many samples are ready

    uint8_t  dirty;
} sid_t;

void sid_init(sid_t *s, uint32_t sid_hz, uint32_t sample_hz);
void sid_step(sid_t *s, uint32_t cpu_cycles);  // returns sample if ready else reuse last

int sid_render_interleaved(sid_t *s, int16_t *dst, int max_frames);

void sid_write(sid_t *sid, uint16_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif // SID8579_H
