#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))
#endif

static inline void* xmalloc(size_t n) {
    void* p = malloc(n);
    if (!p) { fprintf(stderr, "OOM allocating %zu bytes\n", n); exit(1); }
    return p;
}

static inline uint16_t bswap16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static inline uint32_t bswap32(uint32_t v) {
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
}


#ifdef __cplusplus
}
#endif
#endif
