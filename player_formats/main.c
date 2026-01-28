#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <alsa/asoundlib.h>


#include "sidbox/players/sidplay/cpu6502.h"
#include "sidbox/players/sidplay/sid8579.h"
#include "sidbox/players/sidplay/sidplaybus.h"
#include "sidbox/players/sidplay/playsid.h"
#include "sidbox/players/sidplay/ciairq.h"

#include "song.h"

#define PCM_DEVICE "default"


sid_t sid;



static void sleep_for_frames(int frames, int sample_rate) {
    // seconds = frames / rate
    long ns = (long)((1000000000LL * (long long)frames) / sample_rate);
    struct timespec ts = { .tv_sec = 0, .tv_nsec = ns };
    nanosleep(&ts, NULL);
}


static inline void cpu_push8(cpu6502_t *c, uint8_t v) {
    ram[0x0100u | (uint16_t)c->sp] = v;
    c->sp--;
}

static void cpu_call_sub(cpu6502_t *c, uint16_t target, uint16_t ret_addr) {
    // 6502 RTS returns to (pulled+1), and JSR pushes (PC-1).
    // For an external call, push (ret_addr - 1).
    uint16_t retm1 = (uint16_t)(ret_addr - 1);

    cpu_push8(c, (uint8_t)((retm1 >> 8) & 0xFF)); // high first
    cpu_push8(c, (uint8_t)(retm1 & 0xFF));        // then low

    c->pc = target;
}

uint8_t loadfileram[0x10000];


uint32_t loadsidfromfile(char *filename){
    FILE *f;
    uint32_t len;

    if (!filename)
        return 0;

    f = fopen(filename, "rb");
    if (!f)
        return 0;

    fseek(f, 0, SEEK_END);
    len = (uint32_t)ftell(f);
    rewind(f);

    if (len > sizeof(loadfileram))
        len = sizeof(loadfileram);

    fread(loadfileram, 1, len, f);
    fclose(f);

    return len;

}


