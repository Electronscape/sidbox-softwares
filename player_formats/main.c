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

//////////////// SYSTEM FILE REQUESTER /////////////////////////////////
/// \brief prog_hello


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>


static char g_last_dir[PATH_MAX] =
    "/mnt/LinuxDatas/work/sidbox-softwares/player_formats/sid_tunes";

static void strip_newlines(char *s){
    if (!s) return;
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = 0;
}

static void path_dirname_inplace(char *path){
    if (!path || !*path) return;
    strip_newlines(path);
    char *slash = strrchr(path, '/');
    if (!slash) return;
    if (slash == path) path[1] = 0;  // keep "/"
    else *slash = 0;
}

static void ensure_dir_exists(const char *dir){
    if (!dir || !*dir) return;
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", dir);
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static int get_config_paths(char *out_dir, size_t out_dir_sz, char *out_file, size_t out_file_sz){
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home = getenv("HOME");
    if (xdg && *xdg) {
        snprintf(out_dir,  out_dir_sz,  "%s/sidbox_player", xdg);
    } else if (home && *home) {
        snprintf(out_dir,  out_dir_sz,  "%s/.config/sidbox_player", home);
    } else {
        return 0;
    }
    snprintf(out_file, out_file_sz, "%s/lastdir.txt", out_dir);
    return 1;
}

void rsid_lastdir_load(void){
    char dir[PATH_MAX], file[PATH_MAX];
    if (!get_config_paths(dir, sizeof(dir), file, sizeof(file))) return;
    FILE *f = fopen(file, "rb");
    if (!f) return;
    char buf[PATH_MAX];
    if (fgets(buf, sizeof(buf), f)) {
        strip_newlines(buf);
        if (buf[0] == '/' && buf[1] != 0) {   // basic sanity
            snprintf(g_last_dir, sizeof(g_last_dir), "%s", buf);
        }
    }
    fclose(f);
}

void rsid_lastdir_save(void){
    char dir[PATH_MAX], file[PATH_MAX];
    if (!get_config_paths(dir, sizeof(dir), file, sizeof(file))) return;
    ensure_dir_exists(dir);
    FILE *f = fopen(file, "wb");
    if (!f) return;
    fputs(g_last_dir, f);
    fputc('\n', f);
    fclose(f);
}

int pick_sid_file(char *out_path, size_t out_sz){
    if (!out_path || out_sz == 0) return 0;
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "kdialog --getopenfilename \"%s\" \"*|SID files (*)\" \"*|All files\"",
             g_last_dir);

    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;

    if (!fgets(out_path, (int)out_sz, fp)) {
        pclose(fp);
        return 0;
    }

    int rc = pclose(fp);

    strip_newlines(out_path);

    // user cancelled
    if (out_path[0] == 0 || rc != 0) return 0;

    // update last dir
    char tmp[PATH_MAX];
    snprintf(tmp, sizeof(tmp), "%s", out_path);
    path_dirname_inplace(tmp);
    snprintf(g_last_dir, sizeof(g_last_dir), "%s", tmp);

    // persist it
    rsid_lastdir_save();

    return 1;
}

// defaultings
uint8_t playmode_sidtype = SIDPLAY_PLAYMODE_PSID;   // 0 = PSID, 1 = RSID, 2 = some crazy thing i dunno yet


int main(){
    char sidfilename[256];
    uint32_t length;

    setvbuf(stdout, NULL, _IONBF, 0);

    // audio setup ##########################################################
    snd_pcm_t *pcm_handle;
    snd_pcm_sw_params_t *sw;
    snd_pcm_hw_params_t *params;
    unsigned int rate = AUDIO_MIX_FREQ;
    int err;

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

    // AUDIO SETUP SUCCESSFULL ------------------------

    /////// SELECT A FILE NOW ETH ////////////////////////////////////
    rsid_lastdir_load();
    char path[4096];
    if (pick_sid_file(path, sizeof(path))) {
        printf("Picked: %s\n", path);
        sprintf(sidfilename, path);
    } else {
        printf("Cancelled or failed\n");
        return 0;
    }
    playmode_sidtype = CheckSIDType(sidfilename);

    /////////////////////////// TEST ////////////////////////////////////////////////////////////
    if(playmode_sidtype == SIDPLAY_PLAYMODE_PSID){  // TEST AREA //
        if(!PlaySID_Init(sidfilename, 0)){
            printf("Failed to init SID: %s\n", sidfilename);
            snd_pcm_close(pcm_handle);
            return 2;
        }
    }

    if(playmode_sidtype == SIDPLAY_PLAYMODE_RSID){  // TEST AREA //
        if(!PlaySID_InitRSID(sidfilename, 0)){
            printf("Failed to init SID: %s\n", sidfilename);
            snd_pcm_close(pcm_handle);
            return 2;
        }
    }


    /////////////////////////// [END TEST] ////////////////////////////////////////////////////////
    /* Prepare buffer and generate PWM (SID-style) audio */
    int16_t buffer[1024];                         /* stereo interleaved frames */



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
