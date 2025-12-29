#ifndef FASTRAM_H
#define FASTRAM_H

#include <stdint.h>


typedef struct FastBlk {
    uint32_t size;      // payload size in bytes (aligned)
    uint32_t next;      // offset of next block header from FAUXRAM base, 0 = end
    uint32_t flags;     // 0 = free, 1 = used
    uint32_t magic;     // debug / sanity
} FastBlk;

typedef struct {
    uint32_t total;
    uint32_t used_payload;
    uint32_t free_payload;
    uint32_t overhead_bytes;   // headers only
    uint32_t largest_free;
    uint32_t blocks;
    uint32_t used_space;        // literally the entire area used
} FastStats;

#define  fastAllocFail   0
#define  fastAllocOK     1


void      initFastRam(void);     // must launch this before using it
void  *   fastAlloc  (uint32_t bytes);
void      fastFree   (void* p);
void  *   fastRealloc(void* p, uint32_t newSize);

FastStats fastStats(void);
void      fastDump(void);
void      fastDumpHex(uint32_t showsize);

uint32_t  getMemAvail();
uint32_t  getMemTotal();

#endif // FASTRAM_H
