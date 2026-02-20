#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alsa/asoundlib.h>


#include <unistd.h>
#include <fcntl.h>




#include "main.h"
#include "sidbox/players/vgmplay.h"
#include "sidbox/players/sn76496.h"


#define PCM_DEVICE "default"

//////////////// SYSTEM FILE REQUESTER /////////////////////////////////
/// \brief prog_hello


#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>





static char g_last_dir[PATH_MAX] =
    "/mnt/LinuxDatas/work/sidbox-softwares/player_formats/tunes";

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

void music_lastdir_load(void){
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

void music_lastdir_save(void){
    char dir[PATH_MAX], file[PATH_MAX];
    if (!get_config_paths(dir, sizeof(dir), file, sizeof(file))) return;
    ensure_dir_exists(dir);
    FILE *f = fopen(file, "wb");
    if (!f) return;
    fputs(g_last_dir, f);
    fputc('\n', f);
    fclose(f);
}

int pick_music_file(char *out_path, size_t out_sz){
    if (!out_path || out_sz == 0) return 0;
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "kdialog --getopenfilename \"%s\" \"*|files (*)\" \"*|All files\"",
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
    music_lastdir_save();

    return 1;
}



int main(){
    char musicfilename[256];
    uint32_t length;

    setvbuf(stdout, NULL, _IONBF, 0);

    // audio setup ##########################################################
    snd_pcm_t *pcm_handle = NULL;
    snd_pcm_sw_params_t *sw = NULL;
    snd_pcm_hw_params_t *params = NULL;
    unsigned int rate = 44100;
    int err;

    if ((err = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "Error opening PCM device: %s\n", snd_strerror(err));
        return 1;
    }

    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, 2);

    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    unsigned int got = rate;
    snd_pcm_hw_params_get_rate(params, &got, 0);
    printf("ALSA rate got=%u\n", got);

    snd_pcm_uframes_t period = 1024;
    snd_pcm_uframes_t bufferv = period * 2;

    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &period, 0);
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, params, &bufferv);

    if ((err = snd_pcm_hw_params(pcm_handle, params)) < 0) {
        fprintf(stderr, "snd_pcm_hw_params failed: %s\n", snd_strerror(err));
        snd_pcm_hw_params_free(params);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    snd_pcm_hw_params_free(params);

    snd_pcm_sw_params_malloc(&sw);
    snd_pcm_sw_params_current(pcm_handle, sw);

    snd_pcm_sw_params_set_start_threshold(pcm_handle, sw, period);
    snd_pcm_sw_params_set_avail_min(pcm_handle, sw, period);

    if ((err = snd_pcm_sw_params(pcm_handle, sw)) < 0) {
        fprintf(stderr, "snd_pcm_sw_params failed: %s\n", snd_strerror(err));
        snd_pcm_sw_params_free(sw);
        snd_pcm_close(pcm_handle);
        return 1;
    }

    snd_pcm_sw_params_free(sw);

    // Make sure PCM is ready
    if ((err = snd_pcm_prepare(pcm_handle)) < 0) {
        fprintf(stderr, "snd_pcm_prepare failed: %s\n", snd_strerror(err));
        snd_pcm_close(pcm_handle);
        return 1;
    }




    uint32_t frames = (uint32_t)period;

    int16_t buffer[1024 * 2];
    memset(buffer, 0x00, sizeof(buffer));




    printf("sizeof int: %u\n", sizeof(int));

    // AUDIO SETUP SUCCESSFULL ------------------------
    size_t filesize;
    pick_music_file(musicfilename, 256);
    printf("File selected '%s'\n", musicfilename);

    ///// --- this area is for "loading the file" --- /////

    openVgmFile(musicfilename);
    //// START MUSIC ///



    ///// --- done loading file and being playing --- /////
    // make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);


    /////////////////////////// [END TEST] ////////////////////////////////////////////////////////
    int sndbuffer[1024 * 2];
    printf("q and enter to exit\n");

    SN76496Write(0, 0x06);
    SN76496Write(0, 44);
    while (1) {
        // uint32_t frames = 1024;  // DELETE THIS

        memset(sndbuffer, 0, (size_t)frames * 2u * sizeof(int));
        stepVGM(sndbuffer, (int)frames);

        for (uint32_t i = 0; i < frames; i++) {
            buffer[i*2 + 0u] = (int16_t)sndbuffer[i*2 + 0u];
            buffer[i*2 + 1u] = (int16_t)sndbuffer[i*2 + 1u];
        }

        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);
        if (written == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
            continue;
        } else if (written < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
            break;
        } else if ((uint32_t)written != frames) {
            // partial write: simple recovery for now
            snd_pcm_prepare(pcm_handle);
        }

        char ch;
        if (read(STDIN_FILENO, &ch, 1) > 0)
        {
            if (ch == 'q')   // ESC key = ASCII 27
                break;
        }

    }


    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("done\n");


    return(42);

}
