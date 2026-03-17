
#ifndef _GFX_LIB_TEST_H_
#define _GFX_LIB_TEST_H_

#include <stdint.h>


#define SCREEN_TEST 2

#if(SCREEN_TEST == 0)
#define ZOOM 3
#define SCREEN_W  480
#define SCREEN_H  320
#endif

#if(SCREEN_TEST == 1)
#define ZOOM 1
#define SCREEN_W  1900
#define SCREEN_H  1000
#endif

#if(SCREEN_TEST == 2)
#define ZOOM 2
#define SCREEN_W  1900/2
#define SCREEN_H  1000/2
#endif



extern uint32_t clut[256];
extern uint8_t fb[];    // framebuffer (interal)
extern uint32_t pb[];   // the presented buffer for SDL2

typedef enum {
    DITHER_BAYER4X4 = 0,
    DITHER_RANDOM   = 1
} DitherMode;


#define ZBUF_BAND_H  32
extern uint16_t g_depthBufferBand[SCREEN_W * ZBUF_BAND_H];
void resetDepthBufferBand(void);

void drawText(int x, int y, const char *text, uint8_t color);

void videoMemToScreen();// real basic clear screen
void clearScreen(uint8_t colIndex);
void putPixel(int32_t x, int32_t y, uint8_t colIndex);
void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex);


uint8_t shadeColor(uint8_t baseColor, int shade);
void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);
void fillTriangleDither(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t baseColor, float shadeF, DitherMode mode);

// ZORDERED
void fillTriangleDitherZ(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint16_t z0,
    uint16_t z1,
    uint16_t z2,
    uint8_t baseColor,
    float shadeF,
    DitherMode mode
);

void fillTriangleDitherZBayer( int x0, int y0, int x1, int y1, int x2, int y2, uint16_t z0, uint16_t z1, uint16_t z2, uint8_t baseColor, float shadeF);

void resetDepthBufferBand(void);

void fillTriangleDitherZBandBayer(
    int x0, int y0, int x1, int y1, int x2, int y2,
    uint16_t z0, uint16_t z1, uint16_t z2,
    float camz0, float camz1, float camz2,
    uint8_t baseColor, float shadeF, int bandY0, int bandY1);


void fillTriangleZBandFlat(
    int x0, int y0,  int x1, int y1,  int x2, int y2,
    uint16_t z0,  uint16_t z1,  uint16_t z2,
    float camz0, float camz1, float camz2,
    uint8_t baseColor,  float shadeF, int bandY0, int bandY1);


void fillTriangleDitherZBandBayer2Mode(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint16_t z0,
    uint16_t z1,
    uint16_t z2,
    float camz0,
    float camz1,
    float camz2,
    uint8_t baseColor,
    float shadeF,
    int bandY0,
    int bandY1
);

void fillTriangleDither2Mode(
    int x0, int y0,
    int x1, int y1,
    int x2, int y2,
    uint8_t baseColor,
    float shadeF,
    DitherMode mode
);


uint32_t darken(uint32_t c, float f);
void resetRand();


void drawRect(int x, int y, int w, int h, uint8_t col);

#endif