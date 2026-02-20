#ifndef TFMX_H
#define TFMX_H



#include <stdint.h>


extern uint8_t songmemory[];
extern uint8_t samplememory[];

void openVgmFile(unsigned char *filename);
int playVGM(int *sndbuffer);

void stepVGM(int *sndbuffer, int frames);






#endif
