#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../gfx.h"
#include "sb3d.h"

#define USE_BACKFACE_CULL 1

static Entity *renderEntities[WORLD_MAX];
static RenderTri g_renderTris[MAX_RENDER_TRIS];
static int g_renderTriCount = 0;

uint16_t g_depthBufferBand[SCREEN_W * ZBUF_BAND_H];

static int g_enableZOrdering = 0;
static int g_flatMode        = 0;
static int g_twoshadeMode    = 0;
static int g_wireframe       = 0;




static inline float sb3d_proj_f(void)
{
    const float fovRad = 90.0f * (float)(M_PI / 180.0f);
    return (SCREEN_W * 0.5f) / tanf(fovRad * 0.5f);
}

static inline Vec3 lerpVec3(Vec3 a, Vec3 b, float t)
{
    Vec3 out;
    out.x = a.x + ((b.x - a.x) * t);
    out.y = a.y + ((b.y - a.y) * t);
    out.z = a.z + ((b.z - a.z) * t);
    return out;
}

void resetDepthBuffer(void)
{
    memset(g_depthBufferBand, 0xFF, sizeof(g_depthBufferBand));
}

void resetDepthBufferBand(void)
{
    memset(g_depthBufferBand, 0xFF, sizeof(g_depthBufferBand));
}

