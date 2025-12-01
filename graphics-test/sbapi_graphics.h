#ifndef SBAPI_GRAPHICS_H
#define SBAPI_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define     SCR_WIDTH   480
#define     SCR_HEIGHT  320
#define     SCR_STRIDE  320

#define     SCR_RAMSIZE (SCR_WIDTH * SCR_HEIGHT)

extern uint8_t current_fr_colour;
extern uint8_t current_bk_colour;

extern uint8_t VRAM[SCR_RAMSIZE];
extern uint32_t CRAM[256];
extern unsigned char clut_cycle_index[256];
void dopalletecycle();

void sbgfx_fill(uint8_t colour);
void sbgfx_drawbox(int x, int y, int w, int h, uint8_t col);
void gfx_setcolour(unsigned char col);
void draw_text816(int x, int y, const unsigned char* textptr);



#endif // SBAPI_GRAPHICS_H
