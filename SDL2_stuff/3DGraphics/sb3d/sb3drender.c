#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "../gfx.h"

#include "sb3d.h"

#define USE_BACKFACE_CULL 1


static Entity *renderEntities[WORLD_MAX];
static RenderTri g_renderTris[MAX_RENDER_TRIS];
static int g_renderTriCount = 0;

//// ZORDER BUFFER ////
uint16_t g_depthBufferBand[SCREEN_W * ZBUF_BAND_H];
void resetDepthBuffer(void)
{
    for (int i = 0; i < SCREEN_W * ZBUF_BAND_H; i++)
        g_depthBufferBand[i] = 65535; // farthest
}
void resetDepthBufferBand(void)
{
    for (int i = 0; i < SCREEN_W * ZBUF_BAND_H; i++) {
        g_depthBufferBand[i] = 65535;
    }
}


static inline uint16_t encodeZ(float z, const Camera *cam)
{
    // Map nearPlane..farPlane -> 0..65535
    float t = (z - cam->nearPlane) / (cam->farPlane - cam->nearPlane);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (uint16_t)(t * 65535.0f);
}
static inline uint8_t encodeZ8(float z, const Camera *cam)
{
    float t = (z - cam->nearPlane) / (cam->farPlane - cam->nearPlane);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (uint8_t)(t * 255.0f);
}

// global toggle
static int g_enableZOrdering = 0;
static int g_flatMode        = 0;   // flat mode, use non dithered shaded, triangles
static int g_twoshadeMode    = 0;   // this shades in full dither, but only uses the base colour selected, and black (colour 16)
static int g_wireframe       = 0;   // 


// call this to enable/disable
void setDefaultRenderMode(){
    g_enableZOrdering = 1;
    g_flatMode        = 0;   // flat mode, use non dithered shaded, triangles
    g_twoshadeMode    = 0;   // this shades in full dither, but only uses the base colour selected, and black (colour 16)
    g_wireframe       = 0;   // 
}

void enableZOrdering(int enable){ g_enableZOrdering = enable; }
void enableFlatMode(int en) { g_flatMode = en; }
void enableTwoShade(int en) { g_twoshadeMode = en; }
void enableWireFrame(int en){ g_wireframe = en; }


static Vec3 lerpVec3(Vec3 a, Vec3 b, float t)
{
    Vec3 out;
    out.x = a.x + ((b.x - a.x) * t);
    out.y = a.y + ((b.y - a.y) * t);
    out.z = a.z + ((b.z - a.z) * t);
    return out;
}

static float planeEval(Vec3 p, ClipPlane plane, const Camera *cam)
{
    switch (plane) {
        case PLANE_NEAR:   return p.z - cam->nearPlane;
        case PLANE_LEFT:   return p.x + p.z * ((SCREEN_W * 0.5f) / PROJ_F);
        case PLANE_RIGHT:  return (p.z * ((SCREEN_W * 0.5f) / PROJ_F)) - p.x;
        case PLANE_TOP:    return (p.z * ((SCREEN_H * 0.5f) / PROJ_F)) - p.y;
        case PLANE_BOTTOM: return p.y + p.z * ((SCREEN_H * 0.5f) / PROJ_F);
    }

    return -1.0f;
}

static int pointInsidePlane(Vec3 p, ClipPlane plane, const Camera *cam)
{
    return (planeEval(p, plane, cam) >= 0.0f);
}

static Vec3 intersectPlane(Vec3 a, Vec3 b, ClipPlane plane, const Camera *cam)
{
    float fa = planeEval(a, plane, cam);
    float fb = planeEval(b, plane, cam);
    float t  = fa / (fa - fb);

    return lerpVec3(a, b, t);
}

