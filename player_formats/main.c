#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
//#include <string.h>
//#include <math.h>
//#include <time.h>
#include <alsa/asoundlib.h>


#include "sidbox/players/sidplay/cpu6502.h"
#include "sidbox/players/sidplay/sid8579.h"

#include "sidbox/players/sidplay/bus.h"
#include "sidbox/players/sidplay/playsid.h"


#define PCM_DEVICE "default"


static const uint8_t prog_hello[] = {
    0xA2, 0x00,             // LDX #$00
    // loop:
    0xBD, 0x10, 0x08,       // LDA msg,X
    0xF0, 0x07,             // BEQ done
    0x8D, 0xD2, 0xFF,       // STA $FFD2
    0xE8,                   // INX
    0x4C, 0x02, 0x08,       // JMP loop
    // done:
    0x4C, 0x00, 0x08,       // JMP $0800
    // msg at $0810:
    'H','E','L','L','O',' ','6','5','0','2','\n',0
};

static const uint8_t prog_vic_irq[] = {
    // --- $0800 MAIN ---
    0x78,                   // SEI
    0xA9, 0x00,             // LDA #$00
    0x85, 0x02,             // STA $02        ; counter = 0

    // raster compare = 100
    0xA9, 0x64,             // LDA #$64
    0x8D, 0x12, 0xD0,       // STA $D012      ; raster low
    0xAD, 0x11, 0xD0,       // LDA $D011
    0x29, 0x7F,             // AND #$7F       ; clear raster high bit latch
    0x8D, 0x11, 0xD0,       // STA $D011

    // clear pending raster IRQ
    0xA9, 0x01,             // LDA #$01
    0x8D, 0x19, 0xD0,       // STA $D019      ; W1C bit0

    // enable raster IRQ
    0xA9, 0x01,             // LDA #$01
    0x8D, 0x1A, 0xD0,       // STA $D01A

    // (optional extra safety ack right before CLI)
    0xA9, 0x01,             // LDA #$01
    0x8D, 0x19, 0xD0,       // STA $D019

    0x58,                   // CLI

    // spin forever at $081D
    0x4C, 0x1D, 0x08,       // JMP $081D

    // --- pad from $0820 .. $08FF (0xE0 bytes) ---
    // 224 NOPs (0xEA)
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,

    // --- $0900 IRQ HANDLER ---
    0x48,                   // PHA
    0x8A, 0x48,             // TXA / PHA
    0x98, 0x48,             // TYA / PHA

    0xE6, 0x02,             // INC $02
    0xA5, 0x02,             // LDA $02
    0xC9, 0x32,             // CMP #$32       ; 50 interrupts ~= 1 second at 50Hz
    0xD0, 0x0C,             // BNE skip_print

    0xA9, 0x00,             // LDA #$00
    0x85, 0x02,             // STA $02

    0xA9, '*',             // LDA #'*'
    0x8D, 0xF0, 0xD7,       // STA $D7F0
    0xA9, '_',             // LDA #'\n'
    0x8D, 0xF0, 0xD7,       // STA $D7F0

    // skip_print:
    0xA9, 0x01,             // LDA #$01
    0x8D, 0x19, 0xD0,       // STA $D019      ; ACK raster IRQ

    0x68, 0xA8,             // PLA / TAY
    0x68, 0xAA,             // PLA / TAX
    0x68,                   // PLA
    0x40                    // RTI
};

uint8_t playmode_sidtype = SIDPLAY_PLAYMODE_PSID;   // 0 = PSID, 1 = RSID, 2 = some crazy thing i dunno yet














static int run_cmd_capture(const char *cmd, char *out, size_t out_sz){
    FILE *p = popen(cmd, "r");
    if(!p) return 0;

    size_t n = fread(out, 1, out_sz - 1, p);
    out[n] = 0;

    int rc = pclose(p);
    if(rc != 0) return 0;

    // trim trailing newline
    while(n > 0 && (out[n-1] == '\n' || out[n-1] == '\r')) out[--n] = 0;
    return (n > 0);
}

int linux_open_file_dialog(char *path_out, size_t path_out_sz){
    // You can add filters like: --filter "SID files (*.sid)" --filter "All files (*)"
    const char *cmd = "kdialog --getopenfilename /mnt/LinuxDatas/work/sidbox-softwares/player_formats/sid_tunes \"*.sid|SID files (*.sid)\" \"*|All files\"";
    return run_cmd_capture(cmd, path_out, path_out_sz);
}


