#ifndef FASTRAM_H
#define FASTRAM_H

#include <stdint.h>

typedef struct {
    uint32_t total;
    uint32_t used_bytes;
    uint32_t free_bytes;
    uint32_t largest_free;
    uint32_t blocks;
} FastStats;



void initFastRam(void);     // must launch this before using it

void  * fastAlloc  (uint32_t bytes);
void    fastFree   (void* p);
void  * fastRealloc(void* p, uint32_t newSize);

FastStats fastStats(void);
void fastDump(void);

void fastDumpHex(void);


#endif // FASTRAM_H
