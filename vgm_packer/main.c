#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>




#include "main.h"


//////////////// SYSTEM FILE REQUESTER /////////////////////////////////
/// \brief prog_hello


#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>





static char g_last_dir[PATH_MAX] =
    "/mnt/LinuxDatas/work/sidbox-softwares/vgm_packer/tunes";

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
        snprintf(out_dir,  out_dir_sz,  "%s/sidbox_vgmpacker", xdg);
    } else if (home && *home) {
        snprintf(out_dir,  out_dir_sz,  "%s/.config/sidbox_vgmpacker", home);
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


#include "sidbox/vgzpacker.h"
int main(){
    char musicfilename[256];
    uint32_t length;

    setvbuf(stdout, NULL, _IONBF, 0);

    size_t filesize;
    pick_music_file(musicfilename, 256);
    printf("File selected '%s'\n", musicfilename);

    ///// --- done loading file and being playing --- /////
    // make stdin non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);


    /////////////////////////// [END TEST] ////////////////////////////////////////////////////////

    vgp_pack_file(musicfilename, "/mnt/LinuxDatas/work/sidbox-softwares/vgm_packer/__fatout.vgp");















    printf("done\n");


    return(42);
}