int main(){

    char sidfilename[256];
    uint32_t length;



    snd_pcm_t *pcm_handle;
    snd_pcm_sw_params_t *sw;
    snd_pcm_hw_params_t *params;
    unsigned int rate = AUDIO_MIX_FREQ;
    int err;


    setvbuf(stdout, NULL, _IONBF, 0);

    snd_pcm_hw_params_malloc(&params);



    /* Open PCM device for playback */
    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(err));
        return 1;
    }
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 2);

    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params(pcm_handle, params);
    snd_pcm_hw_params_free(params);

    snd_pcm_uframes_t period = 128;
    snd_pcm_uframes_t bufferv = period * 2;


    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &period, 0);
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &bufferv);

    /* Set hardware parameters */

    snd_pcm_sw_params_alloca(&sw);
    // SW params: start quickly, wake us quickly
    snd_pcm_sw_params_current(pcm_handle, sw);

    // Start as soon as we have 1 period queued (or even 1 frame)
    snd_pcm_sw_params_set_start_threshold(pcm_handle, sw, period);

    // Wake up when at least 1 period is free
    snd_pcm_sw_params_set_avail_min(pcm_handle, sw, period);

    // Apply SW params
    err = snd_pcm_sw_params(pcm_handle, sw);
    if (err < 0) {
        fprintf(stderr, "snd_pcm_sw_params failed: %s\n", snd_strerror(err));
        return -1;
    }


    playmode_sidtype = SIDPLAY_PLAYMODE_PSID;
    playmode_sidtype = SIDPLAY_PLAYMODE_RSID;

    /////////////////////////// TEST ////////////////////////////////////////////////////////////
    if(playmode_sidtype == SIDPLAY_PLAYMODE_PSID){  // TEST AREA //
        sprintf(sidfilename, "../../Auf_Wiedersehen_Monty.sid");
        //sprintf(sidfilename, "../../Sonic_the_Hedgehog.sid");
        //sprintf(sidfilename, "../../PayDay-Ingame_tune.sid");
        //sprintf(sidfilename, "../../Euro_Trash.sid");

        if(!PlaySID_Init(sidfilename, 0)){
            printf("Failed to init SID: %s\n", sidfilename);
            snd_pcm_close(pcm_handle);
            return 2;
        }
    }














    if(playmode_sidtype == SIDPLAY_PLAYMODE_RSID){  // TEST AREA //
        //sprintf(sidfilename, "../../rsid_Chimera.sid"); // <-- RSID - plays now :)
        //sprintf(sidfilename, "../../rsid_Robox.sid");   // <-- RSID - Actually plays :O
        //sprintf(sidfilename, "../../rsid_rooter.sid");  // <-- RSID - it PLAYS but only if we choose song 2, never works on song 1 (tho it is there it worked on a buggy RSID play) lol :)
        //sprintf(sidfilename, "../../rsid_rorrol.sid");    // <-- RSID - plays now :)
        //sprintf(sidfilename, "../../rsid_MARRS_Mix.sid");     // <-- ALSO PLAYS!! YEY
        //sprintf(sidfilename, "../../rsid_Flippy.sid");    // THIS PLAYS TOO! WHOOHOO
        //sprintf(sidfilename, "../../rsid_Freak_Out.sid");
        //sprintf(sidfilename, "../../rsid_Digi.sid");
        //sprintf(sidfilename, "../../rsid_One_on_One_Jordan_vs_Bird.sid");
        //sprintf(sidfilename, "../../rsid_Mega_Apocalypse.sid");
        //sprintf(sidfilename, "../../rsid_Great_Giana_Sisters.sid");
        //sprintf(sidfilename, "../../rsid_RoboCop.sid");
        //sprintf(sidfilename, "../../rsid_RoboCop.sid");
        //sprintf(sidfilename, "../../rsid_Having_Sex.sid");
        sprintf(sidfilename, "../../rsid_Jevers_Bannys_and_the_Master_Mixers.sid");


        char path[4096];
        if (linux_open_file_dialog(path, sizeof(path))) {
            printf("Picked: %s\n", path);
            sprintf(sidfilename, path);
        } else {
            printf("Cancelled or failed\n");
            return 0;
        }














        PlaySID_InitRSID(sidfilename);


        /*
            // flimsy song select, doesnt work though but its okay
        uint8_t s = (uint8_t)(3 + 1);

        bus_write8(0x0002, s);     // you already touch $02, some code might read it
        bus_write8(0x0030, s);     // random-ish zp slot some players use
        bus_write8(0x033C, s);     // kernal workspace-ish
        bus_write8(0x033D, 0x00);  // sometimes hi byte / flags
        */

        //PlaySID_LoadProgramRAW(prog_vic_irq, sizeof(prog_vic_irq), 0x0800, 0x0800, 0x0900, 0x0800); //
    }


    /////////////////////////// [END TEST] ////////////////////////////////////////////////////////
    /* Prepare buffer and generate PWM (SID-style) audio */
    int16_t buffer[4096];                         /* stereo interleaved frames */
    //playsid_start_tune(3);



    while(1){   // this will keep looing only because later i'll add stuff to control this later
        // will play the routine here
        // example

        // inside the doPlaySidStep() this does the steps you'll find in the playsid.c and return how many samples, enough to fill the 4096 buffer here
        // change this if you need  but
        //length = doPlaySidStep(buffer);// <-- do what you need here to make this work interleaved buffer output for now

        uint32_t frames = 880;

        // ## FOR PSID PLAY ONLY call THIS and loop  #######################################################
        if(playmode_sidtype == SIDPLAY_PLAYMODE_PSID){
            doPlaySidStep(buffer, frames);    // <-- used in PSID playback
        }
        // ## END PSID ONLY SECTION -- commented out for testing RSID/Prog for now

        //-------------------------------------------------------------------------------------------------

        // ## FOR RSID PLAY ONLY (or programs) call THIS and loop #########################################
        // LIST OF TO DOs:
        // todo-later :LATER: cyc_acc = 0; for now we wont bother this just a reminder
        /////////////////
        if(playmode_sidtype == SIDPLAY_PLAYMODE_RSID){
            doRSIDStep(buffer, frames, AUDIO_MIX_FREQ);
        }

        // ## FOR RSID PLAY ONLY (or programs) call THIS and loop #########################################
        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);
        if(written == -EPIPE){
            snd_pcm_prepare(pcm_handle);
        } else if(written < 0){
            fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
            break;
        }

    }



    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("done\n");


    return(42);

}
