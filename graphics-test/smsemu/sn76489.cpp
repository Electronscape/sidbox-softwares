// this code is all based on the fantastic docs linked below
// SOURCE: https://www.smspower.org/uploads/Development/richard.txt
// SOURCE: https://www.smspower.org/uploads/Development/SN76489-20030421.txt

#include "internal.h"
#include <string.h>

unsigned char bSEGAConfigs;
float bSEGAPWMer = 0.00003f;// is redudent

//#define bSEGAPWMer ((uint16_t)(0.005f * 256)) // about 1.28

#define Q15_ONE     32767       // 1.0 in Q1.15
#define Q15_MAX     32382       // 0.99 in Q1.15
#define Q15_MIN     4915        // 0.15 in Q1.15
#define Q15_DELTA   1           // 0.00003 in Q1.15


#define APU sms->apu

enum {
    LATCH_TYPE_TONE = 0, LATCH_TYPE_NOISE = 0, LATCH_TYPE_VOL = 1,

    WHITE_NOISE = 1, PERIDOIC_NOISE = 0,

    // this depends on the system.
    // this should be a var instead, but ill add that when needed!
    TAPPED_BITS = 0x0009,

    LFSR_RESET_VALUE = 0x8000,
};

static inline void latch_reg_write(struct SMS_Core *sms, uint8_t value) {
    APU.latched_channel = (value >> 5) & 0x3;
    APU.latched_type = (value >> 4) & 0x1;

    const uint8_t data = value & 0xF;

    if (APU.latched_type == LATCH_TYPE_VOL) {
        APU.volume[APU.latched_channel] = data & 0xF;
    } else {
        switch (APU.latched_channel) {
        case 0:
        case 1:
        case 2:
            APU.tone[APU.latched_channel].tone &= 0x3F0;
            APU.tone[APU.latched_channel].tone |= data;
            break;

        case 3:
            APU.noise.lfsr = LFSR_RESET_VALUE;
            APU.noise.shift_rate = data & 0x3;
            APU.noise.mode = (data >> 2) & 0x1;
            break;
        }
    }
}

static inline void data_reg_write(struct SMS_Core *sms, uint8_t value) {
    const uint8_t data = value & 0x3F;

    if (APU.latched_type == LATCH_TYPE_VOL) {
        APU.volume[APU.latched_channel] = data & 0xF;
    } else {
        switch (APU.latched_channel) {
        case 0:
        case 1:
        case 2:
            APU.tone[APU.latched_channel].tone &= 0xF;
            APU.tone[APU.latched_channel].tone |= data << 4;
            break;

        case 3:
            APU.noise.lfsr = LFSR_RESET_VALUE;
            APU.noise.shift_rate = data & 0x3;
            APU.noise.mode = (data >> 2) & 0x1;
            break;
        }
    }
}

void SN76489_reg_write(struct SMS_Core *sms, uint8_t value) {
    // if MSB is set, then this is a latched write,
    // else, its a normal data write
    if (value & 0x80) {
        latch_reg_write(sms, value);
    } else {
        data_reg_write(sms, value);
    }
}




static inline void SN76489_tick_tone(struct SMS_Core *sms, uint8_t index, uint8_t cycles) {
    if (APU.better_sid) {
        uint16_t pwm = APU.tone[index].pwm;     // Q1.15 fixed-point
        uint8_t pwmbit = APU.tone[index].pwmbit;

        // PWM envelope logic
        if (pwmbit) {
            pwm += Q15_DELTA;
            if (pwm > Q15_MAX) {
                pwm = Q15_ONE;
                pwmbit = 0;
            }
        } else {
            pwm -= Q15_DELTA;
            if (pwm < Q15_MIN) {
                pwm = Q15_MIN;
                pwmbit = 1;
            }
        }

        APU.tone[index].pwm = pwm;
        APU.tone[index].pwmbit = pwmbit;

        int counter = APU.tone[index].counter;
        int tone = APU.tone[index].tone;

        if (counter > 0 || tone > 0) {
            counter -= cycles;
            if (counter <= 0) {
                counter += tone * 32;
            }
            APU.tone[index].counter = counter;

            // threshold = (tone * 16 * pwm) >> 15
            int threshold = (tone << 4) * pwm >> 15;
            APU.polarity[index] = (counter < threshold) ? -1 : 1;
        }
    } else {
        int counter = APU.tone[index].counter;
        int tone = APU.tone[index].tone;
        if (counter > 0 || tone > 0) {
            counter -= cycles;
            if (counter <= 0) {
                counter += tone << 4; // tone * 16
                if (tone != 1) {
                    APU.polarity[index] *= -1;
                }
            }
            APU.tone[index].counter = counter;
        }
    }
}

