#include <stdint.h>
#include <stdlib.h>

#include "gfx.h"

// basic colour look up table, for use with the graphics memory look up

uint32_t clut[256] = {
    0x00000000, 0xFFAFAFAF, 0xFFFFFFFF, 0xFF3B67A2, 0xFFAA907C, 0xFF959595, 0xFF7B7B7B, 0xFFFFA997,
    0xFF37A91D, 0xFF7CA9FF, 0xFFBF8112, 0xFFEBBF66, 0xFF78C178, 0xFF3D9318, 0xFFB33418, 0xFFD9311C,
    0xFF000000, 0xFF00000E, 0xFF00001D, 0xFF00002B, 0xFF000139, 0xFF000147, 0xFF000156, 0xFF000164,
    0xFF0001D2, 0xFF0001FF, 0xFFCECECE, 0xFF00FF00, 0xFFB2FF00, 0xFFFFE700, 0xFFFF9600, 0xFFFF1100,
    0xFF491200, 0xFF491355, 0xFF4914AA, 0xFF4916FF, 0xFF5B1700, 0xFF5B1855, 0xFF5B19AA, 0xFF5B1AFF,
    0xFF6D1B00, 0xFF6D1C55, 0xFF00E300, 0xFF85FF54, 0xFFC4FF00, 0xFFFFD900, 0xFFFFA41F, 0xFFE05400,
    0xFFFF0000, 0xFF922655, 0xFF9227AA, 0xFF9228FF, 0xFFA42900, 0xFFA42A55, 0xFFA42BAA, 0xFFA42CFF,
    0xFFB62D00, 0xFFB62F55, 0xFFB630AA, 0xFFB631FF, 0xFFC93200, 0xFFC93355, 0xFFC934AA, 0xFFC935FF,
    0xFFDB3700, 0xFFDB3855, 0xFFDB39AA, 0xFFDB3AFF, 0xFFED3B00, 0xFFED3C55, 0xFFED3DAA, 0xFFED3FFF,
    0xFFFF4000, 0xFFFF4155, 0xFFFF42AA, 0xFFFF43FF, 0xFF004400, 0xFF004555, 0xFF0046AA, 0xFF0048FF,
    0xFFFFFF00, 0xFF12FF55, 0xFF12EE55, 0xFF12B6FF, 0xFF001FFF, 0xFF9D0EC7, 0xFFF10000, 0xFFFF7700,
    0xFF375200, 0xFF375355, 0xFF3754AA, 0xFF3755FF, 0xFF495600, 0xFF495855, 0xFF4959AA, 0xFF495AFF,
    0xFF5B5B00, 0xFF5B5C55, 0xFF5B5DAA, 0xFF5B5EFF, 0xFF6D6000, 0xFF6D6155, 0xFF6D62AA, 0xFF6D63FF,
    0xFF6D6400, 0xFF806555, 0xFF8066AA, 0xFF8067FF, 0xFF926900, 0xFF926A55, 0xFF926BAA, 0xFF926CFF,
    0xFFA46D00, 0xFFA46E55, 0xFFA46FAA, 0xFFA471FF, 0xFFB67200, 0xFFB67355, 0xFFB674AA, 0xFFB675FF,
    0xFFC97600, 0xFFC97755, 0xFFC979AA, 0xFFC97AFF, 0xFFDB7B00, 0xFFDB7C55, 0xFFDB7DAA, 0xFFDB7EFF,
    0xFFED7F00, 0xFFED8055, 0xFFED82AA, 0xFFED83FF, 0xFFFF8400, 0xFFFF8555, 0xFFFF86AA, 0xFFFF87FF,
    0xFF008800, 0xFF008A55, 0xFF008BAA, 0xFF008CFF, 0xFF128D00, 0xFF128E55, 0xFF128FAA, 0xFF1290FF,
    0xFF249200, 0xFF249355, 0xFF2494AA, 0xFF2495FF, 0xFF379600, 0xFF379755, 0xFF3798AA, 0xFF3799FF,
    0xFF499B00, 0xFF499C55, 0xFF499DAA, 0xFF499EFF, 0xFF5B9F00, 0xFF5BA055, 0xFF5BA1AA, 0xFF5BA3FF,
    0xFFA4B5D5, 0xFFA0B0F8, 0xFF94A3E6, 0xFF7C89C1, 0xFF6281C0, 0xFF1C62A1, 0xFF4254EA, 0xFF62A1BD,
    0xFF7093C0, 0xFF4977A1, 0xFF003FAA, 0xFF1554FF, 0xFF1C50B9, 0xFF00B3FF, 0xFF0088AA, 0xFF00B5FF,
    0xFF0E62FF, 0xFF5EB7E3, 0xFFBDC0B9, 0xFF85B9FF, 0xFF006CAF, 0xFF1F81B9, 0xFF3F5BAA, 0xFFC9BEFF,
    0xFF5BAFCB, 0xFFDBC055, 0xFFDBC1AA, 0xFFBDC0C0, 0xFFEDC400, 0xFFEDC555, 0xFFEDC6AA, 0xFFEDC7FF,
    0xFFFFC800, 0xFFFFC955, 0xFFFFCAAA, 0xFFFFCCFF, 0xFF00CD00, 0xFF00CE55, 0xFF00CFAA, 0xFF00D0FF,
    0xFF12D100, 0xFF12D255, 0xFF12D3AA, 0xFF12D5FF, 0xFF24D600, 0xFF24D755, 0xFF24D8AA, 0xFF24D9FF,
    0xFF37DA00, 0xFF37DB55, 0xFF37DDAA, 0xFF37DEFF, 0xFF49DF00, 0xFF49E055, 0xFF49E1AA, 0xFF49E2FF,
    0xFF5BE300, 0xFF5BE555, 0xFF5BE6AA, 0xFF5BE7FF, 0xFF6DE800, 0xFF6DE955, 0xFF6DEAAA, 0xFF6DEBFF,
    0xFF6DEC00, 0xFF80EE55, 0xFF80EFAA, 0xFF80F0FF, 0xFF93CEA2, 0xFF92F255, 0xFF92F3AA, 0xFF92F4FF,
    0xFFA4F600, 0xFFA4F755, 0xFFA4F8AA, 0xFFA4F9FF, 0xFFB6FA00, 0xFFB6FB55, 0xFFB6FCAA, 0xFFB6FEFF,
    0xFFC9FF00, 0xFFC9FF55, 0xFFC9FFAA, 0xFFC9FFFF, 0xFFDBFF00, 0xFFDBFF55, 0xFFDBFFAA, 0xFFDBFFFF,
    0xFFEDFF00, 0xFFEDFF55, 0xFFEDFFAA, 0xFFEDFFFF, 0xFFFFFF00, 0xFFFFFF55, 0xFFFFFFAA, 0xFFFFFFFF
};



