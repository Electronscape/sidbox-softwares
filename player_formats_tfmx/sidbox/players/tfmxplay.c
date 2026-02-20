/***************************************************************************
 *   Copyright (C) 2004 by David Banz                                      *
 *   neko@netcologne.de                                                    *
 *   GPL'ed                                                                *
 ***************************************************************************/

#include "../../main.h"
#include <stdio.h>
#include <string.h>

#include "tfmxplay.h"
#include "player.h"

/* external functions */

uint8_t songmemory  [128 * 1024];        // assign to editbuf // this will be in external ram also


int singleFile=0;
int dosExt=0;



/* this one can also load a single-file TFMX :) */
int load_tfmx(char *mfn, char *sfn){
    FILE *gfd;
    unsigned int x, y, z = 0;
    uint16_t *sh, *lg;

    unsigned int mlen;
    int num_ts,num_pat,num_mac;
    uint32_t nTFhd_offset=0;
    uint32_t nTFhd_mdatsize=0;
    uint32_t nTFhd_smplsize=0;



    printf("MDAT: %s\nSMPL: %s\n", mfn, sfn);

    if (!mfn || !mfn[0]) return 1;

    gfd = fopen(mfn, "rb");                 // <-- BINARY
    if (!gfd) {
        perror("fopen");
        return 1;
    }

    // jump to mdat start if single-file format
    if (singleFile == 1) {
        if (fseek(gfd, (long)nTFhd_offset, SEEK_SET) != 0) {   // <-- SEEK_SET
            perror("fseek(mdat)");
            fclose(gfd);
            return 1;
        }
    }

    // Read header (packed struct must match on-disk layout)
    if (fread(&TFMX_hdr, 1, sizeof(TFMX_hdr), gfd) != sizeof(TFMX_hdr)) {
        perror("fread(hdr)");
        fclose(gfd);
        return 1;
    }

    // Magic checks
    if (strncmp("TFMX-SONG", TFMX_hdr.magic, 9) &&
        strncmp("TFMX_SONG", TFMX_hdr.magic, 9) &&
        strncasecmp("TFMXSONG", TFMX_hdr.magic, 8) &&
        strncmp("TFMX",      TFMX_hdr.magic, 4))
    {
        fclose(gfd);
        return 2;
    }

    // Read MDAT "editbuf" as 32-bit words, not sizeof(int)
    x = (unsigned int)fread(g_tfmx.editbuf, sizeof(uint32_t), 16384, gfd);
    if (x == 0) {
        perror("fread(g_tfmx.editbuf)");
        fclose(gfd);
        return 1;
    }

    // If dual-file, close MDAT now (matches original flow)
    if (singleFile == 0) {
        fclose(gfd);
        gfd = NULL;
    }

    mlen = x;

    // Safe sentinel (don’t write past end if x == 16384)
    if (x < 16384) g_tfmx.editbuf[x] = 0xFFFFFFFFu;
    else           g_tfmx.editbuf[16383] = 0xFFFFFFFFu;

    // Convert header pointers
    if (!TFMX_hdr.trackstart) TFMX_hdr.trackstart = 0x180;
    else                      TFMX_hdr.trackstart = (tfmx_be32(TFMX_hdr.trackstart) - 0x200u) >> 2;

    if (!TFMX_hdr.pattstart)  TFMX_hdr.pattstart  = 0x80;
    else                      TFMX_hdr.pattstart  = (tfmx_be32(TFMX_hdr.pattstart) - 0x200u) >> 2;

    if (!TFMX_hdr.macrostart) TFMX_hdr.macrostart = 0x100;
    else                      TFMX_hdr.macrostart = (tfmx_be32(TFMX_hdr.macrostart) - 0x200u) >> 2;

    if (mlen < 136) return 2;

    // Convert song tables
    for (x = 0; x < 32; x++) {
        TFMX_hdr.start[x] = tfmx_be16(TFMX_hdr.start[x]);
        TFMX_hdr.end[x]   = tfmx_be16(TFMX_hdr.end[x]);
        TFMX_hdr.tempo[x] = tfmx_be16(TFMX_hdr.tempo[x]);
    }

    // Fix macros
    z = TFMX_hdr.macrostart;
    TFMX_macros = (int *) & g_tfmx.editbuf[z];
    for (x = 0; x < 128; x++) {
        y = (tfmx_be32(g_tfmx.editbuf[z]) - 0x200u);
        if ((y & 3u) || ((y >> 2) > mlen)) break;
        g_tfmx.editbuf[z++] = (y >> 2);
    }
    num_mac = x;

    // Fix patterns
    z = TFMX_hdr.pattstart;
    TFMX_patterns = (int *) & g_tfmx.editbuf[z];
    for (x = 0; x < 128; x++) {
        y = (tfmx_be32(g_tfmx.editbuf[z]) - 0x200u);
        if ((y & 3u) || ((y >> 2) > mlen)) break;
        g_tfmx.editbuf[z++] = (y >> 2);
    }
    num_pat = x;

    // Convert tracksteps to host endian
    lg = (uint16_t *) & g_tfmx.editbuf[TFMX_patterns[0]];
    sh = (uint16_t *) & g_tfmx.editbuf[TFMX_hdr.trackstart];
    num_ts = (TFMX_patterns[0] - TFMX_hdr.trackstart) >> 2;

    while (sh < lg) {
        *sh = tfmx_be16(*sh);
        sh++;
    }

    // Load samples
    if (singleFile == 1) {
        uint32_t nSmplPos = nTFhd_offset + nTFhd_mdatsize;

        if (!gfd) {
            // should not happen, but be safe
            gfd = fopen(mfn, "rb");
            if (!gfd) { perror("fopen(reopen)"); return 1; }
        }

        if (fseek(gfd, (long)nSmplPos, SEEK_SET) != 0) {
            perror("fseek(smpl)");
            fclose(gfd);
            return 1;
        }

        TFMX_smplbuf = samplememory;

        if (fread(samplememory, 1, nTFhd_smplsize, gfd) != nTFhd_smplsize) {
            perror("fread(smpl)");
            fclose(gfd);
            return 1;
        }

        fclose(gfd);
    } else {

        FILE *fp = fopen(sfn, "rb");
        if (!fp) {
            perror("fopen(smpl)");
            return 1;
        }

        TFMX_smplbuf = samplememory;
        if (!TFMX_smplbuf) {
            perror("samplememory NULL");
            fclose(fp);
            return 1;
        }

        /* read up to 1MB */
        size_t r = fread(samplememory, 1, 1024 * 1024, fp);

        if (ferror(fp)) {
            perror("fread(smpl)");
            fclose(fp);
            return 1;
        }

        /* r now contains actual file size */
        size_t sample_size = r;
        fclose(fp);
    }
    return 0;
}






