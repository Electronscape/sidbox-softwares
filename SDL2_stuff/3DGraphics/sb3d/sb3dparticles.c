// file: particles.c

#include <stdint.h>
#include <math.h>
#include "sb3d.h"


static SB3DQuadParticle g_quadParticles[SB3D_MAX_QUAD_PARTICLES];

void sb3dParticlesClear(void)
{
    for (int i = 0; i < SB3D_MAX_QUAD_PARTICLES; i++) {
        g_quadParticles[i].active = 0;
        g_quadParticles[i].pos = (Vec3){ 0.0f, 0.0f, 0.0f };
        g_quadParticles[i].size = 1.0f;
        g_quadParticles[i].shadeF = 0.0f;
        g_quadParticles[i].lightStrength = 1.0f;
        g_quadParticles[i].color = DEFAULT_COLOUR;
        g_quadParticles[i].emission = 0;
    }
}



int sb3dParticleSpawnQuad(
    Vec3 pos,
    float size,
    uint8_t color,
    float shadeF,
    uint8_t emission,
    float lightStrength
)
{
    if (size <= 0.0f) {
        size = 1.0f;
    }

    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > MAX_PALETTE_SHADE_INDEX) shadeF = MAX_PALETTE_SHADE_INDEX;

    if (lightStrength < 0.0f) lightStrength = 0.0f;
    if (lightStrength > 1.0f) lightStrength = 1.0f;

    for (int i = 0; i < SB3D_MAX_QUAD_PARTICLES; i++) {
        if (!g_quadParticles[i].active) {
            g_quadParticles[i].active = 1;
            g_quadParticles[i].pos = pos;
            g_quadParticles[i].size = size;
            g_quadParticles[i].color = color;
            g_quadParticles[i].shadeF = shadeF;
            g_quadParticles[i].emission = emission;
            g_quadParticles[i].lightStrength = lightStrength;
            return i;
        }
    }

    return -1;
}

void sb3dParticleSetPosition(int id, Vec3 pos)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;
    g_quadParticles[id].pos = pos;
}

void sb3dParticleSetSize(int id, float size)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;
    if (size <= 0.0f) size = 1.0f;
    g_quadParticles[id].size = size;
}

void sb3dParticleSetLightStrength(int id, float lightStrength)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;

    if (lightStrength < 0.0f) lightStrength = 0.0f;
    if (lightStrength > 1.0f) lightStrength = 1.0f;

    g_quadParticles[id].lightStrength = lightStrength;
}

void sb3dParticleSetShade(int id, float shadeF)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;

    if (shadeF < 0.0f) shadeF = 0.0f;
    if (shadeF > MAX_PALETTE_SHADE_INDEX) shadeF = MAX_PALETTE_SHADE_INDEX;

    g_quadParticles[id].shadeF = shadeF;
}

void sb3dParticleSetColor(int id, uint8_t color)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;
    g_quadParticles[id].color = color;
}

void sb3dParticleSetEmission(int id, uint8_t emission)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    if (!g_quadParticles[id].active) return;
    g_quadParticles[id].emission = emission;
}

void sb3dParticleEnable(int id, uint8_t enable)
{
    if (id < 0 || id >= SB3D_MAX_QUAD_PARTICLES) return;
    g_quadParticles[id].active = enable ? 1 : 0;
}