static const uint8_t bayer4x4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};


uint8_t fb[SCREEN_W * SCREEN_H] = {0};
uint32_t pb[SCREEN_W * SCREEN_H] = {0xFF0000FF};

// converter
void videoMemToScreen(){
    // convert the CLUT image to ARGB for SDL2
    for(int32_t s = 0; s < (SCREEN_H * SCREEN_W); s++){
        pb[s] = clut[ fb[s] ];
    }
}

// real basic clear screen
void clearScreen(uint8_t colIndex){
    for(int32_t s = 0; s < (SCREEN_H * SCREEN_W); s++){
        fb[s] = colIndex;
    }
}

void putPixel(int32_t x, int32_t y, uint8_t colIndex){
    // protect the ram area!
    if(x < 0 || x >= SCREEN_W) return;
    if(y < 0 || y >= SCREEN_H) return;

    fb[(y * SCREEN_W) + x] = colIndex;
}

void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex)
{
    int dx = abs(x1 - x0);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        putPixel(x0, y0, colorIndex);

        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = err * 2;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}



static int hashNoise4bit(int x, int y)
{
    uint32_t n = (uint32_t)x;
    n *= 0x1f123bb5u;
    n += (uint32_t)y * 0x159a55e5u;
    n ^= n >> 15;
    n *= 0x85ebca6bu;
    n ^= n >> 13;
    n *= 0xc2b2ae35u;
    n ^= n >> 16;

    return (int)(n & 15u);
}

static uint32_t g_ditherSeed = 0x12345678;
void resetRand(){
    g_ditherSeed = 0x34188195;
}

static uint32_t fastRand(void)
{
    g_ditherSeed = (g_ditherSeed * 1664525u) + 1013904223u;
    return g_ditherSeed;
}

