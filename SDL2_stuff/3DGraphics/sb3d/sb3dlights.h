#ifndef LIGHTS_H
#define LIGHTS_H

#include "sb3dworld.h"
#include "sb3dmath.h"

extern int g_lightCount;

Light *getLights(void);
int getLightCount(void);
void clearLights(void);
void lightEnable(uint8_t lightIndex, uint8_t enable);
int addPointLight(Vec3 pos, float intensity, int enabled);
int addDirectionalLight(Vec3 dir, float intensity, int enabled);
void setLightPosition(int index, Vec3 pos);
void setLightDirection(int index, Vec3 dir);
float computeTriangleBrightness(Vec3 a, Vec3 b, Vec3 c);
float brightnessToShadeF(float brightness);

// controls
void setLightIntensity(int index, float bright);


static inline uint32_t lerpColor(uint32_t c1, uint32_t c2, float t);
void buildLightingCLUT(uint32_t *clut, uint32_t *baseColors, int numColors, uint32_t target, float shades[5]);

#endif