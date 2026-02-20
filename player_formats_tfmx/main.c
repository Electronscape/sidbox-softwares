#include <stdio.h>
//#include <stdlib.h>
#include <stdint.h>
//#include <string.h>
//#include <math.h>
//#include <time.h>
#include <alsa/asoundlib.h>
#include "main.h"


uint16_t tfmx_be16(uint16_t v) { return __builtin_bswap16(v); }
uint32_t tfmx_be32(uint32_t v) { return __builtin_bswap32(v); }



#define PCM_DEVICE "default"

//////////////// SYSTEM FILE REQUESTER /////////////////////////////////
/// \brief prog_hello


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "sidbox/players/tfmxplay.h"



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

    snd_pcm_uframes_t period = 882;
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



    int16_t buffer[882 * 2];
    memset(buffer, 0x00, sizeof(buffer));




    printf("sizeof int: %u\n", sizeof(int));

    // AUDIO SETUP SUCCESSFULL ------------------------
    size_t filesize;
    pick_music_file(musicfilename, 256);
    printf("File selected '%s'\n", musicfilename);



    TFMX_SystemInit();  // TFMX Memory Object Initiators    // CALL THIS BEFORE anything is loaded or flapped about
    openTFMXFile(musicfilename);
    TfmxInit();
    g_tfmx.songnum = 0;

    printf("tempo0=%u tempo1=%u\n", (unsigned)TFMX_hdr.tempo[0], (unsigned)TFMX_hdr.tempo[1]);
    StartSong(g_tfmx.songnum, 0);



    /////////////////////////// [END TEST] ////////////////////////////////////////////////////////
    while (1)
    {
        uint32_t frames = 882;  // 44100 / 50fps

        if (!TFMX_mdb.PlayerEnable) {
            memset(buffer, 0, sizeof(buffer));
        } else {
            static int eRem_local = 0;
            static int32_t nb = 0;
            int32_t bd = 0;                 // bd only needs to live per-iteration now

            while ((uint32_t)bd < frames && TFMX_mdb.PlayerEnable)
            {
                if (nb <= 0) {
                    tfmxIrqIn();

                    nb = (int32_t)(g_tfmx.eClocks * (g_tfmx.outRate >> 1));
                    printf("writes %8X  \r", nb);
                    eRem_local += (nb % 357955);
                    nb /= 357955;
                    if (eRem_local > 357955) { nb++; eRem_local -= 357955; }

                    if (nb <= 0) continue;
                }

                int32_t n = (int32_t)frames - bd;
                if (n > nb) n = nb;

                mixem((uint32_t)n, (uint32_t)bd);

                bd += n;
                nb -= n;
            }


            // re-interlace for our harness //
            for (uint32_t i = 0; i < frames; i++)            {
                buffer[i*2u + 0u] = (int16_t)tbuf[HALFBUFSIZE + (int)i];
                buffer[i*2u + 1u] = (int16_t)tbuf[(int)i];
                tbuf[HALFBUFSIZE + (int)i] = 0;
                tbuf[(int)i] = 0;
            }


        }

        snd_pcm_sframes_t written = snd_pcm_writei(pcm_handle, buffer, frames);
        if (written == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
        } else if (written < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror((int)written));
            break;
        }
    }


    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    printf("done\n");


    return(42);

}
