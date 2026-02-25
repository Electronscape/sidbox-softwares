#include "rom.h"

int rom_load(Rom* rom, const char* path) {
    memset(rom, 0, sizeof(*rom));

    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "rom_load: failed to open %s\n", path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        fprintf(stderr, "rom_load: invalid size\n");
        fclose(f);
        return 0;
    }

    rom->size = (size_t)sz;
    rom->data = (uint8_t*)xmalloc(rom->size);

    if (fread(rom->data, 1, rom->size, f) != rom->size) {
        fprintf(stderr, "rom_load: read failed\n");
        fclose(f);
        rom_free(rom);
        return 0;
    }
    fclose(f);

    // Many MD ROMs are big-endian word data already (68k is big-endian).
    // We keep bytes as-is and do big-endian reads in the bus.

    // Optional: print a tiny header snippet if present
    if (rom->size >= 0x200) {
        char name[49];
        memset(name, 0, sizeof(name));
        memcpy(name, rom->data + 0x150, 48);
        for (int i = 47; i >= 0; --i) {
            if (name[i] == ' ' || name[i] == '\0') name[i] = '\0';
            else break;
        }
        fprintf(stderr, "ROM loaded: %zu bytes, Name: \"%s\"\n", rom->size, name);
    } else {
        fprintf(stderr, "ROM loaded: %zu bytes\n", rom->size);
    }

    return 1;
}

void rom_free(Rom* rom) {
    if (rom->data) free(rom->data);
    memset(rom, 0, sizeof(*rom));
}