static inline void SN76489_tick_tone2(struct SMS_Core *sms, uint8_t index, uint8_t cycles) {
    // we don't want to keep change the polarity if the counter is already zero,
    // especially if the volume is still not off!
    // otherwise this will cause a hi-pitch screech, can be heard in golden-axe
    // to fix this, i check if the counter > 0 || if we have a value to reload
    // the counter with.
    if (APU.better_sid) {
        if(APU.tone[index].pwmbit){	// increasing
            APU.tone[index].pwm += bSEGAPWMer;
            if (APU.tone[index].pwm > 0.99f){
                APU.tone[index].pwm = 1.00f;
                APU.tone[index].pwmbit=0;
            }
        } else {
            APU.tone[index].pwm -= bSEGAPWMer;
            if (APU.tone[index].pwm < 0.15f){
                APU.tone[index].pwm = 0.15f;
                APU.tone[index].pwmbit=1;
            }
        }

        if (APU.tone[index].counter > 0 || APU.tone[index].tone > 0) {
            APU.tone[index].counter -= cycles;
            if (APU.tone[index].counter <= 0) {	// || APU.tone[index].pwm) {
                APU.tone[index].counter += APU.tone[index].tone * 32;
            }

            if (APU.tone[index].counter < ((APU.tone[index].tone * 16) * APU.tone[index].pwm)) {
                APU.polarity[index] = -1;
            } else
                APU.polarity[index] = 1;

        }
    } else

    if (APU.tone[index].counter > 0 || APU.tone[index].tone > 0) {
        APU.tone[index].counter -= cycles;

        if (APU.tone[index].counter <= 0) {
            // the apu runs x16 slower than cpu!
            APU.tone[index].counter += APU.tone[index].tone * 16;
            /*
                from the docs:

                Sample playback makes use of a feature of the SN76489's tone generators:
                when the half-wavelength (tone value) is set to 1, they output a DC offset
                value corresponding to the volume level (i.e. the wave does not flip-flop).
                By rapidly manipulating the volume, a crude form of PCM is obtained.
            */
            // this effect is used by the sega intro in Tail's Adventure and
            // sonic tripple trouble.
            if (APU.tone[index].tone != 1) {
                // change the polarity
                APU.polarity[index] *= -1;
            }
        }
    }
}

static inline void SN76489_tick_noise(struct SMS_Core *sms, uint8_t cycles) {
    APU.noise.counter -= cycles;
    if (APU.noise.counter <= 0) {
        const uint8_t multi = APU.noise.better_drums ? 1 : 16;
        uint16_t reload = 0;

        switch (APU.noise.shift_rate) {
            case 0x0: reload = 1 * 16 * multi; break;
            case 0x1: reload = 2 * 16 * multi; break;
            case 0x2: reload = 4 * 16 * multi; break;
            case 0x3: reload = APU.tone[2].tone * 16; break;
        }
        APU.noise.counter += reload;
        APU.noise.flip_flop ^= 1;
        if (APU.noise.flip_flop) {
            APU.polarity[3] = (APU.noise.lfsr & 0x1) ? +1 : -1;
            uint16_t lfsr = APU.noise.lfsr;
            if (APU.noise.mode == WHITE_NOISE) {
                lfsr = (lfsr >> 1) | (SMS_parity16(lfsr & TAPPED_BITS) << 15);
            } else {
                lfsr = (lfsr >> 1) | ((lfsr & 0x1) << 15);
            }
            APU.noise.lfsr = lfsr;
        }
    }
}

static inline void SN76489_tick_noise_old(struct SMS_Core *sms, uint8_t cycles) {
    APU.noise.counter -= cycles;

    if (APU.noise.counter <= 0) {
        // the drums sound much better in basically every game if
        // the timer is 16, 32, 64.
        // however, the correct sound is at 256, 512, 1024.
        const uint8_t multi = APU.noise.better_drums ? 1 : 16;

        // the apu runs x16 slower than cpu!
        switch (APU.noise.shift_rate) {
            case 0x0: APU.noise.counter += 1 * 16 * multi; break;
            case 0x1: APU.noise.counter += 2 * 16 * multi; break;
            case 0x2: APU.noise.counter += 4 * 16 * multi; break;
            case 0x3: APU.noise.counter = APU.tone[2].tone * 16; break;
        }

        // change the polarity
        APU.noise.flip_flop = !APU.noise.flip_flop;

        // the nose is clocked every 2 countdowns!
        if (APU.noise.flip_flop == true) {
            // this is the bit used for the mixer
            APU.polarity[3] = (APU.noise.lfsr & 0x1) ? +1 : -1;

            if (APU.noise.mode == WHITE_NOISE) {
                APU.noise.lfsr = (APU.noise.lfsr >> 1) | (SMS_parity16(APU.noise.lfsr & TAPPED_BITS) << 15);
            } else {
                APU.noise.lfsr = (APU.noise.lfsr >> 1) | ((APU.noise.lfsr & 0x1) << 15);
            }
        }
    }
}

static inline int8_t SN76489_sample_channel(struct SMS_Core *sms, uint8_t index) {
    // the volume is inverted, so that 0xF = OFF, 0x0 = MAX
    static const int8_t VOLUME_INVERT_TABLE[0x10] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0 };
    return APU.polarity[index] * VOLUME_INVERT_TABLE[APU.volume[index]];
}

