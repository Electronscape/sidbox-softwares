// file: render.c

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../gfx.h"
#include "sb3d.h"

#define USE_BACKFACE_CULL 1

static Entity *renderEntities[WORLD_MAX];
static RenderTri g_renderTris[MAX_RENDER_TRIS];

// cache
static Vec3 g_worldVertsCache[SB3D_MAX_VERTS];
static Vec3 g_camVertsCache[SB3D_MAX_VERTS];


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

int clipTriangleToFrustum(Vec3 a, Vec3 b, Vec3 c, Vec3 *outVerts, const Camera *cam)
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

static inline int triangleFacingScreen(Vec2 a, Vec2 b, Vec2 c){
    int cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
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

void submitClippedTri(Vec3 a, Vec3 b, Vec3 c, Camera *cam, uint8_t color, uint8_t emission, uint8_t trans, float shadeF)
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
    rt->transparency = trans;
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


int projectPoint(Vec3 p, const Camera *cam, Vec2 *out)
{
    const float invZ = 1.0f / p.z;
    const float f = sb3d_proj_f();

    if (p.z <= cam->nearPlane) {
        return 0;
    }

    out->x = (int)((p.x * f * invZ) + (SCREEN_W * 0.5f) + 0.5f);
    out->y = (int)((-p.y * f * invZ) + (SCREEN_H * 0.5f) + 0.5f);
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

    for (int i = 0; i < WORLD_MAX; i++) {
        Entity *ent = &worldEntities[i];

        if (!ent->active) continue;
        if (!ent->mesh) continue;
        if ((ent->flags & ENTITY_VISIBLE) == 0) continue;

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



uint8_t sidboxFB[SCREEN_H * SCREEN_W];

void DrawSBFBtoDB(void)
{
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            fb[y * SCREEN_W + x] = sidboxFB[x * SCREEN_H + y];
        }
    }
}

#define drawbuffer sidboxFB

void drawFakeHorizonTex(
    const Camera *cam,
    const uint8_t *skyTex,
    const uint8_t *groundTex,
    uint8_t skySolidCol,
    uint8_t groundSolidCol,
    uint8_t lineCol,
    float groundY,
    float skyY,
    float skyFadeDist,
    float skyScale,
    float groundScale,
    int skyScrollU,
    int skyScrollV,
    int groundScrollU,
    int groundScrollV,
    uint8_t transparentZero,
    uint8_t proceduralPatchMode,
    uint8_t skyPatchDensity,
    uint8_t groundPatchDensity
)
{
    if (!cam) return;
    if (!skyTex || !groundTex) return;

    const float f    = cam->projF;
    const float cx   = cam->halfW;
    const float cy   = cam->halfH;
    const float invF = 1.0f / f;

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

    const float Ry = cam->right.y;
    const float Uy = cam->up.y;
    const float Fy = cam->forward.y;

    const float groundNumer = groundY - camPosY;
    const float skyNumer    = skyY    - camPosY;

    const float skyFadeStart = skyFadeDist * 0.65f;
    const float skyFadeEnd   = skyFadeDist;

    const float skyFadeStart2 = skyFadeStart * skyFadeStart;
    const float skyFadeEnd2   = skyFadeEnd   * skyFadeEnd;
    const float skyFadeSpan2  = skyFadeEnd2 - skyFadeStart2;

    for (int x = 0; x < SCREEN_W; x++) {
        const float sx = (((float)x - cx) * invF);
        const float xTerm = sx * Ry;

        const float topDirWorldY    = xTerm + (cy * invF * Uy) + Fy;
        const float bottomDirWorldY = xTerm + (((cy - (float)(SCREEN_H - 1)) * invF) * Uy) + Fy;

        int topGround = 0;
        int botGround = 0;

        if (fabsf(topDirWorldY) >= 0.0001f) {
            topGround = ((groundNumer / topDirWorldY) > 0.0f);
        }
        if (fabsf(bottomDirWorldY) >= 0.0001f) {
            botGround = ((groundNumer / bottomDirWorldY) > 0.0f);
        }

        uint8_t *dst = &drawbuffer[FB_INDEX(x, 0)];

        if (topGround == botGround) {
            if (topGround) {
                /* whole column = ground */
                for (int y = 0; y < SCREEN_H; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = lineCol;
                        continue;
                    }

                    {
                        const float t = groundNumer / dirY;
                        const float hitX = camPosX + (dirX * t);
                        const float hitZ = camPosZ + (dirZ * t);

                        const float u = (hitX * groundScale) + (float)groundScrollU;
                        const float v = (hitZ * groundScale) + (float)groundScrollV;

                        const int iu = (int)floorf(u);
                        const int iv = (int)floorf(v);

                        int tu = iu & 31;
                        int tv = iv & 31;

                        uint8_t texCol;

                        if (proceduralPatchMode) {
                            const int tileU = iu >> 5;
                            const int tileV = iv >> 5;

                            const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                            const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                            if ((hDensity & 255u) > groundPatchDensity) {
                                texCol = groundSolidCol;
                            } else {
                                int su = tu;
                                int sv = tv;
                                int ru, rv;

                                if (hOrient & 1u) su = 31 - su;   /* flip X */
                                if (hOrient & 2u) sv = 31 - sv;   /* flip Y */

                                switch ((hOrient >> 2) & 3u) {
                                    default:
                                    case 0: ru = su;       rv = sv;       break; /*   0 */
                                    case 1: ru = 31 - sv;  rv = su;       break; /*  90 */
                                    case 2: ru = 31 - su;  rv = 31 - sv;  break; /* 180 */
                                    case 3: ru = sv;       rv = 31 - su;  break; /* 270 */
                                }

                                texCol = groundTex[(rv << 5) | ru];
                                if (transparentZero && texCol == 0) texCol = groundSolidCol;
                            }
                        } else {
                            texCol = groundTex[(tv << 5) | tu];
                            if (transparentZero && texCol == 0) texCol = groundSolidCol;
                        }

                        *dst++ = texCol;
                    }
                }
            } else {
                /* whole column = sky */
                for (int y = 0; y < SCREEN_H; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = skySolidCol;
                        continue;
                    }

                    {
                        const float t = skyNumer / dirY;

                        if (t <= 0.0f) {
                            *dst++ = skySolidCol;
                        } else {
                            const float hitX = camPosX + (dirX * t);
                            const float hitZ = camPosZ + (dirZ * t);

                            const float dx = hitX - camPosX;
                            const float dz = hitZ - camPosZ;
                            const float dist2 = (dx * dx) + (dz * dz);

                            const float u = (hitX * skyScale) + (float)skyScrollU;
                            const float v = (hitZ * skyScale) + (float)skyScrollV;

                            const int iu = (int)floorf(u);
                            const int iv = (int)floorf(v);

                            int tu = iu & 31;
                            int tv = iv & 31;

                            uint8_t texCol;

                            if (proceduralPatchMode) {
                                const int tileU = iu >> 5;
                                const int tileV = iv >> 5;

                                const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                                const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                                if ((hDensity & 255u) > skyPatchDensity) {
                                    texCol = skySolidCol;
                                } else {
                                    int su = tu;
                                    int sv = tv;
                                    int ru, rv;

                                    if (hOrient & 1u) su = 31 - su;   /* flip X */
                                    if (hOrient & 2u) sv = 31 - sv;   /* flip Y */

                                    switch ((hOrient >> 2) & 3u) {
                                        default:
                                        case 0: ru = su;       rv = sv;       break; /*   0 */
                                        case 1: ru = 31 - sv;  rv = su;       break; /*  90 */
                                        case 2: ru = 31 - su;  rv = 31 - sv;  break; /* 180 */
                                        case 3: ru = sv;       rv = 31 - su;  break; /* 270 */
                                    }

                                    texCol = skyTex[(rv << 5) | ru];
                                    if (transparentZero && texCol == 0) texCol = skySolidCol;
                                }
                            } else {
                                texCol = skyTex[(tv << 5) | tu];
                                if (transparentZero && texCol == 0) texCol = skySolidCol;
                            }

                            if (dist2 <= skyFadeStart2) {
                                *dst++ = texCol;
                            } else if (dist2 >= skyFadeEnd2) {
                                *dst++ = skySolidCol;
                            } else {
                                const float fadeT = (dist2 - skyFadeStart2) / skyFadeSpan2;
                                const int dither = ((x & 3) + ((y & 3) << 2));
                                *dst++ = ((int)(fadeT * 15.0f) > dither) ? skySolidCol : texCol;
                            }
                        }
                    }
                }
            }
        } else {
            int ySplit;

            if (fabsf(Uy) < 0.0001f) {
                ySplit = SCREEN_H / 2;
            } else {
                const float ySplitF = cy + (f / Uy) * (xTerm + Fy);
                ySplit = (int)lroundf(ySplitF);
            }

            if (ySplit < 0) ySplit = 0;
            if (ySplit > SCREEN_H) ySplit = SCREEN_H;

            if (topGround) {
                /* top = ground, bottom = sky */
                for (int y = 0; y < ySplit; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = lineCol;
                        continue;
                    }

                    {
                        const float t = groundNumer / dirY;
                        const float hitX = camPosX + (dirX * t);
                        const float hitZ = camPosZ + (dirZ * t);

                        const float u = (hitX * groundScale) + (float)groundScrollU;
                        const float v = (hitZ * groundScale) + (float)groundScrollV;

                        const int iu = (int)floorf(u);
                        const int iv = (int)floorf(v);

                        int tu = iu & 31;
                        int tv = iv & 31;

                        uint8_t texCol;

                        if (proceduralPatchMode) {
                            const int tileU = iu >> 5;
                            const int tileV = iv >> 5;

                            const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                            const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                            if ((hDensity & 255u) > groundPatchDensity) {
                                texCol = groundSolidCol;
                            } else {
                                int su = tu;
                                int sv = tv;
                                int ru, rv;

                                if (hOrient & 1u) su = 31 - su;
                                if (hOrient & 2u) sv = 31 - sv;

                                switch ((hOrient >> 2) & 3u) {
                                    default:
                                    case 0: ru = su;       rv = sv;       break;
                                    case 1: ru = 31 - sv;  rv = su;       break;
                                    case 2: ru = 31 - su;  rv = 31 - sv;  break;
                                    case 3: ru = sv;       rv = 31 - su;  break;
                                }

                                texCol = groundTex[(rv << 5) | ru];
                                if (transparentZero && texCol == 0) texCol = groundSolidCol;
                            }
                        } else {
                            texCol = groundTex[(tv << 5) | tu];
                            if (transparentZero && texCol == 0) texCol = groundSolidCol;
                        }

                        *dst++ = texCol;
                    }
                }

                for (int y = ySplit; y < SCREEN_H; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = skySolidCol;
                        continue;
                    }

                    {
                        const float t = skyNumer / dirY;

                        if (t <= 0.0f) {
                            *dst++ = skySolidCol;
                        } else {
                            const float hitX = camPosX + (dirX * t);
                            const float hitZ = camPosZ + (dirZ * t);

                            const float dx = hitX - camPosX;
                            const float dz = hitZ - camPosZ;
                            const float dist2 = (dx * dx) + (dz * dz);

                            const float u = (hitX * skyScale) + (float)skyScrollU;
                            const float v = (hitZ * skyScale) + (float)skyScrollV;

                            const int iu = (int)floorf(u);
                            const int iv = (int)floorf(v);

                            int tu = iu & 31;
                            int tv = iv & 31;

                            uint8_t texCol;

                            if (proceduralPatchMode) {
                                const int tileU = iu >> 5;
                                const int tileV = iv >> 5;

                                const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                                const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                                if ((hDensity & 255u) > skyPatchDensity) {
                                    texCol = skySolidCol;
                                } else {
                                    int su = tu;
                                    int sv = tv;
                                    int ru, rv;

                                    if (hOrient & 1u) su = 31 - su;
                                    if (hOrient & 2u) sv = 31 - sv;

                                    switch ((hOrient >> 2) & 3u) {
                                        default:
                                        case 0: ru = su;       rv = sv;       break;
                                        case 1: ru = 31 - sv;  rv = su;       break;
                                        case 2: ru = 31 - su;  rv = 31 - sv;  break;
                                        case 3: ru = sv;       rv = 31 - su;  break;
                                    }

                                    texCol = skyTex[(rv << 5) | ru];
                                    if (transparentZero && texCol == 0) texCol = skySolidCol;
                                }
                            } else {
                                texCol = skyTex[(tv << 5) | tu];
                                if (transparentZero && texCol == 0) texCol = skySolidCol;
                            }

                            if (dist2 <= skyFadeStart2) {
                                *dst++ = texCol;
                            } else if (dist2 >= skyFadeEnd2) {
                                *dst++ = skySolidCol;
                            } else {
                                const float fadeT = (dist2 - skyFadeStart2) / skyFadeSpan2;
                                const int dither = ((x & 3) + ((y & 3) << 2));
                                *dst++ = ((int)(fadeT * 15.0f) > dither) ? skySolidCol : texCol;
                            }
                        }
                    }
                }
            } else {
                /* top = sky, bottom = ground */
                for (int y = 0; y < ySplit; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = skySolidCol;
                        continue;
                    }

                    {
                        const float t = skyNumer / dirY;

                        if (t <= 0.0f) {
                            *dst++ = skySolidCol;
                        } else {
                            const float hitX = camPosX + (dirX * t);
                            const float hitZ = camPosZ + (dirZ * t);

                            const float dx = hitX - camPosX;
                            const float dz = hitZ - camPosZ;
                            const float dist2 = (dx * dx) + (dz * dz);

                            const float u = (hitX * skyScale) + (float)skyScrollU;
                            const float v = (hitZ * skyScale) + (float)skyScrollV;

                            const int iu = (int)floorf(u);
                            const int iv = (int)floorf(v);

                            int tu = iu & 31;
                            int tv = iv & 31;

                            uint8_t texCol;

                            if (proceduralPatchMode) {
                                const int tileU = iu >> 5;
                                const int tileV = iv >> 5;

                                const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                                const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                                if ((hDensity & 255u) > skyPatchDensity) {
                                    texCol = skySolidCol;
                                } else {
                                    int su = tu;
                                    int sv = tv;
                                    int ru, rv;

                                    if (hOrient & 1u) su = 31 - su;
                                    if (hOrient & 2u) sv = 31 - sv;

                                    switch ((hOrient >> 2) & 3u) {
                                        default:
                                        case 0: ru = su;       rv = sv;       break;
                                        case 1: ru = 31 - sv;  rv = su;       break;
                                        case 2: ru = 31 - su;  rv = 31 - sv;  break;
                                        case 3: ru = sv;       rv = 31 - su;  break;
                                    }

                                    texCol = skyTex[(rv << 5) | ru];
                                    if (transparentZero && texCol == 0) texCol = skySolidCol;
                                }
                            } else {
                                texCol = skyTex[(tv << 5) | tu];
                                if (transparentZero && texCol == 0) texCol = skySolidCol;
                            }

                            if (dist2 <= skyFadeStart2) {
                                *dst++ = texCol;
                            } else if (dist2 >= skyFadeEnd2) {
                                *dst++ = skySolidCol;
                            } else {
                                const float fadeT = (dist2 - skyFadeStart2) / skyFadeSpan2;
                                const int dither = ((x & 3) + ((y & 3) << 2));
                                *dst++ = ((int)(fadeT * 15.0f) > dither) ? skySolidCol : texCol;
                            }
                        }
                    }
                }

                for (int y = ySplit; y < SCREEN_H; y++) {
                    const float sy = (cy - (float)y) * invF;

                    const float dirX = (sx * rx) + (sy * ux) + fx;
                    const float dirY = (sx * ry) + (sy * uy) + fy;
                    const float dirZ = (sx * rz) + (sy * uz) + fz;

                    if (fabsf(dirY) < 0.0001f) {
                        *dst++ = lineCol;
                        continue;
                    }

                    {
                        const float t = groundNumer / dirY;
                        const float hitX = camPosX + (dirX * t);
                        const float hitZ = camPosZ + (dirZ * t);

                        const float u = (hitX * groundScale) + (float)groundScrollU;
                        const float v = (hitZ * groundScale) + (float)groundScrollV;

                        const int iu = (int)floorf(u);
                        const int iv = (int)floorf(v);

                        int tu = iu & 31;
                        int tv = iv & 31;

                        uint8_t texCol;

                        if (proceduralPatchMode) {
                            const int tileU = iu >> 5;
                            const int tileV = iv >> 5;

                            const uint32_t hDensity = sb3d_hash2i(tileU, tileV);
                            const uint32_t hOrient  = sb3d_hash2i(tileU ^ 0x68bc21ebu, tileV ^ 0x02e5be93u);

                            if ((hDensity & 255u) > groundPatchDensity) {
                                texCol = groundSolidCol;
                            } else {
                                int su = tu;
                                int sv = tv;
                                int ru, rv;

                                if (hOrient & 1u) su = 31 - su;
                                if (hOrient & 2u) sv = 31 - sv;

                                switch ((hOrient >> 2) & 3u) {
                                    default:
                                    case 0: ru = su;       rv = sv;       break;
                                    case 1: ru = 31 - sv;  rv = su;       break;
                                    case 2: ru = 31 - su;  rv = 31 - sv;  break;
                                    case 3: ru = sv;       rv = 31 - su;  break;
                                }

                                texCol = groundTex[(rv << 5) | ru];
                                if (transparentZero && texCol == 0) texCol = groundSolidCol;
                            }
                        } else {
                            texCol = groundTex[(tv << 5) | tu];
                            if (transparentZero && texCol == 0) texCol = groundSolidCol;
                        }

                        *dst++ = texCol;
                    }
                }
            }
        }
    }

    DrawSBFBtoDB();
    if (fabsf(Uy) >= 0.0001f) {
        int y0 = (int)lroundf(cy + ((((0.0f) - cx) * Ry) + (f * Fy)) / Uy);
        int y1 = (int)lroundf(cy + (((((float)(SCREEN_W - 1)) - cx) * Ry) + (f * Fy)) / Uy);
        drawLine(0, y0, SCREEN_W - 1, y1, lineCol);
    }
}




