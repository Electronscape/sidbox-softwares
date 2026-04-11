#include "rc3d.h"

#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "rc3d_map.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RC3D_DRAW_MINIMAP      1
#define RC3D_DRAW_HUD          1

#define RC3D_FOV_DEG           75.0f
#define RC3D_TURN_SPEED        2.4f
#define RC3D_MOVE_SPEED        3.0f
#define RC3D_MOUSE_SENS        0.0035f
#define RC3D_EPSILON           0.0001f
#define RC3D_COLLISION_SKIN    0.02f
#define RC3D_FIXED_SHIFT       16
#define RC3D_FIXED_ONE         (1 << RC3D_FIXED_SHIFT)
#define RC3D_TRIG_LUT_BITS     11
#define RC3D_TRIG_LUT_SIZE     (1 << RC3D_TRIG_LUT_BITS)
#define RC3D_TRIG_LUT_MASK     (RC3D_TRIG_LUT_SIZE - 1)
#define RC3D_MAX_RAY_DIST      50.0f
#define RC3D_MAX_PORTAL_STEPS  24   // how many sectors we can look through to render before not bothering anymore
#define RC3D_MAX_MASKED_TRACE_DEPTH 8
#define RC3D_MAX_WALL_SPANS_PER_COLUMN 32
#define RC3D_DEPTH_SCALE       1024.0f

#define RC3D_PLAYER_EYE_HEIGHT 0.5f
#define RC3D_GRAVITY           18.0f
#define RC3D_STEP_SNAP_SPEED   24.0f

#define PLAYER_HEIGHT          0.6f
#define PLAYER_STEPUP          0.35f
#define PLAYER_RADIUS          0.40f

#define RC3D_TEX_SIZE          64
#define RC3D_TEX_MASK          (RC3D_TEX_SIZE - 1)

extern uint8_t spr_man[]; // test sprite


#define RC3D_TEXID_SPRITE_MAN  253
#define RC3D_TEXID_SKYBOX      255
#define RC3D_SKYBOX_W          1024
#define RC3D_SKYBOX_H          256
#define RC3D_SPRITE_TEX_TRANSPARENT 0
#define RC3D_TEST_SPRITE_WIDTH  0.75f
#define RC3D_TEST_SPRITE_HEIGHT 0.75f

static float g_renderEyeZ = 0.0f;
static float g_halfFovRad = 0.0f;
static float g_planeScaleConst = 0.0f;
static float g_projPlaneConst = 0.0f;
static float g_camStepConst = 0.0f;
static float g_angleToLutScale = 0.0f;
static float g_sinLut[RC3D_TRIG_LUT_SIZE];
static float g_cosLut[RC3D_TRIG_LUT_SIZE];
static int g_trigLutInit = 0;



typedef int32_t RC3D_Fixed;

typedef struct {
    RC3D_Fixed x;
    RC3D_Fixed y;
} RC3D_FixedVec2;

typedef struct {
    float x;
    float y;
    RC3D_Fixed xFixed;
    RC3D_Fixed yFixed;
    float z;
    float vz;
    float tHeadbob; // headbob 
    float angle;
    int sector;
} RC3D_Player;





typedef struct {
    float t;
    int wallIndex;
    int hit;
} RC3D_WallHit;

typedef struct {
    int hit;
    int wallIndex;
    float normalX;
    float normalY;
    float penetration;
} RC3D_BlockingContact;

typedef struct {
    uint8_t pix[RC3D_TEX_SIZE * RC3D_TEX_SIZE];
} RC3D_Texture;

typedef struct {
    float x;
    float y;
    RC3D_Fixed xFixed;
    RC3D_Fixed yFixed;
    float baseZ;
    float width;
    float height;
    uint8_t texId;
    uint8_t active;
} RC3D_Sprite;

typedef struct {
    int16_t y0;
    int16_t y1;
    uint16_t depth;
} RC3D_WallDepthSpan;

static RC3D_Player g_player;
static RC3D_Sprite g_testSprite;
static RC3D_Sprite g_testSprite2;

static const RC3D_Map *g_map = &g_rc3dDemoMap;
static RC3D_Map g_loadedMap;
static int g_loadedMapValid = 0;

static int horizonGlobal = 0;
static float wallTexUBaseGlobal = 0.0f;
static float wallTexRotCosGlobal = 1.0f;
static float wallTexRotSinGlobal = 0.0f;
static float projPlaneGlobal = 0.0f;

static RC3D_Texture g_rc3dTextures[256];
static int g_rc3dTexturesInit = 0;


typedef enum {
    RC3D_WALLCLASS_NONE = 0,
    RC3D_WALLCLASS_SOLID,
    RC3D_WALLCLASS_MIDDLE,
    RC3D_WALLCLASS_UPPER_LOWER,
    RC3D_WALLCLASS_PORTAL
} RC3D_WallClass;

typedef enum {
    RC3D_TEX_XMODE_TILE = 0,
    RC3D_TEX_XMODE_CLAMP_RIGHT,
    RC3D_TEX_XMODE_STRETCH
} RC3D_TexXMode;

typedef struct {
    float dx;
    float dy;
    float len;
    float invLenSq;

    float texCos;
    float texSin;

    uint8_t hasTexRotate;
    uint8_t wallClass;
    uint8_t texXMode;

    int32_t backWallIndex;   // matching wall in neighbour sector, or -1
} RC3D_WallCache;

typedef struct {
    float floorInvScaleX;
    float floorInvScaleY;
    float floorCos;
    float floorSin;

    float ceilInvScaleX;
    float ceilInvScaleY;
    float ceilCos;
    float ceilSin;

    uint8_t floorSimple;
    uint8_t ceilSimple;
} RC3D_SectorCache;



static RC3D_WallCache *g_wallCache = NULL;
static int g_wallCacheCount = 0;
static RC3D_SectorCache *g_sectorCache = NULL;
static int g_sectorCacheCount = 0;
static RC3D_FixedVec2 *g_fixedVerts = NULL;
static int g_fixedVertCount = 0;

static float g_invDTable[(SCREEN_H * 2) + 1];
static int g_invDTableInit = 0;
static uint16_t g_skyXTable[SCREEN_W];
static RC3D_WallDepthSpan g_wallDepthSpans[SCREEN_W][RC3D_MAX_WALL_SPANS_PER_COLUMN];
static uint8_t g_wallDepthSpanCount[SCREEN_W];


/* ------------------------------------------------------------------------- */
/* forward decls                                                             */
/* ------------------------------------------------------------------------- */

static int tryMovePlayerSliding(float moveX, float moveY);
void screenupdate(void);

static inline RC3D_Fixed rc3dFloatToFixed(float v)
{
    if (v >= 0.0f) {
        return (RC3D_Fixed)(v * (float)RC3D_FIXED_ONE + 0.5f);
    }

    return (RC3D_Fixed)(v * (float)RC3D_FIXED_ONE - 0.5f);
}

static inline float rc3dFixedToFloat(RC3D_Fixed v)
{
    return (float)v / (float)RC3D_FIXED_ONE;
}

static inline RC3D_Fixed rc3dFixedMul(RC3D_Fixed a, RC3D_Fixed b)
{
    return (RC3D_Fixed)(((int64_t)a * (int64_t)b) >> RC3D_FIXED_SHIFT);
}

static inline int64_t rc3dFixedSq(RC3D_Fixed v)
{
    return (int64_t)v * (int64_t)v;
}

static inline void rc3dSyncPlayerFloatXY(void)
{
    g_player.x = rc3dFixedToFloat(g_player.xFixed);
    g_player.y = rc3dFixedToFloat(g_player.yFixed);
}

static inline void rc3dSetPlayerWorldXYFixed(RC3D_Fixed x, RC3D_Fixed y)
{
    g_player.xFixed = x;
    g_player.yFixed = y;
    rc3dSyncPlayerFloatXY();
}

static inline void rc3dSetSpriteWorldXYFixed(RC3D_Sprite *sprite, RC3D_Fixed x, RC3D_Fixed y)
{
    if (!sprite) return;

    sprite->xFixed = x;
    sprite->yFixed = y;
    sprite->x = rc3dFixedToFloat(x);
    sprite->y = rc3dFixedToFloat(y);
}

static void rc3dBuildTrigTables(void)
{
    if (g_trigLutInit) {
        return;
    }

    for (int i = 0; i < RC3D_TRIG_LUT_SIZE; ++i) {
        const float angle =
            ((float)i * (float)(M_PI * 2.0f)) / (float)RC3D_TRIG_LUT_SIZE;
        g_sinLut[i] = sinf(angle);
        g_cosLut[i] = cosf(angle);
    }

    g_halfFovRad = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    g_planeScaleConst = tanf(g_halfFovRad);
    g_projPlaneConst = (SCREEN_W * 0.5f) / g_planeScaleConst;
    g_camStepConst = 2.0f / (float)(SCREEN_W - 1);
    g_angleToLutScale =
        (float)RC3D_TRIG_LUT_SIZE / (float)(M_PI * 2.0f);

    g_trigLutInit = 1;
}

static inline int rc3dAngleToLutIndex(float angle)
{
    const float scaled = angle * g_angleToLutScale;
    int idx =
        (int)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));

    idx %= RC3D_TRIG_LUT_SIZE;
    if (idx < 0) idx += RC3D_TRIG_LUT_SIZE;

    return idx & RC3D_TRIG_LUT_MASK;
}

static inline void rc3dLookupAngleTrig(float angle, float *outCos, float *outSin)
{
    const int idx = rc3dAngleToLutIndex(angle);

    if (outCos) *outCos = g_cosLut[idx];
    if (outSin) *outSin = g_sinLut[idx];
}

static inline uint16_t rc3dEncodeDepth(float dist)
{
    if (dist <= 0.0f) {
        return 0;
    }

    if (dist >= RC3D_MAX_RAY_DIST) {
        return UINT16_MAX - 1u;
    }

    {
        const float scaled = dist * RC3D_DEPTH_SCALE;
        if (scaled >= (float)(UINT16_MAX - 1u)) {
            return UINT16_MAX - 1u;
        }

        return (uint16_t)(scaled + 0.5f);
    }
}

static inline void rc3dClearWallDepthSpans(void)
{
    memset(g_wallDepthSpanCount, 0, sizeof(g_wallDepthSpanCount));
}

static inline void rc3dRecordWallDepthSpan(int sx, int y0, int y1, float hitDist)
{
    uint8_t count;

    if ((unsigned)sx >= SCREEN_W) return;
    if (y0 > y1) return;

    count = g_wallDepthSpanCount[sx];
    if (count >= RC3D_MAX_WALL_SPANS_PER_COLUMN) {
        return;
    }

    g_wallDepthSpans[sx][count].y0 = (int16_t)y0;
    g_wallDepthSpans[sx][count].y1 = (int16_t)y1;
    g_wallDepthSpans[sx][count].depth = rc3dEncodeDepth(hitDist);
    g_wallDepthSpanCount[sx] = (uint8_t)(count + 1);
}

