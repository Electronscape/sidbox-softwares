#include <stdint.h>
#include <math.h>

#include "../gfx.h"

#include "sb3d.h"



static Entity *renderEntities[WORLD_MAX];
static RenderTri g_renderTris[MAX_RENDER_TRIS];
static int g_renderTriCount = 0;







static Vec3 lerpVec3(Vec3 a, Vec3 b, float t)
{
    Vec3 out;
    out.x = a.x + ((b.x - a.x) * t);
    out.y = a.y + ((b.y - a.y) * t);
    out.z = a.z + ((b.z - a.z) * t);
    return out;
}

static float planeEval(Vec3 p, ClipPlane plane)
{
    switch (plane) {
        case PLANE_NEAR:   return p.z - NEAR_Z;
        case PLANE_LEFT:   return p.x + p.z * ((SCREEN_W * 0.5f) / PROJ_F);
        case PLANE_RIGHT:  return (p.z * ((SCREEN_W * 0.5f) / PROJ_F)) - p.x;
        case PLANE_TOP:    return (p.z * ((SCREEN_H * 0.5f) / PROJ_F)) - p.y;
        case PLANE_BOTTOM: return p.y + p.z * ((SCREEN_H * 0.5f) / PROJ_F);
    }

    return -1.0f;
}

static int pointInsidePlane(Vec3 p, ClipPlane plane)
{
    return (planeEval(p, plane) >= 0.0f);
}

static Vec3 intersectPlane(Vec3 a, Vec3 b, ClipPlane plane)
{
    float fa = planeEval(a, plane);
    float fb = planeEval(b, plane);
    float t  = fa / (fa - fb);

    return lerpVec3(a, b, t);
}

static int clipPolygonAgainstPlane(Vec3 *inVerts, int inCount, Vec3 *outVerts, ClipPlane plane)
{
    int outCount = 0;

    for (int i = 0; i < inCount; i++) {
        Vec3 current = inVerts[i];
        Vec3 prev    = inVerts[(i + inCount - 1) % inCount];

        int currentInside = pointInsidePlane(current, plane);
        int prevInside    = pointInsidePlane(prev, plane);

        if (prevInside && currentInside) {
            outVerts[outCount++] = current;
        }
        else if (prevInside && !currentInside) {
            outVerts[outCount++] = intersectPlane(prev, current, plane);
        }
        else if (!prevInside && currentInside) {
            outVerts[outCount++] = intersectPlane(prev, current, plane);
            outVerts[outCount++] = current;
        }
    }

    return outCount;
}

static int clipTriangleToFrustum(Vec3 a, Vec3 b, Vec3 c, Vec3 *outVerts)
{
    Vec3 poly1[CLIP_MAX_VERTS];
    Vec3 poly2[CLIP_MAX_VERTS];
    int count = 3;

    poly1[0] = a;
    poly1[1] = b;
    poly1[2] = c;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_NEAR);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly2, count, poly1, PLANE_LEFT);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_RIGHT);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly2, count, poly1, PLANE_TOP);
    if (count < 3) return 0;

    count = clipPolygonAgainstPlane(poly1, count, poly2, PLANE_BOTTOM);
    if (count < 3) return 0;

    for (int i = 0; i < count; i++) {
        outVerts[i] = poly2[i];
    }

    return count;
}







static Vec3 clipIntersectNear(Vec3 a, Vec3 b)
{
    float t = (NEAR_Z - a.z) / (b.z - a.z);
    return lerpVec3(a, b, t);
}

static void swapRenderTri(RenderTri *a, RenderTri *b)
{
    RenderTri t = *a;
    *a = *b;
    *b = t;
}

static int clipTriangleToNearPlane(Vec3 in0, Vec3 in1, Vec3 in2, Vec3 *out)
{
    Vec3 inVerts[3] = { in0, in1, in2 };
    Vec3 temp[4];

    int inCount = 0;
    int outCount = 0;

    for (int i = 0; i < 3; i++) {
        Vec3 current = inVerts[i];
        Vec3 prev    = inVerts[(i + 2) % 3];

        int currentInside = (current.z >= NEAR_Z);
        int prevInside    = (prev.z >= NEAR_Z);

        if (currentInside && prevInside) {
            temp[outCount++] = current;
        }
        else if (prevInside && !currentInside) {
            temp[outCount++] = clipIntersectNear(prev, current);
        }
        else if (!prevInside && currentInside) {
            temp[outCount++] = clipIntersectNear(prev, current);
            temp[outCount++] = current;
        }
    }

    for (int i = 0; i < outCount; i++) {
        out[i] = temp[i];
    }

    return outCount;
}