static int clipPolygonAgainstPlane(Vec3 *inVerts, int inCount, Vec3 *outVerts, ClipPlane plane, const Camera *cam)
{
    int outCount = 0;

    for (int i = 0; i < inCount; i++) {
        Vec3 current = inVerts[i];
        Vec3 prev    = inVerts[(i + inCount - 1) % inCount];

        int currentInside = pointInsidePlane(current, plane, cam);
        int prevInside    = pointInsidePlane(prev, plane, cam);

        if (prevInside && currentInside) {
            outVerts[outCount++] = current;
        }
        else if (prevInside && !currentInside) {
            outVerts[outCount++] = intersectPlane(prev, current, plane, cam);
        }
        else if (!prevInside && currentInside) {
            outVerts[outCount++] = intersectPlane(prev, current, plane, cam);
            outVerts[outCount++] = current;
        }
    }

    return outCount;
}

static int clipTriangleToFrustum(Vec3 a, Vec3 b, Vec3 c, Vec3 *outVerts, const Camera *cam)
{
    Vec3 poly1[CLIP_MAX_VERTS];
    Vec3 poly2[CLIP_MAX_VERTS];
    int count = 3;

    poly1[0] = a;
    poly1[1] = b;
    poly1[2] = c;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_NEAR, cam);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly2, count, poly1, PLANE_LEFT, cam);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_RIGHT, cam);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly2, count, poly1, PLANE_TOP, cam);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_BOTTOM, cam);
    if (count < 3) return 0;

    for (int i = 0; i < count; i++) {
        outVerts[i] = poly2[i];
    }

    return count;
}







static void swapRenderTri(RenderTri *a, RenderTri *b)
{
    RenderTri t = *a;
    *a = *b;
    *b = t;
}

static int compareRenderTri(const void *a, const void *b)
{
    const RenderTri *ra = (const RenderTri *)a;
    const RenderTri *rb = (const RenderTri *)b;

    if (ra->depth < rb->depth) return 1;
    if (ra->depth > rb->depth) return -1;
    return 0;
}

static void sortRenderList(void)
{
    qsort(g_renderTris, g_renderTriCount, sizeof(RenderTri), compareRenderTri);
}

static void sortRenderList_OLD(void)
{
    for (int i = 0; i < g_renderTriCount - 1; i++) {
        for (int j = i + 1; j < g_renderTriCount; j++) {
            if (g_renderTris[i].depth < g_renderTris[j].depth) {
                swapRenderTri(&g_renderTris[i], &g_renderTris[j]);
            }
        }
    }
}

/// sorting
static float entityDepth(const Entity *ent, const Camera *cam)
{
    Vec3 center = ent->pos;
    Vec3 camSpace = worldToCamera(center, *cam);
    return camSpace.z;
}

static void swapEntityPtr(Entity **a, Entity **b)
{
    Entity *t = *a;
    *a = *b;
    *b = t;
}

void sortEntityPointersByDepth(Entity **entities, int count, const Camera *cam)
{
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            float da = entityDepth(entities[i], cam);
            float db = entityDepth(entities[j], cam);

            if (da < db) {
                swapEntityPtr(&entities[i], &entities[j]);
            }
        }
    }
}


///////////////// rendering ////////////////////////////

static int triangleFacingScreen(Vec2 a, Vec2 b, Vec2 c)
{
    int cross =
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x);

    return (cross > 0);
}


static void submitClippedTri(Vec3 a, Vec3 b, Vec3 c, Camera *cam, uint8_t color, uint8_t emission, float shadeF)
{
    Vec2 pa, pb, pc;

    if (!projectPoint(a, cam, &pa)) return;
    if (!projectPoint(b, cam, &pb)) return;
    if (!projectPoint(c, cam, &pc)) return;

    if (!triangleFacingScreen(pa, pb, pc)) {
        return;
    }

    if (g_renderTriCount >= MAX_RENDER_TRIS) {
        return;
    }

    RenderTri rt;
    rt.p0 = pa;
    rt.p1 = pb;
    rt.p2 = pc;
    rt.color  = color;
    rt.emission = emission;
    rt.shadeF = shadeF;
    rt.depth  = (a.z + b.z + c.z) / 3.0f;

    rt.z0 = encodeZ(a.z, cam);
    rt.z1 = encodeZ(b.z, cam);
    rt.z2 = encodeZ(c.z, cam);

    g_renderTris[g_renderTriCount++] = rt;
}


