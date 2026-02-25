#ifndef ROM_H
#define ROM_H

#include "common.h"

typedef struct Rom {
    uint8_t* data;
    size_t   size;
} Rom;

int  rom_load(Rom* rom, const char* path);
void rom_free(Rom* rom);

#endif