static inline int rc3dWallSpansBlockPixel(int sx, int y, uint16_t spriteDepth)
{
    const uint8_t count = g_wallDepthSpanCount[sx];

    for (uint8_t i = 0; i < count; ++i) {
        const RC3D_WallDepthSpan *span = &g_wallDepthSpans[sx][i];

        if (y < span->y0 || y > span->y1) {
            continue;
        }

        if (span->depth <= spriteDepth) {
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* texture / tables                                                          */
/* ------------------------------------------------------------------------- */

static inline uint8_t wallTexelFetch(uint8_t texId, int tx, int ty)
{
    const RC3D_Texture *t = &g_rc3dTextures[texId];

    tx &= RC3D_TEX_MASK;
    ty &= RC3D_TEX_MASK;

    return t->pix[(ty * RC3D_TEX_SIZE) + tx];
}

static inline uint8_t texelFetch(uint8_t texId, int tx, int ty)
{
    const RC3D_Texture *t = &g_rc3dTextures[texId];
    tx &= RC3D_TEX_MASK;
    ty &= RC3D_TEX_MASK;
    return t->pix[(ty * RC3D_TEX_SIZE) + tx];
}


extern uint8_t tex_notset[];
extern uint8_t txt_brick1[];
extern uint8_t txt_dirt[];
extern uint8_t txt_lava[];
extern uint8_t txt_water[];
extern uint8_t txt_grass[];
extern uint8_t txt_burnedwood[];
extern uint8_t txt_sky[];



extern uint8_t tex_skybox[];    // the skybox

static void rc3dBuildDefaultTextures(void)
{
    int x, y, i;
    int32_t tindex = 0;

    for (i = 0; i < 256; ++i) {
        for (y = 0; y < RC3D_TEX_SIZE; ++y) {
            for (x = 0; x < RC3D_TEX_SIZE; ++x) {
                g_rc3dTextures[i].pix[(y * RC3D_TEX_SIZE) + x] = tex_notset[tindex++];
            }
        }
    }

    tindex = 0;
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            g_rc3dTextures[0].pix[tindex] = tex_notset[tindex];
            g_rc3dTextures[1].pix[tindex] = txt_brick1[tindex];
            g_rc3dTextures[2].pix[tindex] = txt_dirt[tindex];
            g_rc3dTextures[3].pix[tindex] = txt_grass[tindex];
            g_rc3dTextures[4].pix[tindex] = txt_lava[tindex];
            g_rc3dTextures[5].pix[tindex] = txt_water[tindex];
            g_rc3dTextures[6].pix[tindex] = txt_burnedwood[tindex];
            g_rc3dTextures[7].pix[tindex] = txt_sky[tindex];
            g_rc3dTextures[RC3D_TEXID_SPRITE_MAN].pix[tindex] = spr_man[tindex];
            
            tindex++;
        }
    }

    g_rc3dTexturesInit = 1;
}

#define RC3D_TEX_WALL_ANGLE_SHIFT   4u
#define RC3D_TEX_WALL_ANGLE_MASK    (0xFFFFu << RC3D_TEX_WALL_ANGLE_SHIFT)

static inline float rc3dWallTexAngleFromFlags(uint32_t texture_flags)
{
    const uint32_t packed =
        (texture_flags & RC3D_TEX_WALL_ANGLE_MASK) >> RC3D_TEX_WALL_ANGLE_SHIFT;

    return (float)packed * ((float)(M_PI * 2.0) / 65536.0f);
}

/* ------------------------------------------------------------------------- */
/* binary map loading                                                        */
/* ------------------------------------------------------------------------- */



static void rc3dFreeWallCache(void)
{
    if (g_wallCache) {
        free(g_wallCache);
        g_wallCache = NULL;
    }

    g_wallCacheCount = 0;
}

static void rc3dFreeSectorCache(void)
{
    if (g_sectorCache) {
        free(g_sectorCache);
        g_sectorCache = NULL;
    }

    g_sectorCacheCount = 0;
}

static void rc3dFreeFixedVertCache(void)
{
    if (g_fixedVerts) {
        free(g_fixedVerts);
        g_fixedVerts = NULL;
    }

    g_fixedVertCount = 0;
}

static int rc3dWallSectorIndex(int wallIndex)
{
    for (int s = 0; s < g_map->sectorCount; ++s) {
        const RC3D_Sector *sec = &g_map->sectors[s];
        const int start = sec->wallStart;
        const int end   = start + sec->wallCount;

        if (wallIndex >= start && wallIndex < end) {
            return s;
        }
    }

    return -1;
}

static int rc3dBuildFixedVertCacheForCurrentMap(void)
{
    rc3dFreeFixedVertCache();

    if (!g_map || g_map->vertCount <= 0) {
        return 1;
    }

    g_fixedVerts =
        (RC3D_FixedVec2 *)malloc(sizeof(RC3D_FixedVec2) * (size_t)g_map->vertCount);
    if (!g_fixedVerts) {
        return 0;
    }

    g_fixedVertCount = g_map->vertCount;

    for (int i = 0; i < g_map->vertCount; ++i) {
        g_fixedVerts[i].x = rc3dFloatToFixed(g_map->verts[i].x);
        g_fixedVerts[i].y = rc3dFloatToFixed(g_map->verts[i].y);
    }

    return 1;
}

static int rc3dBuildSectorCacheForCurrentMap(void)
{
    rc3dFreeSectorCache();

    if (!g_map || g_map->sectorCount <= 0) {
        return 1;
    }

    g_sectorCache =
        (RC3D_SectorCache *)malloc(sizeof(RC3D_SectorCache) * (size_t)g_map->sectorCount);
    if (!g_sectorCache) {
        return 0;
    }

    g_sectorCacheCount = g_map->sectorCount;

    for (int i = 0; i < g_map->sectorCount; ++i) {
        const RC3D_Sector *sec = &g_map->sectors[i];
        RC3D_SectorCache *cache = &g_sectorCache[i];

        float floorScaleX = sec->floorTexScaleX;
        float floorScaleY = sec->floorTexScaleY;
        float ceilScaleX = sec->ceilTexScaleX;
        float ceilScaleY = sec->ceilTexScaleY;

        if (fabsf(floorScaleX) < RC3D_EPSILON) floorScaleX = 1.0f;
        if (fabsf(floorScaleY) < RC3D_EPSILON) floorScaleY = 1.0f;
        if (fabsf(ceilScaleX) < RC3D_EPSILON)  ceilScaleX = 1.0f;
        if (fabsf(ceilScaleY) < RC3D_EPSILON)  ceilScaleY = 1.0f;

        cache->floorInvScaleX = 1.0f / floorScaleX;
        cache->floorInvScaleY = 1.0f / floorScaleY;
        cache->ceilInvScaleX  = 1.0f / ceilScaleX;
        cache->ceilInvScaleY  = 1.0f / ceilScaleY;

        if (fabsf(sec->floorTexAngle) > 0.0001f) {
            cache->floorCos = cosf(sec->floorTexAngle);
            cache->floorSin = sinf(sec->floorTexAngle);
        } else {
            cache->floorCos = 1.0f;
            cache->floorSin = 0.0f;
        }

        if (fabsf(sec->ceilTexAngle) > 0.0001f) {
            cache->ceilCos = cosf(sec->ceilTexAngle);
            cache->ceilSin = sinf(sec->ceilTexAngle);
        } else {
            cache->ceilCos = 1.0f;
            cache->ceilSin = 0.0f;
        }

        cache->floorSimple =
            (cache->floorCos > 0.9999f && cache->floorCos < 1.0001f &&
             cache->floorSin > -0.0001f && cache->floorSin < 0.0001f &&
             floorScaleX > 0.9999f && floorScaleX < 1.0001f &&
             floorScaleY > 0.9999f && floorScaleY < 1.0001f) ? 1u : 0u;

        cache->ceilSimple =
            (cache->ceilCos > 0.9999f && cache->ceilCos < 1.0001f &&
             cache->ceilSin > -0.0001f && cache->ceilSin < 0.0001f &&
             ceilScaleX > 0.9999f && ceilScaleX < 1.0001f &&
             ceilScaleY > 0.9999f && ceilScaleY < 1.0001f) ? 1u : 0u;
    }

    return 1;
}



static int rc3dBuildWallCacheForCurrentMap(void)
{
    rc3dFreeWallCache();

    if (!g_map || g_map->wallCount <= 0) {
        return 1;
    }

    g_wallCache = (RC3D_WallCache *)malloc(sizeof(RC3D_WallCache) * (size_t)g_map->wallCount);
    if (!g_wallCache) {
        return 0;
    }

    g_wallCacheCount = g_map->wallCount;

    for (int i = 0; i < g_map->wallCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];
        const RC3D_Vec2 *a = &g_map->verts[w->v0];
        const RC3D_Vec2 *b = &g_map->verts[w->v1];
        const float texAngle = rc3dWallTexAngleFromFlags(w->tex_flags);

        RC3D_WallCache *wc = &g_wallCache[i];

        wc->dx = b->x - a->x;
        wc->dy = b->y - a->y;
        {
            const float lenSq = (wc->dx * wc->dx) + (wc->dy * wc->dy);
            if (lenSq > RC3D_EPSILON) {
                wc->len = sqrtf(lenSq);
                wc->invLenSq = 1.0f / lenSq;
            } else {
                wc->len = 0.0f;
                wc->invLenSq = 0.0f;
            }
        }

        if (fabsf(texAngle) > 0.0001f) {
            wc->texCos = cosf(texAngle);
            wc->texSin = sinf(texAngle);
            wc->hasTexRotate = 1;
        } else {
            wc->texCos = 1.0f;
            wc->texSin = 0.0f;
            wc->hasTexRotate = 0;
        }

        {
            const uint8_t flags = w->flags;

            if (flags & RC3D_WALL_SOLID) {
                wc->wallClass = RC3D_WALLCLASS_SOLID;
            }
            else if ((flags & RC3D_WALL_MIDDLE) && !(flags & RC3D_WALL_PORTAL)) {
                wc->wallClass = RC3D_WALLCLASS_MIDDLE;
            }
            else if (flags & RC3D_WALL_PORTAL) {
                wc->wallClass = RC3D_WALLCLASS_PORTAL;
            }
            else if ((flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
                     !(flags & RC3D_WALL_MIDDLE)) {
                wc->wallClass = RC3D_WALLCLASS_UPPER_LOWER;
            }
            else {
                wc->wallClass = RC3D_WALLCLASS_NONE;
            }
        }

        {
            const int clampXL = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) ? 1 : 0;
            const int clampXR = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) ? 1 : 0;

            if (clampXL && clampXR) {
                wc->texXMode = RC3D_TEX_XMODE_STRETCH;
            } else if (clampXR && !clampXL) {
                wc->texXMode = RC3D_TEX_XMODE_CLAMP_RIGHT;
            } else {
                wc->texXMode = RC3D_TEX_XMODE_TILE;
            }
        }

        wc->backWallIndex = -1;
    }

    for (int i = 0; i < g_map->wallCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];
        if (w->neighbour < 0 || w->neighbour >= g_map->sectorCount) {
            continue;
        }

        const int thisSector = rc3dWallSectorIndex(i);
        if (thisSector < 0) {
            continue;
        }

        const RC3D_Sector *nextSec = &g_map->sectors[w->neighbour];
        const int nextStart = nextSec->wallStart;
        const int nextEnd   = nextStart + nextSec->wallCount;

        for (int j = nextStart; j < nextEnd; ++j) {
            const RC3D_Wall *tw = &g_map->walls[j];

            if (tw->neighbour != thisSector) {
                continue;
            }

            if ((tw->v0 == w->v1 && tw->v1 == w->v0) ||
                (tw->v0 == w->v0 && tw->v1 == w->v1))
            {
                g_wallCache[i].backWallIndex = j;
                break;
            }
        }
    }

    return 1;
}



static void rc3dBuildInvDTable(void)
{
    for (int d = -SCREEN_H; d <= SCREEN_H; ++d) {
        if (d == 0) {
            g_invDTable[d + SCREEN_H] = 0.0f;
        } else {
            g_invDTable[d + SCREEN_H] = 1.0f / (float)d;
        }
    }

    g_invDTableInit = 1;
}







static int readExact(FILE *f, void *dst, size_t size)
{
    return fread(dst, 1, size, f) == size;
}

void rc3dMapFreeBinary(RC3D_Map *map)
{
    if (!map) return;

    if (map->verts)   free((void *)map->verts);
    if (map->walls)   free((void *)map->walls);
    if (map->sectors) free((void *)map->sectors);

    map->verts = NULL;
    map->walls = NULL;
    map->sectors = NULL;

    map->vertCount = 0;
    map->wallCount = 0;
    map->sectorCount = 0;

    map->startSector = -1;
    map->startX = 0.0f;
    map->startY = 0.0f;
    map->startAngle = 0.0f;
}

