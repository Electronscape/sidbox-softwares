#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "font.h"
#include "sbapi_graphics.h"


uint8_t current_fr_colour = 1;
uint8_t current_bk_colour = 0;

uint8_t PROJ_VRAM[SCR_RAMSIZE];

uint32_t PROJ_CRAM[256] = {
    0x00000000, 0x00AFAFAF, 0x00FFFFFF, 0x003B67A2, 0x00AA907C, 0x00959595, 0x007B7B7B, 0x00FFA997,
    0x0037A91D, 0x007CA9FF, 0x00BF8112, 0x00EBBF66, 0x0078C178, 0x003D9318, 0x00B33418, 0x00D9311C,
    0x00000000, 0x0000001C, 0x00000039, 0x00000055, 0x00000071, 0x0000018E, 0x000001AA, 0x000001C6,
    0x000001E3, 0x000001FF, 0x00CECECE, 0x0000FF00, 0x00B2FF00, 0x00FFE700, 0x00FF9600, 0x00FF1100,
    0x00491200, 0x00491355, 0x004914AA, 0x004916FF, 0x005B1700, 0x005B1855, 0x005B19AA, 0x005B1AFF,
    0x006D1B00, 0x006D1C55, 0x0000E300, 0x0085FF54, 0x00C4FF00, 0x00FFD900, 0x00FFA41F, 0x00E05400,
    0x00FF0000, 0x00922655, 0x009227AA, 0x009228FF, 0x00A42900, 0x00A42A55, 0x00A42BAA, 0x00A42CFF,
    0x00B62D00, 0x00B62F55, 0x00B630AA, 0x00B631FF, 0x00C93200, 0x00C93355, 0x00C934AA, 0x00C935FF,
    0x00DB3700, 0x00DB3855, 0x00DB39AA, 0x00DB3AFF, 0x00ED3B00, 0x00ED3C55, 0x00ED3DAA, 0x00ED3FFF,
    0x00FF4000, 0x00FF4155, 0x00FF42AA, 0x00FF43FF, 0x00004400, 0x00004555, 0x000046AA, 0x000048FF,
    0x00FFFF00, 0x0012FF55, 0x0012EE55, 0x0012B6FF, 0x00001FFF, 0x009D0EC7, 0x00F10000, 0x00FF7700,
    0x00375200, 0x00375355, 0x003754AA, 0x003755FF, 0x00495600, 0x00495855, 0x004959AA, 0x00495AFF,
    0x005B5B00, 0x005B5C55, 0x005B5DAA, 0x005B5EFF, 0x006D6000, 0x006D6155, 0x006D62AA, 0x006D63FF,
    0x006D6400, 0x00806555, 0x008066AA, 0x008067FF, 0x00926900, 0x00926A55, 0x00926BAA, 0x00926CFF,
    0x00A46D00, 0x00A46E55, 0x00A46FAA, 0x00A471FF, 0x00B67200, 0x00B67355, 0x00B674AA, 0x00B675FF,
    0x00C97600, 0x00C97755, 0x00C979AA, 0x00C97AFF, 0x00DB7B00, 0x00DB7C55, 0x00DB7DAA, 0x00DB7EFF,
    0x00ED7F00, 0x00ED8055, 0x00ED82AA, 0x00ED83FF, 0x00FF8400, 0x00FF8555, 0x00FF86AA, 0x00FF87FF,
    0x00008800, 0x00008A55, 0x00008BAA, 0x00008CFF, 0x00128D00, 0x00128E55, 0x00128FAA, 0x001290FF,
    0x00249200, 0x00249355, 0x002494AA, 0x002495FF, 0x00379600, 0x00379755, 0x003798AA, 0x003799FF,
    0x00499B00, 0x00499C55, 0x00499DAA, 0x00499EFF, 0x005B9F00, 0x005BA055, 0x005BA1AA, 0x005BA3FF,
    0x00A4B5D5, 0x00A0B0F8, 0x0094A3E6, 0x007C89C1, 0x006281C0, 0x001C62A1, 0x004254EA, 0x0062A1BD,
    0x007093C0, 0x004977A1, 0x00003FAA, 0x001554FF, 0x001C50B9, 0x0000B3FF, 0x000088AA, 0x0000B5FF,
    0x000E62FF, 0x005EB7E3, 0x00BDC0B9, 0x0085B9FF, 0x00006CAF, 0x001F81B9, 0x003F5BAA, 0x00C9BEFF,
    0x005BAFCB, 0x00DBC055, 0x00DBC1AA, 0x00BDC0C0, 0x00EDC400, 0x00EDC555, 0x00EDC6AA, 0x00EDC7FF,
    0x00FFC800, 0x00FFC955, 0x00FFCAAA, 0x00FFCCFF, 0x0000CD00, 0x0000CE55, 0x0000CFAA, 0x0000D0FF,
    0x0012D100, 0x0012D255, 0x0012D3AA, 0x0012D5FF, 0x0024D600, 0x0024D755, 0x0024D8AA, 0x0024D9FF,
    0x00100020, 0x001B0029, 0x00250032, 0x0030003A, 0x003B0043, 0x0045004C, 0x00500055, 0x0049E2FF,
    0x005BE300, 0x005BE555, 0x005BE6AA, 0x005BE7FF, 0x006DE800, 0x006DE955, 0x006DEAAA, 0x006DEBFF,
    0x006DEC00, 0x0080EE55, 0x0080EFAA, 0x0080F0FF, 0x0093CEA2, 0x0092F255, 0x0092F3AA, 0x0092F4FF,
    0x00A4F600, 0x00A4F755, 0x00A4F8AA, 0x00A4F9FF, 0x00B6FA00, 0x00B6FB55, 0x00B6FCAA, 0x00B6FEFF,
    0x00C9FF00, 0x00C9FF55, 0x00C9FFAA, 0x00C9FFFF, 0x00DBFF00, 0x00DBFF55, 0x00DBFFAA, 0x00DBFFFF,
    0x00EDFF00, 0x00EDFF55, 0x00EDFFAA, 0x00EDFFFF, 0x00FFFF00, 0x00FFFF55, 0x00FFFFAA, 0x00FFFFFF,
};