static inline uint16_t encodeZ(float z, const Camera *cam)
{
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

void setDefaultRenderMode(void)
{
    g_enableZOrdering = 1;
    g_flatMode        = 0;
    g_twoshadeMode    = 0;
    g_wireframe       = 0;
}

void enableZOrdering(int enable) { g_enableZOrdering = enable; }
void enableFlatMode(int en)      { g_flatMode = en; }
void enableTwoShade(int en)      { g_twoshadeMode = en; }
void enableWireFrame(int en)     { g_wireframe = en; }

static inline float planeEval(Vec3 p, ClipPlane plane, const Camera *cam)
{
    const float f = sb3d_proj_f();
    const float halfWOverF = (SCREEN_W * 0.5f) / f;
    const float halfHOverF = (SCREEN_H * 0.5f) / f;

    switch (plane) {
        case PLANE_NEAR:   return p.z - cam->nearPlane;
        case PLANE_LEFT:   return p.x + (p.z * halfWOverF);
        case PLANE_RIGHT:  return (p.z * halfWOverF) - p.x;
        case PLANE_TOP:    return (p.z * halfHOverF) - p.y;
        case PLANE_BOTTOM: return p.y + (p.z * halfHOverF);
    }

    return -1.0f;
}

static inline int pointInsidePlane(Vec3 p, ClipPlane plane, const Camera *cam)
{
    return (planeEval(p, plane, cam) >= 0.0f);
}

static inline Vec3 intersectPlane(Vec3 a, Vec3 b, ClipPlane plane, const Camera *cam)
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
    Vec3 *src = poly1;
    Vec3 *dst = poly2;
    Vec3 *tmp;
    int count = 3;

    src[0] = a;
    src[1] = b;
    src[2] = c;

    count = clipPolygonAgainstPlane(src, count, dst, PLANE_NEAR, cam);
    if (count < 3) return 0;
    tmp = src; src = dst; dst = tmp;

    count = clipPolygonAgainstPlane(src, count, dst, PLANE_LEFT, cam);
    if (count < 3) return 0;
    tmp = src; src = dst; dst = tmp;

    count = clipPolygonAgainstPlane(src, count, dst, PLANE_RIGHT, cam);
    if (count < 3) return 0;
    tmp = src; src = dst; dst = tmp;

    count = clipPolygonAgainstPlane(src, count, dst, PLANE_TOP, cam);
    if (count < 3) return 0;
    tmp = src; src = dst; dst = tmp;

    count = clipPolygonAgainstPlane(src, count, dst, PLANE_BOTTOM, cam);
    if (count < 3) return 0;

    for (int i = 0; i < count; i++) {
        outVerts[i] = dst[i];
    }

    return count;
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

static float entityDepth(const Entity *ent, const Camera *cam)
{
    Vec3 camSpace = worldToCamera(ent->pos, *cam);
    return camSpace.z;
}

void sortEntityPointersByDepth(Entity **entities, int count, const Camera *cam)
{
    (void)entities;
    (void)count;
    (void)cam;
    return;
}

static inline int triangleFacingScreen(Vec2 a, Vec2 b, Vec2 c)
{
    int cross =
        (b.x - a.x) * (c.y - a.y) -
        (b.y - a.y) * (c.x - a.x);

    return (cross > 0);
}

static inline int triangleFacingCamera(Vec3 a, Vec3 b, Vec3 c)
{
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float abz = b.z - a.z;

    const float acx = c.x - a.x;
    const float acy = c.y - a.y;
    const float acz = c.z - a.z;

    const float nx = (aby * acz) - (abz * acy);
    const float ny = (abz * acx) - (abx * acz);
    const float nz = (abx * acy) - (aby * acx);

    const float d = (nx * a.x) + (ny * a.y) + (nz * a.z);
    return (d < 0.0f);
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

    RenderTri *rt = &g_renderTris[g_renderTriCount++];

    rt->p0 = pa;
    rt->p1 = pb;
    rt->p2 = pc;

    rt->color    = color;
    rt->emission = emission;
    rt->shadeF   = shadeF;
    rt->depth    = (a.z + b.z + c.z) * (1.0f / 3.0f);

    rt->z0 = encodeZ(a.z, cam);
    rt->z1 = encodeZ(b.z, cam);
    rt->z2 = encodeZ(c.z, cam);

    rt->camz0 = a.z;
    rt->camz1 = b.z;
    rt->camz2 = c.z;

    rt->minY = pa.y;
    rt->maxY = pa.y;
    if (pb.y < rt->minY) rt->minY = pb.y;
    if (pc.y < rt->minY) rt->minY = pc.y;
    if (pb.y > rt->maxY) rt->maxY = pb.y;
    if (pc.y > rt->maxY) rt->maxY = pc.y;
}

static int computeShadePointLight(Vec3 a, Vec3 b, Vec3 c, Vec3 lightPos)
{
    const float abx = b.x - a.x;
    const float aby = b.y - a.y;
    const float abz = b.z - a.z;

    const float acx = c.x - a.x;
    const float acy = c.y - a.y;
    const float acz = c.z - a.z;

    float nx = (aby * acz) - (abz * acy);
    float ny = (abz * acx) - (abx * acz);
    float nz = (abx * acy) - (aby * acx);

    const float nlen2 = (nx * nx) + (ny * ny) + (nz * nz);
    if (nlen2 > 0.000001f) {
        const float invNLen = 1.0f / sqrtf(nlen2);
        nx *= invNLen;
        ny *= invNLen;
        nz *= invNLen;
    } else {
        nx = ny = nz = 0.0f;
    }

    const float cx = (a.x + b.x + c.x) * (1.0f / 3.0f);
    const float cy = (a.y + b.y + c.y) * (1.0f / 3.0f);
    const float cz = (a.z + b.z + c.z) * (1.0f / 3.0f);

    float lx = lightPos.x - cx;
    float ly = lightPos.y - cy;
    float lz = lightPos.z - cz;

    const float dist2 = (lx * lx) + (ly * ly) + (lz * lz);
    if (dist2 > 0.000001f) {
        const float invDist = 1.0f / sqrtf(dist2);
        lx *= invDist;
        ly *= invDist;
        lz *= invDist;
    } else {
        lx = ly = lz = 0.0f;
    }

    float d = (nx * lx) + (ny * ly) + (nz * lz);
    if (d < 0.0f) d = 0.0f;
    if (d > 1.0f) d = 1.0f;

    float brightness = d * (1.0f / (1.0f + dist2 * 0.0002f)) * 1.8f;
    if (brightness < 0.0f) brightness = 0.0f;
    if (brightness > 1.0f) brightness = 1.0f;

    int shade = 4 - (int)(brightness * 4.999f);
    if (shade < 0) shade = 0;
    if (shade > 4) shade = 4;

    return shade;
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

    float falloff = 1.0f / (1.0f + dist2 * 0.00012f);
    float diffuse = ndotl * mat->diffuse;

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

int projectPoint(Vec3 p, const Camera *cam, Vec2 *out)
{
    if (p.z <= cam->nearPlane) {
        return 0;
    }

    const float f = sb3d_proj_f();
    out->x = (int)lroundf((p.x / p.z) * f) + (SCREEN_W / 2);
    out->y = (int)lroundf((-p.y / p.z) * f) + (SCREEN_H / 2);
    return 1;
}

int clipLineToNearPlane(Vec3 *a, Vec3 *b, const Camera *cam)
{
    int a_in = (a->z >= cam->nearPlane);
    int b_in = (b->z >= cam->nearPlane);

    if (!a_in && !b_in) return 0;
    if (a_in && b_in)   return 1;

    const float t = (cam->nearPlane - a->z) / (b->z - a->z);

    Vec3 p;
    p.x = a->x + t * (b->x - a->x);
    p.y = a->y + t * (b->y - a->y);
    p.z = cam->nearPlane;

    if (!a_in) *a = p;
    else       *b = p;

    return 1;
}

void resetRenderList(void)
{
    g_renderTriCount = 0;
}

static int entityVisible(const Entity *ent, const Camera *cam)
{
    Vec3 center = worldToCamera(ent->pos, *cam);
    float r = ent->mesh->boundsRadius;

    if (center.z - r > cam->farPlane) return 0;
    if (center.z + r < cam->nearPlane) return 0;

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

static uint32_t sb3d_hash2i(int x, int z)
{
    uint32_t h = (uint32_t)x * 374761393u;
    h += (uint32_t)z * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= (h >> 16);
    return h;
}


void drawFakeHorizonDots(const Camera *cam, uint8_t dotCol, int spacing, float ylevel, uint8_t density)
{
    if (!cam) return;
    if (spacing < 2) spacing = 2;

    const int rangeCells = 18;
    const float jitter = spacing * 0.35f;

    const float inv255 = 1.0f / 255.0f;
    const float jitterScale = (2.0f * jitter) * inv255;

    const float f = sb3d_proj_f();
    const float halfW = (float)(SCREEN_W * 0.5f);
    const float halfH = (float)(SCREEN_H * 0.5f);
    const float nearPlane = cam->nearPlane;

    const int baseCellX = (int)floorf(cam->pos.x / (float)spacing);
    const int baseCellZ = (int)floorf(cam->pos.z / (float)spacing);

    const float camPosX = cam->pos.x;
    const float camPosY = cam->pos.y;
    const float camPosZ = cam->pos.z;

    const float rx = cam->right.x;
    const float ry = cam->right.y;
    const float rz = cam->right.z;

    const float ux = cam->up.x;
    const float uy = cam->up.y;
    const float uz = cam->up.z;

    const float fx = cam->forward.x;
    const float fy = cam->forward.y;
    const float fz = cam->forward.z;

    const float yOff = ylevel - camPosY;

    for (int gz = baseCellZ - rangeCells; gz <= baseCellZ + rangeCells; gz++) {
        const float worldBaseZ = (float)(gz * spacing);

        for (int gx = baseCellX - rangeCells; gx <= baseCellX + rangeCells; gx++) {
            const uint32_t h = sb3d_hash2i(gx, gz);

            if ((h & 255u) > density) continue;

            const float jx = ((float)((h >> 8)  & 255u) * jitterScale) - jitter;
            const float jz = ((float)((h >> 16) & 255u) * jitterScale) - jitter;

            const float dx = ((float)(gx * spacing) + jx) - camPosX;
            const float dz = (worldBaseZ + jz) - camPosZ;

            const float camX = (dx * rx) + (yOff * ry) + (dz * rz);
            const float camY = (dx * ux) + (yOff * uy) + (dz * uz);
            const float camZ = (dx * fx) + (yOff * fy) + (dz * fz);

            if (camZ <= nearPlane) continue;

            const float invZ = 1.0f / camZ;
            const int sx = (int)(((camX * f) * invZ) + halfW + 0.5f);
            const int sy = (int)(((-camY * f) * invZ) + halfH + 0.5f);

            if ((unsigned)sx < SCREEN_W && (unsigned)sy < SCREEN_H) {
                putPixel(sx, sy, dotCol);
            }
        }
    }
}

void drawFakeHorizon(const Camera *cam, uint8_t skyCol, uint8_t groundCol, uint8_t lineCol, float ylevel)
{
    if (!cam) return;

    const float f = sb3d_proj_f();
    const float cx = (float)(SCREEN_W * 0.5f);
    const float cy = (float)(SCREEN_H * 0.5f);

    const float Ry = cam->right.y;
    const float Uy = cam->up.y;
    const float Fy = cam->forward.y;

    const float invF = 1.0f / f;
    const float yDelta = -Uy * invF;

    const float leftXTerm  = ((0.0f - cx) * invF) * Ry;
    const float rightXTerm = ((((float)(SCREEN_W - 1) - cx) * invF) * Ry);

    const float groundNumer = ylevel - cam->pos.y;
    const float splitMul = (fabsf(Ry) >= 0.0001f) ? (-f / Ry) : 0.0f;

    float dirCamY = cy * invF;
    float rowTerm = (dirCamY * Uy) + Fy;

    for (int y = 0; y < SCREEN_H; y++) {
        const float leftDirWorldY  = leftXTerm  + rowTerm;
        const float rightDirWorldY = rightXTerm + rowTerm;

        int leftGround  = 0;
        int rightGround = 0;

        if (fabsf(leftDirWorldY) >= 0.0001f) {
            leftGround = ((groundNumer / leftDirWorldY) > 0.0f);
        }
        if (fabsf(rightDirWorldY) >= 0.0001f) {
            rightGround = ((groundNumer / rightDirWorldY) > 0.0f);
        }

        uint8_t *row = &fb[y * SCREEN_W];

        if (leftGround == rightGround) {
            memset(row, leftGround ? groundCol : skyCol, SCREEN_W);
        } else {
            int xSplit;

            if (fabsf(Ry) < 0.0001f) {
                xSplit = SCREEN_W / 2;
            } else {
                const float xSplitF = cx + (rowTerm * splitMul);
                xSplit = (int)lroundf(xSplitF);
            }

            if (xSplit < 0) xSplit = 0;
            if (xSplit > SCREEN_W) xSplit = SCREEN_W;

            if (leftGround) {
                if (xSplit > 0) memset(row, groundCol, xSplit);
                if (xSplit < SCREEN_W) memset(row + xSplit, skyCol, SCREEN_W - xSplit);
            } else {
                if (xSplit > 0) memset(row, skyCol, xSplit);
                if (xSplit < SCREEN_W) memset(row + xSplit, groundCol, SCREEN_W - xSplit);
            }
        }

        dirCamY -= invF;
        rowTerm += yDelta;
    }

    if (fabsf(Uy) >= 0.0001f) {
        int y0 = (int)lroundf(cy + ((((0.0f) - cx) * Ry) + (f * Fy)) / Uy);
        int y1 = (int)lroundf(cy + ((((float)(SCREEN_W - 1) - cx) * Ry) + (f * Fy)) / Uy);
        drawLine(0, y0, SCREEN_W - 1, y1, lineCol);
    }
}


enum {
    CLIP_LEFT   = 1,
    CLIP_RIGHT  = 2,
    CLIP_TOP    = 4,
    CLIP_BOTTOM = 8
};

static int sb3d_computeOutCode(int x, int y)
{
    int code = 0;

    if (x < 0) {
        code |= CLIP_LEFT;
    } else if (x >= SCREEN_W) {
        code |= CLIP_RIGHT;
    }

    if (y < 0) {
        code |= CLIP_TOP;
    } else if (y >= SCREEN_H) {
        code |= CLIP_BOTTOM;
    }

    return code;
}

static int sb3d_clipLineToScreen(int *x0, int *y0, int *x1, int *y1)
{
    int out0 = sb3d_computeOutCode(*x0, *y0);
    int out1 = sb3d_computeOutCode(*x1, *y1);

    while (1) {
        if ((out0 | out1) == 0) {
            return 1;   /* fully accepted */
        }

        if (out0 & out1) {
            return 0;   /* fully rejected */
        }

        {
            int out = out0 ? out0 : out1;
            int x = 0;
            int y = 0;

            if (out & CLIP_TOP) {
                if (*y1 == *y0) return 0;
                x = *x0 + (int)(((int64_t)(*x1 - *x0) * (0 - *y0)) / (*y1 - *y0));
                y = 0;
            }
            else if (out & CLIP_BOTTOM) {
                if (*y1 == *y0) return 0;
                x = *x0 + (int)(((int64_t)(*x1 - *x0) * ((SCREEN_H - 1) - *y0)) / (*y1 - *y0));
                y = SCREEN_H - 1;
            }
            else if (out & CLIP_RIGHT) {
                if (*x1 == *x0) return 0;
                y = *y0 + (int)(((int64_t)(*y1 - *y0) * ((SCREEN_W - 1) - *x0)) / (*x1 - *x0));
                x = SCREEN_W - 1;
            }
            else { /* CLIP_LEFT */
                if (*x1 == *x0) return 0;
                y = *y0 + (int)(((int64_t)(*y1 - *y0) * (0 - *x0)) / (*x1 - *x0));
                x = 0;
            }

            if (out == out0) {
                *x0 = x;
                *y0 = y;
                out0 = sb3d_computeOutCode(*x0, *y0);
            } else {
                *x1 = x;
                *y1 = y;
                out1 = sb3d_computeOutCode(*x1, *y1);
            }
        }
    }
}




void drawFakeHorizonGrid(
    const Camera *cam,
    uint8_t gridCol,
    int spacing,
    float ylevel,
    int rangeCells
)
{
    if (!cam) return;
    if (spacing < 2) spacing = 2;
    if (rangeCells < 1) rangeCells = 1;

    const int padCells = 1;

    const int baseCellX = (int)floorf(cam->pos.x / (float)spacing);
    const int baseCellZ = (int)floorf(cam->pos.z / (float)spacing);

    const int minGX = baseCellX - rangeCells - padCells;
    const int maxGX = baseCellX + rangeCells + padCells;
    const int minGZ = baseCellZ - rangeCells - padCells;
    const int maxGZ = baseCellZ + rangeCells + padCells;

    const float f = sb3d_proj_f();
    const float halfW = (float)(SCREEN_W * 0.5f);
    const float halfH = (float)(SCREEN_H * 0.5f);
    const float nearPlane = cam->nearPlane;

    const float camPosX = cam->pos.x;
    const float camPosY = cam->pos.y;
    const float camPosZ = cam->pos.z;

    const float rx = cam->right.x;
    const float ry = cam->right.y;
    const float rz = cam->right.z;

    const float ux = cam->up.x;
    const float uy = cam->up.y;
    const float uz = cam->up.z;

    const float fx = cam->forward.x;
    const float fy = cam->forward.y;
    const float fz = cam->forward.z;

    const float yOff = ylevel - camPosY;

    const float minWorldX = (float)(minGX * spacing);
    //const float maxWorldX = (float)(maxGX * spacing);
    const float minWorldZ = (float)(minGZ * spacing);
    //const float maxWorldZ = (float)(maxGZ * spacing);

    /* step one grid cell in +X */
    const float stepX_camX = (float)spacing * rx;
    const float stepX_camY = (float)spacing * ux;
    const float stepX_camZ = (float)spacing * fx;

    /* step one grid cell in +Z */
    const float stepZ_camX = (float)spacing * rz;
    const float stepZ_camY = (float)spacing * uz;
    const float stepZ_camZ = (float)spacing * fz;

    /*
        Rows: constant Z, X changes
    */
    for (int gz = minGZ; gz <= maxGZ; gz++) {
        const float wz = (float)(gz * spacing);
        const float dz = wz - camPosZ;
        const float dx0 = minWorldX - camPosX;

        float camX = (dx0 * rx) + (yOff * ry) + (dz * rz);
        float camY = (dx0 * ux) + (yOff * uy) + (dz * uz);
        float camZ = (dx0 * fx) + (yOff * fy) + (dz * fz);

        int havePrev = 0;
        float prevCamX = 0.0f;
        float prevCamY = 0.0f;
        float prevCamZ = 0.0f;

        for (int gx = minGX; gx <= maxGX; gx++) {
            if (havePrev) {
                float ax = prevCamX;
                float ay = prevCamY;
                float az = prevCamZ;

                float bx = camX;
                float by = camY;
                float bz = camZ;

                if (!(az <= nearPlane && bz <= nearPlane)) {
                    if (az <= nearPlane || bz <= nearPlane) {
                        const float t = (nearPlane - az) / (bz - az);
                        const float ix = ax + ((bx - ax) * t);
                        const float iy = ay + ((by - ay) * t);

                        if (az <= nearPlane) {
                            ax = ix;
                            ay = iy;
                            az = nearPlane;
                        } else {
                            bx = ix;
                            by = iy;
                            bz = nearPlane;
                        }
                    }

                    if (az > nearPlane && bz > nearPlane) {
                        int x0 = (int)(((ax * f) / az) + halfW + 0.5f);
                        int y0 = (int)(((-ay * f) / az) + halfH + 0.5f);
                        int x1 = (int)(((bx * f) / bz) + halfW + 0.5f);
                        int y1 = (int)(((-by * f) / bz) + halfH + 0.5f);

                        if (sb3d_clipLineToScreen(&x0, &y0, &x1, &y1)) {
                            drawLine(x0, y0, x1, y1, gridCol);
                        }
                    }
                }
            }

            prevCamX = camX;
            prevCamY = camY;
            prevCamZ = camZ;
            havePrev = 1;

            camX += stepX_camX;
            camY += stepX_camY;
            camZ += stepX_camZ;
        }
    }

    /*
        Columns: constant X, Z changes
    */
    for (int gx = minGX; gx <= maxGX; gx++) {
        const float wx = (float)(gx * spacing);
        const float dx = wx - camPosX;
        const float dz0 = minWorldZ - camPosZ;

        float camX = (dx * rx) + (yOff * ry) + (dz0 * rz);
        float camY = (dx * ux) + (yOff * uy) + (dz0 * uz);
        float camZ = (dx * fx) + (yOff * fy) + (dz0 * fz);

        int havePrev = 0;
        float prevCamX = 0.0f;
        float prevCamY = 0.0f;
        float prevCamZ = 0.0f;

        for (int gz = minGZ; gz <= maxGZ; gz++) {
            if (havePrev) {
                float ax = prevCamX;
                float ay = prevCamY;
                float az = prevCamZ;

                float bx = camX;
                float by = camY;
                float bz = camZ;

                if (!(az <= nearPlane && bz <= nearPlane)) {
                    if (az <= nearPlane || bz <= nearPlane) {
                        const float t = (nearPlane - az) / (bz - az);
                        const float ix = ax + ((bx - ax) * t);
                        const float iy = ay + ((by - ay) * t);

                        if (az <= nearPlane) {
                            ax = ix;
                            ay = iy;
                            az = nearPlane;
                        } else {
                            bx = ix;
                            by = iy;
                            bz = nearPlane;
                        }
                    }

                    if (az > nearPlane && bz > nearPlane) {
                        int x0 = (int)(((ax * f) / az) + halfW + 0.5f);
                        int y0 = (int)(((-ay * f) / az) + halfH + 0.5f);
                        int x1 = (int)(((bx * f) / bz) + halfW + 0.5f);
                        int y1 = (int)(((-by * f) / bz) + halfH + 0.5f);

                        if (sb3d_clipLineToScreen(&x0, &y0, &x1, &y1)) {
                            drawLine(x0, y0, x1, y1, gridCol);
                        }
                    }
                }
            }

            prevCamX = camX;
            prevCamY = camY;
            prevCamZ = camZ;
            havePrev = 1;

            camX += stepZ_camX;
            camY += stepZ_camY;
            camZ += stepZ_camZ;
        }
    }
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
            RenderTri *rt = &g_renderTris[i];
            int shade = (int)(rt->shadeF + 0.5f);

            if (shade < 0) shade = 0;
            if (shade > 4) shade = 4;

            if (rt->emission > 0) {
                const float emissiveF = (float)rt->emission / 255.0f;
                int emissiveShade = 4 - (int)(emissiveF * 4.0f + 0.5f);
                if (emissiveShade < 0) emissiveShade = 0;
                if (emissiveShade < shade) shade = emissiveShade;
            }

            const uint8_t col = shadeColor(rt->color, shade);

            drawLine(rt->p0.x, rt->p0.y, rt->p1.x, rt->p1.y, col);
            drawLine(rt->p1.x, rt->p1.y, rt->p2.x, rt->p2.y, col);
            drawLine(rt->p2.x, rt->p2.y, rt->p0.x, rt->p0.y, col);
        }
        return;
    }

    if (!g_enableZOrdering) {
        sortRenderList();

        if (g_flatMode) {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                int shade = (int)(rt->shadeF + 0.5f);

                if (shade < 0) shade = 0;
                if (shade > 4) shade = 4;

                fillTriangle(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    shadeColor(rt->color, shade)
                );
            }
        }
        else if (g_twoshadeMode) {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                fillTriangleDither2Mode(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    rt->color,
                    rt->shadeF,
                    DITHER_BAYER4X4
                );
            }
        }
        else {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                fillTriangleDither(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    rt->color,
                    rt->shadeF,
                    DITHER_BAYER4X4
                );
            }
        }
        return;
    }

    for (int bandY0 = 0; bandY0 < SCREEN_H; bandY0 += ZBUF_BAND_H) {
        int bandY1 = bandY0 + ZBUF_BAND_H - 1;
        if (bandY1 >= SCREEN_H) bandY1 = SCREEN_H - 1;

        resetDepthBufferBand();

        if (g_flatMode) {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                if (rt->maxY < bandY0 || rt->minY > bandY1) continue;

                fillTriangleZBandFlat(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    rt->z0, rt->z1, rt->z2,
                    rt->camz0, rt->camz1, rt->camz2,
                    rt->color,
                    rt->shadeF,
                    bandY0,
                    bandY1
                );
            }
        }
        else if (g_twoshadeMode) {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                if (rt->maxY < bandY0 || rt->minY > bandY1) continue;

                fillTriangleDitherZBandBayer2Mode(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    rt->z0, rt->z1, rt->z2,
                    rt->camz0, rt->camz1, rt->camz2,
                    rt->color,
                    rt->shadeF,
                    bandY0,
                    bandY1
                );
            }
        }
        else {
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];
                if (rt->maxY < bandY0 || rt->minY > bandY1) continue;

                fillTriangleDitherZBandBayer(
                    rt->p0.x, rt->p0.y,
                    rt->p1.x, rt->p1.y,
                    rt->p2.x, rt->p2.y,
                    rt->z0, rt->z1, rt->z2,
                    rt->camz0, rt->camz1, rt->camz2,
                    rt->color,
                    rt->shadeF,
                    bandY0,
                    bandY1
                );
            }
        }
    }
}