int rc3dMapLoadBinary(const char *path, RC3D_Map *outMap)
{
    FILE *f;
    char magic[8];

    uint32_t vertCount;
    uint32_t wallCount;
    uint32_t sectorCount;

    int32_t startSector;
    float startX;
    float startY;
    float startAngle;

    RC3D_Vec2 *verts = NULL;
    RC3D_Wall *walls = NULL;
    RC3D_Sector *sectors = NULL;

    if (!path || !outMap) return 0;

    rc3dMapFreeBinary(outMap);

    f = fopen(path, "rb");
    if (!f) return 0;

    if (!readExact(f, magic, sizeof(magic))) {
        fclose(f);
        return 0;
    }

    if (memcmp(magic, "RC3DMAP1", 8) != 0) {
        fclose(f);
        return 0;
    }

    if (!readExact(f, &vertCount, sizeof(vertCount)) ||
        !readExact(f, &wallCount, sizeof(wallCount)) ||
        !readExact(f, &sectorCount, sizeof(sectorCount))) {
        fclose(f);
        return 0;
    }

    if (!readExact(f, &startSector, sizeof(startSector)) ||
        !readExact(f, &startX, sizeof(startX)) ||
        !readExact(f, &startY, sizeof(startY)) ||
        !readExact(f, &startAngle, sizeof(startAngle))) {
        fclose(f);
        return 0;
    }

    verts   = (RC3D_Vec2 *)malloc(sizeof(RC3D_Vec2) * vertCount);
    walls   = (RC3D_Wall *)malloc(sizeof(RC3D_Wall) * wallCount);
    sectors = (RC3D_Sector *)malloc(sizeof(RC3D_Sector) * sectorCount);

    if ((vertCount && !verts) || (wallCount && !walls) || (sectorCount && !sectors)) {
        fclose(f);
        free(verts);
        free(walls);
        free(sectors);
        return 0;
    }

    for (uint32_t i = 0; i < vertCount; i++) {
        if (!readExact(f, &verts[i].x, sizeof(float)) ||
            !readExact(f, &verts[i].y, sizeof(float))) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }
    }

    for (uint32_t i = 0; i < wallCount; i++) {
        int32_t v0;
        int32_t v1;
        int32_t neighbour;

        if (!readExact(f, &v0, sizeof(v0)) ||
            !readExact(f, &v1, sizeof(v1)) ||
            !readExact(f, &neighbour, sizeof(neighbour)) ||
            !readExact(f, &walls[i].openBottom, sizeof(float)) ||
            !readExact(f, &walls[i].openTop, sizeof(float)) ||
            !readExact(f, &walls[i].upperColor, sizeof(uint8_t)) ||
            !readExact(f, &walls[i].midColor, sizeof(uint8_t)) ||
            !readExact(f, &walls[i].lowerColor, sizeof(uint8_t)) ||
            !readExact(f, &walls[i].flags, sizeof(uint8_t)) ||
            !readExact(f, &walls[i].tex_flags, sizeof(uint32_t))
        ) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }

        walls[i].v0 = (int)v0;
        walls[i].v1 = (int)v1;
        walls[i].neighbour = (int)neighbour;
    }

    for (uint32_t i = 0; i < sectorCount; i++) {
        int32_t wallStart;
        int32_t wallCount_i;
        int32_t boundaryCount;

        if (!readExact(f, &wallStart, sizeof(wallStart)) ||
            !readExact(f, &wallCount_i, sizeof(wallCount_i)) ||
            !readExact(f, &boundaryCount, sizeof(boundaryCount)) ||
            !readExact(f, &sectors[i].floorHeight, sizeof(float)) ||
            !readExact(f, &sectors[i].ceilHeight, sizeof(float)) ||
            !readExact(f, &sectors[i].floorColor, sizeof(uint8_t)) ||
            !readExact(f, &sectors[i].ceilColor, sizeof(uint8_t)) ||
            !readExact(f, &sectors[i].floorTexScaleX, sizeof(float)) ||
            !readExact(f, &sectors[i].floorTexScaleY, sizeof(float)) ||
            !readExact(f, &sectors[i].floorTexAngle, sizeof(float)) ||
            !readExact(f, &sectors[i].ceilTexScaleX, sizeof(float)) ||
            !readExact(f, &sectors[i].ceilTexScaleY, sizeof(float)) ||
            !readExact(f, &sectors[i].ceilTexAngle, sizeof(float))) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }

        sectors[i].wallStart = (int)wallStart;
        sectors[i].wallCount = (int)wallCount_i;
        sectors[i].boundaryCount = (int)boundaryCount;
    }

    fclose(f);

    if (startSector < -1 || startSector >= (int32_t)sectorCount) {
        free(verts);
        free(walls);
        free(sectors);
        return 0;
    }

    for (uint32_t i = 0; i < wallCount; i++) {
        if (walls[i].v0 < 0 || walls[i].v0 >= (int)vertCount ||
            walls[i].v1 < 0 || walls[i].v1 >= (int)vertCount) {
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }

        if (walls[i].neighbour >= (int)sectorCount) {
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }
    }

    for (uint32_t i = 0; i < sectorCount; i++) {
        if (sectors[i].wallStart < 0 ||
            sectors[i].wallCount < 0 ||
            sectors[i].boundaryCount < 0 ||
            (sectors[i].wallStart + sectors[i].wallCount) > (int)wallCount ||
            sectors[i].boundaryCount > sectors[i].wallCount) {
            free(verts);
            free(walls);
            free(sectors);
            return 0;
        }
    }

    outMap->verts = verts;
    outMap->vertCount = (int)vertCount;
    outMap->walls = walls;
    outMap->wallCount = (int)wallCount;
    outMap->sectors = sectors;
    outMap->sectorCount = (int)sectorCount;
    outMap->startSector = (int)startSector;
    outMap->startX = startX;
    outMap->startY = startY;
    outMap->startAngle = startAngle;

    return 1;
}


int rc3dLoadMapBinary(const char *path)
{
    RC3D_Map newMap;

    newMap.verts = NULL;
    newMap.walls = NULL;
    newMap.sectors = NULL;
    newMap.vertCount = 0;
    newMap.wallCount = 0;
    newMap.sectorCount = 0;
    newMap.startSector = -1;
    newMap.startX = 0.0f;
    newMap.startY = 0.0f;
    newMap.startAngle = 0.0f;

    if (!rc3dMapLoadBinary(path, &newMap)) {
        return 0;
    }

    if (g_loadedMapValid) {
        rc3dMapFreeBinary(&g_loadedMap);
        g_loadedMapValid = 0;
    }

    g_loadedMap = newMap;
    g_loadedMapValid = 1;
    g_map = &g_loadedMap;

    rc3dBuildFixedVertCacheForCurrentMap();
    rc3dBuildWallCacheForCurrentMap();
    rc3dBuildSectorCacheForCurrentMap();

    return 1;
}


void rc3dUnloadMapBinary(void)
{
    if (g_loadedMapValid) {
        rc3dMapFreeBinary(&g_loadedMap);
        g_loadedMapValid = 0;
    }

    g_map = &g_rc3dDemoMap;
    rc3dBuildFixedVertCacheForCurrentMap();
    rc3dBuildWallCacheForCurrentMap();
    rc3dBuildSectorCacheForCurrentMap();
}



/* ------------------------------------------------------------------------- */
/* movement / collision                                                      */
/* ------------------------------------------------------------------------- */