uint32_t dxf[480];
uint32_t dyf[320];

void prepXYs(){
    long i;
    for(i=0; i<480; i++){
        dxf[i] = i*320;
    }

    for(i=0; i<320; i++){
        dyf[i] = i*480;
    }
}


unsigned char cyclefrom = 16, cycleto = 25;
char bCycleDirection = 0;

unsigned char clut_cycle_index[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

unsigned char cyclespeed = 5; // special cycle pallet

void sbgfx_pset(int16_t x, int16_t y, uint8_t cindex){
    if ((unsigned)x >= SCR_WIDTH || (unsigned)y >= SCR_HEIGHT)
        return;

    uint8_t *vmem = PROJ_VRAM + (y * SCR_WIDTH + x);
    *vmem = cindex;
}

void sbgfx_fill(uint8_t colour){
    for (int i = 0; i < SCR_RAMSIZE; i++)
        PROJ_VRAM[i] = colour;
}

void sbgfx_drawbox(int x, int y, int w, int h, uint8_t col){
    // Clip to screen bounds
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_WIDTH) w = SCR_WIDTH - x;
    if (y + h > SCR_HEIGHT) h = SCR_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    /*
    for (int fy = 0; fy < h; fy++) {
        uint8_t *row = VRAM + (y + fy) * SCR_WIDTH + x;
        for (int fx = 0; fx < w; fx++) {
            row[fx] = col;
        }
    }
    */
    for (int fx = 0; fx < w; fx++) {
        uint8_t *row = PROJ_VRAM + ((x + fx) * SCR_HEIGHT) + y;
        for (int fy = 0; fy < h; fy++) {
            row[fy] = col;
        }
    }
}