static int computeShadePointLight(Vec3 a, Vec3 b, Vec3 c, Vec3 lightPos)
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

    /* brighter near camera, falls off later */
    float falloff = 1.0f / (1.0f + dist * dist * 0.0002f);

    /* boost light a bit */
    float brightness = d * falloff * 1.8f;

    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    int shade = 4 - (int)(brightness * 4.999f);

    if (shade < 0) shade = 0;
    if (shade > 4) shade = 4;

    return shade;
}

static float computeShadePointLightF(Vec3 a, Vec3 b, Vec3 c, Vec3 lightPos)
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

    float falloff = 1.0f / (1.0f + dist * dist * 0.00012f);
    float brightness = d * falloff * 2.0f;

    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    brightness = sqrtf(brightness);

    /* 0 = brightest shade, 4 = darkest */
    return (1.0f - brightness) * 4.0f;
}

int projectPoint(Vec3 p, const Camera *cam, Vec2 *out)
{
    float fovDeg = 90.0f;
    if (p.z <= cam->nearPlane) {
        return 0;
    }

    // Convert horizontal FOV to focal length in pixels
    float fovRad = fovDeg * (M_PI / 180.0f);
    float f = (SCREEN_W * 0.5f) / tanf(fovRad * 0.5f);

    out->x = (int)lroundf((p.x / p.z) * f) + (SCREEN_W / 2);
    out->y = (int)lroundf((-p.y / p.z) * f) + (SCREEN_H / 2);

    return 1;
}


// fish eye effect
int projectPoint_FishEyeMode(Vec3 p, const Camera *cam, Vec2 *out)
{
    // Fisheye projection
    float fovDeg = 180.0f;
    if (p.z <= cam->nearPlane) return 0;

    // Convert to radians
    float fovRad = fovDeg * (M_PI / 180.0f);

    // Compute angle from forward axis
    float theta = atan2f(p.x, p.z); // horizontal angle
    float phi   = atanf(p.y / sqrtf(p.x*p.x + p.z*p.z)); // vertical angle

    // radius in pixels from screen center
    float r = (SCREEN_W / 2.0f) * (theta / (fovRad / 2.0f));
    float s = (SCREEN_H / 2.0f) * (phi / (fovRad / 2.0f));

    out->x = (int)lroundf((SCREEN_W / 2.0f) + r);
    out->y = (int)lroundf((SCREEN_H / 2.0f) - s);

    // Optional: cull points outside screen bounds
    if (out->x < 0 || out->x >= SCREEN_W || out->y < 0 || out->y >= SCREEN_H) return 0;

    return 1;
}



int clipLineToNearPlane(Vec3 *a, Vec3 *b, const Camera *cam)
{
    int a_in = (a->z >= cam->nearPlane);
    int b_in = (b->z >= cam->nearPlane);

    if (!a_in && !b_in) {
        return 0;
    }

    if (a_in && b_in) {
        return 1;
    }

    float t = (cam->nearPlane - a->z) / (b->z - a->z);

    Vec3 p;
    p.x = a->x + t * (b->x - a->x);
    p.y = a->y + t * (b->y - a->y);
    p.z = cam->nearPlane;

    if (!a_in) {
        *a = p;
    } else {
        *b = p;
    }

    return 1;
}