void sb3dParticlesRender(const Camera *cam)
{
    if (!cam) return;

    const float rx = cam->right.x;
    const float ry = cam->right.y;
    const float rz = cam->right.z;

    const float ux = cam->up.x;
    const float uy = cam->up.y;
    const float uz = cam->up.z;

    /* billboard normal faces back toward camera */
    const float nx = -cam->forward.x;
    const float ny = -cam->forward.y;
    const float nz = -cam->forward.z;

    Light *lights = lightsGet();
    const int lightCount = lightsGetCount();

    for (int i = 0; i < SB3D_MAX_QUAD_PARTICLES; i++) {
        const SB3DQuadParticle *p = &g_quadParticles[i];
        float brightness;
        float finalShadeF;

        if (!p->active) continue;
        if (p->size <= 0.0f) continue;

        /*
            Particle lighting:
            cheap billboard lighting using one normal and particle center.
            No specular, no shadows, no material.
        */
        brightness = 0.10f; /* tiny ambient baseline */

        for (int li = 0; li < lightCount; li++) {
            const Light *ls = &lights[li];
            float lx, ly, lz;
            float attenuation = 1.0f;
            float ndotl;

            if (!ls->enabled) continue;

            if (ls->type == LIGHT_POINT) {
                lx = ls->pos.x - p->pos.x;
                ly = ls->pos.y - p->pos.y;
                lz = ls->pos.z - p->pos.z;

                {
                    const float dist2   = (lx * lx) + (ly * ly) + (lz * lz);
                    const float near2   = ls->near   * ls->near;
                    const float beyond2 = ls->beyond * ls->beyond;

                    if (dist2 >= beyond2) {
                        continue;
                    }

                    if (dist2 > 0.000001f) {
                        const float invDist = 1.0f / sqrtf(dist2);
                        const float dist = dist2 * invDist;
                        float tval;

                        lx *= invDist;
                        ly *= invDist;
                        lz *= invDist;

                        if (dist2 <= near2) {
                            attenuation = 1.0f;
                        } else {
                            if (ls->far <= ls->near) {
                                continue;
                            }
                            else if (ls->beyond <= ls->far) {
                                if (dist >= ls->far) {
                                    continue;
                                }

                                tval = (dist - ls->near) / (ls->far - ls->near);
                                if (tval < 0.0f) tval = 0.0f;
                                if (tval > 1.0f) tval = 1.0f;
                                attenuation = 1.0f - tval;
                            }
                            else if (dist <= ls->far) {
                                tval = (dist - ls->near) / (ls->far - ls->near);
                                if (tval < 0.0f) tval = 0.0f;
                                if (tval > 1.0f) tval = 1.0f;
                                attenuation = 1.0f - (tval * 0.75f);
                            } else {
                                tval = (dist - ls->far) / (ls->beyond - ls->far);
                                if (tval < 0.0f) tval = 0.0f;
                                if (tval > 1.0f) tval = 1.0f;
                                attenuation = 0.25f * (1.0f - tval);
                            }

                            if (attenuation <= 0.0f) {
                                continue;
                            }
                        }
                    } else {
                        lx = ly = lz = 0.0f;
                        attenuation = 1.0f;
                    }
                }
            } else {
                lx = -ls->dir.x;
                ly = -ls->dir.y;
                lz = -ls->dir.z;
            }

            ndotl = (nx * lx) + (ny * ly) + (nz * lz);
            if (ndotl <= 0.0f) continue;

            brightness += ndotl * ls->intensity * attenuation;
        }

        /* emission boosts brightness floor */
        {
            const float faceEmission = (float)p->emission / 255.0f;
            if (brightness < faceEmission) brightness = faceEmission;
        }

        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;

        /*
            combine live lighting with programmer-set shade.
            darker of the two would be too harsh, so use lighting result directly.
            If you want manual control to still matter, see note below.
        */
        float litShadeF;
        float s;

        litShadeF = brightnessToShadeF(brightness);
        s = p->lightStrength;

        /*
            s = 0.0 -> use programmer shade only
            s = 1.0 -> use lit shade only
            in-between = blend both
        */
        finalShadeF = (p->shadeF * (1.0f - s)) + (litShadeF * s);

        if (finalShadeF < 0.0f) finalShadeF = 0.0f;
        if (finalShadeF > MAX_PALETTE_SHADE_INDEX) finalShadeF = MAX_PALETTE_SHADE_INDEX;

        {
            const float half = p->size * 0.5f;

            const Vec3 p0 = {
                p->pos.x - (rx * half) + (ux * half),
                p->pos.y - (ry * half) + (uy * half),
                p->pos.z - (rz * half) + (uz * half)
            };

            const Vec3 p1 = {
                p->pos.x + (rx * half) + (ux * half),
                p->pos.y + (ry * half) + (uy * half),
                p->pos.z + (rz * half) + (uz * half)
            };

            const Vec3 p2 = {
                p->pos.x + (rx * half) - (ux * half),
                p->pos.y + (ry * half) - (uy * half),
                p->pos.z + (rz * half) - (uz * half)
            };

            const Vec3 p3 = {
                p->pos.x - (rx * half) - (ux * half),
                p->pos.y - (ry * half) - (uy * half),
                p->pos.z - (rz * half) - (uz * half)
            };

            const Vec3 c0 = worldToCamera(p0, *cam);
            const Vec3 c1 = worldToCamera(p1, *cam);
            const Vec3 c2 = worldToCamera(p2, *cam);
            const Vec3 c3 = worldToCamera(p3, *cam);

            if (c0.z <= cam->nearPlane &&
                c1.z <= cam->nearPlane &&
                c2.z <= cam->nearPlane &&
                c3.z <= cam->nearPlane) {
                continue;
            }

            {
                Vec3 clipped[CLIP_MAX_VERTS];
                int clippedCount;

                clippedCount = clipTriangleToFrustum(c0, c1, c2, clipped, cam);
                if (clippedCount >= 3) {
                    for (int k = 1; k < clippedCount - 1; k++) {
                        submitClippedTri(
                            clipped[0],
                            clipped[k],
                            clipped[k + 1],
                            (Camera *)cam,
                            p->color,
                            p->emission,
                            finalShadeF
                        );
                    }
                }

                clippedCount = clipTriangleToFrustum(c0, c2, c3, clipped, cam);
                if (clippedCount >= 3) {
                    for (int k = 1; k < clippedCount - 1; k++) {
                        submitClippedTri(
                            clipped[0],
                            clipped[k],
                            clipped[k + 1],
                            (Camera *)cam,
                            p->color,
                            p->emission,
                            finalShadeF
                        );
                    }
                }
            }
        }
    }
}