static float wrapAngle(float a)
{
    while (a < -(float)M_PI) a += (float)(M_PI * 2.0f);
    while (a >  (float)M_PI) a -= (float)(M_PI * 2.0f);
    return a;
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


static int pointInSectorFixed(RC3D_Fixed px, RC3D_Fixed py, int sectorIndex)
{
    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_FixedVec2 *verts = g_fixedVerts;

    int inside = 0;
    const int start = sec->wallStart;
    const int end   = start + sec->boundaryCount;

    for (int wi = start; wi < end; wi++) {
        const RC3D_Wall *w = &walls[wi];
        const RC3D_FixedVec2 *a = &verts[w->v0];
        const RC3D_FixedVec2 *b = &verts[w->v1];

        if ((a->y > py) != (b->y > py)) {
            const int64_t xHit =
                (int64_t)a->x +
                (((int64_t)(py - a->y) * (int64_t)(b->x - a->x)) / (int64_t)(b->y - a->y));
            if (px < xHit) inside ^= 1;
        }
    }

    return inside;
}

static int findSectorForPointFixed(RC3D_Fixed x, RC3D_Fixed y)
{
    const float playerFeet   = g_player.z - RC3D_PLAYER_EYE_HEIGHT;
    const float playerHeight = PLAYER_HEIGHT;
    const float playerHead   = playerFeet + playerHeight;
    const float maxStepUp    = PLAYER_STEPUP;

    int bestSector = -1;
    float bestScore = 1e30f;

    if ((unsigned)g_player.sector < (unsigned)g_map->sectorCount) {
        const RC3D_Sector *sec = &g_map->sectors[g_player.sector];

        if (pointInSectorFixed(x, y, g_player.sector) &&
            (sec->ceilHeight - sec->floorHeight) >= playerHeight &&
            sec->floorHeight <= (playerFeet + maxStepUp) &&
            playerHead <= sec->ceilHeight)
        {
            return g_player.sector;
        }
    }

    for (int i = 0; i < g_map->sectorCount; i++) {
        const RC3D_Sector *sec = &g_map->sectors[i];

        if (i == g_player.sector) continue;
        if (!pointInSectorFixed(x, y, i)) continue;
        if ((sec->ceilHeight - sec->floorHeight) < playerHeight) continue;
        if (sec->floorHeight > (playerFeet + maxStepUp)) continue;
        if (playerHead > sec->ceilHeight) continue;

        {
            const float score = fabsf(sec->floorHeight - playerFeet);
            if (score < bestScore) {
                bestScore = score;
                bestSector = i;
            }
        }
    }

    return bestSector;
}

static int findSectorForSpritePosition(float x, float y, int preferredSector)
{
    const RC3D_Fixed fx = rc3dFloatToFixed(x);
    const RC3D_Fixed fy = rc3dFloatToFixed(y);

    if ((unsigned)preferredSector < (unsigned)g_map->sectorCount) {
        if (pointInSectorFixed(fx, fy, preferredSector)) {
            return preferredSector;
        }
    }

    for (int i = 0; i < g_map->sectorCount; ++i) {
        if (pointInSectorFixed(fx, fy, i)) {
            return i;
        }
    }

    return -1;
}

static int64_t pointSegmentDistSqFixed(
    RC3D_Fixed px, RC3D_Fixed py,
    RC3D_Fixed ax, RC3D_Fixed ay,
    RC3D_Fixed bx, RC3D_Fixed by,
    RC3D_Fixed *outClosestX,
    RC3D_Fixed *outClosestY
){
    const RC3D_Fixed abx = bx - ax;
    const RC3D_Fixed aby = by - ay;
    const RC3D_Fixed apx = px - ax;
    const RC3D_Fixed apy = py - ay;
    const int64_t abLenSq = rc3dFixedSq(abx) + rc3dFixedSq(aby);
    RC3D_Fixed cx = ax;
    RC3D_Fixed cy = ay;

    if (abLenSq > 0) {
        const int64_t dot = ((int64_t)apx * (int64_t)abx) + ((int64_t)apy * (int64_t)aby);
        int64_t tDen = abLenSq >> RC3D_FIXED_SHIFT;
        RC3D_Fixed t;

        if (tDen <= 0) tDen = 1;

        t = (RC3D_Fixed)(dot / tDen);
        if (t < 0) t = 0;
        if (t > RC3D_FIXED_ONE) t = RC3D_FIXED_ONE;

        cx = ax + rc3dFixedMul(abx, t);
        cy = ay + rc3dFixedMul(aby, t);
    }

    if (outClosestX) *outClosestX = cx;
    if (outClosestY) *outClosestY = cy;

    return rc3dFixedSq(px - cx) + rc3dFixedSq(py - cy);
}

static int wallOpeningAllowsPlayerPassage(const RC3D_Wall *w, int sectorIndex)
{
    const float playerFeet = g_player.z - RC3D_PLAYER_EYE_HEIGHT;
    float transitionFeet = playerFeet;
    float openBottom = w->openBottom;
    float openTop = w->openTop;

    if (!(w->flags & RC3D_WALL_PORTAL)) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if ((unsigned)w->neighbour >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    {
        const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
        const RC3D_Sector *nextSec = &g_map->sectors[w->neighbour];

        if (sec->floorHeight > transitionFeet) {
            transitionFeet = sec->floorHeight;
        }

        if (nextSec->floorHeight > transitionFeet) {
            transitionFeet = nextSec->floorHeight;
        }

        if (sec->ceilHeight < openTop) {
            openTop = sec->ceilHeight;
        }

        if (nextSec->ceilHeight < openTop) {
            openTop = nextSec->ceilHeight;
        }

        if (sec->floorHeight > openBottom) {
            openBottom = sec->floorHeight;
        }

        if (nextSec->floorHeight > openBottom) {
            openBottom = nextSec->floorHeight;
        }
    }

    if (transitionFeet > (playerFeet + PLAYER_STEPUP + RC3D_EPSILON)) {
        return 0;
    }

    if ((openTop - openBottom) < (PLAYER_HEIGHT - RC3D_EPSILON)) {
        return 0;
    }

    if (transitionFeet < (openBottom - RC3D_EPSILON)) {
        return 0;
    }

    if ((transitionFeet + PLAYER_HEIGHT) > (openTop + RC3D_EPSILON)) {
        return 0;
    }

    return 1;
}

static int wallBlocksPlayerMovement(const RC3D_Wall *w, int sectorIndex)
{
    if (w->flags & RC3D_WALL_SOLID) {
        return 1;
    }

    if (w->flags & RC3D_WALL_PORTAL) {
        return !wallOpeningAllowsPlayerPassage(w, sectorIndex);
    }

    if (w->flags & (RC3D_WALL_MIDDLE | RC3D_WALL_UPPER | RC3D_WALL_LOWER)) {
        return 1;
    }

    return 0;
}

static int findBlockingContactInSectorFixed(
    RC3D_Fixed px, RC3D_Fixed py,
    RC3D_Fixed radius,
    int sectorIndex,
    float fallbackNX,
    float fallbackNY,
    RC3D_BlockingContact *outContact
){
    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    {
        const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
        const RC3D_Wall *walls = g_map->walls;
        const RC3D_FixedVec2 *verts = g_fixedVerts;
        const int64_t radiusSq = rc3dFixedSq(radius);
        const float radiusF = rc3dFixedToFloat(radius);
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        RC3D_BlockingContact best;
        int64_t bestDistSq = radiusSq;

        best.hit = 0;
        best.wallIndex = -1;
        best.normalX = fallbackNX;
        best.normalY = fallbackNY;
        best.penetration = 0.0f;

        for (int wi = start; wi < end; ++wi) {
            const RC3D_Wall *w = &walls[wi];

            if (!wallBlocksPlayerMovement(w, sectorIndex)) {
                continue;
            }

            {
                const RC3D_FixedVec2 *a = &verts[w->v0];
                const RC3D_FixedVec2 *b = &verts[w->v1];
                const RC3D_Fixed abx = b->x - a->x;
                const RC3D_Fixed aby = b->y - a->y;
                const int64_t abLenSq = rc3dFixedSq(abx) + rc3dFixedSq(aby);
                RC3D_Fixed cx = a->x;
                RC3D_Fixed cy = a->y;
                RC3D_Fixed dx;
                RC3D_Fixed dy;
                int64_t distSq;

                distSq = pointSegmentDistSqFixed(
                    px, py,
                    a->x, a->y,
                    b->x, b->y,
                    &cx, &cy);

                if (distSq >= bestDistSq) {
                    continue;
                }

                {
                    float nx = fallbackNX;
                    float ny = fallbackNY;
                    float dist = 0.0f;

                    dx = px - cx;
                    dy = py - cy;

                    if (distSq > 0) {
                        const float dxF = rc3dFixedToFloat(dx);
                        const float dyF = rc3dFixedToFloat(dy);

                        dist = sqrtf((dxF * dxF) + (dyF * dyF));
                        if (dist > RC3D_EPSILON) {
                            nx = dxF / dist;
                            ny = dyF / dist;
                        }
                    } else if (abLenSq > 0) {
                        const float abxF = rc3dFixedToFloat(abx);
                        const float abyF = rc3dFixedToFloat(aby);
                        const float invWallLen =
                            1.0f / sqrtf((abxF * abxF) + (abyF * abyF));
                        nx = -abyF * invWallLen;
                        ny =  abxF * invWallLen;

                        if (((nx * fallbackNX) + (ny * fallbackNY)) < 0.0f) {
                            nx = -nx;
                            ny = -ny;
                        }
                    }

                    best.hit = 1;
                    best.wallIndex = wi;
                    best.normalX = nx;
                    best.normalY = ny;
                    best.penetration = radiusF - dist;
                    bestDistSq = distSq;
                }
            }
        }

        if (best.hit) {
            if (outContact) {
                *outContact = best;
            }
            return 1;
        }
    }

    return 0;
}

static int findBlockingContactAtPositionFixed(
    RC3D_Fixed px, RC3D_Fixed py,
    RC3D_Fixed radius,
    int currentSector,
    int newSector,
    float fallbackNX,
    float fallbackNY,
    RC3D_BlockingContact *outContact
){
    RC3D_BlockingContact best;
    int found = 0;
    float bestPenetration = -1.0f;

    best.hit = 0;
    best.wallIndex = -1;
    best.normalX = fallbackNX;
    best.normalY = fallbackNY;
    best.penetration = 0.0f;

    if (findBlockingContactInSectorFixed(
            px, py, radius, currentSector, fallbackNX, fallbackNY, &best))
    {
        found = 1;
        bestPenetration = best.penetration;
    }

    if (newSector != currentSector && newSector >= 0) {
        RC3D_BlockingContact other;

        if (findBlockingContactInSectorFixed(
                px, py, radius, newSector, fallbackNX, fallbackNY, &other) &&
            (!found || other.penetration > bestPenetration))
        {
            best = other;
            found = 1;
            bestPenetration = other.penetration;
        }
    }

    if (found && outContact) {
        *outContact = best;
    }

    return found;
}

static int findBlockingWallInSectorFixed(
    RC3D_Fixed px, RC3D_Fixed py,
    RC3D_Fixed radius,
    int sectorIndex,
    int *outWallIndex
){
    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_FixedVec2 *verts = g_fixedVerts;
    const int64_t radiusSq = rc3dFixedSq(radius);

    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;

    for (int wi = start; wi < end; ++wi) {
        const RC3D_Wall *w = &walls[wi];

        if (!wallBlocksPlayerMovement(w, sectorIndex)) {
            continue;
        }

        const RC3D_FixedVec2 *a = &verts[w->v0];
        const RC3D_FixedVec2 *b = &verts[w->v1];

        if (pointSegmentDistSqFixed(px, py, a->x, a->y, b->x, b->y, NULL, NULL) < radiusSq) {
            if (outWallIndex) *outWallIndex = wi;
            return 1;
        }
    }

    return 0;
}

static int positionHitsBlockingWallsInSectorFixed(
    RC3D_Fixed px,
    RC3D_Fixed py,
    RC3D_Fixed radius,
    int sectorIndex)
{
    return findBlockingWallInSectorFixed(px, py, radius, sectorIndex, NULL);
}

static int canMoveToPositionFixed(RC3D_Fixed px, RC3D_Fixed py, int newSector)
{
    if (newSector < 0) {
        return 0;
    }

    if (positionHitsBlockingWallsInSectorFixed(
            px, py, rc3dFloatToFixed(PLAYER_RADIUS), g_player.sector))
    {
        return 0;
    }

    if (newSector != g_player.sector) {
        if (positionHitsBlockingWallsInSectorFixed(
                px, py, rc3dFloatToFixed(PLAYER_RADIUS), newSector))
        {
            return 0;
        }
    }

    return 1;
}

static int tryMovePlayerSliding(float moveX, float moveY)
{
    const RC3D_Fixed startX = g_player.xFixed;
    const RC3D_Fixed startY = g_player.yFixed;
    const RC3D_Fixed moveXFixed = rc3dFloatToFixed(moveX);
    const RC3D_Fixed moveYFixed = rc3dFloatToFixed(moveY);
    const RC3D_Fixed playerRadiusFixed = rc3dFloatToFixed(PLAYER_RADIUS);
    const float startXFloat = rc3dFixedToFloat(startX);
    const float startYFloat = rc3dFixedToFloat(startY);
    const float moveStepX = rc3dFixedToFloat(moveXFixed);
    const float moveStepY = rc3dFixedToFloat(moveYFixed);
    float pushNX = 0.0f;
    float pushNY = 0.0f;

    {
        const float moveLen = sqrtf((moveStepX * moveStepX) + (moveStepY * moveStepY));
        if (moveLen > RC3D_EPSILON) {
            pushNX = -moveStepX / moveLen;
            pushNY = -moveStepY / moveLen;
        }
    }

    const RC3D_Fixed tryX = startX + moveXFixed;
    const RC3D_Fixed tryY = startY + moveYFixed;
    const int newSector = findSectorForPointFixed(tryX, tryY);


    if (canMoveToPositionFixed(tryX, tryY, newSector)) {
        rc3dSetPlayerWorldXYFixed(tryX, tryY);
        g_player.sector = newSector;
        return 1;
    }

    {
        RC3D_BlockingContact contact;

        if (findBlockingContactAtPositionFixed(
                tryX, tryY,
                playerRadiusFixed,
                g_player.sector,
                newSector,
                pushNX,
                pushNY,
                &contact))
        {
            const RC3D_Wall *w = &g_map->walls[contact.wallIndex];
            const RC3D_Vec2 *a = &g_map->verts[w->v0];
            const RC3D_Vec2 *b = &g_map->verts[w->v1];
            const float contactNudge =
                fminf(contact.penetration + RC3D_EPSILON, RC3D_COLLISION_SKIN * 0.5f);
            const float nudgeX = contact.normalX * contactNudge;
            const float nudgeY = contact.normalY * contactNudge;

            float wallX = b->x - a->x;
            float wallY = b->y - a->y;
            const float wallLen = sqrtf((wallX * wallX) + (wallY * wallY));

            if (wallLen > RC3D_EPSILON) {
                wallX /= wallLen;
                wallY /= wallLen;

                const float slideAmt = (moveStepX * wallX) + (moveStepY * wallY);
                const float slideX = wallX * slideAmt;
                const float slideY = wallY * slideAmt;

                const RC3D_Fixed slideTryX =
                    rc3dFloatToFixed(startXFloat + slideX + nudgeX);
                const RC3D_Fixed slideTryY =
                    rc3dFloatToFixed(startYFloat + slideY + nudgeY);
                const int slideSector = findSectorForPointFixed(slideTryX, slideTryY);

                if (canMoveToPositionFixed(slideTryX, slideTryY, slideSector)) {
                    rc3dSetPlayerWorldXYFixed(slideTryX, slideTryY);
                    g_player.sector = slideSector;
                    return 1;
                }
            }
        }
    }

    {
        const RC3D_Fixed xOnlyX = startX + moveXFixed;
        const RC3D_Fixed xOnlyY = startY;
        const int xSector = findSectorForPointFixed(xOnlyX, xOnlyY);

        if (canMoveToPositionFixed(xOnlyX, xOnlyY, xSector)) {
            rc3dSetPlayerWorldXYFixed(xOnlyX, xOnlyY);
            g_player.sector = xSector;
            return 1;
        }
    }

    {
        const RC3D_Fixed yOnlyX = startX;
        const RC3D_Fixed yOnlyY = startY + moveYFixed;
        const int ySector = findSectorForPointFixed(yOnlyX, yOnlyY);

        if (canMoveToPositionFixed(yOnlyX, yOnlyY, ySector)) {
            rc3dSetPlayerWorldXYFixed(yOnlyX, yOnlyY);
            g_player.sector = ySector;
            return 1;
        }
    }

    return 0;
}



/* ------------------------------------------------------------------------- */
/* drawing helpers                                                           */
/* ------------------------------------------------------------------------- */

static inline void drawVerticalSpanFast(int x, int y0, int y1, uint8_t col)
{
    if ((unsigned)x >= SCREEN_W) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    if (y0 > y1) return;

    uint8_t *dst = &fb[(y0 * SCREEN_W) + x];
    const int count = y1 - y0 + 1;

    for (int i = 0; i < count; i++) {
        *dst = col;
        dst += SCREEN_W;
    }
}


static inline void drawTexturedPlaneSpan(
    int sx,
    int y0,
    int y1,
    float rayDirX,
    float rayDirY,
    float planeZ,
    uint8_t texId,
    int horizon,
    const RC3D_Sector *sec,
    int isCeiling
){
    if (texId == RC3D_TEXID_SKYBOX) return;
    if (!sec) return;
    if ((unsigned)sx >= SCREEN_W) return;
    if (y0 > y1) return;

    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    if (y0 > y1) return;

    {
        const RC3D_SectorCache *secCache = NULL;
        const float eyeX = g_player.x;
        const float eyeY = g_player.y;
        const float eyeZ = g_renderEyeZ;
        const float planeFactor = (planeZ - eyeZ) * projPlaneGlobal;
        const float *invTable = &g_invDTable[SCREEN_H];
        const uint8_t *texels = g_rc3dTextures[texId].pix;

        uint8_t *dst = &fb[(y0 * SCREEN_W) + sx];

        if (g_sectorCache && sec >= g_map->sectors) {
            const ptrdiff_t secIndex = sec - g_map->sectors;
            if (secIndex >= 0 && secIndex < g_sectorCacheCount) {
                secCache = &g_sectorCache[secIndex];
            }
        }

        /* ------------------------------------------------------------- */
        /* FAST PATH: no rotation, no scaling                            */
        /* ------------------------------------------------------------- */
        if (secCache && (isCeiling ? secCache->ceilSimple : secCache->floorSimple))
        {
            for (int y = y0; y <= y1; ++y) {
                const int d = horizon - y;

                if (d != 0) {
                    const float t = planeFactor * invTable[d];
                    const int tx = (int)((eyeX + (rayDirX * t)) * (float)RC3D_TEX_SIZE);
                    const int ty = (int)((eyeY + (rayDirY * t)) * (float)RC3D_TEX_SIZE);
                    *dst = texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];
                }

                dst += SCREEN_W;
            }

            return;
        }

        /* ------------------------------------------------------------- */
        /* general path                                                  */
        /* ------------------------------------------------------------- */
        {
            const float ca =
                secCache ? (isCeiling ? secCache->ceilCos : secCache->floorCos) :
                cosf(isCeiling ? sec->ceilTexAngle : sec->floorTexAngle);
            const float sa =
                secCache ? (isCeiling ? secCache->ceilSin : secCache->floorSin) :
                sinf(isCeiling ? sec->ceilTexAngle : sec->floorTexAngle);
            const float invScaleX =
                secCache ? (isCeiling ? secCache->ceilInvScaleX : secCache->floorInvScaleX) :
                (1.0f / ((fabsf(isCeiling ? sec->ceilTexScaleX : sec->floorTexScaleX) < RC3D_EPSILON)
                    ? 1.0f : (isCeiling ? sec->ceilTexScaleX : sec->floorTexScaleX)));
            const float invScaleY =
                secCache ? (isCeiling ? secCache->ceilInvScaleY : secCache->floorInvScaleY) :
                (1.0f / ((fabsf(isCeiling ? sec->ceilTexScaleY : sec->floorTexScaleY) < RC3D_EPSILON)
                    ? 1.0f : (isCeiling ? sec->ceilTexScaleY : sec->floorTexScaleY)));

            for (int y = y0; y <= y1; ++y) {
                const int d = horizon - y;

                if (d != 0) {
                    const float t = planeFactor * invTable[d];
                    const float wx = eyeX + (rayDirX * t);
                    const float wy = eyeY + (rayDirY * t);

                    const float rx = ((wx * ca) + (wy * sa)) * invScaleX;
                    const float ry = (-(wx * sa) + (wy * ca)) * invScaleY;
                    const int tx = (int)(rx * (float)RC3D_TEX_SIZE);
                    const int ty = (int)(ry * (float)RC3D_TEX_SIZE);

                    *dst = texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];
                }

                dst += SCREEN_W;
            }
        }
    }
}