void submitEntitySolid(const Entity *ent, const Camera *cam)
{
    const Mesh *mesh = ent->mesh;
    const Material *mat = &mesh->material;

    Light *lights = getLights();
    const int lightCount = getLightCount();

    const float matAmbient  = mat->ambient;
    const float matEmissive = mat->emissive;
    const float matDiffuse  = mat->diffuse;
    const float matSpec     = mat->specularStrength;
    const float matShiny    = mat->shininess;

    const float camPosX = cam->pos.x;
    const float camPosY = cam->pos.y;
    const float camPosZ = cam->pos.z;

    for (int i = 0; i < mesh->triCount; i++) {
        const Tri t = mesh->tris[i];

        const Vec3 wa = entityLocalToWorld(ent, mesh->verts[t.a]);
        const Vec3 wb = entityLocalToWorld(ent, mesh->verts[t.b]);
        const Vec3 wc = entityLocalToWorld(ent, mesh->verts[t.c]);

        const float abx = wb.x - wa.x;
        const float aby = wb.y - wa.y;
        const float abz = wb.z - wa.z;

        const float acx = wc.x - wa.x;
        const float acy = wc.y - wa.y;
        const float acz = wc.z - wa.z;

        float nx = (aby * acz) - (abz * acy);
        float ny = (abz * acx) - (abx * acz);
        float nz = (abx * acy) - (aby * acx);

        const float nlen2 = (nx * nx) + (ny * ny) + (nz * nz);
        if (nlen2 > 0.000001f) {
            const float invNLen = 1.0f / sqrtf(nlen2);
            nx *= invNLen;
            ny *= invNLen;
            nz *= invNLen;
        } else {
            nx = ny = nz = 0.0f;
        }

        const float cx = (wa.x + wb.x + wc.x) * (1.0f / 3.0f);
        const float cy = (wa.y + wb.y + wc.y) * (1.0f / 3.0f);
        const float cz = (wa.z + wb.z + wc.z) * (1.0f / 3.0f);

        const float faceEmission = (float)t.emission / 255.0f;
        float brightness = matAmbient + matEmissive;

        for (int li = 0; li < lightCount; li++) {
            const Light *ls = &lights[li];
            if (!ls->enabled) continue;

            float lx, ly, lz;
            float attenuation = 1.0f;
            float ndotl;

            if (ls->type == LIGHT_POINT) {
                lx = ls->pos.x - cx;
                ly = ls->pos.y - cy;
                lz = ls->pos.z - cz;

                const float dist2 = (lx * lx) + (ly * ly) + (lz * lz);
                if (dist2 > 0.000001f) {
                    const float invDist = 1.0f / sqrtf(dist2);
                    lx *= invDist;
                    ly *= invDist;
                    lz *= invDist;
                } else {
                    lx = ly = lz = 0.0f;
                }

                attenuation = 1.0f / (1.0f + dist2 * 0.00012f);
            } else {
                Vec3 nd = vec3Normalize(vec3Scale(ls->dir, -1.0f));
                lx = nd.x;
                ly = nd.y;
                lz = nd.z;
            }

            ndotl = (nx * lx) + (ny * ly) + (nz * lz);
            if (ndotl < 0.0f) ndotl = 0.0f;

            brightness += ndotl * ls->intensity * attenuation * matDiffuse;

            if (matSpec > 0.0f && ndotl > 0.0f) {
                float rx = (2.0f * ndotl * nx) - lx;
                float ry = (2.0f * ndotl * ny) - ly;
                float rz = (2.0f * ndotl * nz) - lz;

                const float rlen2 = (rx * rx) + (ry * ry) + (rz * rz);
                if (rlen2 > 0.000001f) {
                    const float invRLen = 1.0f / sqrtf(rlen2);
                    rx *= invRLen;
                    ry *= invRLen;
                    rz *= invRLen;
                }

                float vx = camPosX - cx;
                float vy = camPosY - cy;
                float vz = camPosZ - cz;

                const float vlen2 = (vx * vx) + (vy * vy) + (vz * vz);
                if (vlen2 > 0.000001f) {
                    const float invVLen = 1.0f / sqrtf(vlen2);
                    vx *= invVLen;
                    vy *= invVLen;
                    vz *= invVLen;
                } else {
                    vx = vy = vz = 0.0f;
                }

                float rdotv = (rx * vx) + (ry * vy) + (rz * vz);
                if (rdotv < 0.0f) rdotv = 0.0f;

                brightness +=
                    powf(rdotv, matShiny) *
                    matSpec *
                    ls->intensity *
                    attenuation;
            }
        }

        if (brightness < faceEmission) brightness = faceEmission;
        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;

        const float shadeF = brightnessToShadeF(brightness);

        const Vec3 a = worldToCamera(wa, *cam);
        const Vec3 b = worldToCamera(wb, *cam);
        const Vec3 c = worldToCamera(wc, *cam);

        if (a.z > cam->farPlane && b.z > cam->farPlane && c.z > cam->farPlane) {
            continue;
        }

    #if USE_BACKFACE_CULL
        if (!triangleFacingCamera(a, b, c)) {
            continue;
        }
    #endif

        Vec3 clipped[CLIP_MAX_VERTS];
        const int clippedCount = clipTriangleToFrustum(a, b, c, clipped, cam);
        if (clippedCount < 3) continue;

        for (int k = 1; k < clippedCount - 1; k++) {
            submitClippedTri(
                clipped[0],
                clipped[k],
                clipped[k + 1],
                (Camera *)cam,
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

    if (!clipLineToNearPlane(&a, &b, cam)) return;
    if (!projectPoint(a, cam, &pa)) return;
    if (!projectPoint(b, cam, &pb)) return;

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