int main() {
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 44100;
    int err;




    ///////////// cpu testing first ///////////////////////////

#if(0)
    int cputestres;
    cputestres = do_cpuTest();  // <-- this is where we were testing the CPU this part might not be needed now
    return cputestres;
#endif
   /////////////////////////////////////////////////////////////

    /* Open PCM device for playback */
    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(err));
        return 1;
    }

    /* Set hardware parameters */
    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 2);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);
    snd_pcm_hw_params_free(params);

    /* Prepare buffer and generate PWM (SID-style) audio */
    int16_t buffer[4096];                         /* stereo interleaved frames */
    int frames_per_write = sizeof(buffer) / 4;    /* 4 bytes per frame: L+R */


    //// OUR SID PLAY engine hoist ////////////////////////

    cpu6502_t c;
    cpu6502_bus_t bus = {0};

    bus.user = NULL;
    bus.read8 = bus_read8;
    bus.write8 = bus_write8;
    cpu6502_init(&c, bus);


    clear_ram();    /// oh we so need to clear memory


    uint32_t length;
    //int rc = psid_load_from_bytes(Wiklund___Cheese, Wiklund___Cheese_len, ram, &ps);
    //int rc = psid_load_from_bytes(turrican, turrican_len, ram, &ps);

    //length = loadsidfromfile("../../Bionic_Commando(+).sid");
    //length = loadsidfromfile("../../Auf_Wiedersehen_Monty.sid");  // <-- psid, plays fine ;)


    length = loadsidfromfile("../../rsid_Chimera.sid"); // <-- RSID - doesnt play :(
    //length = loadsidfromfile("../../rsid_Robox.sid"); // <-- RSID - Actually plays :O
    //length = loadsidfromfile("../../rsid_rooter.sid"); // <-- RSID - doesnt play :(


    sid_program_t prg;
    int rc = sid_load_from_bytes(loadfileram, length, ram, &prg);
    if (rc != 0) { /* error */ }


    sid_init(&sid, 985248, 44100);
    if (prg.fmt == SIDFMT_PSID) {
        // init song before play :)

        cpu6502_reset_to(&c, 0x0000);   // we will call INIT via jsr, PC doesn't matter
        c.x = 0;
        c.y = 0;
        //ps.start_song = 5;
        int init_cycles = cpu6502_jsr(&c, prg.init_addr, (uint8_t)((prg.start_song > 0) ? (prg.start_song - 1) : 0));
        sid_step(&sid, (uint32_t)init_cycles);  // kick start the sid chip

        printf("PSID loaded: load=$%04X init=$%04X play=$%04X songs=%u start=%u\n", prg.load_addr, prg.init_addr, prg.play_addr, prg.songs, prg.start_song);

    } else {
        // RSID: do NOT call play from host loop.

        ram[0xFFFC] = (uint8_t)(prg.init_addr & 0xFF);
        ram[0xFFFD] = (uint8_t)(prg.init_addr >> 8);

        kernal_write_u8(0xFF00, 0xAD);
        kernal_write_u8(0xFF01, 0x0D);
        kernal_write_u8(0xFF02, 0xDC);
        kernal_write_u8(0xFF03, 0x40); // RTI


        kernal_write_u8(0xFFFC, (uint8_t)(prg.init_addr & 0xFF));
        kernal_write_u8(0xFFFD, (uint8_t)(prg.init_addr >> 8));
        kernal_write_u8(0xFFFE, 0x00);
        kernal_write_u8(0xFFFF, 0xFF);   // IRQ vector = $FF00

        uint8_t lo = bus_read8(NULL, 0xFFFC);
        uint8_t hi = bus_read8(NULL, 0xFFFD);


        printf("RESET (bus) %02X %02X -> %04X\n", lo, hi, (uint16_t)(lo | (hi<<8)));
        cia_write(&cia1, 0x0D, 0x81); // enable Timer A interrupt
        cia_write(&cia1, 0x0E, 0x11); // load + start

        cia_write(&cia1, 0x04, 0xF9);   // TALO
        cia_write(&cia1, 0x05, 0x4C);   // TAHI
        cia_write(&cia1, 0x0D, 0x81);   // enable Timer A interrupt
        cia_write(&cia1, 0x0E, 0x11);   // load + start


        romloadeds();
        cpu6502_reset(&c);   // uses $FFFC

    }




    int playing = 1;
    uint32_t writes = 0;

    uint32_t play_period_cycles = sid.sid_hz / 50; // ~19704 for PAL 985248
    uint32_t play_countdown = play_period_cycles;

    while (playing) {

        // how many CPU cycles correspond to one audio buffer worth of time?
        uint32_t cycles_target = (uint32_t)(((uint64_t)sid.sid_hz * (uint64_t)frames_per_write) / (uint64_t)rate);
        uint32_t cycles_done = 0;

        writes++;
        static uint32_t irq_hits = 0;

        while (cycles_done < cycles_target) {

            int cycles = cpu6502_step(&c);
            if (cycles <= 0) { playing = 0; break; }
            cycles_done += (uint32_t)cycles;


            if(prg.fmt == SIDFMT_PSID){
                // 50Hz play call
                if (play_countdown <= (uint32_t)cycles) {
                    play_countdown += play_period_cycles;
                    int play_cycles = cpu6502_jsr(&c, prg.play_addr, 0);
                } else {
                    play_countdown -= (uint32_t)cycles;
                }
            } else {
                // for RSID use
                cia_tick(&cia1, (uint32_t)cycles);
                cia_tick(&cia2, (uint32_t)cycles);

                // CIA1 drives IRQ on a real C64
                vic_tick(&vic, (uint32_t)cycles);

                // combine IRQ sources (CIA + VIC)
                static uint8_t irq_prev = 0;
                uint8_t irq_level = (cia1.irq_level || vic.irq_line) ? 1 : 0;
                cpu6502_irq(&c, irq_level);
                irq_prev = irq_level;

            }

            sid_step(&sid, (uint32_t)cycles);
        }
        /*
        if ((writes % 150) == 0) {
            printf("irq_level=%d irq_hits=%u\n", cia1.irq_level, irq_hits);
            fflush(stdout);
        }
    */

        int frames = sid_render_interleaved(&sid, buffer, frames_per_write);
        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);


        if (written == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
        } else if (written < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
            break;
        }
    }

//*/

    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("done\n");

    return 0;
}

#if(0)

/* SID-like PWM parameters */
double freq = 440.0;     /* A4 tone */
double lfo_rate = 2.0;   /* 2 Hz duty modulation */
double lfo_depth = 0.35; /* pulse modulation depth */
double base_duty = 0.5;  /* 50% square wave */
double amp = 1600.0;

/* Phase accumulators */
double phase = 0.0;
double lfo_phase = 0.0;
double phase_inc = freq / (double)rate;
double lfo_inc  = lfo_rate / (double)rate;

/* Playback duration */
int total_writes = 40; /* around 4–5 seconds */

for (int w = 0; w < total_writes; ++w) {

    for (int f = 0; f < frames_per_write; ++f) {

        /* LFO (0..1) */
        double lfo = (sin(2.0 * M_PI * lfo_phase) * 0.5) + 0.5;

        /* Modulated duty */
        double duty = base_duty + ((lfo - 0.5) * 2.0 * lfo_depth);
        if (duty < 0.01) duty = 0.01;
        if (duty > 0.99) duty = 0.99;

        /* Pulse wave: +1 or -1 */
        double sample = (phase < duty) ? 1.0 : -1.0;

        /* Soft shaping to reduce harshness */
        sample *= (1.0 - 0.2 * fabs(sample));

        int16_t out = (int16_t)(sample * amp);

        buffer[f * 2 + 0] = out; /* Left */
        buffer[f * 2 + 1] = out; /* Right */

        /* Advance phases */
        phase += phase_inc;
        if (phase >= 1.0) phase -= 1.0;

        lfo_phase += lfo_inc;
        if (lfo_phase >= 1.0) lfo_phase -= 1.0;
    }

    snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames_per_write);

    if (written == -EPIPE) {
        snd_pcm_prepare(pcm_handle);
    }
    else if (written < 0) {
        fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
    }
}
#endif
