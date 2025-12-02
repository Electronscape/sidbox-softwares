#include "stm32ram.h"

// 4MEG system ram for testing, Sidbox has 6MEG of user RAM, 2 is for the OS and API, and 1 MEG for the Firmware stack

uint8_t SYSRAM[SYSRAM_SIZE];



#ifdef __linux__
unsigned char  duunmap_types[64][64];			// world data types
unsigned char  duunmap_textureid[4][64][64];	// textures: floor+wall(side a)/ walls(side b)/ ceiling
signed   short duunmap_lm[64][64];				// light levels
uint8_t        textures[48][64 * 64];
uint8_t        spriteTextures[32][32*32];
#endif