static inline void renderTexturedBandIfVisible(
    int sx,
    int y0,
    int y1,
    uint8_t texId,
    uint32_t texFlags,
    float vTopWorld,
    float vBotWorld,
    float hitDist,
    float projPlane,
    int clipTop,
    int clipBottom
){
    const int origY0 = y0;
    const int origY1 = y1;

    const int clampYT =
        (texFlags & RC3D_TEX_FLAG_CLAMPYT) ? 1 : 0;
    const int clampYB =
        (texFlags & RC3D_TEX_FLAG_CLAMPYB) ? 1 : 0;
    const int stretchY =
        (clampYT && clampYB) ? 1 : 0;

    if (texId == RC3D_TEXID_SKYBOX) return;
    if (origY0 > origY1) return;

    y0 = clampi(y0, clipTop, clipBottom);
    y1 = clampi(y1, clipTop, clipBottom);
    if (y0 > y1) return;

    rc3dRecordWallDepthSpan(sx, y0, y1, hitDist);

    {
        const float worldSpan = vTopWorld - vBotWorld;
        const float eyeZ = g_renderEyeZ;

        if (worldSpan <= RC3D_EPSILON) return;

        {
            const float k = hitDist / projPlane;
            const float texPerWorld = (float)RC3D_TEX_SIZE;
            const float baseWorldFromTop =
                vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            if (stretchY) {
                const float scale = texPerWorld / worldSpan;
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * scale;
                vStep   = k * scale;
            } else if (clampYB && !clampYT) {
                vLocal0 = ((baseWorldFromTop + ((float)y0 * k)) - worldSpan) * texPerWorld;
                vStep   = k * texPerWorld;
            } else {
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * texPerWorld;
                vStep   = k * texPerWorld;
            }

            {
                uint8_t *dst = &fb[(y0 * SCREEN_W) + sx];

                /* --------------------------------------------------------- */
                /* FAST PATH: no wall texture rotation                       */
                /* --------------------------------------------------------- */
                if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f) &&
                    (wallTexRotCosGlobal >  0.9999f)  && (wallTexRotCosGlobal < 1.0001f))
                {
                    const int32_t uFixed = (int32_t)(wallTexUBaseGlobal * 65536.0f);
                    int32_t vFixed = (int32_t)(vLocal0 * 65536.0f);
                    const int32_t vStepFixed = (int32_t)(vStep * 65536.0f);

                    const int tx = (uFixed >> 16);

                    for (int y = y0; y <= y1; ++y) {
                        *dst = wallTexelFetch(texId, tx, (vFixed >> 16));
                        dst += SCREEN_W;
                        vFixed += vStepFixed;
                    }
                }
                else
                {
                    /* ----------------------------------------------------- */
                    /* rotated path                                          */
                    /* ----------------------------------------------------- */
                    float uRotF = (wallTexUBaseGlobal * wallTexRotCosGlobal) - (vLocal0 * wallTexRotSinGlobal);
                    float vRotF = (wallTexUBaseGlobal * wallTexRotSinGlobal) + (vLocal0 * wallTexRotCosGlobal);

                    const float uStepF = -vStep * wallTexRotSinGlobal;
                    const float vStepF =  vStep * wallTexRotCosGlobal;

                    int32_t uRot = (int32_t)(uRotF * 65536.0f);
                    int32_t vRot = (int32_t)(vRotF * 65536.0f);
                    const int32_t uStep = (int32_t)(uStepF * 65536.0f);
                    const int32_t vStepRot = (int32_t)(vStepF * 65536.0f);

                    for (int y = y0; y <= y1; ++y) {
                        *dst = wallTexelFetch(texId, (uRot >> 16), (vRot >> 16));
                        dst += SCREEN_W;

                        uRot += uStep;
                        vRot += vStepRot;
                    }
                }
            }
        }
    }
}

static inline void renderMaskedTexturedBandIfVisible(
    int sx,
    int y0,
    int y1,
    uint8_t texId,
    uint32_t texFlags,
    float vTopWorld,
    float vBotWorld,
    float hitDist,
    float projPlane,
    int clipTop,
    int clipBottom
){
    const int origY0 = y0;
    const int origY1 = y1;

    const int clampYT =
        (texFlags & RC3D_TEX_FLAG_CLAMPYT) ? 1 : 0;
    const int clampYB =
        (texFlags & RC3D_TEX_FLAG_CLAMPYB) ? 1 : 0;
    const int stretchY =
        (clampYT && clampYB) ? 1 : 0;

    if (texId == RC3D_TEXID_SKYBOX) return;
    if (origY0 > origY1) return;

    y0 = clampi(y0, clipTop, clipBottom);
    y1 = clampi(y1, clipTop, clipBottom);
    if (y0 > y1) return;

    {
        const float worldSpan = vTopWorld - vBotWorld;
        const float eyeZ = g_renderEyeZ;

        if (worldSpan <= RC3D_EPSILON) return;

        {
            const float k = hitDist / projPlane;
            const float texPerWorld = (float)RC3D_TEX_SIZE;
            const float baseWorldFromTop =
                vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            if (stretchY) {
                const float scale = texPerWorld / worldSpan;
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * scale;
                vStep   = k * scale;
            } else if (clampYB && !clampYT) {
                vLocal0 = ((baseWorldFromTop + ((float)y0 * k)) - worldSpan) * texPerWorld;
                vStep   = k * texPerWorld;
            } else {
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * texPerWorld;
                vStep   = k * texPerWorld;
            }

            {
                uint8_t *dst = &fb[(y0 * SCREEN_W) + sx];
                int opaqueSpanStart = -1;

                if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f) &&
                    (wallTexRotCosGlobal >  0.9999f)  && (wallTexRotCosGlobal < 1.0001f))
                {
                    const int32_t uFixed = (int32_t)(wallTexUBaseGlobal * 65536.0f);
                    int32_t vFixed = (int32_t)(vLocal0 * 65536.0f);
                    const int32_t vStepFixed = (int32_t)(vStep * 65536.0f);

                    const int tx = (uFixed >> 16);

                    for (int y = y0; y <= y1; ++y) {
                        const uint8_t texel = wallTexelFetch(texId, tx, (vFixed >> 16));

                        if (texel != RC3D_SPRITE_TEX_TRANSPARENT) {
                            *dst = texel;
                            if (opaqueSpanStart < 0) {
                                opaqueSpanStart = y;
                            }
                        } else if (opaqueSpanStart >= 0) {
                            rc3dRecordWallDepthSpan(sx, opaqueSpanStart, y - 1, hitDist);
                            opaqueSpanStart = -1;
                        }

                        dst += SCREEN_W;
                        vFixed += vStepFixed;
                    }
                }
                else
                {
                    float uRotF = (wallTexUBaseGlobal * wallTexRotCosGlobal) - (vLocal0 * wallTexRotSinGlobal);
                    float vRotF = (wallTexUBaseGlobal * wallTexRotSinGlobal) + (vLocal0 * wallTexRotCosGlobal);

                    const float uStepF = -vStep * wallTexRotSinGlobal;
                    const float vStepF =  vStep * wallTexRotCosGlobal;

                    int32_t uRot = (int32_t)(uRotF * 65536.0f);
                    int32_t vRot = (int32_t)(vRotF * 65536.0f);
                    const int32_t uStep = (int32_t)(uStepF * 65536.0f);
                    const int32_t vStepRot = (int32_t)(vStepF * 65536.0f);

                    for (int y = y0; y <= y1; ++y) {
                        const uint8_t texel = wallTexelFetch(texId, (uRot >> 16), (vRot >> 16));

                        if (texel != RC3D_SPRITE_TEX_TRANSPARENT) {
                            *dst = texel;
                            if (opaqueSpanStart < 0) {
                                opaqueSpanStart = y;
                            }
                        } else if (opaqueSpanStart >= 0) {
                            rc3dRecordWallDepthSpan(sx, opaqueSpanStart, y - 1, hitDist);
                            opaqueSpanStart = -1;
                        }

                        dst += SCREEN_W;
                        uRot += uStep;
                        vRot += vStepRot;
                    }
                }

                if (opaqueSpanStart >= 0) {
                    rc3dRecordWallDepthSpan(sx, opaqueSpanStart, y1, hitDist);
                }
            }
        }
    }
}



static inline RC3D_WallHit findNearestWallInSector(
    int sectorIndex,
    float rox, float roy,
    float rdx, float rdy,
    int ignoreWallIndexA,
    int ignoreWallIndexB,
    float minT
){
    RC3D_WallHit hit;
    hit.t = RC3D_MAX_RAY_DIST;
    hit.wallIndex = -1;
    hit.hit = 0;

    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_Vec2 *verts = g_map->verts;
    const RC3D_WallCache *cache = g_wallCache;

    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;

    if (cache) {
        for (int wallIndex = start; wallIndex < end; ++wallIndex) {
            if (wallIndex == ignoreWallIndexA || wallIndex == ignoreWallIndexB) {
                continue;
            }

            const RC3D_Wall *w = &walls[wallIndex];
            const RC3D_Vec2 *a = &verts[w->v0];
            const RC3D_WallCache *wc = &cache[wallIndex];
            const float sx = wc->dx;
            const float sy = wc->dy;
            const float denom = (rdx * sy) - (rdy * sx);

            if (denom >= -RC3D_EPSILON) {
                continue;
            }

            {
                const float qpx = a->x - rox;
                const float qpy = a->y - roy;
                const float invDenom = 1.0f / denom;
                const float t = ((qpx * sy) - (qpy * sx)) * invDenom;

                if (t < minT || t >= hit.t) {
                    continue;
                }

                {
                    const float u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
                    if (u < 0.0f || u > 1.0f) {
                        continue;
                    }
                }

                hit.t = t;
                hit.wallIndex = wallIndex;
                hit.hit = 1;
            }
        }
    } else {
        for (int wallIndex = start; wallIndex < end; ++wallIndex) {
            if (wallIndex == ignoreWallIndexA || wallIndex == ignoreWallIndexB) {
                continue;
            }

            const RC3D_Wall *w = &walls[wallIndex];
            const RC3D_Vec2 *a = &verts[w->v0];
            const RC3D_Vec2 *b = &verts[w->v1];
            const float sx = b->x - a->x;
            const float sy = b->y - a->y;
            const float denom = (rdx * sy) - (rdy * sx);

            if (denom >= -RC3D_EPSILON) {
                continue;
            }

            {
                const float qpx = a->x - rox;
                const float qpy = a->y - roy;
                const float invDenom = 1.0f / denom;
                const float t = ((qpx * sy) - (qpy * sx)) * invDenom;

                if (t < minT || t >= hit.t) {
                    continue;
                }

                {
                    const float u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
                    if (u < 0.0f || u > 1.0f) {
                        continue;
                    }
                }

                hit.t = t;
                hit.wallIndex = wallIndex;
                hit.hit = 1;
            }
        }
    }

    return hit;
}





static inline void fillSectorColumnSpan(
    int sx,
    int y0,
    int y1,
    const RC3D_Sector *sec,
    int horizon,
    float rayDirX,
    float rayDirY
){
    if (!sec) return;
    if (y0 > y1) return;

    if (y0 < horizon) {
        int topEnd = horizon - 1;
        if (topEnd > y1) topEnd = y1;

        if (y0 <= topEnd) {
            drawTexturedPlaneSpan(
                sx,
                y0,
                topEnd,
                rayDirX,
                rayDirY,
                sec->ceilHeight,
                sec->ceilColor,
                horizon,
                sec,
                1
            );
        }
    }

    if (y1 >= horizon) {
        int botBeg = horizon;
        if (botBeg < y0) botBeg = y0;

        if (botBeg <= y1) {
            drawTexturedPlaneSpan(
                sx,
                botBeg,
                y1,
                rayDirX,
                rayDirY,
                sec->floorHeight,
                sec->floorColor,
                horizon,
                sec,
                0
            );
        }
    }
}





