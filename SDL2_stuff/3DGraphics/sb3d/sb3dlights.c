#include <stdint.h>
#include <math.h>

#include "sb3d.h"

static Light     g_lights[MAX_LIGHTS];
int g_lightCount = 0;

// linearly interpolate between two colors
static inline uint32_t lerpColor(uint32_t c1, uint32_t c2, float t)
{
    uint8_t r1 = (c1 >> 24) & 0xFF;
    uint8_t g1 = (c1 >> 16) & 0xFF;
    uint8_t b1 = (c1 >> 8)  & 0xFF;
    uint8_t a1 = (c1)       & 0xFF;

    uint8_t r2 = (c2 >> 24) & 0xFF;
    uint8_t g2 = (c2 >> 16) & 0xFF;
    uint8_t b2 = (c2 >> 8)  & 0xFF;
    uint8_t a2 = (c2)       & 0xFF;

    uint8_t r = r1 + (uint8_t)((r2 - r1) * t);
    uint8_t g = g1 + (uint8_t)((g2 - g1) * t);
    uint8_t b = b1 + (uint8_t)((b2 - b1) * t);
    uint8_t a = a1 + (uint8_t)((a2 - a1) * t);

    return (r << 24) | (g << 16) | (b << 8) | a;
}

// generate a CLUT from baseColors towards a target colour
void buildLightingCLUT(uint32_t *clut, uint32_t *baseColors, int numColors, uint32_t target, float shades[5])
{
    // palette starts at COLOUR_OFFSET
    for (int ci = 0; ci < numColors; ci++) {
        for (int s = 0; s < 5; s++) {
            float t = 1.0f - shades[s]; // 1.0 = fully target, 0 = fully base
            clut[COLOUR_OFFSET + (s * numColors) + ci] = lerpColor(baseColors[ci], target, t);
        }
    }
}



float brightnessToShadeF(float brightness)
{
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    //brightness = sqrtf(brightness);
    brightness = brightness * (1.5f - (0.5f * brightness));

    return (1.0f - brightness) * 5.0f;
}




void lightSetRanges(int lightId, float near, float far, float beyond)
{
    if (lightId < 0 || lightId >= g_lightCount) return;
    g_lights[lightId].near   = near;
    g_lights[lightId].far    = far;
    g_lights[lightId].beyond = beyond;
}


Light *lightsGet(void)
{
    return g_lights;
}

int lightsGetCount(void)
{
    return g_lightCount;
}

void lightsClear(void)
{
    g_lightCount = 0;
}

void lightEnable(uint8_t lightIndex, uint8_t enable){
    if(lightIndex >= g_lightCount) return;

    g_lights[lightIndex].enabled = enable;
}

int addPointLight(Vec3 pos, float intensity, int enabled)
{
    if (g_lightCount >= MAX_LIGHTS) {
        return -1;
    }

    g_lights[g_lightCount].type = LIGHT_POINT;
    g_lights[g_lightCount].pos = pos;
    g_lights[g_lightCount].dir = (Vec3){0.0f, 0.0f, 0.0f};
    g_lights[g_lightCount].intensity = intensity;
    g_lights[g_lightCount].near = 100.0f;
    g_lights[g_lightCount].far = 230.0f;
    g_lights[g_lightCount].beyond = 520.0f;

    g_lights[g_lightCount].enabled = enabled;

    g_lightCount++;
    return g_lightCount - 1;
}


int addDirectionalLight(Vec3 dir, float intensity, int enabled)
{
    int id = g_lightCount;
    if (id >= MAX_LIGHTS) return -1;

    g_lights[id].type = LIGHT_DIRECTIONAL;
    g_lights[id].pos = (Vec3){0.0f, 0.0f, 0.0f};
    g_lights[id].dir = vec3Normalize(dir);
    g_lights[id].intensity = intensity;
    g_lights[id].enabled = enabled;
    g_lightCount++;
    return id;
}









void lightSetPosition(int index, Vec3 pos)
{
    if (index < 0 || index >= g_lightCount) {
        return;
    }

    g_lights[index].pos = pos;
}

void setLightDirection(int index, Vec3 dir)
{
    if (index < 0 || index >= g_lightCount) return;
    g_lights[index].dir = vec3Normalize(dir);
}


void lightSetIntensity(int index, float bright)
{
    if (index < 0 || index >= g_lightCount) {
        return;
    }
    g_lights[index].intensity = bright;
}

