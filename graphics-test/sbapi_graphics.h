#ifndef SBAPI_GRAPHICS_H
#define SBAPI_GRAPHICS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


#define     SCR_WIDTH   480
#define     SCR_HEIGHT  320
#define     SCR_RAMSIZE (SCR_WIDTH * SCR_HEIGHT)



extern uint8_t VRAM[SCR_RAMSIZE];
extern uint32_t CRAM[256];

void sbgfx_fill(uint8_t colour);
void sbgfx_drawbox(int x, int y, int w, int h, uint8_t col);

#endif // SBAPI_GRAPHICS_H
