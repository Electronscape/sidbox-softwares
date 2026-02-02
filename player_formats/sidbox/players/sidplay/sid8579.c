#include <stdio.h>
#include <stdint.h>
#include "sid8579.h"
#include "bus.h"
#include <string.h>

// ---- Your globals/knobs (kept similar) ----
volatile unsigned long CHIPCONFIGS = 0;

#define SIDHV_CHANNEL_STEREO (1u<<0) // you can map this properly later

static unsigned char SidChipType[2] = { Chip6581, Chip6581 };
//static unsigned char SidChipType[2] = { Chip8580, Chip8580 };
static unsigned char SidVoicesEn[2] = { 0x7, 0x7 };

void SetSidChipTypes(unsigned char chip, unsigned char type){ SidChipType[chip & 1] = type; }
unsigned char GetSidChipType(unsigned char chip){ return SidChipType[chip & 1]; }
void SetSidChipVoices(unsigned char chip, unsigned char voices){ SidVoicesEn[chip & 1] = voices; }
unsigned char GetSidChipVoices(unsigned char chip){ return SidVoicesEn[chip & 1]; }

static unsigned char bDualChipMode = 1;        // 1=single, 2=dual detected
unsigned char bSidPlay2SIDmode = 0;     // address-based "2SID mode"
static int mixing_frequency;
static int freqmul;
static int filtmul_1, filtmul_2;

// --- fixed-point helper (your pfloat) ---
static inline int pfloat_ConvertFromInt(int i) { return i << 16; }
static inline int pfloat_ConvertFromFloat(float f) { return (int)(f * (1L << 16)); }
static inline int pfloat_Multiply(int a, int b) { return (a >> 8) * (b >> 8); }
static inline int pfloat_ConvertToInt(int i) { return i >> 16; }

struct sidfiltercaps { float capacitor; float banda; float bandb; long adsrclk; };

static struct sidfiltercaps filtermult[2] = {
    { 2.41415946f, 1.4f,  0.04f, 0x30 }, // 6581
    { 21.5332031f, 1.0f,  0.04f, 0x18 }  // 8580
};

// SID reg layout
struct s6581 {
    struct sidvoice {
        dword freq;
        dword pulse;
        byte wave;
        byte ad;
        byte sr;
    } v[3];
    byte ffreqlo;
    byte ffreqhi;
    byte res_ftv;
    byte ftp_vol;
};

struct sidosc {
    dword freq;
    dword pulse;
    byte wave;
    byte filter;
    dword attack;
    dword decay;
    dword sustain;
    dword release;
    dword counter;
    signed int envval;
    byte envphase;
    dword noisepos;
    dword noiseval;
    byte noiseout;
};

struct sidflt {
    int freq;
    byte l_ena, b_ena, h_ena;
    byte v3ena;
    int stray;
    int vol;
    int rez;
    int h, b, l;
};

__attribute__((aligned(32)))
static struct s6581 sid[2];
__attribute__((aligned(32)))
static struct sidosc osc[2][3];
__attribute__((aligned(32)))
static struct sidflt filter[2];

static uint8_t vol_dac[2];
static uint8_t vol_last[2];
static int32_t digi_dc[2];   // for optional DC blocking

#define DIGI_GAIN 800


__attribute__((aligned(32)))
static const float attackTimes[16] = {
        0.0022528606f, 0.0080099577f, 0.0157696042f, 0.0237795619f,
        0.0372963655f, 0.0550684591f, 0.0668330845f, 0.0783473987f,
        0.0981219818f, 0.244554021f,  0.489108042f,  0.782472742f,
        0.977715461f,  2.93364701f,   4.88907793f,   7.82272493f
};

__attribute__((aligned(32)))
static const float decayReleaseTimes[16] = {
        0.00891777693f, 0.024594051f,  0.0484185907f, 0.0730116639f,
        0.114512475f,   0.169078356f,  0.205199432f,  0.240551975f,
        0.301266125f,   0.750858245f,  1.50171551f,   2.40243682f,
        3.00189298f,    9.00721405f,   15.010998f,    24.0182111f
};

__attribute__((aligned(32)))
static int attacks[2][16];
__attribute__((aligned(32)))
static int releases[2][16];

static inline byte get_bit(dword val, byte b){ return (byte)((val >> b) & 1); }

