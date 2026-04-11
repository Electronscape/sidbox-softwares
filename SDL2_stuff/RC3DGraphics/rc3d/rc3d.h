#ifndef RC3D_H
#define RC3D_H

#include <stdint.h>

#define RC3D_SKYBOX_W          1024
#define RC3D_SKYBOX_H          240




void rc3dInit(void);
void rc3dPreparePalette();
void rc3dSetLightRange(float brightRange, float midRange, float darkRange);
void rc3dLightRange(float brightRange, float midRange, float darkRange);
void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx);
void rc3dRender(void);

int rc3dLoadMapBinary(const char *path);
void rc3dUnloadMapBinary(void);

#endif
