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

    brightness = sqrtf(brightness);

    return (1.0f - brightness) * 5.0f;
}


static float computePointLightBrightness(Vec3 a, Vec3 b, Vec3 c, Vec3 lightPos, float intensity)
{
    Vec3 ab = vec3Sub(b, a);
    Vec3 ac = vec3Sub(c, a);
    Vec3 n = vec3Cross(ab, ac);
    n = vec3Normalize(n);

    Vec3 center = triangleCenter(a, b, c);
    Vec3 lightVec = vec3Sub(lightPos, center);

    float dist = sqrtf(vec3Dot(lightVec, lightVec));
    lightVec = vec3Normalize(lightVec);

    float d = vec3Dot(n, lightVec);
    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;

    /* knobs */
    float fullBrightRadius = 100.0f;  /* bright core */
    float dimRadius        = 230.0f;  /* end of first fade */
    float blackRadius      = 520.0f;  /* completely black beyond this */

    float midLevel = 0.25f;           /* brightness level reached at dimRadius */

    float falloff;

    if (dist <= fullBrightRadius) {
        falloff = 1.0f;
    }
    else if (dist <= dimRadius) {
        float t = (dist - fullBrightRadius) / (dimRadius - fullBrightRadius);
        falloff = 1.0f + (midLevel - 1.0f) * t;
    }
    else if (dist <= blackRadius) {
        float t = (dist - dimRadius) / (blackRadius - dimRadius);
        falloff = midLevel * (1.0f - t);
    }
    else {
        falloff = 0.0f;
    }

    float brightness = d * falloff * intensity;

    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    return brightness;
}


static float computeDirectionalLightBrightness(Vec3 a, Vec3 b, Vec3 c, Vec3 lightDir, float intensity)
{
    Vec3 ab = vec3Sub(b, a);
    Vec3 ac = vec3Sub(c, a);
    Vec3 n = vec3Cross(ab, ac);
    n = vec3Normalize(n);

    lightDir = vec3Normalize(lightDir);

    /* lightDir points from light toward world, so flip it for surface-to-light */
    Vec3 toLight = vec3Scale(lightDir, -1.0f);

    float d = vec3Dot(n, toLight);
    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;

    float brightness = d * intensity;

    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    return brightness;
}


float computeTriangleBrightness(Vec3 a, Vec3 b, Vec3 c)
{
    float brightness = 0.0f;

    for (int i = 0; i < g_lightCount; i++) {
        if (!g_lights[i].enabled) {
            continue;
        }

        if (g_lights[i].type == LIGHT_POINT) {
            brightness += computePointLightBrightness(
                a, b, c,
                g_lights[i].pos,
                g_lights[i].intensity
            );
        }
        else if (g_lights[i].type == LIGHT_DIRECTIONAL) {
            brightness += computeDirectionalLightBrightness(
                a, b, c,
                g_lights[i].dir,
                g_lights[i].intensity
            );
        }
    }

    if (brightness > 1.0f) brightness = 1.0f;

    return brightness;
}



Light *getLights(void)
{
    return g_lights;
}

int getLightCount(void)
{
    return g_lightCount;
}

void clearLights(void)
{
    g_lightCount = 0;
}

void lightEnable(uint8_t lightIndex, uint8_t enable){
    if(lightIndex < 0 || lightIndex >= g_lightCount) return;

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
    g_lights[g_lightCount].enabled = enabled;

    g_lightCount++;
    return g_lightCount - 1;
}

int addDirectionalLight(Vec3 dir, float intensity, int enabled)
{
    if (g_lightCount >= MAX_LIGHTS) {
        return -1;
    }

    g_lights[g_lightCount].type = LIGHT_DIRECTIONAL;
    g_lights[g_lightCount].pos = (Vec3){0.0f, 0.0f, 0.0f};
    g_lights[g_lightCount].dir = vec3Normalize(dir);
    g_lights[g_lightCount].intensity = intensity;
    g_lights[g_lightCount].enabled = enabled;

    g_lightCount++;
    return g_lightCount - 1;
}

void setLightPosition(int index, Vec3 pos)
{
    if (index < 0 || index >= g_lightCount) {
        return;
    }

    g_lights[index].pos = pos;
}

void setLightDirection(int index, Vec3 dir)
{
    if (index < 0 || index >= g_lightCount) {
        return;
    }

    g_lights[index].dir = vec3Normalize(dir);
}

void setLightIntensity(int index, float bright)
{
    if (index < 0 || index >= g_lightCount) {
        return;
    }
    g_lights[index].intensity = bright;
}