static void CalcFilts(void){
    // TODO: make this "dirty" update ONLY if something was changed
    if(!mixing_frequency) return;

    // filtmul for each chip based on selected type
    filtmul_1 = pfloat_ConvertFromFloat(filtermult[SidChipType[0]].capacitor) / mixing_frequency;
    filtmul_2 = pfloat_ConvertFromFloat(filtermult[SidChipType[1]].capacitor) / mixing_frequency;

    for(int i=0;i<16;i++){
        attacks [Chip6581][i] = (long)((filtermult[Chip6581].adsrclk * 0x100000) / (attackTimes[i] * mixing_frequency));
        releases[Chip6581][i] = (long)((filtermult[Chip6581].adsrclk * 0x100000) / (decayReleaseTimes[i] * mixing_frequency));

        attacks [Chip8580][i] = (long)((filtermult[Chip8580].adsrclk * 0x100000) / (attackTimes[i] * mixing_frequency));
        releases[Chip8580][i] = (long)((filtermult[Chip8580].adsrclk * 0x100000) / (decayReleaseTimes[i] * mixing_frequency));
    }
}

void restartSidChipModes(void){
    //bSidPlay2SIDmode = 0;
    bDualChipMode = 1;
    printf("is this being reset???");
}

void synth_init(uint32_t mixfrq){
    mixing_frequency = (int)mixfrq;
    freqmul = (int)(15872000 / mixfrq); // your original
    CalcFilts();

    memset(sid, 0, sizeof(sid));
    memset(osc, 0, sizeof(osc));
    memset(filter, 0, sizeof(filter));

    for(int c=0;c<2;c++){
        for(int v=0;v<3;v++){
            osc[c][v].noiseval = 0xffffff;
        }
    }

    // set volume to something sane so silence isn't "muted by default"
    sid[0].ftp_vol = 15;
    sid[1].ftp_vol = 15;
}

void synth_prep_per_step(void){
    for(int chip=0; chip<2; chip++){
        for(int v=0; v<3; v++){
            osc[chip][v].pulse   = (sid[chip].v[v].pulse & 0xfff) << 16;
            osc[chip][v].filter  = get_bit(sid[chip].res_ftv, (byte)v);
            osc[chip][v].attack  = attacks[SidChipType[chip]][sid[chip].v[v].ad >> 4];
            osc[chip][v].decay   = releases[SidChipType[chip]][sid[chip].v[v].ad & 0xf];
            osc[chip][v].sustain = sid[chip].v[v].sr & 0xf0;
            osc[chip][v].release = releases[SidChipType[chip]][sid[chip].v[v].sr & 0xf];
            osc[chip][v].wave    = sid[chip].v[v].wave;
            osc[chip][v].freq    = (dword)(((long long)sid[chip].v[v].freq) * (long long)freqmul);
        }

        long filtmul = (chip==0) ? filtmul_1 : filtmul_2;

        // resonance (your simplified version)
        if(SidChipType[chip] == Chip6581){
            filter[chip].rez = pfloat_ConvertFromFloat(1.2f) - pfloat_ConvertFromFloat(0.04f) * (sid[chip].res_ftv >> 4);
            if(filter[chip].rez > 21200) filter[chip].rez = 21200;
            if(filter[chip].rez < 100)   filter[chip].rez = 100;
        } else {
            filter[chip].rez = pfloat_ConvertFromFloat(1.0f) - pfloat_ConvertFromFloat(0.04f) * (sid[chip].res_ftv >> 4);
        }
        filter[chip].rez >>= 8;

        filter[chip].freq = (int)((16L * sid[chip].ffreqhi + (sid[chip].ffreqlo & 0x7)) * filtmul);
        if(filter[chip].freq > pfloat_ConvertFromInt(1)) filter[chip].freq = pfloat_ConvertFromInt(1);

        filter[chip].l_ena = get_bit(sid[chip].ftp_vol, 4);
        filter[chip].b_ena = get_bit(sid[chip].ftp_vol, 5);
        filter[chip].h_ena = get_bit(sid[chip].ftp_vol, 6);
        filter[chip].v3ena = !get_bit(sid[chip].ftp_vol, 7);
        filter[chip].vol   = (sid[chip].ftp_vol & 0xf);
    }
}