static void sortRenderList(void)
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


static void submitClippedTri(Vec3 a, Vec3 b, Vec3 c, uint8_t color, float shadeF)
{
    Vec2 pa, pb, pc;

    if (!projectPoint(a, &pa)) return;
    if (!projectPoint(b, &pb)) return;
    if (!projectPoint(c, &pc)) return;

    if (!triangleFacingScreen(pa, pb, pc)) { return; }

    if (g_renderTriCount >= MAX_RENDER_TRIS) {
        return;
    }

    RenderTri rt;
    rt.p0 = pa;
    rt.p1 = pb;
    rt.p2 = pc;
    rt.color  = color;
    rt.shadeF = shadeF;
    rt.depth  = (a.z + b.z + c.z) / 3.0f;

    g_renderTris[g_renderTriCount++] = rt;
}

static int triangleFullyBehindNearPlane(Vec3 a, Vec3 b, Vec3 c)
{
    if (a.z <= NEAR_Z && b.z <= NEAR_Z && c.z <= NEAR_Z) {
        return 1;
    }
    return 0;
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



int projectPoint(Vec3 p, Vec2 *out)
{
    float f = 200.0f;

    if (p.z <= NEAR_Z) {
        return 0;
    }

    //out->x = (int)((p.x / p.z) * f) + (SCREEN_W / 2);
    //out->y = (int)((-p.y / p.z) * f) + (SCREEN_H / 2);

    out->x = (int)lroundf((p.x / p.z) * f) + (SCREEN_W / 2);
    out->y = (int)lroundf((-p.y / p.z) * f) + (SCREEN_H / 2);

    return 1;
}





int clipLineToNearPlane(Vec3 *a, Vec3 *b)
{
    int a_in = (a->z >= NEAR_Z);
    int b_in = (b->z >= NEAR_Z);

    if (!a_in && !b_in) {
        return 0;
    }

    if (a_in && b_in) {
        return 1;
    }

    float t = (NEAR_Z - a->z) / (b->z - a->z);

    Vec3 p;
    p.x = a->x + t * (b->x - a->x);
    p.y = a->y + t * (b->y - a->y);
    p.z = NEAR_Z;

    if (!a_in) {
        *a = p;
    } else {
        *b = p;
    }

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

    float halfW = (SCREEN_W * 0.5f) / PROJ_F;
    float halfH = (SCREEN_H * 0.5f) / PROJ_F;

    /* near plane */
    if (center.z + r < NEAR_Z) {
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



void Render3D(const Camera *cam)
{

    submitWorldEntities(cam);
    sortRenderList();

    for (int i = 0; i < g_renderTriCount; i++) {
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

static int triangleFacingCamera(Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 ab = vec3Sub(b, a);
    Vec3 ac = vec3Sub(c, a);
    Vec3 n  = vec3Cross(ab, ac);

    /* camera is at origin in camera-space */
    float d = vec3Dot(n, a);

    return (d < 0.0f);
}

void submitEntitySolid(const Entity *ent, const Camera *cam)
{
    for (int i = 0; i < ent->mesh->triCount; i++) {
        Tri t = ent->mesh->tris[i];

        Vec3 wa = entityLocalToWorld(ent, ent->mesh->verts[t.a]);
        Vec3 wb = entityLocalToWorld(ent, ent->mesh->verts[t.b]);
        Vec3 wc = entityLocalToWorld(ent, ent->mesh->verts[t.c]);

        float brightness = computeTriangleBrightness(wa, wb, wc);
        float shadeF = brightnessToShadeF(brightness);

        Vec3 a = worldToCamera(wa, *cam);
        Vec3 b = worldToCamera(wb, *cam);
        Vec3 c = worldToCamera(wc, *cam);

        //if (!triangleFacingCamera(a, b, c)) { continue; }

        Vec3 clipped[CLIP_MAX_VERTS];
        int clippedCount = clipTriangleToFrustum(a, b, c, clipped);

        if (clippedCount < 3) {
            continue;
        }

        for (int k = 1; k < clippedCount - 1; k++) {
            submitClippedTri(
                clipped[0],
                clipped[k],
                clipped[k + 1],
                t.color,
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

    if (!clipLineToNearPlane(&a, &b)) {
        return;
    }

    if (!projectPoint(a, &pa)) {
        return;
    }

    if (!projectPoint(b, &pb)) {
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