/*
static void SN76489_sample(struct SMS_Core* sms)
{
const int8_t tone0 = SN76489_sample_channel(sms, 0);
const int8_t tone1 = SN76489_sample_channel(sms, 1);
const int8_t tone2 = SN76489_sample_channel(sms, 2);
const int8_t noise = SN76489_sample_channel(sms, 3);

struct SMS_ApuCallbackData data =
{
.tone0 = { tone0 * APU.tone0_left, tone0 * APU.tone0_right },
.tone1 = { tone1 * APU.tone1_left, tone1 * APU.tone1_right },
.tone2 = { tone2 * APU.tone2_left, tone2 * APU.tone2_right },
.noise = { noise * APU.noise_left, noise * APU.noise_right },
};

sms->apu_callback(sms->apu_callback_user, &data);
}
 */

#ifndef __linux__
#include "snd_eq.h"
extern volatile EQSTATE eqleft;
extern volatile EQSTATE eqright;
#endif

volatile char audioMode1 = 0;

void audioMode(char mode){
    audioMode1 = mode;
}


void SN76489_rend(struct SMS_Core *sms, int8_t *tonesL, int8_t *tonesR) {
    const int8_t tone0 = SN76489_sample_channel(sms, 0);
    const int8_t tone1 = SN76489_sample_channel(sms, 1);
    const int8_t tone2 = SN76489_sample_channel(sms, 2);
    const int8_t noise = SN76489_sample_channel(sms, 3);

    struct SMS_ApuCallbackData data = {
        .tone0 = { (int8_t)(tone0 * APU.tone0_left), (int8_t)(tone0 * APU.tone0_right) },
        .tone1 = { (int8_t)(tone1 * APU.tone1_left), (int8_t)(tone1 * APU.tone1_right) },
        .tone2 = { (int8_t)(tone2 * APU.tone2_left), (int8_t)(tone2 * APU.tone2_right) },
        .noise = { (int8_t)(noise * APU.noise_left), (int8_t)(noise * APU.noise_right) }
    };

    //*tonesL = (data.tone0[0] + data.tone1[0] + data.tone2[0] + data.noise[0]);
    //*tonesR = (data.tone0[1] + data.tone1[1] + data.tone2[1] + data.noise[1]);

    if (audioMode1 == 1) {
#ifndef __linux__
        signed short ail, air;
        signed short ar=0, al=0;
        ail = (data.tone0[0] + data.tone1[0] + data.tone2[0] + data.noise[0]);
        air = (data.tone0[1] + data.tone1[1] + data.tone2[1] + data.noise[1]);

        air <<= 4;
        ail <<= 4;


        // ####### TRI-BANDING EQUALISER ###############################################
        //if (!eq3bandoverride)
        {	// we might need to over ride this, for TAP/TZX files
            al = (ail); // off set to dc 0 allowing +/- values
            al = do_3band(&eqleft, al);
            //*al = aud + 2048;

            ar = (air); // off set to dc 0 allowing +/- values
            ar = do_3band(&eqright, ar);
            //*al = aud + 2048;
        }

        *tonesL = (al >> 4);
        *tonesR = (ar >> 4); //(data.tone0[1] + data.tone1[1] + data.tone2[1] + data.noise[1]);
#else
        *tonesL = (data.tone0[0] + data.tone1[0] + data.tone2[0] + data.noise[0]);
        *tonesR = (data.tone0[1] + data.tone1[1] + data.tone2[1] + data.noise[1]);
#endif
    } else {
        *tonesL = (data.tone0[0] + data.tone1[0] + data.tone2[0] + data.noise[0]);
        *tonesR = (data.tone0[1] + data.tone1[1] + data.tone2[1] + data.noise[1]);
    }
    //sms->apu_callback(sms->apu_callback_user, &data);
}

void SN76489_run(struct SMS_Core *sms, uint8_t cycles) {
    SN76489_tick_tone(sms, 0, cycles);
    SN76489_tick_tone(sms, 1, cycles);
    SN76489_tick_tone(sms, 2, cycles);
    SN76489_tick_noise(sms, cycles);
    /*

     if (sms->apu_callback)
     {
     sms->apu_callback_counter -= cycles;

     if (sms->apu_callback_counter <= 0)
     {
     sms->apu_callback_counter += (CPU_CLOCK / sms->apu_callback_freq);
     //SN76489_sample(sms);
     }
     }
     */
}

void SN76489_init(struct SMS_Core *sms) {
    memset(APU.polarity, 1, sizeof(APU.polarity));
    memset(APU.volume, 0xF, sizeof(APU.volume));

    APU.noise.lfsr = LFSR_RESET_VALUE;
    APU.noise.mode = 0;
    APU.noise.shift_rate = 0;
    APU.noise.flip_flop = false;
    APU.noise.better_drums = false;

    APU.latched_channel = 0;

    // by default, all channels are enabled in GG mode.
    // as sms is mono, these values will not be changed elsewhere
    // (so always enabled!).
    APU.tone0_left = 1;
    APU.tone1_left = 1;
    APU.tone2_left = 1;
    APU.noise_left = 1;

    APU.tone0_right = 1;
    APU.tone1_right = 1;
    APU.tone2_right = 1;
    APU.noise_right = 1;
}
