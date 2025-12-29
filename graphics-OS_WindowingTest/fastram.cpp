#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "fastram.h"

#define FASTRAM_SIZE ((1024u * 1024u) * 4u) // 4 MB
static uint8_t FAUXRAM[FASTRAM_SIZE];


#define FAST_ALIGN  8u
#define FAST_MAGIC  0xA5D00117u // LOOK CLOSER ;)  0x1701D0A5   -- USS FASTRAM

static inline uint32_t align_up_u32(uint32_t v, uint32_t a) {
    return (v + (a - 1u)) & ~(a - 1u);
}

typedef struct FastBlk {
    uint32_t size;      // payload size in bytes (aligned)
    uint32_t next;      // offset of next block header from FAUXRAM base, 0 = end
    uint32_t flags;     // 0 = free, 1 = used
    uint32_t magic;     // debug / sanity
} FastBlk;

//_Static_assert(sizeof(FastBlk) == 16, "FastBlk must be 16 bytes");

static uint32_t g_fast_head = 0; // offset of first block header

static inline FastBlk* blk_from_off(uint32_t off) {
    return (FastBlk*)(void*)(FAUXRAM + off);
}

static inline uint32_t off_from_blk(FastBlk* b) {
    return (uint32_t)((uint8_t*)(void*)b - FAUXRAM);
}

static inline void* payload_from_blk(FastBlk* b) {
    return (void*)((uint8_t*)(void*)b + sizeof(FastBlk));
}

static inline FastBlk* blk_from_payload(void* p) {
    return (FastBlk*)((uint8_t*)p - sizeof(FastBlk));
}

// Create one big free block covering the entire FAUXRAM.
void initFastRam(void) {
    g_fast_head = 0;

    FastBlk* h = blk_from_off(g_fast_head);
    h->size  = (uint32_t)(FASTRAM_SIZE - sizeof(FastBlk));
    h->next  = 0;
    h->flags = 0; // free
    h->magic = FAST_MAGIC;
}

// Internal: split block if there is enough room to form a new block.
static void split_block(FastBlk* b, uint32_t want_size) {
    // b is free and b->size >= want_size
    uint32_t remaining = b->size - want_size;

    // Need room for a new header + at least FAST_ALIGN bytes payload to be useful
    if (remaining < (uint32_t)(sizeof(FastBlk) + FAST_ALIGN)) {
        return; // don't split
    }

    uint32_t b_off = off_from_blk(b);
    uint32_t new_off = b_off + (uint32_t)sizeof(FastBlk) + want_size;

    FastBlk* nb = blk_from_off(new_off);
    nb->size  = remaining - (uint32_t)sizeof(FastBlk);
    nb->next  = b->next;
    nb->flags = 0; // free
    nb->magic = FAST_MAGIC;

    b->size = want_size;
    b->next = new_off;
}

// Allocate bytes from FAUXRAM, returns pointer to payload or NULL.
void* fastAlloc(uint32_t bytes) {
    if (bytes == 0) return NULL;

    uint32_t want = align_up_u32(bytes, FAST_ALIGN);

    uint32_t off = g_fast_head;
    while (off != 0 || off == g_fast_head) {
        FastBlk* b = blk_from_off(off);

        if (b->magic != FAST_MAGIC) {
            // corruption / misuse
            return NULL;
        }

        if (b->flags == 0 && b->size >= want) {
            split_block(b, want);
            b->flags = 1;

            // Optional debug fill:
            // memset(payload_from_blk(b), 0xCD, b->size);

            return payload_from_blk(b);
        }

        if (b->next == 0) break;
        off = b->next;
    }

    return NULL;
}

// Internal: coalesce b with its next if both are free and adjacent.
static void coalesce_next(FastBlk* b) {
    if (!b || b->next == 0) return;

    FastBlk* n = blk_from_off(b->next);
    if (b->magic != FAST_MAGIC || n->magic != FAST_MAGIC) return;

    if (b->flags != 0 || n->flags != 0) return; // both must be free

    // Are they physically adjacent? They should be by construction, but we verify.
    uint32_t b_off = off_from_blk(b);
    uint32_t expected_next = b_off + (uint32_t)sizeof(FastBlk) + b->size;
    if (expected_next != b->next) return;

    // Merge: b grows to include next header + payload; link skips n
    b->size = b->size + (uint32_t)sizeof(FastBlk) + n->size;
    b->next = n->next;

    // Optional: poison old header
    n->magic = 0;
}