//DrawSBFBtoDB();

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

        uint8_t *row = &sidboxFB[y * SCREEN_W];

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

void drawFakeSkyDots(const Camera *cam, uint8_t dotCol, int azSteps, int elSteps, uint8_t density)
{
    //if (!cam) return;
    //if (density == 0) return;

    if (azSteps < 16) azSteps = 16;
    if (elSteps < 8)  elSteps = 8;

    const float twoPi  = 6.28318530718f;
    const float halfPi = 1.57079632679f;

    const float invAz = 1.0f / (float)azSteps;
    const float invEl = 1.0f / (float)elSteps;

    const float rx = cam->right.x;
    const float ry = cam->right.y;
    const float rz = cam->right.z;

    const float ux = cam->up.x;
    const float uy = cam->up.y;
    const float uz = cam->up.z;

    const float fx = cam->forward.x;
    const float fy = cam->forward.y;
    const float fz = cam->forward.z;


    for (int el = 0; el < elSteps; el++) {
        const float elT = ((float)el + 0.5f) * invEl;
        const float elev = elT * halfPi;

        const float sinElev = sinf(elev);
        const float cosElev = cosf(elev);

        for (int az = 0; az < azSteps; az++) {
            uint32_t h = sb3d_hash2i(az, el);
            if ((h & 255u) > density) continue;

            const float jitterA = ((float)((h >> 8)  & 255u)) * (1.0f / 255.0f);
            const float jitterE = ((float)((h >> 16) & 255u)) * (1.0f / 255.0f);

            const float ang =
                (((float)az + jitterA) * invAz) * twoPi;

            const float elevJ =
                (((float)el + jitterE) * invEl) * halfPi;

            const float sinE = sinf(elevJ);
            const float cosE = cosf(elevJ);
            const float sinA = sinf(ang);
            const float cosA = cosf(ang);

            /* upper hemisphere world direction */
            const float worldX = cosE * cosA;
            const float worldY = sinE;
            const float worldZ = cosE * sinA;

            /* world dir -> camera dir */
            const float camX = (worldX * rx) + (worldY * ry) + (worldZ * rz);
            const float camY = (worldX * ux) + (worldY * uy) + (worldZ * uz);
            const float camZ = (worldX * fx) + (worldY * fy) + (worldZ * fz);

            /* only stars in front of camera */
            if (camZ <= 0.001f) continue;

            {
                const float invZ = 1.0f / camZ;
                const int sx = (int)((camX * cam->projF * invZ) + cam->halfW + 0.5f);
                const int sy = (int)((-camY * cam->projF * invZ) + cam->halfH + 0.5f);

                if ((unsigned)sx < SCREEN_W && (unsigned)sy < SCREEN_H) {
                    putPixel(sx, sy, dotCol);
                }
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
    sb3dParticlesRender(cam);

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
                int emissiveShade = (int)MAX_PALETTE_SHADE_INDEX - (int)(emissiveF * MAX_PALETTE_SHADE_INDEX + 0.5f);
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
        else {  // normal render
            for (int i = 0; i < g_renderTriCount; i++) {
                RenderTri *rt = &g_renderTris[i];



                if (rt->maxY < bandY0 || rt->minY > bandY1) continue;

                if(rt->color & TRI_FLAG_TRANSPARENT)
                {
                    uint8_t tStrenth = rt->transparency;

                    fillTriangleDitherZBandBayerT(
                        rt->p0.x, rt->p0.y,
                        rt->p1.x, rt->p1.y,
                        rt->p2.x, rt->p2.y,
                        rt->z0, rt->z1, rt->z2,
                        rt->camz0, rt->camz1, rt->camz2,
                        rt->color,
                        rt->shadeF,
                        tStrenth,
                        bandY0,
                        bandY1
                    );
                } else {
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
}

void submitEntitySolid(const Entity *ent, const Camera *cam)
{
    const Mesh *mesh;
    const Material *mat;
    Light *lights;
    int lightCount;

    float matAmbient;
    float matEmissive;
    float matDiffuse;
    float matSpec;
    float matShiny;

    float entPosX, entPosY, entPosZ;
    float erx, ery, erz;
    float eux, euy, euz;
    float efx, efy, efz;

    float camPosX, camPosY, camPosZ;
    float crx, cry, crz;
    float cux, cuy, cuz;
    float cfx, cfy, cfz;

    if (!ent || !cam) return;
    if (!ent->mesh) return;

    mesh = ent->mesh;
    if (!mesh->verts || !mesh->tris) return;
    if (mesh->vertCount <= 0 || mesh->vertCount > SB3D_MAX_VERTS) return;
    if (mesh->triCount <= 0) return;

    mat = &mesh->material;
    lights = lightsGet();
    lightCount = lightsGetCount();

    matAmbient  = mat->ambient;
    matEmissive = mat->emissive;
    matDiffuse  = mat->diffuse;
    matSpec     = mat->specularStrength;
    matShiny    = mat->shininess;

    entPosX = ent->pos.x;
    entPosY = ent->pos.y;
    entPosZ = ent->pos.z;

    erx = ent->right.x;
    ery = ent->right.y;
    erz = ent->right.z;

    eux = ent->up.x;
    euy = ent->up.y;
    euz = ent->up.z;

    efx = ent->forward.x;
    efy = ent->forward.y;
    efz = ent->forward.z;

    camPosX = cam->pos.x;
    camPosY = cam->pos.y;
    camPosZ = cam->pos.z;

    crx = cam->right.x;
    cry = cam->right.y;
    crz = cam->right.z;

    cux = cam->up.x;
    cuy = cam->up.y;
    cuz = cam->up.z;

    cfx = cam->forward.x;
    cfy = cam->forward.y;
    cfz = cam->forward.z;

    /*
        Build world-space + camera-space vertex caches once per entity
    */
    for (int vi = 0; vi < mesh->vertCount; vi++) {
        const Vec3 *lv = &mesh->verts[vi];
        float wx, wy, wz;
        float dx, dy, dz;

        wx = entPosX + (erx * lv->x) + (eux * lv->y) + (efx * lv->z);
        wy = entPosY + (ery * lv->x) + (euy * lv->y) + (efy * lv->z);
        wz = entPosZ + (erz * lv->x) + (euz * lv->y) + (efz * lv->z);

        g_worldVertsCache[vi].x = wx;
        g_worldVertsCache[vi].y = wy;
        g_worldVertsCache[vi].z = wz;

        dx = wx - camPosX;
        dy = wy - camPosY;
        dz = wz - camPosZ;

        g_camVertsCache[vi].x = (dx * crx) + (dy * cry) + (dz * crz);
        g_camVertsCache[vi].y = (dx * cux) + (dy * cuy) + (dz * cuz);
        g_camVertsCache[vi].z = (dx * cfx) + (dy * cfy) + (dz * cfz);
    }

    for (int i = 0; i < mesh->triCount; i++) {
        const Tri *t;
        const Vec3 *wa;
        const Vec3 *wb;
        const Vec3 *wc;
        const Vec3 *a;
        const Vec3 *b;
        const Vec3 *c;

        float abx, aby, abz;
        float acx, acy, acz;
        float nx, ny, nz;
        float nlen2;

        float faceCX, faceCY, faceCZ;
        float faceEmission;
        float brightness;
        uint8_t renderColor;

        float vx, vy, vz;
        float vlen2;

        Vec3 clipped[CLIP_MAX_VERTS];
        int clippedCount;

        t = &mesh->tris[i];

        wa = &g_worldVertsCache[t->a];
        wb = &g_worldVertsCache[t->b];
        wc = &g_worldVertsCache[t->c];

        a = &g_camVertsCache[t->a];
        b = &g_camVertsCache[t->b];
        c = &g_camVertsCache[t->c];

        if (a->z > cam->farPlane && b->z > cam->farPlane && c->z > cam->farPlane) {
            continue;
        }

    #if USE_BACKFACE_CULL
        if (!triangleFacingCamera(*a, *b, *c)) {
            continue;
        }
    #endif

        clippedCount = clipTriangleToFrustum(*a, *b, *c, clipped, cam);
        if (clippedCount < 3) {
            continue;
        }

        /*
            World-space face normal
        */
        abx = wb->x - wa->x;
        aby = wb->y - wa->y;
        abz = wb->z - wa->z;

        acx = wc->x - wa->x;
        acy = wc->y - wa->y;
        acz = wc->z - wa->z;

        nx = (aby * acz) - (abz * acy);
        ny = (abz * acx) - (abx * acz);
        nz = (abx * acy) - (aby * acx);

        nlen2 = (nx * nx) + (ny * ny) + (nz * nz);
        if (nlen2 > 0.000001f) {
            if (nlen2 < 0.999f || nlen2 > 1.001f) {
                const float invNLen = 1.0f / sqrtf(nlen2);
                nx *= invNLen;
                ny *= invNLen;
                nz *= invNLen;
            }
        } else {
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        }

        /*
            World-space face center
        */
        faceCX = (wa->x + wb->x + wc->x) * (1.0f / 3.0f);
        faceCY = (wa->y + wb->y + wc->y) * (1.0f / 3.0f);
        faceCZ = (wa->z + wb->z + wc->z) * (1.0f / 3.0f);

        faceEmission = (float)t->emission / 255.0f;
        brightness = matAmbient + matEmissive;

        /*
            View vector once per face
        */
        vx = camPosX - faceCX;
        vy = camPosY - faceCY;
        vz = camPosZ - faceCZ;

        vlen2 = (vx * vx) + (vy * vy) + (vz * vz);
        if (vlen2 > 0.000001f) {
            if (vlen2 < 0.999f || vlen2 > 1.001f) {
                const float invVLen = 1.0f / sqrtf(vlen2);
                vx *= invVLen;
                vy *= invVLen;
                vz *= invVLen;
            }
        } else {
            vx = 0.0f;
            vy = 0.0f;
            vz = 0.0f;
        }

        for (int li = 0; li < lightCount; li++) {
            const Light *ls;
            float lx, ly, lz;
            float attenuation;
            float ndotl;

            ls = &lights[li];
            if (!ls->enabled) continue;

            attenuation = 1.0f;

            if (ls->type == LIGHT_POINT) {
                float dist2;
                float near2;
                float beyond2;

                lx = ls->pos.x - faceCX;
                ly = ls->pos.y - faceCY;
                lz = ls->pos.z - faceCZ;

                dist2   = (lx * lx) + (ly * ly) + (lz * lz);
                near2   = ls->near   * ls->near;
                beyond2 = ls->beyond * ls->beyond;

                if (dist2 >= beyond2) {
                    continue;
                }

                if (dist2 > 0.000001f) {
                    const float invDist = 1.0f / sqrtf(dist2);
                    const float dist = dist2 * invDist;

                    lx *= invDist;
                    ly *= invDist;
                    lz *= invDist;

                    if (dist2 <= near2) {
                        attenuation = 1.0f;
                    } else {
                        float tval;

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
                    lx = 0.0f;
                    ly = 0.0f;
                    lz = 0.0f;
                    attenuation = 1.0f;
                }
            } else {
                lx = -ls->dir.x;
                ly = -ls->dir.y;
                lz = -ls->dir.z;
            }

            ndotl = (nx * lx) + (ny * ly) + (nz * lz);
            if (ndotl <= 0.0f) {
                continue;
            }

            brightness += ndotl * ls->intensity * attenuation * matDiffuse;

            if (matSpec > 0.0f) {
                float rdx, rdy, rdz;
                float rdotv;

                rdx = (2.0f * ndotl * nx) - lx;
                rdy = (2.0f * ndotl * ny) - ly;
                rdz = (2.0f * ndotl * nz) - lz;

                rdotv = (rdx * vx) + (rdy * vy) + (rdz * vz);

                if (rdotv > 0.0f) {
                    float specPow;

                    if (matShiny == 8.0f) {
                        float s = rdotv * rdotv;
                        s *= s;
                        s *= s;
                        specPow = s;
                    }
                    else if (matShiny == 16.0f) {
                        float s = rdotv * rdotv;
                        s *= s;
                        s *= s;
                        s *= s;
                        specPow = s;
                    }
                    else {
                        specPow = powf(rdotv, matShiny);
                    }

                    brightness += specPow * matSpec * ls->intensity * attenuation;
                }
            }
        }

        if (brightness < faceEmission) brightness = faceEmission;
        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;

        {
            const float shadeF = brightnessToShadeF(brightness);
            renderColor = (uint8_t)(t->color & TRI_COLOUR_MASK);
            if(t->transparency > 0)
                renderColor |= TRI_FLAG_TRANSPARENT;

            for (int k = 1; k < clippedCount - 1; k++) {
                submitClippedTri(
                    clipped[0],
                    clipped[k],
                    clipped[k + 1],
                    (Camera *)cam,
                    renderColor,
                    t->emission,
                    t->transparency,
                    shadeF
                );
            }
        }
    }
}

















void submitEntitySolid_OLD(const Entity *ent, const Camera *cam)
{
    const Mesh *mesh = ent->mesh;
    const Material *mat = &mesh->material;

    Light *lights = lightsGet();
    const int lightCount = lightsGetCount();

    const float matAmbient  = mat->ambient;
    const float matEmissive = mat->emissive;
    const float matDiffuse  = mat->diffuse;
    const float matSpec     = mat->specularStrength;
    const float matShiny    = mat->shininess;

    const float entPosX = ent->pos.x;
    const float entPosY = ent->pos.y;
    const float entPosZ = ent->pos.z;

    const float erx = ent->right.x;
    const float ery = ent->right.y;
    const float erz = ent->right.z;

    const float eux = ent->up.x;
    const float euy = ent->up.y;
    const float euz = ent->up.z;

    const float efx = ent->forward.x;
    const float efy = ent->forward.y;
    const float efz = ent->forward.z;

    const float camPosX = cam->pos.x;
    const float camPosY = cam->pos.y;
    const float camPosZ = cam->pos.z;

    const float crx = cam->right.x;
    const float cry = cam->right.y;
    const float crz = cam->right.z;

    const float cux = cam->up.x;
    const float cuy = cam->up.y;
    const float cuz = cam->up.z;

    const float cfx = cam->forward.x;
    const float cfy = cam->forward.y;
    const float cfz = cam->forward.z;

    for (int i = 0; i < mesh->triCount; i++) {
        const Tri t = mesh->tris[i];

        const Vec3 la = mesh->verts[t.a];
        const Vec3 lb = mesh->verts[t.b];
        const Vec3 lc = mesh->verts[t.c];

        /* local -> world */
        const float wax = entPosX + (erx * la.x) + (eux * la.y) + (efx * la.z);
        const float way = entPosY + (ery * la.x) + (euy * la.y) + (efy * la.z);
        const float waz = entPosZ + (erz * la.x) + (euz * la.y) + (efz * la.z);

        const float wbx = entPosX + (erx * lb.x) + (eux * lb.y) + (efx * lb.z);
        const float wby = entPosY + (ery * lb.x) + (euy * lb.y) + (efy * lb.z);
        const float wbz = entPosZ + (erz * lb.x) + (euz * lb.y) + (efz * lb.z);

        const float wcx = entPosX + (erx * lc.x) + (eux * lc.y) + (efx * lc.z);
        const float wcy = entPosY + (ery * lc.x) + (euy * lc.y) + (efy * lc.z);
        const float wcz = entPosZ + (erz * lc.x) + (euz * lc.y) + (efz * lc.z);

        /* world -> camera */
        const float adx = wax - camPosX;
        const float ady = way - camPosY;
        const float adz = waz - camPosZ;

        const float bdx = wbx - camPosX;
        const float bdy = wby - camPosY;
        const float bdz = wbz - camPosZ;

        const float cdx = wcx - camPosX;
        const float cdy = wcy - camPosY;
        const float cdz = wcz - camPosZ;

        const Vec3 a = {
            (adx * crx) + (ady * cry) + (adz * crz),
            (adx * cux) + (ady * cuy) + (adz * cuz),
            (adx * cfx) + (ady * cfy) + (adz * cfz)
        };

        const Vec3 b = {
            (bdx * crx) + (bdy * cry) + (bdz * crz),
            (bdx * cux) + (bdy * cuy) + (bdz * cuz),
            (bdx * cfx) + (bdy * cfy) + (bdz * cfz)
        };

        const Vec3 c = {
            (cdx * crx) + (cdy * cry) + (cdz * crz),
            (cdx * cux) + (cdy * cuy) + (cdz * cuz),
            (cdx * cfx) + (cdy * cfy) + (cdz * cfz)
        };

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
        if (clippedCount < 3) {
            continue;
        }

        /* world-space normal */
        const float abx = wbx - wax;
        const float aby = wby - way;
        const float abz = wbz - waz;

        const float acx = wcx - wax;
        const float acy = wcy - way;
        const float acz = wcz - waz;

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
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        }

        const float cx = (wax + wbx + wcx) * (1.0f / 3.0f);
        const float cy = (way + wby + wcy) * (1.0f / 3.0f);
        const float cz = (waz + wbz + wcz) * (1.0f / 3.0f);

        const float faceEmission = (float)t.emission / 255.0f;
        float brightness = matAmbient + matEmissive;

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
            vx = 0.0f;
            vy = 0.0f;
            vz = 0.0f;
        }

        for (int li = 0; li < lightCount; li++) {
            const Light *ls = &lights[li];
            if (!ls->enabled) continue;

            float lx, ly, lz;
            float attenuation = 1.0f;

            if (ls->type == LIGHT_POINT) {
                lx = ls->pos.x - cx;
                ly = ls->pos.y - cy;
                lz = ls->pos.z - cz;

                const float dist2 = (lx * lx) + (ly * ly) + (lz * lz);
                const float near2   = ls->near   * ls->near;
                const float beyond2 = ls->beyond * ls->beyond;

                if (dist2 >= beyond2) {
                    continue;
                }

                if (dist2 > 0.000001f) {
                    if (dist2 <= near2) {
                        const float invDist = 1.0f / sqrtf(dist2);
                        lx *= invDist;
                        ly *= invDist;
                        lz *= invDist;
                        attenuation = 1.0f;
                    } else {
                        const float invDist = 1.0f / sqrtf(dist2);
                        const float dist = dist2 * invDist;
                        float tval;

                        lx *= invDist;
                        ly *= invDist;
                        lz *= invDist;

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
                    lx = 0.0f;
                    ly = 0.0f;
                    lz = 0.0f;
                    attenuation = 1.0f;
                }
            } else {
                lx = -ls->dir.x;
                ly = -ls->dir.y;
                lz = -ls->dir.z;
            }

            float ndotl = (nx * lx) + (ny * ly) + (nz * lz);
            if (ndotl <= 0.0f) {
                continue;
            }

            brightness += ndotl * ls->intensity * attenuation * matDiffuse;

            if (matSpec > 0.0f) {
                const float rx = (2.0f * ndotl * nx) - lx;
                const float ry = (2.0f * ndotl * ny) - ly;
                const float rz = (2.0f * ndotl * nz) - lz;

                float rdotv = (rx * vx) + (ry * vy) + (rz * vz);
                if (rdotv > 0.0f) {
                    float specPow;

                    if (matShiny == 8.0f) {
                        float s = rdotv * rdotv;
                        s *= s;
                        s *= s;
                        specPow = s;
                    } else if (matShiny == 16.0f) {
                        float s = rdotv * rdotv;
                        s *= s;
                        s *= s;
                        s *= s;
                        specPow = s;
                    } else {
                        specPow = powf(rdotv, matShiny);
                    }

                    brightness += specPow * matSpec * ls->intensity * attenuation;
                }
            }
        }

        if (brightness < faceEmission) brightness = faceEmission;
        if (brightness < 0.0f) brightness = 0.0f;
        if (brightness > 1.0f) brightness = 1.0f;

        {
            const float shadeF = brightnessToShadeF(brightness);

            for (int k = 1; k < clippedCount - 1; k++) {
                submitClippedTri(
                    clipped[0],
                    clipped[k],
                    clipped[k + 1],
                    (Camera *)cam,
                    t.color,
                    t.emission,
                    255,
                    shadeF
                );
            }
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


static int sb3dRayTriangleHitDetailed(
    Vec3 rayOrig,
    Vec3 rayDir,
    float maxDist,
    Vec3 v0,
    Vec3 v1,
    Vec3 v2,
    float *outT,
    Vec3 *outNormal
)
{
    const float EPS = 0.0001f;

    const float e1x = v1.x - v0.x;
    const float e1y = v1.y - v0.y;
    const float e1z = v1.z - v0.z;

    const float e2x = v2.x - v0.x;
    const float e2y = v2.y - v0.y;
    const float e2z = v2.z - v0.z;

    const float px = (rayDir.y * e2z) - (rayDir.z * e2y);
    const float py = (rayDir.z * e2x) - (rayDir.x * e2z);
    const float pz = (rayDir.x * e2y) - (rayDir.y * e2x);

    const float det = (e1x * px) + (e1y * py) + (e1z * pz);

    if (det > -EPS && det < EPS) {
        return 0;
    }

    {
        const float tx = rayOrig.x - v0.x;
        const float ty = rayOrig.y - v0.y;
        const float tz = rayOrig.z - v0.z;

        const float invDet = 1.0f / det;
        const float u = ((tx * px) + (ty * py) + (tz * pz)) * invDet;

        if (u < 0.0f || u > 1.0f) {
            return 0;
        }

        const float qx = (ty * e1z) - (tz * e1y);
        const float qy = (tz * e1x) - (tx * e1z);
        const float qz = (tx * e1y) - (ty * e1x);

        const float v = ((rayDir.x * qx) + (rayDir.y * qy) + (rayDir.z * qz)) * invDet;

        if (v < 0.0f || (u + v) > 1.0f) {
            return 0;
        }

        {
            const float tHit = ((e2x * qx) + (e2y * qy) + (e2z * qz)) * invDet;

            if (tHit <= EPS || tHit > maxDist) {
                return 0;
            }

            if (outT) {
                *outT = tHit;
            }
        }
    }

    if (outNormal) {
        float nx = (e1y * e2z) - (e1z * e2y);
        float ny = (e1z * e2x) - (e1x * e2z);
        float nz = (e1x * e2y) - (e1y * e2x);

        const float nlen2 = (nx * nx) + (ny * ny) + (nz * nz);

        if (nlen2 > 0.000001f) {
            if (nlen2 < 0.999f || nlen2 > 1.001f) {
                const float invNLen = 1.0f / sqrtf(nlen2);
                nx *= invNLen;
                ny *= invNLen;
                nz *= invNLen;
            }
        } else {
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        }

        outNormal->x = nx;
        outNormal->y = ny;
        outNormal->z = nz;
    }

    return 1;
}

static void sb3dBuildHitBasis(
    Vec3 normal,
    Vec3 preferredForward,
    Vec3 *outRight,
    Vec3 *outUp,
    Vec3 *outForward
)
{
    float fx = normal.x;
    float fy = normal.y;
    float fz = normal.z;

    float ux, uy, uz;
    float rx, ry, rz;

    {
        const float flen2 = (fx * fx) + (fy * fy) + (fz * fz);
        if (flen2 > 0.000001f) {
            if (flen2 < 0.999f || flen2 > 1.001f) {
                const float invLen = 1.0f / sqrtf(flen2);
                fx *= invLen;
                fy *= invLen;
                fz *= invLen;
            }
        } else {
            fx = 0.0f;
            fy = 0.0f;
            fz = 1.0f;
        }
    }

    {
        const float d =
            (preferredForward.x * fx) +
            (preferredForward.y * fy) +
            (preferredForward.z * fz);

        ux = preferredForward.x - (fx * d);
        uy = preferredForward.y - (fy * d);
        uz = preferredForward.z - (fz * d);
    }

    {
        const float ulen2 = (ux * ux) + (uy * uy) + (uz * uz);

        if (ulen2 <= 0.000001f) {
            if (fy > -0.9f && fy < 0.9f) {
                ux = -fx * fy;
                uy = 1.0f - (fy * fy);
                uz = -fz * fy;
            } else {
                ux = 1.0f - (fx * fx);
                uy = -fy * fx;
                uz = -fz * fx;
            }
        }
    }

    {
        const float ulen2 = (ux * ux) + (uy * uy) + (uz * uz);

        if (ulen2 > 0.000001f) {
            const float invLen = 1.0f / sqrtf(ulen2);
            ux *= invLen;
            uy *= invLen;
            uz *= invLen;
        } else {
            ux = 0.0f;
            uy = 1.0f;
            uz = 0.0f;
        }
    }

    rx = (uy * fz) - (uz * fy);
    ry = (uz * fx) - (ux * fz);
    rz = (ux * fy) - (uy * fx);

    {
        const float rlen2 = (rx * rx) + (ry * ry) + (rz * rz);
        if (rlen2 > 0.000001f) {
            const float invLen = 1.0f / sqrtf(rlen2);
            rx *= invLen;
            ry *= invLen;
            rz *= invLen;
        } else {
            rx = 1.0f;
            ry = 0.0f;
            rz = 0.0f;
        }
    }

    uy = (fz * rx) - (fx * rz);
    uz = (fx * ry) - (fy * rx);
    ux = (fy * rz) - (fz * ry);

    {
        const float ulen2 = (ux * ux) + (uy * uy) + (uz * uz);
        if (ulen2 > 0.000001f) {
            const float invLen = 1.0f / sqrtf(ulen2);
            ux *= invLen;
            uy *= invLen;
            uz *= invLen;
        } else {
            ux = 0.0f;
            uy = 1.0f;
            uz = 0.0f;
        }
    }

    if (outRight) {
        outRight->x = rx;
        outRight->y = ry;
        outRight->z = rz;
    }

    if (outUp) {
        outUp->x = ux;
        outUp->y = uy;
        outUp->z = uz;
    }

    if (outForward) {
        outForward->x = fx;
        outForward->y = fy;
        outForward->z = fz;
    }
}

static int sb3dRayEntityCandidate(
    Vec3 rayOrig,
    Vec3 rayDir,
    float maxDist,
    const Entity *ent
)
{
    float vx, vy, vz;
    float t;
    float dx, dy, dz;
    float dist2;
    float r;

    if (!ent || !ent->mesh) return 0;

    vx = ent->pos.x - rayOrig.x;
    vy = ent->pos.y - rayOrig.y;
    vz = ent->pos.z - rayOrig.z;
    r  = ent->mesh->boundsRadius;

    t = (vx * rayDir.x) + (vy * rayDir.y) + (vz * rayDir.z);

    if (t < -r) {
        return 0;
    }

    if (t > (maxDist + r)) {
        return 0;
    }

    if (t <= 0.0f) {
        dx = rayOrig.x - ent->pos.x;
        dy = rayOrig.y - ent->pos.y;
        dz = rayOrig.z - ent->pos.z;
    }
    else if (t >= maxDist) {
        dx = (rayOrig.x + (rayDir.x * maxDist)) - ent->pos.x;
        dy = (rayOrig.y + (rayDir.y * maxDist)) - ent->pos.y;
        dz = (rayOrig.z + (rayDir.z * maxDist)) - ent->pos.z;
    }
    else {
        dx = (rayOrig.x + (rayDir.x * t)) - ent->pos.x;
        dy = (rayOrig.y + (rayDir.y * t)) - ent->pos.y;
        dz = (rayOrig.z + (rayDir.z * t)) - ent->pos.z;
    }

    dist2 = (dx * dx) + (dy * dy) + (dz * dz);

    return (dist2 <= (r * r));
}

int sb3dRaycastWorld(
    Vec3 rayOrig,
    Vec3 rayDir,
    float maxDist,
    SB3DRaycastHit *outHit
)
{
    int found = 0;
    float bestT = maxDist;

    if (!outHit) return 0;

    outHit->hit = 0;
    outHit->entityId = -1;
    outHit->triIndex = -1;
    outHit->distance = 0.0f;
    outHit->point = (Vec3){ 0.0f, 0.0f, 0.0f };
    outHit->normal = (Vec3){ 0.0f, 0.0f, 0.0f };
    outHit->right = (Vec3){ 1.0f, 0.0f, 0.0f };
    outHit->up = (Vec3){ 0.0f, 1.0f, 0.0f };
    outHit->forward = (Vec3){ 0.0f, 0.0f, 1.0f };

    {
        const float len2 =
            (rayDir.x * rayDir.x) +
            (rayDir.y * rayDir.y) +
            (rayDir.z * rayDir.z);

        if (len2 <= 0.000001f) {
            return 0;
        }

        if (len2 < 0.999f || len2 > 1.001f) {
            const float invLen = 1.0f / sqrtf(len2);
            rayDir.x *= invLen;
            rayDir.y *= invLen;
            rayDir.z *= invLen;
        }
    }

    for (int ei = 0; ei < WORLD_MAX; ei++) {
        Entity *ent = &worldEntities[ei];
        const Mesh *mesh;
        const Vec3 *verts;
        const Tri *tris;
        int triCount;

        float px, py, pz;
        float rx, ry, rz;
        float ux, uy, uz;
        float fx, fy, fz;

        if (!ent->active) continue;
        if (!ent->mesh) continue;
        if ((ent->flags & ENTITY_HITTEST) == 0) continue;
        if (!sb3dRayEntityCandidate(rayOrig, rayDir, bestT, ent)) continue;

        mesh = ent->mesh;
        verts = mesh->verts;
        tris = mesh->tris;
        triCount = mesh->triCount;

        if (!verts || !tris || triCount <= 0) continue;

        px = ent->pos.x;
        py = ent->pos.y;
        pz = ent->pos.z;

        rx = ent->right.x;
        ry = ent->right.y;
        rz = ent->right.z;

        ux = ent->up.x;
        uy = ent->up.y;
        uz = ent->up.z;

        fx = ent->forward.x;
        fy = ent->forward.y;
        fz = ent->forward.z;

        for (int ti = 0; ti < triCount; ti++) {
            const Tri *t = &tris[ti];
            const Vec3 *la = &verts[t->a];
            const Vec3 *lb = &verts[t->b];
            const Vec3 *lc = &verts[t->c];

            Vec3 w0, w1, w2;
            float hitT;
            Vec3 hitNormal;

            w0.x = px + (rx * la->x) + (ux * la->y) + (fx * la->z);
            w0.y = py + (ry * la->x) + (uy * la->y) + (fy * la->z);
            w0.z = pz + (rz * la->x) + (uz * la->y) + (fz * la->z);

            w1.x = px + (rx * lb->x) + (ux * lb->y) + (fx * lb->z);
            w1.y = py + (ry * lb->x) + (uy * lb->y) + (fy * lb->z);
            w1.z = pz + (rz * lb->x) + (uz * lb->y) + (fz * lb->z);

            w2.x = px + (rx * lc->x) + (ux * lc->y) + (fx * lc->z);
            w2.y = py + (ry * lc->x) + (uy * lc->y) + (fy * lc->z);
            w2.z = pz + (rz * lc->x) + (uz * lc->y) + (fz * lc->z);

            if (sb3dRayTriangleHitDetailed(
                    rayOrig,
                    rayDir,
                    bestT,
                    w0,
                    w1,
                    w2,
                    &hitT,
                    &hitNormal))
            {
                bestT = hitT;
                found = 1;

                outHit->hit = 1;
                outHit->entityId = ei;
                outHit->triIndex = ti;
                outHit->distance = hitT;

                outHit->point.x = rayOrig.x + (rayDir.x * hitT);
                outHit->point.y = rayOrig.y + (rayDir.y * hitT);
                outHit->point.z = rayOrig.z + (rayDir.z * hitT);

                outHit->normal = hitNormal;

                {
                    Vec3 preferredForward;

                    preferredForward.x = -rayDir.x;
                    preferredForward.y = -rayDir.y;
                    preferredForward.z = -rayDir.z;

                    sb3dBuildHitBasis(
                        hitNormal,
                        preferredForward,
                        &outHit->right,
                        &outHit->up,
                        &outHit->forward
                    );
                }
            }
        }
    }

    return found;
}


int sb3dRaycastFromCamera(
    const Camera *cam,
    float maxDist,
    SB3DRaycastHit *outHit
)
{
    if (!cam || !outHit) return 0;
    return sb3dRaycastWorld(cam->pos, cam->forward, maxDist, outHit);
}
