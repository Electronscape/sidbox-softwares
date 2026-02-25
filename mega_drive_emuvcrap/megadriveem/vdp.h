#ifndef VDP_H
#define VDP_H

#include "common.h"

#define VDP_VRAM_SIZE  (64 * 1024)
#define VDP_CRAM_WORDS 64
#define VDP_VSRAM_WORDS 40

typedef struct Vdp {
    uint8_t  vram[VDP_VRAM_SIZE];
    uint16_t cram[VDP_CRAM_WORDS];
    uint16_t vsram[VDP_VSRAM_WORDS];

    uint8_t  regs[0x20];

    // Control port state
    uint16_t pending_ctrl;
    int      ctrl_latch; // 0 = expecting first word, 1 = expecting second word

    // Current command / address
    uint32_t addr;       // 16-bit addr for VRAM/CRAM/VSRAM; stored in 32 for convenience
    uint8_t  code;       // command code bits
    uint8_t  target;     // 0=VRAM, 1=CRAM, 2=VSRAM (simplified)
    uint8_t  autoinc;    // from reg 0x0F typically

    // Output framebuffer (RGB888)
    int fb_w, fb_h;
    uint32_t* framebuffer;
} Vdp;

void vdp_init(Vdp* vdp, int fb_w, int fb_h);
void vdp_reset(Vdp* vdp);
void vdp_free(Vdp* vdp);

// Memory-mapped ports:
// 0xC00000 data (16-bit)
// 0xC00004 control (16-bit)
uint16_t vdp_read_data(Vdp* vdp);
uint16_t vdp_read_ctrl(Vdp* vdp);
void     vdp_write_data(Vdp* vdp, uint16_t value);
void     vdp_write_ctrl(Vdp* vdp, uint16_t value);

void vdp_render_frame(Vdp* vdp); // minimal: fills with CRAM[0] color

#endif
