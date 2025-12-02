#ifndef STM32RAM_H
#define STM32RAM_H
//////////////////////////////////////////////////////////////////////////////////////////////////////////



#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SYSRAM_SIZE     (4 * 1024 * 1024)

extern uint8_t SYSRAM[SYSRAM_SIZE]; // the virtual memory

extern unsigned char  duunmap_types[64][64];			// world data types
extern unsigned char  duunmap_textureid[4][64][64];	// textures: floor+wall(side a)/ walls(side b)/ ceiling
extern signed   short duunmap_lm[64][64];				// light levels
extern uint8_t        textures[48][64 * 64];
extern uint8_t        spriteTextures[32][32*32];












//////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // STM32RAM_H
