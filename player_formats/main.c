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

int psid_song_uses_cia(const sid_program_t *p, uint16_t song1_based){
    uint16_t idx = (song1_based > 0) ? (song1_based - 1) : 0;
    if (idx >= 32) idx = 0; // fallback
    return ((p->speed_bits >> idx) & 1u) != 0;
}

c64_video_t sid_pick_video(const sid_program_t *p){
    uint16_t clock = (p->flags >> 2) & 0x03;
    if (clock == 2) return C64_NTSC;
    return C64_PAL; // default for 0/1/3
}


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

static void install_irq_stub(void){
    uint16_t p = 0xFF00;

    kernal_write_u8(p++, 0x48);                 // PHA
    kernal_write_u8(p++, 0x8A); kernal_write_u8(p++, 0x48); // TXA, PHA
    kernal_write_u8(p++, 0x98); kernal_write_u8(p++, 0x48); // TYA, PHA
    kernal_write_u8(p++, 0x08);                 // PHP   <<< IMPORTANT

    kernal_write_u8(p++, 0xAD); kernal_write_u8(p++, 0x19); kernal_write_u8(p++, 0xD0);
    kernal_write_u8(p++, 0x8D); kernal_write_u8(p++, 0x19); kernal_write_u8(p++, 0xD0);

    kernal_write_u8(p++, 0xAD); kernal_write_u8(p++, 0x0D); kernal_write_u8(p++, 0xDC);
    kernal_write_u8(p++, 0xAD); kernal_write_u8(p++, 0x0D); kernal_write_u8(p++, 0xDD);

    kernal_write_u8(p++, 0x6C); kernal_write_u8(p++, 0x14); kernal_write_u8(p++, 0x03);

}


static void install_nmi_stub(void){
    uint16_t p = 0xFF20;

    kernal_write_u8(p++, 0x48);
    kernal_write_u8(p++, 0x8A); kernal_write_u8(p++, 0x48);
    kernal_write_u8(p++, 0x98); kernal_write_u8(p++, 0x48);

    kernal_write_u8(p++, 0xAD); kernal_write_u8(p++, 0x0D); kernal_write_u8(p++, 0xDD);

    kernal_write_u8(p++, 0x6C); kernal_write_u8(p++, 0x18); kernal_write_u8(p++, 0x03);
}

static void install_rti_stub(void){
    uint16_t p = 0xFF40;

    kernal_write_u8(p++, 0x28); // PLP   <<< RESTORE FLAGS
    kernal_write_u8(p++, 0x68); kernal_write_u8(p++, 0xA8);
    kernal_write_u8(p++, 0x68); kernal_write_u8(p++, 0xAA);
    kernal_write_u8(p++, 0x68);
    kernal_write_u8(p++, 0x40);

}