static float computeTriangleMaterialBrightness(
    Vec3 wa, Vec3 wb, Vec3 wc,
    Vec3 lightPos,
    Vec3 cameraPos,
    const Material *mat
)
{
    Vec3 ab = vec3Sub(wb, wa);
    Vec3 ac = vec3Sub(wc, wa);
    Vec3 n = vec3Normalize(vec3Cross(ab, ac));

    Vec3 center = triangleCenter(wa, wb, wc);

    Vec3 L = vec3Sub(lightPos, center);
    float dist2 = vec3Dot(L, L);
    float dist = sqrtf(dist2);
    if (dist > 0.0001f) {
        L = vec3Scale(L, 1.0f / dist);
    }

    Vec3 V = vec3Sub(cameraPos, center);
    float vlen = sqrtf(vec3Dot(V, V));
    if (vlen > 0.0001f) {
        V = vec3Scale(V, 1.0f / vlen);
    }

    float ndotl = vec3Dot(n, L);
    if (ndotl < 0.0f) ndotl = 0.0f;

    /* cheap distance falloff */
    float falloff = 1.0f / (1.0f + dist2 * 0.00012f);

    /* diffuse */
    float diffuse = ndotl * mat->diffuse;

    /* reflected vector */
    Vec3 R = vec3Sub(vec3Scale(n, 2.0f * vec3Dot(n, L)), L);
    R = vec3Normalize(R);

    float rdotv = vec3Dot(R, V);
    if (rdotv < 0.0f) rdotv = 0.0f;

    float specular = 0.0f;
    if (mat->specularStrength > 0.0f) {
        specular = powf(rdotv, mat->shininess) * mat->specularStrength;
    }

    float brightness =
        mat->ambient +
        ((diffuse + specular) * falloff) +
        mat->emissive;

    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    return brightness;
}


void resetRenderList(void)
{
    g_renderTriCount = 0;
}


static int entityVisible(const Entity *ent, const Camera *cam)
{
    Vec3 center = worldToCamera(ent->pos, *cam);
    float r = ent->mesh->boundsRadius;

    float halfW = (SCREEN_W * 0.5f) / PROJ_F;
    float halfH = (SCREEN_H * 0.5f) / PROJ_F;

    /* far plane */
    if (center.z - r > cam->farPlane) {
        return 0;
    }
    
    /* near plane */
    if (center.z + r < cam->nearPlane) {
        return 0;
    }

    /* left/right planes */
    if (center.x < -(center.z * halfW) - r) {
        return 0;
    }
    if (center.x >  (center.z * halfW) + r) {
        return 0;
    }

    /* top/bottom planes */
    if (center.y < -(center.z * halfH) - r) {
        return 0;
    }
    if (center.y >  (center.z * halfH) + r) {
        return 0;
    }

    return 1;
}


void submitWorldEntities(const Camera *cam)
{
    int visibleCount = 0;

    for (int i = 0; i < worldEntityCount; i++) {
        Entity *ent = &worldEntities[i];

        if (entityVisible(ent, cam)) {
            renderEntities[visibleCount++] = ent;
        }
    }

    sortEntityPointersByDepth(renderEntities, visibleCount, cam);

    for (int i = 0; i < visibleCount; i++) {
        submitEntitySolid(renderEntities[i], cam);
    }
}


int getRenderTriCount(void)
{
    return g_renderTriCount;
}

