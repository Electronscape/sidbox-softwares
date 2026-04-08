#ifndef RC3D_H
#define RC3D_H

#include <stdint.h>

void rc3dInit(void);
void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx);
void rc3dRender(void);

int rc3dLoadMapBinary(const char *path);
void rc3dUnloadMapBinary(void);

#endif