static uint8_t shadeColor(uint8_t baseColor, int shade)
{
    if (shade < 0) {
        shade = 0;
    }

    if (shade >= 5) {
        return 16;   /* or 0 if you prefer */
    }

    uint8_t family = baseColor & 15;

    return (uint8_t)(32 + family + (shade * 16));
}

static uint8_t ditherShadeColor(uint8_t baseColor, float shadeF, int x, int y, DitherMode mode)
{
    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > 5.0f) shadeF = 5.0f;

    int s0 = (int)shadeF;
    int s1 = s0 + 1;

    if (s1 > 5) s1 = 5;

    float frac = shadeF - (float)s0;

    int threshold;

    if (mode == DITHER_RANDOM) {
        threshold = hashNoise4bit(x, y);
    } else {
        threshold = bayer4x4[y & 3][x & 3];
    }

    if ((frac * 16.0f) > (float)threshold) {
        return shadeColor(baseColor, s1);
    } else {
        return shadeColor(baseColor, s0);
    }
}

void fillTriangleDither(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t baseColor, float shadeF, DitherMode mode)
{
    int minX = x0;
    int minY = y0;
    int maxX = x0;
    int maxY = y0;

    if (x1 < minX) minX = x1;
    if (x2 < minX) minX = x2;
    if (y1 < minY) minY = y1;
    if (y2 < minY) minY = y2;

    if (x1 > maxX) maxX = x1;
    if (x2 > maxX) maxX = x2;
    if (y1 > maxY) maxY = y1;
    if (y2 > maxY) maxY = y2;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= SCREEN_W) maxX = SCREEN_W - 1;
    if (maxY >= SCREEN_H) maxY = SCREEN_H - 1;

    int area = ((x1 - x0) * (y2 - y0)) - ((y1 - y0) * (x2 - x0));
    if (area == 0) {
        return;
    }

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int w0 = ((x1 - x0) * (y - y0)) - ((y1 - y0) * (x - x0));
            int w1 = ((x2 - x1) * (y - y1)) - ((y2 - y1) * (x - x1));
            int w2 = ((x0 - x2) * (y - y2)) - ((y0 - y2) * (x - x2));

            if (area > 0) {
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    putPixel(x, y, ditherShadeColor(baseColor, shadeF, x, y, mode));
                }
            } else {
                if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                    putPixel(x, y, ditherShadeColor(baseColor, shadeF, x, y, mode));
                }
            }
        }
    }
}


void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color)
{
    int minX = x0;
    int minY = y0;
    int maxX = x0;
    int maxY = y0;

    if (x1 < minX) minX = x1;
    if (x2 < minX) minX = x2;
    if (y1 < minY) minY = y1;
    if (y2 < minY) minY = y2;

    if (x1 > maxX) maxX = x1;
    if (x2 > maxX) maxX = x2;
    if (y1 > maxY) maxY = y1;
    if (y2 > maxY) maxY = y2;

    if (minX < 0) minX = 0;
    if (minY < 0) minY = 0;
    if (maxX >= SCREEN_W) maxX = SCREEN_W - 1;
    if (maxY >= SCREEN_H) maxY = SCREEN_H - 1;

    int area = ((x1 - x0) * (y2 - y0)) - ((y1 - y0) * (x2 - x0));
    if (area == 0) {
        return;
    }

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {

            int w0 = ((x1 - x0) * (y - y0)) - ((y1 - y0) * (x - x0));
            int w1 = ((x2 - x1) * (y - y1)) - ((y2 - y1) * (x - x1));
            int w2 = ((x0 - x2) * (y - y2)) - ((y0 - y2) * (x - x2));

            if (area > 0) {
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    putPixel(x, y, color);
                }
            } else {
                if (w0 <= 0 && w1 <= 0 && w2 <= 0) {
                    putPixel(x, y, color);
                }
            }
        }
    }
}

uint32_t darken(uint32_t c, float f)
{
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8)  & 0xFF;
    uint8_t b = (c >> 0)  & 0xFF;

    r = (uint8_t)(r * f);
    g = (uint8_t)(g * f);
    b = (uint8_t)(b * f);

    return (0xFF<<24) | (r<<16) | (g<<8) | b;
}



void drawRect(int x, int y, int w, int h, uint8_t col){
    for(int tx=0; tx < w; tx++){
        for(int ty=0; ty < h; ty++){
            putPixel(tx + x, ty + y, col);
        }
    }



}