// Your sidPoke logic, exposed as "sid_write"
static void sidPoke(int reg, unsigned char val){
    int voice = 0;
    int sidchipIndex = 0;

    if(reg >= 32){
        //if(bSidPlay2SIDmode) printf("using 2SIDS!\n");
        //bSidPlay2SIDmode = 1;

        sidchipIndex = 1;
        reg -= 32;
        if(bDualChipMode != 2) bDualChipMode = 2;
    }

    if(reg <= 6) voice = 0;
    else if(reg <= 13) voice = 1;
    else if(reg <= 20) voice = 2;

    switch(reg){
        case 0x00: case 0x07: case 0x0E:
            if(!bSidPlay2SIDmode){
                sid[0].v[voice].freq = (sid[0].v[voice].freq & 0xff00) + val;
                sid[1].v[voice].freq = (sid[1].v[voice].freq & 0xff00) + val;
            } else {
                sid[sidchipIndex].v[voice].freq = (sid[sidchipIndex].v[voice].freq & 0xff00) + val;
            }
            break;

        case 0x01: case 0x08: case 0x0F:
            if(!bSidPlay2SIDmode){
                sid[0].v[voice].freq = (sid[0].v[voice].freq & 0xff) + ((dword)val << 8);
                sid[1].v[voice].freq = (sid[1].v[voice].freq & 0xff) + ((dword)val << 8);
            } else {
                sid[sidchipIndex].v[voice].freq = (sid[sidchipIndex].v[voice].freq & 0xff) + ((dword)val << 8);
            }
            break;

        case 0x02: case 0x09: case 0x10:
            if(!bSidPlay2SIDmode){
                sid[0].v[voice].pulse = (sid[0].v[voice].pulse & 0xff00) + val;
                sid[1].v[voice].pulse = (sid[1].v[voice].pulse & 0xff00) + val;
            } else {
                sid[sidchipIndex].v[voice].pulse = (sid[sidchipIndex].v[voice].pulse & 0xff00) + val;
            }
            break;

        case 0x03: case 0x0A: case 0x11:
            if(!bSidPlay2SIDmode){
                sid[0].v[voice].pulse = (sid[0].v[voice].pulse & 0xff) + ((dword)val << 8);
                sid[1].v[voice].pulse = (sid[1].v[voice].pulse & 0xff) + ((dword)val << 8);
            } else {
                sid[sidchipIndex].v[voice].pulse = (sid[sidchipIndex].v[voice].pulse & 0xff) + ((dword)val << 8);
            }
            break;

        case 0x04: case 0x0B: case 0x12:
            if(!bSidPlay2SIDmode){
                sid[0].v[voice].wave = val;
                if((val & 0x01) == 0) osc[0][voice].envphase = 3;
                else if(osc[0][voice].envphase == 3) osc[0][voice].envphase = 0;

                sid[1].v[voice].wave = val;
                if((val & 0x01) == 0) osc[1][voice].envphase = 3;
                else if(osc[1][voice].envphase == 3) osc[1][voice].envphase = 0;
            } else {
                sid[sidchipIndex].v[voice].wave = val;
                if((val & 0x01) == 0) osc[sidchipIndex][voice].envphase = 3;
                else if(osc[sidchipIndex][voice].envphase == 3) osc[sidchipIndex][voice].envphase = 0;
            }
            break;

        case 0x05: case 0x0C: case 0x13:
            if(!bSidPlay2SIDmode){ sid[0].v[voice].ad = val; sid[1].v[voice].ad = val; }
            else sid[sidchipIndex].v[voice].ad = val;
            break;

        case 0x06: case 0x0D: case 0x14:
            if(!bSidPlay2SIDmode){ sid[0].v[voice].sr = val; sid[1].v[voice].sr = val; }
            else sid[sidchipIndex].v[voice].sr = val;
            break;

        case 0x15:
            if(!bSidPlay2SIDmode) sid[0].ffreqlo = val;
            else sid[sidchipIndex].ffreqlo = val;
            break;

        case 0x16:
            if(!bSidPlay2SIDmode) sid[0].ffreqhi = val;
            else sid[sidchipIndex].ffreqhi = val;
            break;

        case 0x17:
            if(!bSidPlay2SIDmode) sid[0].res_ftv = val;
            else sid[sidchipIndex].res_ftv = val;
            break;

        case 0x18: {
            uint8_t newv = val & 0x0F;

            if (bSidPlay2SIDmode) {
                sid[sidchipIndex].ftp_vol = val;
                vol_dac[sidchipIndex] = newv;
            } else {
                sid[0].ftp_vol = val;
                vol_dac[0] = newv;
                // IMPORTANT: you mirror SID regs to chip1 later,
                // mirror the digi nibble too (otherwise chip1 stays stale)
                vol_dac[1] = newv;
            }
            break;
        }

    }

    if(!bSidPlay2SIDmode){
        sid[1].ftp_vol = sid[0].ftp_vol;
        sid[1].res_ftv = sid[0].res_ftv;
        sid[1].ffreqlo = sid[0].ffreqlo;
        sid[1].ffreqhi = sid[0].ffreqhi;
    }
}

