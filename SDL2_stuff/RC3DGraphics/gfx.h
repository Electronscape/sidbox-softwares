
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

#if(SCREEN_TEST == 3)
#define ZOOM 2
#define SCREEN_W  800
#define SCREEN_H  480
#endif



extern uint32_t clut[256];
extern uint8_t fb[];    // framebuffer (interal)
extern uint32_t pb[];   // the presented buffer for SDL2


#define FB_INDEX(x, y) (((x) * SCREEN_H) + (y))

void drawText(int x, int y, const char *text, uint8_t color);

void videoMemToScreen();// real basic clear screen
void clearScreen(uint8_t colIndex);
void putPixel(int32_t x, int32_t y, uint8_t colIndex);
void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex);


void resetRand();

void drawRect(int x, int y, int w, int h, uint8_t col);

#endif