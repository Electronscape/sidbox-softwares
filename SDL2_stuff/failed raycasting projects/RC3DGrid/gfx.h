
#ifndef _GFX_LIB_TEST_H_
#define _GFX_LIB_TEST_H_

#include <stdint.h>


#ifndef SCREEN_W
#define SCREEN_W 160
#endif

#ifndef SCREEN_H
#define SCREEN_H 120
#endif

#ifndef ZOOM
#define ZOOM 4
#endif

extern uint8_t fb[SCREEN_W * SCREEN_H];
extern uint32_t pb[SCREEN_W * SCREEN_H];
extern uint32_t clut[256];

void videoMemToScreen(void);
void clearScreen(uint8_t colIndex);
void putPixel(int32_t x, int32_t y, uint8_t colIndex);
void drawLine(int x0, int y0, int x1, int y1, uint8_t colorIndex);
void drawRect(int x, int y, int w, int h, uint8_t col);
void drawChar(int x, int y, char c, uint8_t color);
void drawText(int x, int y, const char *text, uint8_t color);

#endif