void Render3D(const Camera *cam)
{
    resetRenderList();
    submitWorldEntities(cam);

    if (g_wireframe) {
        if (g_enableZOrdering) {
            sortRenderList();
        }

        for (int i = 0; i < g_renderTriCount; i++) {
            int shade = (int)(g_renderTris[i].shadeF + 0.5f);
            uint8_t col;

            if (shade < 0) shade = 0;
            if (shade > 4) shade = 4;

            /* keep emissive faces brighter in wireframe too */
            if (g_renderTris[i].emission > 0) {
                float emissiveF = (float)g_renderTris[i].emission / 255.0f;
                int emissiveShade = 4 - (int)(emissiveF * 4.0f + 0.5f);

                if (emissiveShade < 0) emissiveShade = 0;
                if (emissiveShade < shade) shade = emissiveShade;
            }

            col = shadeColor(g_renderTris[i].color, shade);

            drawLine(
                g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                col
            );

            drawLine(
                g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                col
            );

            drawLine(
                g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                col
            );
        }

        return;
    }


    if (!g_enableZOrdering) {
        sortRenderList();

        for (int i = 0; i < g_renderTriCount; i++) {
            if (g_flatMode) {
                int shade = (int)(g_renderTris[i].shadeF + 0.5f);
                if (shade < 0) shade = 0;
                if (shade > 4) shade = 4;

                fillTriangle(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    shadeColor(g_renderTris[i].color, shade)
                );
            }
            else if (g_twoshadeMode) {
                fillTriangleDither2Mode(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    g_renderTris[i].color,
                    g_renderTris[i].shadeF,
                    DITHER_BAYER4X4
                );
            }
            else {
                fillTriangleDither(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    g_renderTris[i].color,
                    g_renderTris[i].shadeF,
                    DITHER_BAYER4X4
                );
            }
        }
        return;
    }

    for (int bandY0 = 0; bandY0 < SCREEN_H; bandY0 += ZBUF_BAND_H) {
        int bandY1 = bandY0 + ZBUF_BAND_H - 1;
        if (bandY1 >= SCREEN_H) {
            bandY1 = SCREEN_H - 1;
        }

        resetDepthBufferBand();

        for (int i = 0; i < g_renderTriCount; i++) {
            int tyMin = g_renderTris[i].p0.y;
            int tyMax = g_renderTris[i].p0.y;

            if (g_renderTris[i].p1.y < tyMin) tyMin = g_renderTris[i].p1.y;
            if (g_renderTris[i].p2.y < tyMin) tyMin = g_renderTris[i].p2.y;
            if (g_renderTris[i].p1.y > tyMax) tyMax = g_renderTris[i].p1.y;
            if (g_renderTris[i].p2.y > tyMax) tyMax = g_renderTris[i].p2.y;

            if (tyMax < bandY0 || tyMin > bandY1) {
                continue;
            }

            if (g_flatMode) {
                fillTriangleZBandFlat(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    g_renderTris[i].z0,
                    g_renderTris[i].z1,
                    g_renderTris[i].z2,
                    g_renderTris[i].color,
                    g_renderTris[i].shadeF,
                    bandY0,
                    bandY1
                );
            }
            else if (g_twoshadeMode) {
                fillTriangleDitherZBandBayer2Mode(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    g_renderTris[i].z0,
                    g_renderTris[i].z1,
                    g_renderTris[i].z2,
                    g_renderTris[i].color,
                    g_renderTris[i].shadeF,
                    bandY0,
                    bandY1
                );
            }
            else {
                fillTriangleDitherZBandBayer(
                    g_renderTris[i].p0.x, g_renderTris[i].p0.y,
                    g_renderTris[i].p1.x, g_renderTris[i].p1.y,
                    g_renderTris[i].p2.x, g_renderTris[i].p2.y,
                    g_renderTris[i].z0,
                    g_renderTris[i].z1,
                    g_renderTris[i].z2,
                    g_renderTris[i].color,
                    g_renderTris[i].shadeF,
                    bandY0,
                    bandY1
                );
            }
        }
    }
}

static int triangleFacingCamera(Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 ab = vec3Sub(b, a);
    Vec3 ac = vec3Sub(c, a);
    Vec3 n  = vec3Cross(ab, ac);

    /* camera is at origin in camera-space */
    float d = vec3Dot(n, a);

    return (d < 0.0f);
}

//#define USE_BACKFACE_CULL 1