extern uint16_t g_sid2_base;

void sid_write(uint16_t addr, uint8_t v){
    // SID1 mirrors: D400-D41F across D400-D7FF
    if ((addr & 0xFFE0) == 0xD400) {
        sidPoke((int)(addr & 0x1F), v);
        return;
    }

    // SID2 at D420 (your chosen base)
    if ((addr & 0xFFE0) == 0xD420) {
        sidPoke(32 + (int)(addr & 0x1F), v);
        return;
    }
}

// --- Render one stereo sample (your sidMixer, but one-shot) ---
void sid_render_sample(int16_t *outL, int16_t *outR){
    // a trimmed version of your sidMixer inner loop
    int finalL = 0, finalR = 0;

    //synth_prep_per_step();
    int chiplen = 1;
    if((CHIPCONFIGS & SIDHV_CHANNEL_STEREO) || bDualChipMode) chiplen = 2;
    for(int chip=0; chip<chiplen; chip++){
        int outf = 0;
        int outo = 0;
        unsigned char tVoice = 0x7;
        if(bDualChipMode==1){
            tVoice = (unsigned char)(GetSidChipVoices((unsigned char)chip) & 0x7);
        }

        for(int v=0; v<3; v++){
            osc[chip][v].counter = (osc[chip][v].counter + osc[chip][v].freq) & 0x0FFFFFFF;

            if(osc[chip][v].wave & 0x08){
                osc[chip][v].counter = 0;
                osc[chip][v].noisepos = 0;
                osc[chip][v].noiseval = 0xffffff;
            }

            int refosc = v ? (v - 1) : 2;
            if(osc[chip][v].wave & 0x02){
                if(osc[chip][refosc].counter < osc[chip][refosc].freq){
                    // ---------------- THIS WAS CHANGED ----------------
                    // Protect against Division By Zero if master freq is 0
                    if(osc[chip][refosc].freq > 0) {
                        osc[chip][v].counter = osc[chip][refosc].counter * osc[chip][v].freq / osc[chip][refosc].freq;
                    } else {
                        osc[chip][v].counter = 0;
                    }
                    // --------------------------------------------------
                }
            }

            byte triout = (byte)(osc[chip][v].counter >> 19);
            if(osc[chip][v].counter >> 27) triout ^= 0xff;
            byte sawout = (byte)(osc[chip][v].counter >> 20);
            byte plsout = (byte)((osc[chip][v].counter > osc[chip][v].pulse) - 1);

            if(osc[chip][v].noisepos != (osc[chip][v].counter >> 24)){
                osc[chip][v].noisepos = osc[chip][v].counter >> 24;
                osc[chip][v].noiseval = (osc[chip][v].noiseval << 1) | (get_bit(osc[chip][v].noiseval, 22) ^ get_bit(osc[chip][v].noiseval, 17));
                osc[chip][v].noiseout =
                    (get_bit(osc[chip][v].noiseval,22) << 7) |
                    (get_bit(osc[chip][v].noiseval,20) << 6) |
                    (get_bit(osc[chip][v].noiseval,16) << 5) |
                    (get_bit(osc[chip][v].noiseval,13) << 4) |
                    (get_bit(osc[chip][v].noiseval,11) << 3) |
                    (get_bit(osc[chip][v].noiseval, 7) << 2) |
                    (get_bit(osc[chip][v].noiseval, 4) << 1) |
                    (get_bit(osc[chip][v].noiseval, 2) << 0);
            }
            byte nseout = osc[chip][v].noiseout;

            if(osc[chip][v].wave & 0x04){
                if(osc[chip][refosc].counter < 0x8000000) triout ^= 0xff;
            }

            byte outv = 0xff;
            if(osc[chip][v].wave & 0x10) outv &= triout;
            if(osc[chip][v].wave & 0x20) outv &= sawout;
            if(osc[chip][v].wave & 0x40) outv &= plsout;
            if(osc[chip][v].wave & 0x80) outv &= nseout;

            if(!(osc[chip][v].wave & 0x01)) osc[chip][v].envphase = 3;
            else if(osc[chip][v].envphase == 3) osc[chip][v].envphase = 0;

            switch(osc[chip][v].envphase){
            case 0:
                osc[chip][v].envval += osc[chip][v].attack;
                if(osc[chip][v].envval >= 0xFFFFFF){ osc[chip][v].envval = 0xFFFFFF; osc[chip][v].envphase = 1; }
                break;
            case 1:
                osc[chip][v].envval -= osc[chip][v].decay;
                if((signed int)osc[chip][v].envval <= (signed int)(osc[chip][v].sustain << 16)){
                    osc[chip][v].envval = (signed int)(osc[chip][v].sustain << 16);
                    osc[chip][v].envphase = 2;
                }
                break;
            case 2:
                if((signed int)osc[chip][v].envval != (signed int)(osc[chip][v].sustain << 16)) osc[chip][v].envphase = 1;
                break;
            case 3:
                osc[chip][v].envval -= osc[chip][v].release;
                if(osc[chip][v].envval < 0x40000) osc[chip][v].envval = 0x40000;
                break;
            }

            // filtered/non-filtered routing
            if((v < 2) || filter[chip].v3ena){
                long tform = 0;
                if(tVoice & (1 << v)) tform = (((int)(outv - 0x80)) * osc[chip][v].envval) >> 22;
                if(osc[chip][v].filter) outf += (int)tform;
                else outo += (int)tform;
            }
        }

        // filter step
        if(filter[chip].freq < 2000) filter[chip].freq = 2000;

        filter[chip].h = pfloat_ConvertFromInt(outf) - ((filter[chip].b >> 8) * filter[chip].rez) - filter[chip].l;
        filter[chip].b += pfloat_Multiply(filter[chip].freq, filter[chip].h);
        filter[chip].l += pfloat_Multiply(filter[chip].freq, filter[chip].b);

        outf = 0;
        if(filter[chip].l_ena) outf += pfloat_ConvertToInt(filter[chip].l);
        if(filter[chip].b_ena) outf += pfloat_ConvertToInt(filter[chip].b);
        if(filter[chip].h_ena) outf += pfloat_ConvertToInt(filter[chip].h);

        int mixed = filter[chip].vol * (outo + outf);

        /// digi-compensators ////////////////////////
        int digi = ((int)vol_dac[chip] - 8);
        mixed += digi * DIGI_GAIN;


        /////////////////////////////////////////////

        if(!(CHIPCONFIGS & SIDHV_CHANNEL_STEREO)){
            finalL += mixed;
            finalR += mixed;
        } else {
            if(chip == 0) finalL += mixed;
            if(chip == 1) finalR += mixed;
        }
    }

    // scaling/clamp to S16
    // Your original used some shifting; here we do a safe clamp.
    finalL >>= 1;
    finalR >>= 1;

    if(finalL < -32767) finalL = -32767;
    if(finalL >  32767) finalL =  32767;
    if(finalR < -32767) finalR = -32767;
    if(finalR >  32767) finalR =  32767;

    *outL = (int16_t)finalL;
    *outR = (int16_t)finalR;
}







