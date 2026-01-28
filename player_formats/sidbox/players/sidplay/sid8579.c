
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "sid8579.h"


// PAL C64 clock ≈ 985248 Hz
// NTSC ≈ 1022727 Hz
// You already pass this in, so we trust you 😉

// --- SID register offsets (shadowed as 0x00..0x1F for $D400..$D41F) ---

/* Put these at file scope */
#define ENV_ATTACK   0
#define ENV_DECAY    1
#define ENV_SUSTAIN  2
#define ENV_RELEASE  3

#define CTRL_GATE    0x01
#define CTRL_SYNC    0x02
#define CTRL_RING    0x04
#define CTRL_TEST    0x08

#define WF_TRI       0x10
#define WF_SAW       0x20
#define WF_PULSE     0x40
#define WF_NOISE     0x80

#define U24_MASK     0xFFFFFFu





// Chip model selection (keep it simple for now)
#ifndef SID_CHIP_6581
#define SID_CHIP_6581 1
#endif

static const float attackTimes[16] = {
    0.0022528606f, 0.0080099577f, 0.0157696042f, 0.0237795619f,
    0.0372963655f, 0.0550684591f, 0.0668330845f, 0.0783473987f,
    0.0981219818f, 0.244554021f,  0.489108042f,  0.782472742f,
    0.977715461f,  2.93364701f,   4.88907793f,   7.82272493f
};

static const float decayReleaseTimes[16] = {
    0.00891777693f, 0.024594051f,  0.0484185907f, 0.0730116639f,
    0.114512475f,   0.169078356f,  0.205199432f,  0.240551975f,
    0.301266125f,   0.750858245f,  1.50171551f,   2.40243682f,
    3.00189298f,    9.00721405f,   15.010998f,    24.0182111f
};

static inline uint8_t get_bit_u8(uint8_t v, uint8_t b){ return (v >> b) & 1u; }

static inline int32_t pfloat_from_int(int32_t i){ return i << 16; }
static inline int32_t pfloat_from_float(float f){ return (int32_t)(f * 65536.0f); }
static inline int32_t pfloat_mul(int32_t a, int32_t b){ return (a >> 8) * (b >> 8); }
static inline int32_t pfloat_to_int(int32_t a){ return a >> 16; }


void sid_init(sid_t *s, uint32_t sid_hz, uint32_t sample_hz)
{
    memset(s, 0, sizeof(*s));
    s->sid_hz    = sid_hz;
    s->sample_hz = sample_hz;
    s->cycles_acc = 0;
    s->dirty = 1;

    s->freqmul = (int32_t)(15872000u / sample_hz);
    s->filtmul = pfloat_from_float(8.11415946f) / (int32_t)sample_hz;

    // ADSR clk for 6581 was 0x30 (8580 was 0x18)
    const int adsrclk = 0x30;

    for (int i = 0; i < 16; i++) {
        // matches: (adsrclk*0x100000)/(time*mixing_frequency)
        s->attacks[i]  = (uint32_t)((adsrclk * 0x100000u) / (attackTimes[i] * (float)sample_hz));
        s->releases[i] = (uint32_t)((adsrclk * 0x100000u) / (decayReleaseTimes[i] * (float)sample_hz));
        if (s->attacks[i]  == 0) s->attacks[i]  = 1;
        if (s->releases[i] == 0) s->releases[i] = 1;
    }

    // init noise like old
    for (int v = 0; v < 3; v++) {
        s->cv[v].noiseval = 0xFFFFFFu;
        s->cv[v].envval = 0;
        s->cv[v].envphase = 3; // release
    }
}

int sid_render_interleaved(sid_t *s, int16_t *dst, int max_frames){
    int n = (s->out_count < (uint32_t)max_frames) ? (int)s->out_count : max_frames;

    for (int i = 0; i < n; i++) {
        dst[i*2 + 0] = s->out_l[i];
        dst[i*2 + 1] = s->out_r[i];
    }

    // shift remaining samples down (usually none left)
    for (uint32_t i = (uint32_t)n; i < s->out_count; i++) {
        s->out_l[i - (uint32_t)n] = s->out_l[i];
        s->out_r[i - (uint32_t)n] = s->out_r[i];
    }

    s->out_count -= (uint32_t)n;
    return n;
}


