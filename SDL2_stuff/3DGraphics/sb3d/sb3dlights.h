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

#endif