// ===================== RSID cycle sync state =====================
// fractional accumulator: (cpu_cycles * mixing_frequency) / C64_CPU_HZ_PAL
static uint64_t g_rsid_acc = 0;
// ================================================================


// advance SID internal time by real CPU cycles (RSID path)
// This now actually ADVANCES osc/env/filter state for the number of "ticks" elapsed.
void sid_clock_cycles(uint32_t cpu_cycles){
    if(cpu_cycles == 0) return;

    // accumulate fractional "ticks"
    g_rsid_acc += (uint64_t)cpu_cycles * (uint64_t)mixing_frequency;

    // how many whole ticks elapsed?
    uint32_t ticks = (uint32_t)(g_rsid_acc / (uint64_t)C64_CPU_HZ_PAL);
    if(!ticks) return;

    // keep remainder
    g_rsid_acc -= (uint64_t)ticks * (uint64_t)C64_CPU_HZ_PAL;

    int chiplen = 1;
    if((CHIPCONFIGS & SIDHV_CHANNEL_STEREO) || bDualChipMode) chiplen = 2;

    // Run your ORIGINAL stepping logic "ticks" times, but WITHOUT producing output samples.
    while(ticks--){
        for(int chip=0; chip<chiplen; chip++){
            int outf = 0;
            int outo = 0;

            unsigned char tVoice = 0x7;
            if(bDualChipMode==1){
                tVoice = (unsigned char)(GetSidChipVoices((unsigned char)chip) & 0x7);
            }

            for(int v=0; v<3; v++){
                // --- ORIGINAL osc advance ---
                osc[chip][v].counter = (osc[chip][v].counter + osc[chip][v].freq) & 0x0FFFFFFF;

                if(osc[chip][v].wave & 0x08){
                    osc[chip][v].counter = 0;
                    osc[chip][v].noisepos = 0;
                    osc[chip][v].noiseval = 0xffffff;
                }

                int refosc = v ? (v - 1) : 2;
                if(osc[chip][v].wave & 0x02){
                    if(osc[chip][refosc].counter < osc[chip][refosc].freq){
                        // ---------------- THIS WAS CHANGED ----------------
                        if(osc[chip][refosc].freq > 0) {
                            osc[chip][v].counter = osc[chip][refosc].counter * osc[chip][v].freq / osc[chip][refosc].freq;
                        } else {
                            osc[chip][v].counter = 0;
                        }
                        // --------------------------------------------------
                    }
                }

                byte triout = (byte)(osc[chip][v].counter >> 19);
                if(osc[chip][v].counter >> 27) triout ^= 0xff;
                byte sawout = (byte)(osc[chip][v].counter >> 20);
                byte plsout = (byte)((osc[chip][v].counter > osc[chip][v].pulse) - 1);

                if(osc[chip][v].noisepos != (osc[chip][v].counter  >> 24)){
                    osc[chip][v].noisepos =  osc[chip][v].counter  >> 24;
                    osc[chip][v].noiseval = (osc[chip][v].noiseval << 1) | (get_bit(osc[chip][v].noiseval, 22) ^ get_bit(osc[chip][v].noiseval, 17));
                    osc[chip][v].noiseout =
                        (get_bit(osc[chip][v].noiseval,22) << 7) |
                        (get_bit(osc[chip][v].noiseval,20) << 6) |
                        (get_bit(osc[chip][v].noiseval,16) << 5) |
                        (get_bit(osc[chip][v].noiseval,13) << 4) |
                        (get_bit(osc[chip][v].noiseval,11) << 3) |
                        (get_bit(osc[chip][v].noiseval, 7) << 2) |
                        (get_bit(osc[chip][v].noiseval, 4) << 1) |
                        (get_bit(osc[chip][v].noiseval, 2) << 0);
                }
                byte nseout = osc[chip][v].noiseout;

                if(osc[chip][v].wave & 0x04){
                    if(osc[chip][refosc].counter < 0x8000000) triout ^= 0xff;
                }

                byte outv = 0xFF;
                if(osc[chip][v].wave & 0x10) outv &= triout;
                if(osc[chip][v].wave & 0x20) outv &= sawout;
                if(osc[chip][v].wave & 0x40) outv &= plsout;
                if(osc[chip][v].wave & 0x80) outv &= nseout;

                // --- ORIGINAL gate/env phase logic ---
                if(!(osc[chip][v].wave & 0x01)) osc[chip][v].envphase = 3;
                else if(osc[chip][v].envphase == 3) osc[chip][v].envphase = 0;

                // --- ORIGINAL envelope stepping ---
                switch(osc[chip][v].envphase){
                case 0:
                    osc[chip][v].envval += osc[chip][v].attack;
                    if(osc[chip][v].envval >= 0xFFFFFF){ osc[chip][v].envval = 0xFFFFFF; osc[chip][v].envphase = 1; }
                    break;
                case 1:
                    osc[chip][v].envval -= osc[chip][v].decay;
                    if((signed int)osc[chip][v].envval <= (signed int)(osc[chip][v].sustain << 16)){
                        osc[chip][v].envval = (signed int)(osc[chip][v].sustain << 16);
                        osc[chip][v].envphase = 2;
                    }
                    break;
                case 2:
                    if((signed int)osc[chip][v].envval != (signed int)(osc[chip][v].sustain << 16)) osc[chip][v].envphase = 1;
                    break;
                case 3:
                    osc[chip][v].envval -= osc[chip][v].release;
                    if(osc[chip][v].envval < 0x40000) osc[chip][v].envval = 0x40000;
                    break;
                }

                // --- feed filter integrator with real input, but we don’t output audio here ---
                if((v < 2) || filter[chip].v3ena){
                    long tform = 0;
                    if(tVoice & (1 << v)) tform = (((int)(outv - 0x80)) * osc[chip][v].envval) >> 22;
                    if(osc[chip][v].filter) outf += (int)tform;
                    else outo += (int)tform;
                }
            }

            // --- ORIGINAL filter integrator step (state must advance or it sounds wrong) ---
            if(filter[chip].freq < 2000) filter[chip].freq = 2000;

            filter[chip].h = pfloat_ConvertFromInt(outf) - ((filter[chip].b >> 8) * filter[chip].rez) - filter[chip].l;
            filter[chip].b += pfloat_Multiply(filter[chip].freq, filter[chip].h);
            filter[chip].l += pfloat_Multiply(filter[chip].freq, filter[chip].b);

            // NOTE: no output mixing here; this is just "advance time"
            (void)outo;
        }
    }
}