static void drawBackground(void)
{
    const float fullTurn  = (float)(M_PI * 2.0f);
    const float leftAng   = g_player.angle - g_halfFovRad;
    const float skyRepeat = 4.0f;
    const int32_t oneFixed = 1 << 16;

    const float uStepRaw =
        (((g_halfFovRad * 2.0f) / (float)SCREEN_W) / fullTurn) * skyRepeat;

    float uStart = (leftAng / fullTurn) * skyRepeat;
    uStart -= floorf(uStart);

    {
        int32_t uFixed = (int32_t)(uStart * 65536.0f);
        const int32_t uStepFixed = (int32_t)(uStepRaw * 65536.0f);

        for (int x = 0; x < SCREEN_W; ++x) {
            g_skyXTable[x] = (uint16_t)(((uint32_t)uFixed * (uint32_t)RC3D_SKYBOX_W) >> 16);
            uFixed += uStepFixed;

            if (uFixed >= oneFixed) {
                uFixed -= oneFixed;
            } else if (uFixed < 0) {
                uFixed += oneFixed;
            }
        }
    }

    for (int y = 0; y < (SCREEN_H / 2)+2; ++y) {
        const int ty = (y * RC3D_SKYBOX_H) / (SCREEN_H / 2)-1.5f;
        const uint8_t *srcRow = &tex_skybox[ty * RC3D_SKYBOX_W];
        uint8_t *dst = &fb[y * SCREEN_W];

        for (int x = 0; x < SCREEN_W; ++x) {
            dst[x] = srcRow[g_skyXTable[x]];
        }
    }
}



