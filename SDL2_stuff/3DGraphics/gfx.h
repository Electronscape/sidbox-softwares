
#ifndef _GFX_LIB_TEST_H_
#define _GFX_LIB_TEST_H_

#include <stdint.h>




#define SCREEN_W  640
#define SCREEN_H  480


extern uint32_t clut[256];
extern uint8_t fb[];    // framebuffer (interal)
extern uint32_t pb[];   // the presented buffer for SDL2

typedef enum {
    DITHER_BAYER4X4 = 0,
    DITHER_RANDOM   = 1
} DitherMode;

void videoMemToScreen();// real basic clear screen
void clearScreen(uint8_t colIndex);
void putPixel(int32_t x, int32_t y, uint8_t colIndex);
void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex);

void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t color);
void fillTriangleDither(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t baseColor, float shadeF, DitherMode mode);
uint32_t darken(uint32_t c, float f);
void resetRand();


void drawRect(int x, int y, int w, int h, uint8_t col);

#endif