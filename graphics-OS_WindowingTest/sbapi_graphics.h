#ifndef SBAPI_GRAPHICS_H
#define SBAPI_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// this is the "hardware level" code.
#define     SCR_WIDTH   480
#define     SCR_HEIGHT  320

//#define     SCR_WIDTH   1920
//#define     SCR_HEIGHT  960

//#define     SCR_WIDTH   800
//#define     SCR_HEIGHT  480


#define     SCR_STRIDE  SCR_HEIGHT

#define     SCR_RAMSIZE (SCR_WIDTH * SCR_HEIGHT)

extern uint8_t current_fr_colour;
extern uint8_t current_bk_colour;

extern uint8_t PROJ_VRAM[SCR_RAMSIZE];
extern uint32_t PROJ_CRAM[256];
extern unsigned char clut_cycle_index[256];
void dopalletecycle();

void sbgfx_fill(uint8_t colour);
void sbgfx_drawbox(int x, int y, int w, int h, uint8_t col);
void gfx_setcolour(unsigned char col);
void draw_text816(int x, int y, const unsigned char* textptr);

void sbgfx_pixel(int16_t x, int16_t y, uint8_t col);
void sbgfx_ppixel(int16_t x, int16_t y);    // raw pixel draw
void sbgfx_glyph(int16_t x, int16_t y, uint8_t *src);

void sbgfx_drawhline(int x, int y, int w);
void sbgfx_drawvline(int x, int y, int w);

#endif // SBAPI_GRAPHICS_H