static void sid_prepare(sid_t *s)
{
    // Step 1: convert SID regs into cached fast values (like synth_start)

    for (int v = 0; v < 3; v++) {
        const uint8_t base = (uint8_t)(v * 7);

        const uint16_t freq = (uint16_t)(
            (uint16_t)s->reg[base + 0] |
            ((uint16_t)s->reg[base + 1] << 8)
            );

        const uint16_t pw = (uint16_t)(
            (uint16_t)s->reg[base + 2] |
            ((uint16_t)(s->reg[base + 3] & 0x0F) << 8)
            );

        const uint8_t wave = s->reg[base + 4];
        const uint8_t ad   = s->reg[base + 5];
        const uint8_t sr   = s->reg[base + 6];

        s->cv[v].pulse_fp = ((uint32_t)(pw & 0x0FFFu)) << 16;
        s->cv[v].filter_en = (uint8_t)get_bit_u8(s->reg[0x17], (uint8_t)v);

        s->cv[v].attack  = s->attacks[(ad >> 4) & 0x0F];
        s->cv[v].decay   = s->releases[(ad >> 0) & 0x0F];
        s->cv[v].sustain = (uint32_t)(sr & 0xF0u);          // nibble in high half
        s->cv[v].release = s->releases[(sr >> 0) & 0x0F];

        s->cv[v].wave = wave;

        // THIS is the old core pitch scaling
        s->cv[v].freq_inc = (uint32_t)((uint64_t)freq * (uint64_t)s->freqmul);
    }

    // Filter cache (old USE_FILTER section)
    // rez = pfloat(1.2) - pfloat(0.04)*(res_ftv>>4) for 6581
    int32_t rez = pfloat_from_float(1.2f) - pfloat_from_float(0.04f) * (int32_t)(s->reg[0x17] >> 4);
    if (rez > 21200) rez = 21200;
    if (rez < 100)   rez = 100;
    rez >>= 8;
    s->filt.rez = rez;

    // freq = (16*hi + (lo&7)) * filtmul ; clamp <= 1.0
    int32_t f = (int32_t)(16u * (uint32_t)s->reg[0x16] + ((uint32_t)s->reg[0x15] & 7u));
    f = (int32_t)((int64_t)f * (int64_t)s->filtmul);

    if (f > pfloat_from_int(1)) f = pfloat_from_int(1);
    s->filt.freq = f;

    const uint8_t fv = s->reg[0x18];
    s->filt.l_ena = get_bit_u8(fv, 4);
    s->filt.b_ena = get_bit_u8(fv, 5);
    s->filt.h_ena = get_bit_u8(fv, 6);
    s->filt.v3ena = (uint8_t)!get_bit_u8(fv, 7);
    s->filt.vol   = (uint8_t)(fv & 0x0F);
}



/* NOTE: This sid_step intentionally does NOT fake ADSR timing tables.
 * It implements the real structure + noise + waveform AND mixing.
 * You can drop in a real EG period table later without changing architecture.
 */
