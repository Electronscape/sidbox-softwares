#include "bus.h"


static uint32_t g_vdp_w_data16 = 0;
static uint32_t g_vdp_w_ctrl16 = 0;


// Helper: read 16-bit big-endian from a byte array
static inline uint16_t be16(const uint8_t* p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static uint8_t rom_read8(const Rom* rom, uint32_t addr) {
    if (!rom->data || rom->size == 0) return 0xFF;
    if (addr >= rom->size) return 0xFF;
    return rom->data[addr];
}

void bus_init(Bus* bus, int fb_w, int fb_h) {
    memset(bus, 0, sizeof(*bus));
    vdp_init(&bus->vdp, fb_w, fb_h);
    bus_reset(bus);
}

void bus_reset(Bus* bus) {
    memset(bus->ram, 0, sizeof(bus->ram));
    vdp_reset(&bus->vdp);
}

void bus_free(Bus* bus) {
    rom_free(&bus->rom);
    vdp_free(&bus->vdp);
    memset(bus, 0, sizeof(*bus));
}

uint8_t bus_read8(Bus* bus, uint32_t addr)
{
    addr &= 0xFFFFFFu;

    // ROM
    if (addr < bus->rom.size) {
        return bus->rom.data[addr];
    }

    // VDP ports (mirrored)
    if ((addr & 0xE00000u) == 0xC00000u) {
            return 0xFF;
        uint32_t port = addr & 0x1Fu;
        // VDP is word ports; emulate byte read by reading word and selecting byte
        uint16_t w;
        if ((port & 0x1Cu) == 0x00) w = vdp_read_data(&bus->vdp);
        else if ((port & 0x1Cu) == 0x04) w = vdp_read_ctrl(&bus->vdp);
        else w = 0xFFFF;

        return (addr & 1) ? (uint8_t)(w & 0xFFu) : (uint8_t)(w >> 8);
    }

    // Work RAM
    if (addr >= 0xFF0000u) {
        return bus->ram[addr & 0xFFFFu];
    }

    return 0xFF;
}
uint16_t bus_read16(Bus* bus, uint32_t addr)
{
    addr &= 0xFFFFFFu;

    // ROM (big-endian)
    if (addr + 1 < bus->rom.size) {
        const uint8_t* p = &bus->rom.data[addr & ~1u];
        return be16(p);
    }

    // VDP ports (mirrored)
    if ((addr & 0xE00000u) == 0xC00000u) {
        uint32_t port = addr & 0x1Fu;
        if ((port & 0x1Cu) == 0x00) return vdp_read_data(&bus->vdp);
        if ((port & 0x1Cu) == 0x04) return vdp_read_ctrl(&bus->vdp);
        return 0xFFFF;
    }

    // RAM
    if (addr >= 0xFF0000u) {
        uint32_t o = addr & 0xFFFFu;
        return (uint16_t)((bus->ram[o] << 8) | bus->ram[(o + 1) & 0xFFFFu]);
    }

    return 0xFFFF;
}


void bus_write8(Bus* bus, uint32_t addr, uint8_t val)
{
    addr &= 0xFFFFFFu;

    // VDP ports (mirrored)
    if ((addr & 0xE00000u) == 0xC00000u) {
        return;
        uint32_t port = addr & 0x1Fu;

        // The VDP is a 16-bit device. Do NOT read-modify-write (reads have side effects).
        // Common simple behavior: treat byte writes as word writes with the byte duplicated.
        uint16_t w = (uint16_t)((val << 8) | val);

        // normalize: treat 0x00..0x03 as data, 0x04..0x07 as control
        if ((port & 0x1Cu) == 0x00) {
            vdp_write_data(&bus->vdp, w);
            return;
        }
        if ((port & 0x1Cu) == 0x04) {
            vdp_write_ctrl(&bus->vdp, w);
            return;
        }

        return;
    }

    // RAM
    if (addr >= 0xFF0000u) {
        bus->ram[addr & 0xFFFFu] = val;
        return;
    }

    // Ignore writes elsewhere
}

void bus_write16(Bus* bus, uint32_t addr, uint16_t val)
{
    addr &= 0xFFFFFFu;

    // VDP ports (mirrored)
    if ((addr & 0xE00000u) == 0xC00000u) {
        uint32_t port = addr & 0x1Fu;

        // normalize: treat 0x00..0x03 as data, 0x04..0x07 as control
        if ((port & 0x1Cu) == 0x00) {
            g_vdp_w_data16++;
            vdp_write_data(&bus->vdp, val);
            return;
        }
        if ((port & 0x1Cu) == 0x04) {
            g_vdp_w_ctrl16++;
            vdp_write_ctrl(&bus->vdp, val);
            return;
        }

        return;
    }

    // RAM (big-endian)
    if (addr >= 0xFF0000u) {
        uint32_t o = addr & 0xFFFFu;
        bus->ram[o] = (uint8_t)(val >> 8);
        bus->ram[(o + 1) & 0xFFFFu] = (uint8_t)(val & 0xFFu);
        return;
    }

    // Ignore writes elsewhere (ROM/unmapped) for now
}