static inline void renderColumnPortalTraceClipped(
    int sx,
    float rdx,
    float rdy,
    float dirX,
    float dirY,
    float projPlane,
    int horizon,
    int currentSector,
    int clipTop,
    int clipBottom,
    int ignoreWallIndexA,
    int ignoreWallIndexB,
    float rayMinT,
    int maskedDepth
){
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_renderEyeZ;

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) {
        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return;
        }

        if (clipTop > clipBottom) {
            return;
        }

        {
            const RC3D_Sector *sec = &g_map->sectors[currentSector];

            const RC3D_WallHit hit = findNearestWallInSector(
                currentSector,
                playerX, playerY,
                rdx, rdy,
                ignoreWallIndexA,
                ignoreWallIndexB,
                rayMinT
            );

            if (!hit.hit) {
                fillSectorColumnSpan(
                    sx,
                    clipTop,
                    clipBottom,
                    sec,
                    horizon,
                    rdx,
                    rdy
                );
                return;
            }

            {
                const RC3D_Wall *w = &g_map->walls[hit.wallIndex];
                const RC3D_WallCache *wc =
                    (g_wallCache && hit.wallIndex >= 0 && hit.wallIndex < g_wallCacheCount)
                        ? &g_wallCache[hit.wallIndex] : NULL;

                const RC3D_Vec2 *va = &g_map->verts[w->v0];
                const RC3D_Vec2 *vb = &g_map->verts[w->v1];

                const float hitX = playerX + (rdx * hit.t);
                const float hitY = playerY + (rdy * hit.t);

                const float correctedDist =
                    ((hitX - playerX) * dirX) +
                    ((hitY - playerY) * dirY);

                if (correctedDist <= RC3D_EPSILON) {
                    return;
                }

                {
                    const float wallDx = wc ? wc->dx : (vb->x - va->x);
                    const float wallDy = wc ? wc->dy : (vb->y - va->y);

                    float uNorm = 0.0f;
                    float distAlongWall = 0.0f;
                    float wallLen = 0.0f;

                    if (wc && wc->invLenSq > 0.0f) {
                        uNorm = (((hitX - va->x) * wallDx) + ((hitY - va->y) * wallDy)) * wc->invLenSq;
                        wallLen = wc->len;
                        distAlongWall = uNorm * wallLen;
                    } else {
                        const float wallLenSq = (wallDx * wallDx) + (wallDy * wallDy);
                        if (wallLenSq > RC3D_EPSILON) {
                            wallLen = sqrtf(wallLenSq);
                            uNorm = (((hitX - va->x) * wallDx) + ((hitY - va->y) * wallDy)) / wallLenSq;
                            distAlongWall = uNorm * wallLen;
                        }
                    }

                    if (wc) {
                        wallTexRotCosGlobal = wc->texCos;
                        wallTexRotSinGlobal = wc->texSin;
                    } else {
                        const float texAngle = rc3dWallTexAngleFromFlags(w->tex_flags);
                        if (fabsf(texAngle) > 0.0001f) {
                            wallTexRotCosGlobal = cosf(texAngle);
                            wallTexRotSinGlobal = sinf(texAngle);
                        } else {
                            wallTexRotCosGlobal = 1.0f;
                            wallTexRotSinGlobal = 0.0f;
                        }
                    }

                    if (wc && wc->texXMode == RC3D_TEX_XMODE_STRETCH) {
                        if (uNorm < 0.0f) uNorm = 0.0f;
                        if (uNorm > 1.0f) uNorm = 1.0f;
                        wallTexUBaseGlobal = uNorm * (float)RC3D_TEX_SIZE;
                    } else if (wc && wc->texXMode == RC3D_TEX_XMODE_CLAMP_RIGHT) {
                        wallTexUBaseGlobal =
                            (distAlongWall * (float)RC3D_TEX_SIZE) -
                            (wallLen * (float)RC3D_TEX_SIZE);
                    } else {
                        wallTexUBaseGlobal = distAlongWall * (float)RC3D_TEX_SIZE;
                    }
                }

                {
                    const float scale = projPlane / correctedDist;

                    const int secTop = (int)(horizon - ((sec->ceilHeight  - playerZ) * scale));
                    const int secBot = (int)(horizon - ((sec->floorHeight - playerZ) * scale));

                    const uint8_t wallClass = wc ? wc->wallClass : RC3D_WALLCLASS_NONE;
                    const int wallMasked =
                        ((w->flags & RC3D_WALL_TRANSPARENCY) != 0);

                    switch (wallClass) {
                        case RC3D_WALLCLASS_SOLID:
                        {
                            fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);

                            if (wallMasked && maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH) {
                                const float savedWallTexUBase   = wallTexUBaseGlobal;
                                const float savedWallTexRotCos  = wallTexRotCosGlobal;
                                const float savedWallTexRotSin  = wallTexRotSinGlobal;

                                const int maskedClipTop    = clampi(secTop, clipTop, clipBottom);
                                const int maskedClipBottom = clampi(secBot, clipTop, clipBottom);

                                if (maskedClipTop <= maskedClipBottom) {
                                    /* --------------------------------------------------------- */
                                    /* If this masked solid wall is also a portal, trace into    */
                                    /* the neighbour sector. Otherwise trace farther in the same */
                                    /* sector so walls behind it can show through.              */
                                    /* --------------------------------------------------------- */
                                    if ((w->flags & RC3D_WALL_PORTAL) &&
                                        ((unsigned)w->neighbour < (unsigned)g_map->sectorCount))
                                    {
                                        int entryWallInNext = -1;

                                        if (wc) {
                                            entryWallInNext = wc->backWallIndex;
                                        }

                                        renderColumnPortalTraceClipped(
                                            sx,
                                            rdx,
                                            rdy,
                                            dirX,
                                            dirY,
                                            projPlane,
                                            horizon,
                                            w->neighbour,
                                            maskedClipTop,
                                            maskedClipBottom,
                                            hit.wallIndex,
                                            entryWallInNext,
                                            hit.t + RC3D_EPSILON,
                                            maskedDepth + 1
                                        );
                                    } else {
                                        renderColumnPortalTraceClipped(
                                            sx,
                                            rdx,
                                            rdy,
                                            dirX,
                                            dirY,
                                            projPlane,
                                            horizon,
                                            currentSector,
                                            maskedClipTop,
                                            maskedClipBottom,
                                            hit.wallIndex,
                                            -1,
                                            hit.t + RC3D_EPSILON,
                                            maskedDepth + 1
                                        );
                                    }
                                }

                                /* restore THIS wall's UV basis after recursive trace */
                                wallTexUBaseGlobal  = savedWallTexUBase;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;

                                renderMaskedTexturedBandIfVisible(
                                    sx, secTop, secBot,
                                    w->midColor,
                                    w->tex_flags,
                                    sec->ceilHeight,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            } else {
                                renderTexturedBandIfVisible(
                                    sx, secTop, secBot,
                                    w->midColor,
                                    w->tex_flags,
                                    sec->ceilHeight,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            return;
                        }

                        case RC3D_WALLCLASS_MIDDLE:
                        {
                            const int midTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
                            const int midBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

                            fillSectorColumnSpan(sx, clipTop, midTopY - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, midBotY + 1, clipBottom, sec, horizon, rdx, rdy);

                            if (wallMasked && maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH) {
                                int maskedClipTop = clipTop;
                                int maskedClipBottom = clipBottom;

                                if (midTopY > maskedClipTop) maskedClipTop = midTopY;
                                if (midBotY < maskedClipBottom) maskedClipBottom = midBotY;

                                if (maskedClipTop <= maskedClipBottom) {
                                    renderColumnPortalTraceClipped(
                                        sx,
                                        rdx,
                                        rdy,
                                        dirX,
                                        dirY,
                                        projPlane,
                                        horizon,
                                        currentSector,
                                        maskedClipTop,
                                        maskedClipBottom,
                                        hit.wallIndex,
                                        -1,
                                        hit.t + RC3D_EPSILON,
                                        maskedDepth + 1
                                    );
                                }

                                renderMaskedTexturedBandIfVisible(
                                    sx, midTopY, midBotY,
                                    w->midColor,
                                    w->tex_flags,
                                    w->openTop,
                                    w->openBottom,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            } else {
                                renderTexturedBandIfVisible(
                                    sx, midTopY, midBotY,
                                    w->midColor,
                                    w->tex_flags,
                                    w->openTop,
                                    w->openBottom,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }
                            return;
                        }

                        case RC3D_WALLCLASS_UPPER_LOWER:
                        {
                            const int openTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
                            const int openBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

                            fillSectorColumnSpan(sx, clipTop, openTopY - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, openBotY + 1, clipBottom, sec, horizon, rdx, rdy);

                            if (w->flags & RC3D_WALL_UPPER) {
                                renderTexturedBandIfVisible(
                                    sx, secTop, openTopY - 1,
                                    w->upperColor,
                                    w->tex_flags,
                                    sec->ceilHeight,
                                    w->openTop,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            if (w->flags & RC3D_WALL_LOWER) {
                                renderTexturedBandIfVisible(
                                    sx, openBotY + 1, secBot,
                                    w->lowerColor,
                                    w->tex_flags,
                                    w->openBottom,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            return;
                        }

                        case RC3D_WALLCLASS_PORTAL:
                        {
                            const int nextSectorIndex = w->neighbour;
                            if ((unsigned)nextSectorIndex >= (unsigned)g_map->sectorCount) {
                                return;
                            }

                            const RC3D_Sector *nextSec = &g_map->sectors[nextSectorIndex];

                            const int nextTop = (int)(horizon - ((nextSec->ceilHeight  - playerZ) * scale));
                            const int nextBot = (int)(horizon - ((nextSec->floorHeight - playerZ) * scale));

                            const int wallOpenTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
                            const int wallOpenBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

                            int openTop = secTop;
                            int openBot = secBot;

                            if (nextTop > openTop)      openTop = nextTop;
                            if (wallOpenTopY > openTop) openTop = wallOpenTopY;
                            if (clipTop > openTop)      openTop = clipTop;

                            if (nextBot < openBot)      openBot = nextBot;
                            if (wallOpenBotY < openBot) openBot = wallOpenBotY;
                            if (clipBottom < openBot)   openBot = clipBottom;

                            fillSectorColumnSpan(sx, clipTop, openTop - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, openBot + 1, clipBottom, sec, horizon, rdx, rdy);

                            if (w->flags & RC3D_WALL_UPPER) {
                                renderTexturedBandIfVisible(
                                    sx, secTop, openTop - 1,
                                    w->upperColor,
                                    w->tex_flags,
                                    sec->ceilHeight,
                                    w->openTop,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            if (w->flags & RC3D_WALL_LOWER) {
                                renderTexturedBandIfVisible(
                                    sx, openBot + 1, secBot,
                                    w->lowerColor,
                                    w->tex_flags,
                                    w->openBottom,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            if (openTop > openBot) {
                                return;
                            }

                            currentSector = nextSectorIndex;
                            clipTop = openTop;
                            clipBottom = openBot;

                            {
                                int entryWallInNext = -1;

                                if (wc) {
                                    entryWallInNext = wc->backWallIndex;
                                }

                                ignoreWallIndexA = hit.wallIndex;
                                ignoreWallIndexB = entryWallInNext;
                                rayMinT = hit.t;
                            }

                            continue;
                        }

                        default:
                        {
                            fillSectorColumnSpan(
                                sx,
                                clipTop,
                                clipBottom,
                                sec,
                                horizon,
                                rdx,
                                rdy
                            );
                            return;
                        }
                    }
                }
            }
        }
    }
}


static inline void renderColumnPortalTrace(
    int sx,
    float rdx,
    float rdy,
    float dirX,
    float dirY,
    float projPlane,
    int horizon
){
    renderColumnPortalTraceClipped(
        sx,
        rdx,
        rdy,
        dirX,
        dirY,
        projPlane,
        horizon,
        g_player.sector,
        0,
        SCREEN_H - 1,
        -1,
        -1,
        0.0f,
        0
    );
}






static void renderCurrentSectorColumns(void)
{
    const float projPlane = g_projPlaneConst;
    const int horizon     = SCREEN_H / 2;

    const float bobZ = sinf(g_player.tHeadbob) * 0.075f;

    g_renderEyeZ    = g_player.z + fabsf(bobZ);
    projPlaneGlobal = projPlane;
    horizonGlobal   = horizon;

    float dirX = 1.0f;
    float dirY = 0.0f;

    rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

    const float planeScale = g_planeScaleConst;
    const float planeX = -dirY * planeScale;
    const float planeY =  dirX * planeScale;

    const float rayStepX = planeX * g_camStepConst;
    const float rayStepY = planeY * g_camStepConst;

    float rdx = dirX - planeX;
    float rdy = dirY - planeY;

    for (int sx = 0; sx < SCREEN_W; ++sx) {
        renderColumnPortalTrace(
            sx,
            rdx,
            rdy,
            dirX,
            dirY,
            projPlane,
            horizon
        );

        rdx += rayStepX;
        rdy += rayStepY;
    }
}

static int rc3dTraceVisibleSectorAtDepth(
    int sx,
    float targetDepth,
    float dirX,
    float dirY,
    float planeX,
    float planeY,
    int *outSector,
    int *outClipTop,
    int *outClipBottom
){
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_renderEyeZ;

    const float cameraX =
        -1.0f + (2.0f * ((float)sx / (float)(SCREEN_W - 1)));

    const float rdx = dirX + (planeX * cameraX);
    const float rdy = dirY + (planeY * cameraX);

    int currentSector = g_player.sector;
    int clipTop = 0;
    int clipBottom = SCREEN_H - 1;
    int ignoreWallIndexA = -1;
    int ignoreWallIndexB = -1;
    float rayMinT = 0.0f;

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) {
        RC3D_WallHit hit;
        const RC3D_Sector *sec;
        const RC3D_Wall *w;
        const RC3D_WallCache *wc = NULL;
        uint8_t flags;
        uint8_t wallClass;
        int wallMasked;
        float correctedDist;
        float scale;
        int secTop;
        int secBot;

        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return 0;
        }

        if (clipTop > clipBottom) {
            return 0;
        }

        sec = &g_map->sectors[currentSector];

        hit = findNearestWallInSector(
            currentSector,
            playerX, playerY,
            rdx, rdy,
            ignoreWallIndexA,
            ignoreWallIndexB,
            rayMinT
        );

        if (!hit.hit) {
            if (outSector) *outSector = currentSector;
            if (outClipTop) *outClipTop = clipTop;
            if (outClipBottom) *outClipBottom = clipBottom;
            return 1;
        }

        w = &g_map->walls[hit.wallIndex];

        if (g_wallCache && hit.wallIndex >= 0 && hit.wallIndex < g_wallCacheCount) {
            wc = &g_wallCache[hit.wallIndex];
            wallClass = wc->wallClass;
        } else {
            if (w->flags & RC3D_WALL_SOLID) {
                wallClass = RC3D_WALLCLASS_SOLID;
            }
            else if ((w->flags & RC3D_WALL_MIDDLE) && !(w->flags & RC3D_WALL_PORTAL)) {
                wallClass = RC3D_WALLCLASS_MIDDLE;
            }
            else if (w->flags & RC3D_WALL_PORTAL) {
                wallClass = RC3D_WALLCLASS_PORTAL;
            }
            else if ((w->flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
                     !(w->flags & RC3D_WALL_MIDDLE)) {
                wallClass = RC3D_WALLCLASS_UPPER_LOWER;
            }
            else {
                wallClass = RC3D_WALLCLASS_NONE;
            }
        }

        correctedDist = hit.t * ((rdx * dirX) + (rdy * dirY));
        if (correctedDist <= RC3D_EPSILON) {
            return 0;
        }

        if (targetDepth <= correctedDist) {
            if (outSector) *outSector = currentSector;
            if (outClipTop) *outClipTop = clipTop;
            if (outClipBottom) *outClipBottom = clipBottom;
            return 1;
        }

        scale = projPlaneGlobal / correctedDist;
        secTop = (int)(horizonGlobal - ((sec->ceilHeight  - playerZ) * scale));
        secBot = (int)(horizonGlobal - ((sec->floorHeight - playerZ) * scale));

        wallMasked = ((w->flags & RC3D_WALL_TRANSPARENCY) != 0);
        flags = w->flags;

        if (wallClass == RC3D_WALLCLASS_PORTAL) {
            const int nextSectorIndex = w->neighbour;
            const RC3D_Sector *nextSec;
            int nextTop, nextBot;
            int wallOpenTopY, wallOpenBotY;
            int openTop, openBot;
            int entryWallInNext = -1;

            if ((unsigned)nextSectorIndex >= (unsigned)g_map->sectorCount) {
                return 0;
            }

            nextSec = &g_map->sectors[nextSectorIndex];

            nextTop = (int)(horizonGlobal - ((nextSec->ceilHeight  - playerZ) * scale));
            nextBot = (int)(horizonGlobal - ((nextSec->floorHeight - playerZ) * scale));

            wallOpenTopY = (int)(horizonGlobal - ((w->openTop    - playerZ) * scale));
            wallOpenBotY = (int)(horizonGlobal - ((w->openBottom - playerZ) * scale));

            openTop = secTop;
            openBot = secBot;

            if (nextTop > openTop)      openTop = nextTop;
            if (wallOpenTopY > openTop) openTop = wallOpenTopY;
            if (clipTop > openTop)      openTop = clipTop;

            if (nextBot < openBot)      openBot = nextBot;
            if (wallOpenBotY < openBot) openBot = wallOpenBotY;
            if (clipBottom < openBot)   openBot = clipBottom;

            if (openTop > openBot) {
                return 0;
            }

            if (wc) {
                entryWallInNext = wc->backWallIndex;
            }

            currentSector = nextSectorIndex;
            clipTop = openTop;
            clipBottom = openBot;
            ignoreWallIndexA = hit.wallIndex;
            ignoreWallIndexB = entryWallInNext;
            rayMinT = hit.t;
            continue;
        }

        if (wallMasked) {
            if ((flags & RC3D_WALL_SOLID) && (flags & RC3D_WALL_PORTAL) &&
                ((unsigned)w->neighbour < (unsigned)g_map->sectorCount))
            {
                int entryWallInNext = -1;

                if (secTop > clipTop) clipTop = secTop;
                if (secBot < clipBottom) clipBottom = secBot;
                if (clipTop > clipBottom) return 0;

                if (wc) {
                    entryWallInNext = wc->backWallIndex;
                }

                currentSector = w->neighbour;
                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = entryWallInNext;
                rayMinT = hit.t + RC3D_EPSILON;
                continue;
            }

            if (wallClass == RC3D_WALLCLASS_SOLID) {
                if (secTop > clipTop) clipTop = secTop;
                if (secBot < clipBottom) clipBottom = secBot;
                if (clipTop > clipBottom) return 0;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t + RC3D_EPSILON;
                continue;
            }

            if (wallClass == RC3D_WALLCLASS_MIDDLE) {
                const int midTopY =
                    (int)(horizonGlobal - ((w->openTop    - playerZ) * scale));
                const int midBotY =
                    (int)(horizonGlobal - ((w->openBottom - playerZ) * scale));

                if (midTopY > clipTop) clipTop = midTopY;
                if (midBotY < clipBottom) clipBottom = midBotY;
                if (clipTop > clipBottom) return 0;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t + RC3D_EPSILON;
                continue;
            }
        }

        return 0;
    }

    return 0;
}





static void rc3dRefreshSpritePlacement(RC3D_Sprite *sprite)
{
    int sector;

    if (!sprite || !sprite->active) return;

    sector = findSectorForSpritePosition(sprite->x, sprite->y, -1);
    if ((unsigned)sector >= (unsigned)g_map->sectorCount) {
        return;
    }

    sprite->baseZ = g_map->sectors[sector].floorHeight;
}




static void renderBillboardSprite(const RC3D_Sprite *sprite)
{
    const float screenCenterX = (float)SCREEN_W * 0.5f;
    const float eyeZ = g_renderEyeZ;
    const uint8_t *texels;

    float dirX = 1.0f;
    float dirY = 0.0f;
    float rightVecX;
    float rightVecY;
    float planeX;
    float planeY;

    float toSpriteX;
    float toSpriteY;
    float camX;
    float camDepth;
    float scale;

    int leftX;
    int rightX;
    int topY;
    int bottomY;
    int unclampedWidth;
    int unclampedHeight;

    uint16_t spriteDepth;

    if (!sprite || !sprite->active) return;

    texels = g_rc3dTextures[sprite->texId].pix;
    rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

    rightVecX = -dirY;
    rightVecY =  dirX;

    planeX = -dirY * g_planeScaleConst;
    planeY =  dirX * g_planeScaleConst;

    toSpriteX = sprite->x - g_player.x;
    toSpriteY = sprite->y - g_player.y;

    camX     = (toSpriteX * -dirY) + (toSpriteY * dirX);
    camDepth = (toSpriteX *  dirX) + (toSpriteY * dirY);

    if (camDepth <= RC3D_EPSILON || camDepth >= RC3D_MAX_RAY_DIST) {
        return;
    }

    scale = projPlaneGlobal / camDepth;

    leftX   = (int)(screenCenterX + ((camX - (sprite->width  * 0.5f)) * scale));
    rightX  = (int)(screenCenterX + ((camX + (sprite->width  * 0.5f)) * scale));
    topY    = (int)(horizonGlobal - (((sprite->baseZ + sprite->height) - eyeZ) * scale));
    bottomY = (int)(horizonGlobal - (( sprite->baseZ                   - eyeZ) * scale));

    if (leftX > rightX || topY > bottomY) {
        return;
    }

    if (rightX < 0 || leftX >= SCREEN_W || bottomY < 0 || topY >= SCREEN_H) {
        return;
    }

    unclampedWidth  = (rightX - leftX) + 1;
    unclampedHeight = (bottomY - topY) + 1;
    if (unclampedWidth <= 0 || unclampedHeight <= 0) {
        return;
    }

    spriteDepth = rc3dEncodeDepth(camDepth);

    {
        const int drawLeft  = (leftX  < 0) ? 0 : leftX;
        const int drawRight = (rightX >= SCREEN_W) ? (SCREEN_W - 1) : rightX;

        int preferredSector = g_player.sector;

        for (int sx = drawLeft; sx <= drawRight; ++sx) {
            const int tx = ((sx - leftX) * RC3D_TEX_SIZE) / unclampedWidth;

            int colTop    = (topY    < 0) ? 0 : topY;
            int colBottom = (bottomY >= SCREEN_H) ? (SCREEN_H - 1) : bottomY;

            /* --------------------------------------------------------- */
            /* clip against the sector the sprite itself occupies        */
            /* at this sprite width sample                               */
            /* --------------------------------------------------------- */
            {
                const float u = ((((float)(sx - leftX) + 0.5f) / (float)unclampedWidth) - 0.5f);
                const float localX = u * sprite->width;

                const float sampleWorldX = sprite->x + (rightVecX * localX);
                const float sampleWorldY = sprite->y + (rightVecY * localX);

                const int spriteSector =
                    findSectorForSpritePosition(sampleWorldX, sampleWorldY, preferredSector);

                if ((unsigned)spriteSector < (unsigned)g_map->sectorCount) {
                    const RC3D_Sector *sec = &g_map->sectors[spriteSector];
                    const int secTopY =
                        (int)(horizonGlobal - ((sec->ceilHeight  - eyeZ) * scale));
                    const int secBotY =
                        (int)(horizonGlobal - ((sec->floorHeight - eyeZ) * scale));

                    if (colTop < secTopY) {
                        colTop = secTopY;
                    }

                    if (colBottom > secBotY) {
                        colBottom = secBotY;
                    }

                    preferredSector = spriteSector;
                }
            }

            /* --------------------------------------------------------- */
            /* clip against the ACTUALLY visible sector in this screen   */
            /* column at the sprite depth                                */
            /* --------------------------------------------------------- */
            {
                int visSector = -1;
                int visClipTop = 0;
                int visClipBottom = SCREEN_H - 1;

                if (!rc3dTraceVisibleSectorAtDepth(
                        sx,
                        camDepth,
                        dirX,
                        dirY,
                        planeX,
                        planeY,
                        &visSector,
                        &visClipTop,
                        &visClipBottom))
                {
                    continue;
                }

                if (colTop < visClipTop) {
                    colTop = visClipTop;
                }

                if (colBottom > visClipBottom) {
                    colBottom = visClipBottom;
                }

                if ((unsigned)visSector < (unsigned)g_map->sectorCount) {
                    const RC3D_Sector *visSec = &g_map->sectors[visSector];
                    const int visSecTopY =
                        (int)(horizonGlobal - ((visSec->ceilHeight  - eyeZ) * scale));
                    const int visSecBotY =
                        (int)(horizonGlobal - ((visSec->floorHeight - eyeZ) * scale));

                    if (colTop < visSecTopY) {
                        colTop = visSecTopY;
                    }

                    if (colBottom > visSecBotY) {
                        colBottom = visSecBotY;
                    }
                }
            }

            if (colTop > colBottom) {
                continue;
            }

            for (int sy = colTop; sy <= colBottom; ++sy) {
                const int ty = ((sy - topY) * RC3D_TEX_SIZE) / unclampedHeight;
                const uint8_t texel = texels[(ty * RC3D_TEX_SIZE) + tx];

                if (texel == RC3D_SPRITE_TEX_TRANSPARENT) {
                    continue;
                }

                if (rc3dWallSpansBlockPixel(sx, sy, spriteDepth)) {
                    continue;
                }

                fb[(sy * SCREEN_W) + sx] = texel;
            }
        }
    }
}

static void renderSprites(void)
{
    renderBillboardSprite(&g_testSprite);
    renderBillboardSprite(&g_testSprite2);
}





/* ------------------------------------------------------------------------- */
/* minimap                                                                   */
/* ------------------------------------------------------------------------- */

static int minimapOutCode(int x, int y, int left, int top, int right, int bottom)
{
    int code = 0;
    if (x < left)   code |= 1;
    if (x > right)  code |= 2;
    if (y < top)    code |= 4;
    if (y > bottom) code |= 8;
    return code;
}

static int clipLineToRect(
    int *x0, int *y0,
    int *x1, int *y1,
    int left, int top,
    int right, int bottom
){
    int c0 = minimapOutCode(*x0, *y0, left, top, right, bottom);
    int c1 = minimapOutCode(*x1, *y1, left, top, right, bottom);

    for (;;) {
        if ((c0 | c1) == 0) return 1;
        if (c0 & c1) return 0;

        {
            int out = c0 ? c0 : c1;
            int x = 0;
            int y = 0;

            if (out & 4) {
                if (*y1 == *y0) return 0;
                x = *x0 + (*x1 - *x0) * (top - *y0) / (*y1 - *y0);
                y = top;
            } else if (out & 8) {
                if (*y1 == *y0) return 0;
                x = *x0 + (*x1 - *x0) * (bottom - *y0) / (*y1 - *y0);
                y = bottom;
            } else if (out & 2) {
                if (*x1 == *x0) return 0;
                y = *y0 + (*y1 - *y0) * (right - *x0) / (*x1 - *x0);
                x = right;
            } else if (out & 1) {
                if (*x1 == *x0) return 0;
                y = *y0 + (*y1 - *y0) * (left - *x0) / (*x1 - *x0);
                x = left;
            }

            if (out == c0) {
                *x0 = x;
                *y0 = y;
                c0 = minimapOutCode(*x0, *y0, left, top, right, bottom);
            } else {
                *x1 = x;
                *y1 = y;
                c1 = minimapOutCode(*x1, *y1, left, top, right, bottom);
            }
        }
    }
}

static void drawMiniMap(void)
{
    const int mapY = 8;
    const int mapW = 140;
    const int mapH = 100;
    const int mapX = (SCREEN_W - mapW) - 8;

    const int left   = mapX + 1;
    const int top    = mapY + 1;
    const int right  = mapX + mapW - 2;
    const int bottom = mapY + mapH - 2;

    const int centerX = mapX + (mapW / 2);
    const int centerY = mapY + (mapH / 2);

    const float scale = 4.0f;

    drawRect(mapX, mapY, mapW, mapH, 16);

    drawLine(mapX,             mapY,             mapX + mapW - 1, mapY,             15);
    drawLine(mapX,             mapY,             mapX,            mapY + mapH - 1,  15);
    drawLine(mapX + mapW - 1,  mapY,             mapX + mapW - 1, mapY + mapH - 1,  15);
    drawLine(mapX,             mapY + mapH - 1,  mapX + mapW - 1, mapY + mapH - 1,  15);

    {
        const RC3D_Wall *walls = g_map->walls;
        const RC3D_Vec2 *verts = g_map->verts;

        for (int s = 0; s < g_map->sectorCount; s++) {
            const RC3D_Sector *sec = &g_map->sectors[s];
            const int start = sec->wallStart;
            const int end   = start + sec->wallCount;

            for (int wi = start; wi < end; wi++) {
                const RC3D_Wall *w = &walls[wi];
                const RC3D_Vec2 *a = &verts[w->v0];
                const RC3D_Vec2 *b = &verts[w->v1];

                int x0 = centerX + (int)((a->x - g_player.x) * scale);
                int y0 = centerY + (int)((a->y - g_player.y) * scale);
                int x1 = centerX + (int)((b->x - g_player.x) * scale);
                int y1 = centerY + (int)((b->y - g_player.y) * scale);

                if (clipLineToRect(&x0, &y0, &x1, &y1, left, top, right, bottom)) {
                    drawLine(x0, y0, x1, y1, (w->neighbour >= 0) ? 27 : 2);
                }
            }
        }
    }

    if (centerX >= left && centerX <= right && centerY >= top && centerY <= bottom) {
        drawRect(centerX - 1, centerY - 1, 3, 3, 15);
    }

    {
        int x0 = centerX;
        int y0 = centerY;
        float dirX = 1.0f;
        float dirY = 0.0f;

        rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

        int x1 = centerX + (int)(dirX * 12.0f);
        int y1 = centerY + (int)(dirY * 12.0f);

        if (clipLineToRect(&x0, &y0, &x1, &y1, left, top, right, bottom)) {
            drawLine(x0, y0, x1, y1, 31);
        }
    }
}

static void rc3dInitTestSprite(void)
{
    memset(&g_testSprite, 0, sizeof(g_testSprite));
    memset(&g_testSprite2, 0, sizeof(g_testSprite2));

    rc3dSetSpriteWorldXYFixed(&g_testSprite, rc3dFloatToFixed(g_map->startX), rc3dFloatToFixed(g_map->startY));
    rc3dSetSpriteWorldXYFixed(&g_testSprite2, rc3dFloatToFixed(g_map->startX), rc3dFloatToFixed(g_map->startY));

    if (g_map->startSector >= 0 && g_map->startSector < g_map->sectorCount) {
        g_testSprite.baseZ = g_map->sectors[g_map->startSector].floorHeight;
        g_testSprite2.baseZ = g_map->sectors[g_map->startSector].floorHeight;
    }

    g_testSprite.width = RC3D_TEST_SPRITE_WIDTH;
    g_testSprite.height = RC3D_TEST_SPRITE_HEIGHT;
    g_testSprite.texId = RC3D_TEXID_SPRITE_MAN;
    g_testSprite.active = 1;

    g_testSprite2.width = RC3D_TEST_SPRITE_WIDTH;
    g_testSprite2.height = RC3D_TEST_SPRITE_HEIGHT;
    g_testSprite2.texId = RC3D_TEXID_SPRITE_MAN;
    g_testSprite2.active = 1;
}

void moveSprite(){
    g_testSprite.x -= 0.0005;
    rc3dRefreshSpritePlacement(&g_testSprite);
}

/* ------------------------------------------------------------------------- */
/* public api                                                                */
/* ------------------------------------------------------------------------- */

void rc3dInit(void)
{
    if (!g_rc3dTexturesInit) rc3dBuildDefaultTextures();
    if (!g_invDTableInit)    rc3dBuildInvDTable();
    rc3dBuildTrigTables();

    rc3dBuildFixedVertCacheForCurrentMap();
    rc3dBuildWallCacheForCurrentMap();
    rc3dBuildSectorCacheForCurrentMap();

    rc3dSetPlayerWorldXYFixed(
        rc3dFloatToFixed(g_map->startX),
        rc3dFloatToFixed(g_map->startY));
    g_player.angle = g_map->startAngle;
    g_player.sector = g_map->startSector;
    g_player.tHeadbob = 0.0f;

    if (g_player.sector >= 0 && g_player.sector < g_map->sectorCount) {
        g_player.z = g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;
    } else {
        g_player.z = RC3D_PLAYER_EYE_HEIGHT;
    }

    g_player.vz = 0.0f;
    rc3dInitTestSprite();
}



void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx)
{
    float moveX = 0.0f;
    float moveY = 0.0f;
    int isMoving = 0;

    float forwardX = 1.0f;
    float forwardY = 0.0f;

    rc3dLookupAngleTrig(g_player.angle, &forwardX, &forwardY);

    const float rightX   = -forwardY;
    const float rightY   =  forwardX;

    if (keys[SDL_SCANCODE_Q]) g_player.angle -= RC3D_TURN_SPEED * dt;
    if (keys[SDL_SCANCODE_E]) g_player.angle += RC3D_TURN_SPEED * dt;

    g_player.angle += (float)mouseDx * RC3D_MOUSE_SENS;
    g_player.angle = wrapAngle(g_player.angle);

    if (keys[SDL_SCANCODE_W]) { moveX += forwardX; moveY += forwardY; }
    if (keys[SDL_SCANCODE_S]) { moveX -= forwardX; moveY -= forwardY; }
    if (keys[SDL_SCANCODE_A]) { moveX -= rightX;   moveY -= rightY;   }
    if (keys[SDL_SCANCODE_D]) { moveX += rightX;   moveY += rightY;   }

    if ((moveX != 0.0f) || (moveY != 0.0f)) {
        const float len = sqrtf((moveX * moveX) + (moveY * moveY));
        const float step = RC3D_MOVE_SPEED * dt;

        moveX = (moveX / len) * step;
        moveY = (moveY / len) * step;

        if (tryMovePlayerSliding(moveX, moveY)) {
            isMoving = 1;
        }
    }

    /* headbob timer */
    if (isMoving) {
        g_player.tHeadbob += dt * 7.0f;   /* speed of bob */
        if (g_player.tHeadbob > (float)(M_PI)) {
            g_player.tHeadbob -= (float)(M_PI);
        }
    } else {
        /* settle back nicely when stopping */
        if (g_player.tHeadbob > 0.0f) {
            g_player.tHeadbob += dt * 12.0f;
            if (g_player.tHeadbob > (float)(M_PI)) {
                g_player.tHeadbob = 0.0f;
            }
        }
        
    }

    if (g_player.sector >= 0 && g_player.sector < g_map->sectorCount) {
        const float targetZ =
            g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;

        if (g_player.z > targetZ) {
            g_player.vz -= RC3D_GRAVITY * dt;
            g_player.z  += g_player.vz * dt;

            if (g_player.z <= targetZ) {
                g_player.z = targetZ;
                g_player.vz = 0.0f;
            }
        } else {
            const float dz = targetZ - g_player.z;

            if (dz > 0.0f) {
                const float riseSpeed = dz * 10.0f;
                const float maxRise   = RC3D_STEP_SNAP_SPEED * dt;
                float stepUp = riseSpeed * dt;

                if (stepUp > maxRise) stepUp = maxRise;
                if (stepUp > dz)      stepUp = dz;

                g_player.z += stepUp;
            }

            g_player.vz = 0.0f;
        }
    }
}




void rc3dRender(void)
{
    rc3dClearWallDepthSpans();
    drawBackground();
    renderCurrentSectorColumns();
    renderSprites();

#if RC3D_DRAW_MINIMAP
    drawMiniMap();
#endif

#if RC3D_DRAW_HUD
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "SECTOR %d", g_player.sector);
        drawText(8, 18, buf, 2);
    }
#endif
}
