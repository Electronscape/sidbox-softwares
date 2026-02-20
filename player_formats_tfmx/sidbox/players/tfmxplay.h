#ifndef TFMX_H
#define TFMX_H



#include <stdint.h>


extern uint8_t songmemory[];
extern uint8_t samplememory[];



int load_tfmx(char *mfn, char *sfn);
uint8_t openTFMXFile(char *filename);

void TfmxInit();
void StartSong();
void TfmxTakedown();







#endif