void submitEntitySolid(const Entity *ent, const Camera *cam)
{
    for (int i = 0; i < ent->mesh->triCount; i++) {
        Tri t = ent->mesh->tris[i];

        Vec3 wa = entityLocalToWorld(ent, ent->mesh->verts[t.a]);
        Vec3 wb = entityLocalToWorld(ent, ent->mesh->verts[t.b]);
        Vec3 wc = entityLocalToWorld(ent, ent->mesh->verts[t.c]);

        Vec3 ab = vec3Sub(wb, wa);
        Vec3 ac = vec3Sub(wc, wa);
        Vec3 n  = vec3Normalize(vec3Cross(ab, ac));
        Vec3 center = triangleCenter(wa, wb, wc);

        float faceEmission = (float)t.emission / 255.0f;
        float brightness = ent->mesh->material.ambient + ent->mesh->material.emissive;

        Light *lights = getLights();
        int lightCount = getLightCount();

        for (int li = 0; li < lightCount; li++) {
            if (!lights[li].enabled) {
                continue;
            }

            Vec3 L;
            float attenuation = 1.0f;

            if (lights[li].type == LIGHT_POINT) {
                L = vec3Sub(lights[li].pos, center);

                float dist2 = vec3Dot(L, L);
                float dist = sqrtf(dist2);

                if (dist > 0.0001f) {
                    L = vec3Scale(L, 1.0f / dist);
                }

                attenuation = 1.0f / (1.0f + dist2 * 0.00012f);
            }
            else {
                L = vec3Normalize(vec3Scale(lights[li].dir, -1.0f));
                attenuation = 1.0f;
            }

            Vec3 V = vec3Sub(cam->pos, center);
            float vlen = sqrtf(vec3Dot(V, V));
            if (vlen > 0.0001f) {
                V = vec3Scale(V, 1.0f / vlen);
            }

            float ndotl = vec3Dot(n, L);
            if (ndotl < 0.0f) ndotl = 0.0f;

            brightness += ndotl * lights[li].intensity * attenuation * ent->mesh->material.diffuse;

            if (ent->mesh->material.specularStrength > 0.0f && ndotl > 0.0f) {
                Vec3 R = vec3Sub(vec3Scale(n, 2.0f * ndotl), L);
                R = vec3Normalize(R);

                float rdotv = vec3Dot(R, V);
                if (rdotv < 0.0f) rdotv = 0.0f;

                brightness +=
                    powf(rdotv, ent->mesh->material.shininess) *
                    ent->mesh->material.specularStrength *
                    lights[li].intensity *
                    attenuation;
            }
        }

        /* per-face emission acts like a minimum light floor */
        if (brightness < faceEmission) {
            brightness = faceEmission;
        }

        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;

        float shadeF = brightnessToShadeF(brightness);

        Vec3 a = worldToCamera(wa, *cam);
        Vec3 b = worldToCamera(wb, *cam);
        Vec3 c = worldToCamera(wc, *cam);

        if (a.z > cam->farPlane && b.z > cam->farPlane && c.z > cam->farPlane) {
            continue;
        }

    #if USE_BACKFACE_CULL
        if (!triangleFacingCamera(a, b, c)) {
            continue;
        }
    #endif

        Vec3 clipped[CLIP_MAX_VERTS];
        int clippedCount = clipTriangleToFrustum(a, b, c, clipped, cam);

        if (clippedCount < 3) {
            continue;
        }

        for (int k = 1; k < clippedCount - 1; k++) {
            submitClippedTri(
                clipped[0],
                clipped[k],
                clipped[k + 1],
                cam,
                t.color,
                t.emission,
                shadeF
            );
        }
    }
}


void drawWorldLine(Vec3 a, Vec3 b, const Camera *cam, uint8_t color)
{
    Vec2 pa, pb;

    a = worldToCamera(a, *cam);
    b = worldToCamera(b, *cam);

    if (!clipLineToNearPlane(&a, &b, cam)) {
        return;
    }

    if (!projectPoint(a, cam, &pa)) {
        return;
    }

    if (!projectPoint(b, cam, &pb)) {
        return;
    }

    drawLine(pa.x, pa.y, pb.x, pb.y, color);
}



void drawEntity(const Entity *ent, const Camera *cam, uint8_t color)
{
    for (int i = 0; i < ent->mesh->edgeCount; i++) {
        Edge e = ent->mesh->edges[i];

        Vec3 a = entityLocalToWorld(ent, ent->mesh->verts[e.a]);
        Vec3 b = entityLocalToWorld(ent, ent->mesh->verts[e.b]);

        drawWorldLine(a, b, cam, color);
    }
}

void drawEntitySolid(const Entity *ent, const Camera *cam)
{
    submitEntitySolid(ent, cam);
}