// Free a previously allocated pointer (must come from fastAlloc).
void fastFree(void* p) {
    if (!p) return;

    // Basic bounds check
    uint8_t* up = (uint8_t*)p;
    if (up < (FAUXRAM + sizeof(FastBlk)) || up >= (FAUXRAM + FASTRAM_SIZE)) {
        return; // invalid pointer
    }

    FastBlk* b = blk_from_payload(p);
    if (b->magic != FAST_MAGIC) {
        return; // invalid / double free / corruption
    }

    b->flags = 0;

    // Optional debug fill:
    memset(payload_from_blk(b), 0xDD, b->size);

    // Coalesce forward as much as possible
    while (b->next != 0) {
        uint32_t before = b->next;
        coalesce_next(b);
        if (b->next == before) break;
    }

    // Coalesce backward: need to scan from head to find previous
    uint32_t off = g_fast_head;
    FastBlk* prev = NULL;
    while (off != 0 || off == g_fast_head) {
        FastBlk* cur = blk_from_off(off);
        if (cur->next == off_from_blk(b)) { prev = cur; break; }
        if (cur->next == 0) break;
        off = cur->next;
    }
    if (prev) {
        coalesce_next(prev);
    }
}






void* fastRealloc(void* p, uint32_t newSize) {
    if (!p) {
        return fastAlloc(newSize);
    }

    if (newSize == 0) {
        fastFree(p);
        return NULL;
    }

    FastBlk* b = blk_from_payload(p);
    if (b->magic != FAST_MAGIC) {
        return NULL; // corruption / invalid pointer
    }

    uint32_t want = align_up_u32(newSize, FAST_ALIGN);

    // Case 1: shrinking
    if (want <= b->size) {
        split_block(b, want);
        return p;
    }

    // Case 2: try to grow in place by stealing next block
    if (b->next != 0) {
        FastBlk* n = blk_from_off(b->next);
        if (n->magic == FAST_MAGIC && n->flags == 0) {

            uint32_t combined =
                b->size + (uint32_t)sizeof(FastBlk) + n->size;

            if (combined >= want) {
                // Merge b + n
                b->size = combined;
                b->next = n->next;

                // Now split to exact size if needed
                split_block(b, want);
                return p;
            }
        }
    }

    // Case 3: must move
    void* np = fastAlloc(want);
    if (!np) {
        return NULL; // IMPORTANT: old block is untouched
    }

    // Copy only old payload size
    uint32_t copy = b->size < want ? b->size : want;
    for (uint32_t i = 0; i < copy; i++) {
        ((uint8_t*)np)[i] = ((uint8_t*)p)[i];
    }

    fastFree(p);
    return np;
}















// Stats: free bytes + largest free block + used bytes.

FastStats fastStats(void) {
    FastStats s = {0};
    s.total = FASTRAM_SIZE;

    uint32_t off = g_fast_head;
    while (off != 0 || off == g_fast_head) {
        FastBlk* b = blk_from_off(off);
        if (b->magic != FAST_MAGIC) break;

        s.blocks++;
        if (b->flags) s.used_bytes += b->size;
        else {
            s.free_bytes += b->size;
            if (b->size > s.largest_free) s.largest_free = b->size;
        }

        if (b->next == 0) break;
        off = b->next;
    }
    return s;
}



void fastDump(void) {
    FastStats s = fastStats();
    printf("\n\n-------------------------------------------------------------\n");
    printf("[FASTRAM] total=%u used=%u free=%u largest_free=%u blocks=%u\n",
           s.total, s.used_bytes, s.free_bytes, s.largest_free, s.blocks);
}


void fastDumpHex(void){
    const uint32_t width = 32;
    for (uint32_t off = 0; off < 1024; off += width) {

        // Offset
        printf("%08X  ", off);

        // Hex bytes
        for (uint32_t i = 0; i < width; i++) {
            if (off + i < FASTRAM_SIZE)
                printf("%02X ", FAUXRAM[off + i]);
            else
                printf("   ");
        }

        printf(" |");

        // ASCII view
        for (uint32_t i = 0; i < width; i++) {
            if (off + i < FASTRAM_SIZE) {
                uint8_t c = FAUXRAM[off + i];
                printf("%c", isprint(c) ? c : '.');
            }
        }

        printf("|\n");
    }
}