void gfx_setcolour(unsigned char col){
    current_fr_colour = col;
}



void dopalletecycle() {
    static char cbd;
    static unsigned char i;
    static unsigned long tmp;
    static unsigned char tmpold;

    static unsigned short SpeedStep;

    /*  // this is STM32 specific
    if(cbd) {
        cbd=0;
        HAL_DMA2D_CLUTLoad_IT(&hdma2d, clut1, 1);
    } else {
        cbd=1;
        HAL_DMA2D_CLUTLoad_IT(&hdma2d, clut1, 0);
    }
    */

    if(cyclespeed==255) return;


    SpeedStep++;
    if (SpeedStep > cyclespeed) {
        SpeedStep = 0;
        if (bCycleDirection == 0) {
            tmp = PROJ_CRAM[cyclefrom];
            tmpold = clut_cycle_index[cyclefrom];
            for (i = cyclefrom; i < cycleto; i++) {
                PROJ_CRAM[i] = PROJ_CRAM[i + 1];
                clut_cycle_index[i] = clut_cycle_index[i + 1];
            }
            PROJ_CRAM[i] = tmp;
            clut_cycle_index[i] = tmpold;

        } else {
            tmp = PROJ_CRAM[cycleto];
            tmpold = clut_cycle_index[cycleto];
            for (i = cycleto; i > cyclefrom; i--) {
                PROJ_CRAM[i] = PROJ_CRAM[i - 1];
                clut_cycle_index[i] = clut_cycle_index[i - 1];
            }
            PROJ_CRAM[i] = tmp;
            clut_cycle_index[i] = tmpold;
        }
    }
}


void draw_text816(int x, int y, const unsigned char* textptr) {
    int32_t start_x = x;
    uint8_t colf = current_fr_colour;


    for (int32_t i = 0; textptr[i] != '\0'; ++i) {
        if (textptr[i] == '\n') {
            x = start_x;
            y += 16;
            continue;
        }

        if ((uint32_t)x >= SCR_WIDTH || (uint32_t)y >= SCR_HEIGHT - 14)
            continue;

        const uint8_t* pixeldata = DEFAULT_SYSFONT[textptr[i]];
        uint32_t dx = x * SCR_HEIGHT;
        uint8_t* drawptr = PROJ_VRAM + dx + y;

        for (int j = 0; j < 8; ++j) {
            if ((uint32_t)x >= SCR_WIDTH)
                break;

            uint8_t pixdat = pixeldata[j];
            uint8_t* dp = PROJ_VRAM + x * SCR_HEIGHT + y;

            // Draw 2 pixels vertically every 2 pixels (H=2)
            if (pixdat & 0x01) { dp[0] = colf; dp[1] = colf; } //else { dp[0] = colb; dp[1] = colb; }
            if (pixdat & 0x02) { dp[2] = colf; dp[3] = colf; } //else { dp[2] = colb; dp[3] = colb; }
            if (pixdat & 0x04) { dp[4] = colf; dp[5] = colf; } //else { dp[4] = colb; dp[5] = colb; }
            if (pixdat & 0x08) { dp[6] = colf; dp[7] = colf; } //else { dp[6] = colb; dp[7] = colb; }
            if (pixdat & 0x10) { dp[8] = colf; dp[9] = colf; } //else { dp[8] = colb; dp[9] = colb; }
            if (pixdat & 0x20) { dp[10] = colf; dp[11] = colf; } //else { dp[10] = colb; dp[11] = colb; }
            if (pixdat & 0x40) { dp[12] = colf; dp[13] = colf; } //else { dp[12] = colb; dp[13] = colb; }
            if (pixdat & 0x80) { dp[14] = colf; dp[15] = colf; } //else { dp[14] = colb; dp[15] = colb; }

            x++;
        }
    }
}

//-------------------------------------- DEDICATED TO THE SMS RENDERER -----------------------------------------------------------