// Produce one sample from CURRENT state (no counters/envelopes/filter stepping)
// (your existing one is fine — keep it exactly like you wrote)
void sid_render_sample_noadvance(int16_t *outL, int16_t *outR){
    int finalL = 0, finalR = 0;

    int chiplen = 1;
    if((CHIPCONFIGS & SIDHV_CHANNEL_STEREO) || bDualChipMode) chiplen = 2;

    for(int chip=0; chip<chiplen; chip++){
        int outf = 0;
        int outo = 0;

        unsigned char tVoice = 0x7;
        if(bDualChipMode==1){
            tVoice = (unsigned char)(GetSidChipVoices((unsigned char)chip) & 0x7);
        }

        for(int v=0; v<3; v++){
            int refosc = v ? (v - 1) : 2;

            byte triout = (byte)(osc[chip][v].counter >> 19);
            if(osc[chip][v].counter >> 27) triout ^= 0xff;
            byte sawout = (byte)(osc[chip][v].counter >> 20);
            byte plsout = (byte)((osc[chip][v].counter > osc[chip][v].pulse) - 1);

            byte nseout = osc[chip][v].noiseout;

            if(osc[chip][v].wave & 0x04){
                if(osc[chip][refosc].counter < 0x8000000) triout ^= 0xff;
            }

            byte outv = 0xFF;
            if(osc[chip][v].wave & 0x10) outv &= triout;
            if(osc[chip][v].wave & 0x20) outv &= sawout;
            if(osc[chip][v].wave & 0x40) outv &= plsout;
            if(osc[chip][v].wave & 0x80) outv &= nseout;

            if((v < 2) || filter[chip].v3ena){
                long tform = 0;
                if(tVoice & (1 << v)) tform = (((int)(outv - 0x80)) * osc[chip][v].envval) >> 22;
                if(osc[chip][v].filter) outf += (int)tform;
                else outo += (int)tform;
            }
        }

        int fsum = 0;
        if(filter[chip].l_ena) fsum += pfloat_ConvertToInt(filter[chip].l);
        if(filter[chip].b_ena) fsum += pfloat_ConvertToInt(filter[chip].b);
        if(filter[chip].h_ena) fsum += pfloat_ConvertToInt(filter[chip].h);

        int mixed = filter[chip].vol * (outo + fsum);

        if(!(CHIPCONFIGS & SIDHV_CHANNEL_STEREO)){
            finalL += mixed;
            finalR += mixed;
        } else {
            if(chip == 0) finalL += mixed;
            if(chip == 1) finalR += mixed;
        }
    }

    finalL >>= 1;
    finalR >>= 1;

    if(finalL < -32767) finalL = -32767;
    if(finalL >  32767) finalL =  32767;
    if(finalR < -32767) finalR = -32767;
    if(finalR >  32767) finalR =  32767;

    *outL = (int16_t)finalL;
    *outR = (int16_t)finalR;
}