uint8_t openTFMXFile(char *filename){
    // --- Build mdat/smpl paths like original tfmxplay does (minimal) ---
    char mfn[PATHNAME_LENGTH];
    char sfn[PATHNAME_LENGTH];

    snprintf(mfn, sizeof(mfn), "%s", filename);
    snprintf(sfn, sizeof(sfn), "%s", filename);

    // If user picked "mdat.*" or "tfmx.*", derive smpl.* next to it
    char *c = strrchr(sfn, '/');
    c = (c ? (c + 1) : sfn);

    singleFile = 0;     // keep as your original global if it exists; else local here is fine only for harness logic
    dosExt = 0;
    g_tfmx.editbuf = (uint32_t *)songmemory;

    char *tfxloc = strchr(c, '\0');
    if ((tfxloc - 4) > c) {
        tfxloc -= 4;
        if (0 == strncasecmp(tfxloc, ".tfx", 4)) dosExt = 1;
    }

    if (!dosExt) {
        if (strncasecmp(c, "mdat.", 5)) {
            if (strncasecmp(c, "tfmx.", 5)) {
                printf("Warning: file does not start with mdat./tfmx.\n");
            } else {
                singleFile = 1;
                sfn[0] = '\0';
            }
        }
        if (!singleFile) {
            // case-preserving mdat -> smpl
            (*c++) ^= 'm' ^ 's';
            (*c++) ^= 'd' ^ 'm';
            (*c++) ^= 'a' ^ 'p';
            (*c++) ^= 't' ^ 'l';
        }
    } else {
        // .tfx -> .sam
        tfxloc++; // skip '.'
        (*tfxloc++) ^= 't' ^ 's';
        (*tfxloc++) ^= 'f' ^ 'm';
        (*tfxloc++) ^= 'x' ^ 'p';
    }

    int x = load_tfmx(mfn, sfn);
    if (x == 1) { fprintf(stderr, "load_tfmx failed\n");      return 1; }
    if (x == 2) { fprintf(stderr, "Not an MDAT/TFMX file\n"); return 1; }

}















uint32_t RenderTFXBlock(int16_t *left, int16_t *right, uint32_t frames){
    // TODO: this would be rendered here, and then passed through left and right

    return frames;
}