static void install_vectors(void){
    // NMI -> $FF20
    kernal_write_u8(0xFFFA, 0x20);
    kernal_write_u8(0xFFFB, 0xFF);

    // IRQ/BRK -> $FF00
    kernal_write_u8(0xFFFE, 0x00);
    kernal_write_u8(0xFFFF, 0xFF);
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
    setvbuf(stdout, NULL, _IONBF, 0);



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
    clear_ram();    /// oh we so need to clear memory






    uint32_t length;
    //length = loadsidfromfile("../../Bionic_Commando(+).sid");
    //length = loadsidfromfile("../../Sonic_the_Hedgehog.sid");
    //length = loadsidfromfile("../../Auf_Wiedersehen_Monty.sid");  // <-- psid, plays fine ;)
    //length = loadsidfromfile("../../PayDay-Ingame_tune.sid");
    //length = loadsidfromfile("../../Euro_Trash.sid");

    //length = loadsidfromfile("../../rsid_Chimera.sid"); // <-- RSID - doesnt play :(
    //length = loadsidfromfile("../../rsid_Robox.sid"); // <-- RSID - Actually plays :O
    //length = loadsidfromfile("../../rsid_rooter.sid"); // <-- RSID - doesnt play :(
    length = loadsidfromfile("../../rsid_rorrol.sid"); // <-- RSID - plays, but does something else


    sid_program_t prg;
    int rc = sid_load_from_bytes(loadfileram, length, ram, &prg);
    if (rc != 0) { /* error */ }

    //vic_set_video(&vic, C64_NTSC );
    int video_mode = sid_pick_video(&prg);     // whatever type sid_pick_video returns (enum/int)
    vic_set_video(&vic, video_mode);


    uint16_t song = prg.start_song ? prg.start_song : 1;

    int uses_cia = psid_song_uses_cia(&prg, song);

    sid_init(&sid, cpuHz, 44100);
    if (prg.fmt == SIDFMT_PSID) {
        // init song before play :)
        cpu6502_init(&c, bus);
        cpu6502_reset_to(&c, 0x0000);   // we will call INIT via jsr, PC doesn't matter
        c.x = 0;
        c.y = 0;
        prg.start_song = 1;
        int init_cycles = cpu6502_jsr(&c, prg.init_addr, (uint8_t)((prg.start_song > 0) ? (prg.start_song - 1) : 0));


        if (prg.fmt == SIDFMT_PSID && uses_cia) {

            // Put IRQ stub in RAM at $0200 (any free RAM address works)
            const uint16_t IRQ_STUB = 0x0200;

            uint8_t stub[] = {
                0x48,             // PHA
                0x8A, 0x48,       // TXA, PHA
                0x98, 0x48,       // TYA, PHA

                0x20, 0x00, 0x00, // JSR $???? (play)  <-- patch below

                0xAD, 0x0D, 0xDC, // LDA $DC0D  (ACK CIA1 IRQ)
                0xAD, 0x0D, 0xDD,   // LDA

                0x68, 0xA8,       // PLA, TAY
                0x68, 0xAA,       // PLA, TAX
                0x68,             // PLA
                0x40              // RTI
            };

            // Patch the JSR target to prg.play_addr
            stub[6] = (uint8_t)(prg.play_addr & 0xFF);
            stub[7] = (uint8_t)(prg.play_addr >> 8);

            // Copy stub into RAM
            for (unsigned i = 0; i < sizeof(stub); i++) {
                ram[IRQ_STUB + i] = stub[i];
            }

            // Point IRQ vector ($0314/$0315) to our stub
            ram[0x0314] = (uint8_t)(IRQ_STUB & 0xFF);
            ram[0x0315] = (uint8_t)(IRQ_STUB >> 8);

            // Optional: also set BRK vector ($0316/$0317) same place
            ram[0x0316] = (uint8_t)(IRQ_STUB & 0xFF);
            ram[0x0317] = (uint8_t)(IRQ_STUB >> 8);

            ram[0xFFFE] = (uint8_t)(IRQ_STUB & 0xFF);
            ram[0xFFFF] = (uint8_t)(IRQ_STUB >> 8);

            c.p &= (uint8_t)~0x04;   // clear I flag (0x04 is typical for 6502 I bit)

            uint32_t play_hz = 50; // or 60 if NTSC
            uint16_t latch = (uint16_t)(cpuHz / play_hz);
            cia_write(&cia1, 0x04, (uint8_t)(latch & 0xFF)); // TALO
            cia_write(&cia1, 0x05, (uint8_t)(latch >> 8));   // TAHI
            cia_write(&cia1, 0x0D, 0x81);                    // enable Timer A IRQ
            cia_write(&cia1, 0x0E, 0x11);                    // load + start Timer A

            printf("CIA Driven\n");
        }



        sid_step(&sid, (uint32_t)init_cycles);  // kick start the sid chip

        printf("PSID loaded: load=$%04X init=$%04X play=$%04X songs=%u start=%u\n", prg.load_addr, prg.init_addr, prg.play_addr, prg.songs, prg.start_song);

    } else {
        // RSID: do NOT call play from host loop.

        ram[0xFFFC] = (uint8_t)(prg.init_addr & 0xFF);
        ram[0xFFFD] = (uint8_t)(prg.init_addr >> 8);

        /*
        kernal_write_u8(0xFF00, 0xAD);
        kernal_write_u8(0xFF01, 0x0D);
        kernal_write_u8(0xFF02, 0xDC);
        kernal_write_u8(0xFF03, 0x40); // RTI


        kernal_write_u8(0xFFFC, (uint8_t)(prg.init_addr & 0xFF));
        kernal_write_u8(0xFFFD, (uint8_t)(prg.init_addr >> 8));
        kernal_write_u8(0xFFFE, 0x00);
        kernal_write_u8(0xFFFF, 0xFF);   // IRQ vector = $FF00
        */





        kernal_write_u8(0xFF00, 0xAD);
        kernal_write_u8(0xFF01, 0x0D);
        kernal_write_u8(0xFF02, 0xDC);
        kernal_write_u8(0xFF03, 0x40); // RTI


        kernal_write_u8(0xFFFC, (uint8_t)(prg.init_addr & 0xFF));
        kernal_write_u8(0xFFFD, (uint8_t)(prg.init_addr >> 8));
        kernal_write_u8(0xFFFE, 0x00);
        kernal_write_u8(0xFFFF, 0xFF);   // IRQ vector = $FF00



        romloadeds();
        cpu6502_init(&c, bus);
        cpu6502_reset(&c);   // uses $FFFC


        uint8_t lo = bus_read8(NULL, 0xFFFC);
        uint8_t hi = bus_read8(NULL, 0xFFFD);
        printf("BEFORE RESET (bus) %02X %02X -> %04X\n", lo, hi, (uint16_t)(lo | (hi<<8)));

        cia_write(&cia1, 0x0D, 0x81); // enable Timer A interrupt
        cia_write(&cia1, 0x0E, 0x11); // load + start

        cia_write(&cia1, 0x04, 0xF9);   // TALO
        cia_write(&cia1, 0x05, 0x4C);   // TAHI
        cia_write(&cia1, 0x0D, 0x81);   // enable Timer A interrupt
        cia_write(&cia1, 0x0E, 0x11);   // load + start

        cia_write(&cia2, 0x0D, 0x81);   // enable Timer A interrupt

        install_irq_stub();
        install_nmi_stub();
        install_rti_stub();
        install_vectors();


        lo = bus_read8(NULL, 0xFFFC);
        hi = bus_read8(NULL, 0xFFFD);
        printf("AFTER RESET (bus) %02X %02X -> %04X\n", lo, hi, (uint16_t)(lo | (hi<<8)));

    }




    int playing = 1;
    uint32_t writes = 0;

    uint32_t play_hz = (video_mode == C64_NTSC) ? 60u : 50u;
    uint32_t play_period_cycles = sid.sid_hz / play_hz;
    uint32_t play_countdown = play_period_cycles;


    while (playing) {

        // how many CPU cycles correspond to one audio buffer worth of time?
        uint32_t cycles_target = (uint32_t)(((uint64_t)sid.sid_hz * (uint64_t)frames_per_write) / (uint64_t)rate);
        uint32_t cycles_done = 0;

        writes++;
        static uint32_t irq_hits = 0;
        int cycles;

        int play_cycles;

        while (cycles_done < cycles_target){
            if(prg.fmt == SIDFMT_PSID){ /// PSID SECTION ONLY
                if (!uses_cia) {
                    uint32_t remaining = cycles_target - cycles_done;
                    uint32_t slice = remaining;
                    if (slice > play_countdown) slice = play_countdown;
                    cycles = cpu6502_run(&c, (int)slice);
                    if (cycles <= 0) { playing = 0; break; }
                    cycles_done += (uint32_t)cycles;
                    if (play_countdown <= (uint32_t)cycles) {
                        play_countdown = play_period_cycles;
                        int play_cycles = cpu6502_jsr(&c, prg.play_addr, 0);
                        if (play_cycles <= 0) { playing = 0; break; }
                        //sid_step(&sid, (uint32_t)play_cycles);
                        cycles_done += (uint32_t)play_cycles;
                    } else {
                        play_countdown -= (uint32_t)cycles;
                    }
                }
                if(uses_cia){
                    cycles = cpu6502_run(&c, 880);
                    if (cycles <= 0) { playing = 0; break; }
                    cia_tick(&cia1, (uint32_t)cycles);
                    //cia_tick(&cia2, (uint32_t)cycles);
                    //vic_tick(&vic, (uint32_t)cycles);

                    uint8_t irq_level = (cia1.irq_level || vic.irq_line) ? 1 : 0;
                    cpu6502_irq(&c, irq_level);
                    //cpu6502_nmi(&c, cia2.irq_level ? 1 : 0);

                }


            } else {    /// RSID AREA
                cycles = cpu6502_step(&c);

                // for RSID use
                cia_tick(&cia1, (uint32_t)cycles);
                cia_tick(&cia2, (uint32_t)cycles);
                vic_tick(&vic,  (uint32_t)cycles);


                uint8_t irq_level = (cia1.irq_level || vic.irq_line) ? 1 : 0;
                cpu6502_irq(&c, irq_level);
                cpu6502_nmi(&c, cia2.irq_level ? 1 : 0);    // this internally latches




                //if(irq_level)
                //vic.irq_line = 0; // <-- added as some reason vic is not clearly the bit yet
            }


            if (cycles <= 0) { playing = 0; break; }
            cycles_done += (uint32_t)cycles;
            sid_step(&sid, (uint32_t)cycles);
        }


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




#if(0)
// IRQ routine
kernal_write_u8(0xea31, 0x4C);  // JMP w
kernal_write_u8(0xea32, 0x7e);
kernal_write_u8(0xea33, 0xea);

kernal_write_u8(0xea7e, 0x0C);  // NOPa Clear IRQ
kernal_write_u8(0xea7f, 0x0d);
kernal_write_u8(0xea80, 0xdc);
kernal_write_u8(0xea81, 0x68);  // PLAn Restore registers
kernal_write_u8(0xea82, 0xA8);  // TAYn
kernal_write_u8(0xea83, 0x68);  // PLAn
kernal_write_u8(0xea84, 0xAA);  // TAXn
kernal_write_u8(0xea85, 0x68);  // PLAn
kernal_write_u8(0xea86, 0x40);  // RTIn Return from interrupt

// Reset
kernal_write_u8(0xfce2, 0x02); // Halt

// NMI entry point
kernal_write_u8(0xfe43, 0x78);  // SEIn
kernal_write_u8(0xfe44, 0x6C);  // JMPi / Jump to NMI routine (Default: $FE47)
kernal_write_u8(0xfe45, 0x18);
kernal_write_u8(0xfe46, 0x03);

// NMI routine
kernal_write_u8(0xfe47, 0x40);  // RTIn

// IRQ entry point
kernal_write_u8(0xff48, 0x48); // PHAn // Save regs
kernal_write_u8(0xff49, 0x8A);  // TXAn
kernal_write_u8(0xff4a, 0x48);  // PHAn
kernal_write_u8(0xff4b, 0x98);  // TYAn
kernal_write_u8(0xff4c, 0x48);  // PHAn
kernal_write_u8(0xff4d, 0x6C); // JMPi Jump to IRQ routine (Default: $EA31)
kernal_write_u8(0xff4e, 0x14);
kernal_write_u8(0xff4f, 0x03);

// Hardware vectors
kernal_write_u8(0xfffa, 0x43); // NMI vector $FE43
kernal_write_u8(0xfffb, 0xfe);
kernal_write_u8(0xfffc, 0xe2); // RESET vector $FCE2
kernal_write_u8(0xfffd, 0xfc);
kernal_write_u8(0xfffe, 0x48); // IRQ/BRK vector $FF48
kernal_write_u8(0xffff, 0xff);
#endif
