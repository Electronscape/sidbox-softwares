#ifndef RC3D_H
#define RC3D_H

#include <stdint.h>

#define RC3D_SKYBOX_W          1024
#define RC3D_SKYBOX_H          240
#define RC3D_MAX_SPRITES       64
#define RC3D_INVALID_SPRITE    (-1)
#define RC3D_SPRITE_TEX_MAN    253

extern int g_viewport_top;
extern int g_viewport_left;
extern int g_viewport_width;
extern int g_viewport_height;
extern float g_draw_distance;



void rc3dSetViewport(int left, int top, int width, int height);
void rc3dResetViewport(void);
void rc3dSetDrawDistance(float distance);
void rc3dInit(void);
void rc3dPreparePalette();
void rc3dSetLightRange(float brightRange, float midRange, float darkRange);
void rc3dLightRange(float brightRange, float midRange, float darkRange);
void rc3dClearSprites(void);
int rc3dSpriteCreate(float x, float y, float width, float height, uint8_t texId);
void rc3dSpriteDestroy(int spriteId);
void rc3dSpriteSetActive(int spriteId, int active);
void rc3dSpriteSetPosition(int spriteId, float x, float y);
void rc3dSpriteSetPositionFixed(int spriteId, int32_t xFixed, int32_t yFixed);
void rc3dSpriteSetSize(int spriteId, float width, float height);
void rc3dSpriteSetTexture(int spriteId, uint8_t texId);
void rc3dSpriteSetBaseZ(int spriteId, float baseZ);
void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx);
void rc3dRender(void);

int rc3dLoadMapBinary(const char *path);
void rc3dUnloadMapBinary(void);

#endif