// source://smsemu/vdp.cpp
extern float stretch_wide;	// allows for stretching across the screen
static void (*sms_plot)(int32_t x, int32_t y, uint8_t c);

void vdp_plot(int32_t x, int32_t y, uint8_t c){
    /*
    uint8_t *dfxz;
    uint8_t *dfxz2;

    x *= stretch_wide;
    y *= 1.66f;

    if(x<0) return;
    if(x>478) return;
    if(y<0) return;
    if(y>318) return;

    dfxz  = noxgf_current_buffer + dxf[x++] + y;
    dfxz2 = noxgf_current_buffer + dxf[x] + y;

    *dfxz++ =  c; *dfxz =  c;
    *dfxz2++ = c; *dfxz2 = c;
*/
    uint8_t *p1, *p2;
    uint32_t dx1, dx2;
    uint8_t *base;  // our current graphics draw buffer (though only using our project it has double buffering already, draw, copy to front method)
    uint16_t cc;

    // Use hardware FPU for fast float ops (M7 supports single-precision in hardware)
    x = (int)(x * stretch_wide);
    y = (int)(y * 1.66f);

    // Fast bounds check (unsigned to avoid branches and negative checks)
    if ((uint32_t)x > 478u || (uint32_t)y > 318u) return;

    // Combine color into 16-bit word: two pixels
    cc = (uint16_t)(c << 8) | c;

    // Get row base pointer
    base = PROJ_VRAM + y;

    // Compute column offsets
    dx1 = dxf[x];
    dx2 = dxf[x + 1];

    // Compute destination pointers
    p1 = base + dx1;
    p2 = base + dx2;

    // Write 2 pixels to each column using a single 16-bit store (assumes 2-byte alignment)
    *(uint16_t *)p1 = cc;
    *(uint16_t *)p2 = cc;
}

void vdp_plotGG(int32_t x, int32_t y, uint8_t c){
    /*
    uint8_t *dfxz;
    uint8_t *dfxz2;
    uint8_t *dfxz3;

    // Basic Working Aspect
    x-= 56;
    y-= 46;
    x *=2.7;
    y *=2;
    if(x<16) return;
    if(x>448) return;
    if(y<0) return;
    if(y>318) return;

    dfxz  = noxgf_current_buffer + dxf[x++] + y;
    dfxz2 = noxgf_current_buffer + dxf[x++] + y;
    dfxz3 = noxgf_current_buffer + dxf[x] + y;

    *dfxz++ = c; *dfxz = c;
    *dfxz2++ = c; *dfxz2 = c;
    *dfxz3++ = c; *dfxz3 = c;
*/
    uint8_t *p1, *p2, *p3;
    uint32_t dx1, dx2, dx3;
    uint8_t *base;
    uint16_t cc;

    // Apply translation and scaling (hardware FPU: fast)
    x = (int)((x - 56) * 2.7f);  // scale x
    y = (int)((y - 46) * 2.0f);  // scale y

    // Fast unsigned bounds check (branchless)
    if ((uint32_t)(x - 16) > (448 - 16) || (uint32_t)y > 318u) return;

    // Combine two pixels into 16-bit word
    cc = (uint16_t)(c << 8) | c;

    // Get pointer to the start of line
    base = PROJ_VRAM + y;

    // Compute offsets for 3 columns
    dx1 = dxf[x];
    dx2 = dxf[x + 1];
    dx3 = dxf[x + 2];

    // Compute pixel pointers
    p1 = base + dx1;
    p2 = base + dx2;
    p3 = base + dx3;

    // Write two pixels to each column (16-bit aligned stores)
    *(uint16_t *)p1 = cc;
    *(uint16_t *)p2 = cc;
    *(uint16_t *)p3 = cc;
}

void setSMSPlotter(uint8_t mode){
    if(mode)	// gamegear plotter
        sms_plot = &vdp_plotGG;
    else
        sms_plot = &vdp_plot;
}

void SMSPlot(int32_t x, int32_t y, uint8_t c){
    sms_plot(x,y,c);
}

