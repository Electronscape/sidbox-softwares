#ifndef LIGHTS_H
#define LIGHTS_H

#include "sb3dworld.h"
#include "sb3dmath.h"

extern int g_lightCount;

Light *lightsGet(void);
int lightsGetCount(void);
void lightsClear(void);


void lightEnable(uint8_t lightIndex, uint8_t enable);


int addPointLight(Vec3 pos, float intensity, int enabled);
int addDirectionalLight(Vec3 dir, float intensity, int enabled);


void lightSetPosition(int index, Vec3 pos);
void lightSetDirection(int index, Vec3 dir);
void lightSetIntensity(int index, float bright);

float brightnessToShadeF(float brightness);

// controls
void buildLightingCLUT(uint32_t *clut, uint32_t *baseColors, int numColors, uint32_t target, float shades[5]);

#endif