void sid_step(sid_t *s, uint32_t cpu_cycles)
{
    uint32_t cycles_per_sample;
    uint8_t  vol4;
    uint8_t  wrapped[3];
    uint32_t prev_phase[3];
    int vi;

    /* -------- EG timing tables (integer, derived from your floats) --------
       Values are microseconds for full 0..255 attack or full 255..0 decay/release. */
    //static const uint32_t atk_us[16] = { 2253, 8010, 15770, 23780, 37296, 55068, 66833, 78347, 98122, 244554, 489108, 782473, 977715, 2933647, 4889078, 7822725 };
    //static const uint32_t dr_us[16] = { 8918, 24594, 48419, 73012, 114512, 169078, 205199, 240552, 301266, 750858, 1501716, 2402437, 3001893, 9007214, 15010998, 24018211 };

    /* Convert table -> samples-per-env-step for THIS sample rate (once) */
    //static uint32_t cached_hz = 0;
    //static uint32_t atk_step_samp[16];
    //static uint32_t dr_step_samp[16];

    s->cycles_acc += cpu_cycles;

    cycles_per_sample = (s->sid_hz / s->sample_hz);
    if (cycles_per_sample == 0) return;
    if (s->dirty) {
        sid_prepare(s);
        s->dirty = 0;
    }

    while (s->cycles_acc >= cycles_per_sample) {
        s->cycles_acc -= cycles_per_sample;
        if (s->out_count >= SID_AUDIO_BUF) { continue; }

        vol4 = (uint8_t)(s->reg[0x18] & 0x0F);

        int32_t outf = 0;
        int32_t outo = 0;

        for (int v = 0; v < 3; v++) {
            // update wave counter
            s->cv[v].counter = (s->cv[v].counter + s->cv[v].freq_inc) & 0x0FFFFFFFu;

            // TEST bit resets counter + noise
            if (s->cv[v].wave & CTRL_TEST) {
                s->cv[v].counter = 0;
                s->cv[v].noisepos = 0;
                s->cv[v].noiseval = 0xFFFFFFu;
            }

            const uint8_t refosc = (v ? (uint8_t)(v - 1) : 2);

            // SYNC
            if (s->cv[v].wave & CTRL_SYNC) {
                if (s->cv[refosc].counter < s->cv[refosc].freq_inc) {
                    // original: scale by ratio
                    if (s->cv[refosc].freq_inc != 0) {
                        s->cv[v].counter =
                            (uint32_t)((uint64_t)s->cv[refosc].counter * (uint64_t)s->cv[v].freq_inc / (uint64_t)s->cv[refosc].freq_inc);
                    } else {
                        s->cv[v].counter = 0;
                    }
                }
            }

            // --- waveforms ---
            uint8_t triout = (uint8_t)(s->cv[v].counter >> 19);
            if (s->cv[v].counter >> 27) triout ^= 0xFF;

            uint8_t sawout = (uint8_t)(s->cv[v].counter >> 20);

            uint8_t plsout = (uint8_t)((s->cv[v].counter > s->cv[v].pulse_fp) - 1);

            // noise (23-bit LFSR)
            if (s->cv[v].noisepos != (s->cv[v].counter >> 24)) {
                s->cv[v].noisepos = (s->cv[v].counter >> 24);

                // newbit = bit22 xor bit17
                uint32_t nv = s->cv[v].noiseval;
                uint32_t newb = ((nv >> 22) ^ (nv >> 17)) & 1u;
                nv = ((nv << 1) | newb) & 0x7FFFFFu;
                s->cv[v].noiseval = nv;

                s->cv[v].noiseout = (uint8_t)(
                    (((nv >> 22) & 1u) << 7) |
                    (((nv >> 20) & 1u) << 6) |
                    (((nv >> 16) & 1u) << 5) |
                    (((nv >> 13) & 1u) << 4) |
                    (((nv >> 11) & 1u) << 3) |
                    (((nv >>  7) & 1u) << 2) |
                    (((nv >>  4) & 1u) << 1) |
                    (((nv >>  2) & 1u) << 0)
                    );
            }
            uint8_t nseout = s->cv[v].noiseout;

            // ringmod
            if (s->cv[v].wave & CTRL_RING) {
                if (s->cv[refosc].counter < 0x8000000u) triout ^= 0xFF;
            }

            // mix waveforms (AND-combo)
            uint8_t outv = 0xFF;
            if (s->cv[v].wave & WF_TRI)   outv &= triout;
            if (s->cv[v].wave & WF_SAW)   outv &= sawout;
            if (s->cv[v].wave & WF_PULSE) outv &= plsout;
            if (s->cv[v].wave & WF_NOISE) outv &= nseout;

            // --- envelope (old behaviour) ---
            if (!(s->cv[v].wave & CTRL_GATE)) {
                s->cv[v].envphase = 3;
            } else if (s->cv[v].envphase == 3) {
                s->cv[v].envphase = 0;
            }

            switch (s->cv[v].envphase) {
            case 0: // attack
                s->cv[v].envval += (int32_t)s->cv[v].attack;
                if (s->cv[v].envval >= (int32_t)0xFFFFFF) {
                    s->cv[v].envval = (int32_t)0xFFFFFF;
                    s->cv[v].envphase = 1;
                }
                break;

            case 1: // decay
                s->cv[v].envval -= (int32_t)s->cv[v].decay;
                if (s->cv[v].envval <= (int32_t)(s->cv[v].sustain << 16)) {
                    s->cv[v].envval = (int32_t)(s->cv[v].sustain << 16);
                    s->cv[v].envphase = 2;
                }
                break;

            case 2: // sustain
                if (s->cv[v].envval != (int32_t)(s->cv[v].sustain << 16)) {
                    s->cv[v].envphase = 1;
                }
                break;

            case 3: // release
            default:
                s->cv[v].envval -= (int32_t)s->cv[v].release;
                if (s->cv[v].envval < (int32_t)0x40000) s->cv[v].envval = (int32_t)0x40000;
                break;
            }

            // route to filter/non-filter, obey voice3 enable
            if ((v < 2) || s->filt.v3ena) {
                int32_t tform = (((int32_t)((int)outv - 0x80)) * (int32_t)s->cv[v].envval) >> 22;
                if (s->cv[v].filter_en) outf += tform;
                else                    outo += tform;
            }
        }

        // --- filter stage ---
        if (s->filt.freq < 2000) s->filt.freq = 2000;

        s->filt.h = pfloat_from_int(outf) - (s->filt.b >> 8) * s->filt.rez - s->filt.l;
        s->filt.b += pfloat_mul(s->filt.freq, s->filt.h);
        s->filt.l += pfloat_mul(s->filt.freq, s->filt.b);

        int32_t outf2 = 0;
        if (s->filt.l_ena) outf2 += pfloat_to_int(s->filt.l);
        if (s->filt.b_ena) outf2 += pfloat_to_int(s->filt.b);
        if (s->filt.h_ena) outf2 += pfloat_to_int(s->filt.h);

        int32_t final = (int32_t)s->filt.vol * (outo + outf2);

        // old code did >>1 after GenerateDigi; you don't have digis here yet,
        // so mimic the gain staging:
        //final >>= 1;

        if (final < -32700) final = -32700;
        if (final >  32700) final =  32700;

        s->out_l[s->out_count] = (int16_t)final;
        s->out_r[s->out_count] = (int16_t)final;
        s->out_count++;

    }
}





void sid_write(sid_t *sid, uint16_t addr, uint8_t val){
    static uint32_t sid_writes = 0;


    sid_writes++;
//    if (sid_writes > 30)
    {
        sid_writes = 0;
        printf("sid writes: %u  D418=%02X  V1 ctrl=%02X\n",
               sid_writes,
               sid->reg[0x18],
               sid->reg[0x04]);
        fflush(stdout);
    }


    if (addr < 0xD400 || addr > 0xD41F) return;
    sid->reg[addr - 0xD400] = val;
    sid->dirty = 1;


}
