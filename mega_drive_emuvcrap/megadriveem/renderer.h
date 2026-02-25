#ifndef RENDERER_H
#define RENDERER_H

#include "common.h"
#include "vdp.h"

// Writes the VDP framebuffer as a binary PPM (P6).
// Filename should include extension.
int renderer_write_ppm(const char* filename, const Vdp* vdp);

#endif
