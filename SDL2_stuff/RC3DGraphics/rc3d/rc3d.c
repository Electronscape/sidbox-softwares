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

#define RC3D_VIEWPORT_LEFT      0
#define RC3D_VIEWPORT_TOP       0
#define RC3D_VIEWPORT_WIDTH     SCREEN_W    // these can be adjusted
#define RC3D_VIEWPORT_HEIGHT    SCREEN_H

int g_viewport_top    = RC3D_VIEWPORT_TOP;
int g_viewport_left   = RC3D_VIEWPORT_LEFT;
int g_viewport_width  = RC3D_VIEWPORT_WIDTH;
int g_viewport_height = RC3D_VIEWPORT_HEIGHT;


#define RC3D_DRAW_MINIMAP      1
#define RC3D_DRAW_HUD          1
#define RC3D_DRAW_PROFILER     1

#define RC3D_FOV_DEG           90.0f
#define RC3D_TURN_SPEED        2.4f
#define RC3D_MOVE_SPEED        3.0f
#define RC3D_MOUSE_SENS        0.0035f
#define RC3D_EPSILON           0.00001f
#define RC3D_COLLISION_SKIN    0.02f
#define RC3D_FIXED_SHIFT       16
#define RC3D_FIXED_ONE         (1 << RC3D_FIXED_SHIFT)
#define RC3D_TRIG_LUT_BITS     11
#define RC3D_TRIG_LUT_SIZE     (1 << RC3D_TRIG_LUT_BITS)
#define RC3D_TRIG_LUT_MASK     (RC3D_TRIG_LUT_SIZE - 1)
#define RC3D_MAX_RAY_DIST      40.0f
#define RC3D_MAX_PORTAL_STEPS  24   // how many sectors we can look through to render before not bothering anymore
#define RC3D_MAX_MASKED_TRACE_DEPTH 8
#define RC3D_MAX_WALL_SPANS_PER_COLUMN 32
#define RC3D_DEPTH_SCALE       1024.0f


int bShowProfiler = 0;
float g_draw_distance = RC3D_MAX_RAY_DIST;

#define RC3D_PLAYER_EYE_HEIGHT 0.5f
#define RC3D_GRAVITY           18.0f
#define RC3D_STEP_SNAP_SPEED   24.0f

#define PLAYER_HEIGHT          0.6f
#define PLAYER_STEPUP          0.35f
#define PLAYER_RADIUS          0.40f

#define RC3D_TEX_SIZE          64
#define RC3D_TEX_MASK          (RC3D_TEX_SIZE - 1)

// colour bank size, palette offset 64, length of each palette back is 64
#define RC3D_LIGHT_BANK_START            64
#define RC3D_LIGHT_BANK_SIZE             64

#define RC3D_LIGHT_BLACK_INDEX           16
#define RC3D_LIGHT_MID_BRIGHTNESS        0.75f
#define RC3D_LIGHT_DARK_BRIGHTNESS       0.35f
#define RC3D_LIGHT_DEFAULT_BRIGHT_RANGE  8.0f
#define RC3D_LIGHT_DEFAULT_MID_RANGE     6.0f
#define RC3D_LIGHT_DEFAULT_DARK_RANGE    12.0f
#define RC3D_LIGHT_BRIGHT_BLEND_RATIO    1.00f
#define RC3D_LIGHT_WALL_DIST_SCALE       1.00f
#define RC3D_LIGHT_PLANE_DIST_SCALE      0.98f
#define RC3D_LIGHT_SPRITE_DIST_SCALE     0.70f

extern uint8_t spr_man[]; // test sprite


#define RC3D_TEXID_SPRITE_MAN  RC3D_SPRITE_TEX_MAN
#define RC3D_TEXID_SKYBOX      255

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
static float g_lightBrightRange = RC3D_LIGHT_DEFAULT_BRIGHT_RANGE;
static float g_lightMidRange = RC3D_LIGHT_DEFAULT_MID_RANGE;
static float g_lightDarkRange = RC3D_LIGHT_DEFAULT_DARK_RANGE;

#if RC3D_DRAW_PROFILER
typedef struct {
    double backgroundMs;
    double wallsMs;
    double spritesMs;
    double minimapMs;
    double totalMs;

    double avgBackgroundMs;
    double avgWallsMs;
    double avgSpritesMs;
    double avgMinimapMs;
    double avgTotalMs;

    uint32_t rays;
    uint32_t wallTests;
    uint32_t portalSteps;
    uint32_t spriteColumns;
    uint32_t spriteSectorTraces;
} RC3D_Profiler;

static RC3D_Profiler g_profiler;
static Uint64 g_profilerFreq = 0;
static int g_profilerHasHistory = 0;

static inline double rc3dProfilerTicksToMs(Uint64 ticks)
{
    if (g_profilerFreq == 0) {
        g_profilerFreq = SDL_GetPerformanceFrequency();
    }

    if (g_profilerFreq == 0) {
        return 0.0;
    }

    return ((double)ticks * 1000.0) / (double)g_profilerFreq;
}

static inline void rc3dProfilerBeginFrame(void)
{
    g_profiler.backgroundMs = 0.0;
    g_profiler.wallsMs = 0.0;
    g_profiler.spritesMs = 0.0;
    g_profiler.minimapMs = 0.0;
    g_profiler.totalMs = 0.0;

    g_profiler.rays = 0u;
    g_profiler.wallTests = 0u;
    g_profiler.portalSteps = 0u;
    g_profiler.spriteColumns = 0u;
    g_profiler.spriteSectorTraces = 0u;
}

static inline void rc3dProfilerBlendAverages(void)
{
    const double alpha = 0.20;

    if (!g_profilerHasHistory) {
        g_profiler.avgBackgroundMs = g_profiler.backgroundMs;
        g_profiler.avgWallsMs = g_profiler.wallsMs;
        g_profiler.avgSpritesMs = g_profiler.spritesMs;
        g_profiler.avgMinimapMs = g_profiler.minimapMs;
        g_profiler.avgTotalMs = g_profiler.totalMs;
        g_profilerHasHistory = 1;
        return;
    }

    g_profiler.avgBackgroundMs += (g_profiler.backgroundMs - g_profiler.avgBackgroundMs) * alpha;
    g_profiler.avgWallsMs += (g_profiler.wallsMs - g_profiler.avgWallsMs) * alpha;
    g_profiler.avgSpritesMs += (g_profiler.spritesMs - g_profiler.avgSpritesMs) * alpha;
    g_profiler.avgMinimapMs += (g_profiler.minimapMs - g_profiler.avgMinimapMs) * alpha;
    g_profiler.avgTotalMs += (g_profiler.totalMs - g_profiler.avgTotalMs) * alpha;
}

static void rc3dDrawProfiler(void)
{
    char buf[80];
    const int textW = 8;
    const int lineH = 8;
    const int panelX = 6;
    const int panelW = (27 * textW) + 8;
    const int panelH = (10 * lineH) + 8;
    const int panelY = SCREEN_H - panelH - 6;
    int textY = panelY + 4;

    drawRect(panelX, panelY, panelW, panelH, 16);
    drawLine(panelX, panelY, panelX + panelW - 1, panelY, 15);
    drawLine(panelX, panelY, panelX, panelY + panelH - 1, 15);
    drawLine(panelX + panelW - 1, panelY, panelX + panelW - 1, panelY + panelH - 1, 15);
    drawLine(panelX, panelY + panelH - 1, panelX + panelW - 1, panelY + panelH - 1, 15);

    drawText(panelX + 4, textY, "PROFILER (AVG MS)", 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "TOTAL FRAME   %.2f",
        g_profiler.avgTotalMs);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "SKY/BG        %.2f",
        g_profiler.avgBackgroundMs);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "WALLS         %.2f",
        g_profiler.avgWallsMs);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "SPRITES       %.2f",
        g_profiler.avgSpritesMs);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "MINIMAP       %.2f",
        g_profiler.avgMinimapMs);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "RAYS %u  WALL TESTS %u",
        g_profiler.rays,
        g_profiler.wallTests);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "PORTAL STEPS %u",
        g_profiler.portalSteps);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "SPRITE COLS %u",
        g_profiler.spriteColumns);
    drawText(panelX + 4, textY, buf, 15);
    textY += lineH;

    snprintf(
        buf, sizeof(buf),
        "SPRITE TRACES %u  DD %.1f",
        g_profiler.spriteSectorTraces,
        g_draw_distance);
    drawText(panelX + 4, textY, buf, 15);
}
#endif



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
    uint8_t inUse;
    uint8_t active;
    uint8_t reserved;
} RC3D_Sprite;

typedef struct {
    int spriteId;
    float camDepth;
} RC3D_SpriteOrderEntry;

typedef struct {
    int16_t y0;
    int16_t y1;
    uint16_t depth;
} RC3D_WallDepthSpan;

static RC3D_Player g_player;
static RC3D_Sprite g_sprites[RC3D_MAX_SPRITES];
static RC3D_SpriteOrderEntry g_spriteOrder[RC3D_MAX_SPRITES];
static int g_demoSpriteA = RC3D_INVALID_SPRITE;
static int g_demoSpriteB = RC3D_INVALID_SPRITE;

static const RC3D_Map *g_map = &g_rc3dDemoMap;
static RC3D_Map g_loadedMap;
static int g_loadedMapValid = 0;

static int horizonGlobal = 0;
static float wallTexUBaseGlobal = 0.0f;
static float wallTexInvScaleYGlobal = 1.0f;
static float wallTexRotCosGlobal = 1.0f;
static float wallTexRotSinGlobal = 0.0f;
static float projPlaneGlobal = 0.0f;

static RC3D_Texture g_rc3dTextures[256];
static int g_rc3dTexturesInit = 0;
static uint8_t g_lightVariantBright[256];
static uint8_t g_lightVariantMid[256];
static uint8_t g_lightVariantDark[256];
static uint8_t g_lightVariantBlack[256];
static float g_glowDistScale[RC3D_TEX_WALL_GLOW_MAX + 1u];
static int g_lightVariantTablesInit = 0;


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
    float texInvScaleX;
    float texInvScaleY;

    float texCos;
    float texSin;

    uint8_t hasTexRotate;
    uint8_t wallClass;
    uint8_t texXMode;

    int32_t ownerSector;
    int32_t backWallIndex;   // matching wall in neighbour sector, or -1

    float portalOpenBottom;
    float portalOpenTop;
    uint8_t portalHasUpper;
    uint8_t portalHasLower;
} RC3D_WallCache;

typedef struct {
    RC3D_Fixed minX;
    RC3D_Fixed maxX;
    RC3D_Fixed minY;
    RC3D_Fixed maxY;

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

typedef struct {
    float openBottom;
    float openTop;
    uint8_t hasUpper;
    uint8_t hasLower;
} RC3D_PortalView;

typedef struct {
    float rdx;
    float rdy;
    float planeLightLen;
} RC3D_ColumnRayCache;

typedef struct {
    int16_t sector;
    int16_t clipTop;
    int16_t clipBottom;
    uint16_t depthLimit;
} RC3D_VisibleTraceSegment;

typedef enum {
    RC3D_SHADEMODE_BRIGHT = 0,
    RC3D_SHADEMODE_MID,
    RC3D_SHADEMODE_DARK,
    RC3D_SHADEMODE_BLACK,
    RC3D_SHADEMODE_DITHER_BRIGHT_MID,
    RC3D_SHADEMODE_DITHER_MID_DARK,
    RC3D_SHADEMODE_DITHER_DARK_BLACK
} RC3D_ShadeMode;

typedef struct {
    uint8_t mode;
    uint8_t threshold;
} RC3D_ShadeProfile;



static RC3D_WallCache *g_wallCache = NULL;
static int g_wallCacheCount = 0;
static RC3D_SectorCache *g_sectorCache = NULL;
static int g_sectorCacheCount = 0;
static RC3D_FixedVec2 *g_fixedVerts = NULL;
static int g_fixedVertCount = 0;

static float g_invDTable[(SCREEN_H * 2) + 1];
static int g_invDTableInit = 0;
static uint16_t g_skyXTable[SCREEN_W];
static RC3D_ColumnRayCache g_columnRayCache[SCREEN_W];
static int g_columnRayCacheCount = 0;
static RC3D_VisibleTraceSegment g_visibleTraceCache[SCREEN_W][RC3D_MAX_PORTAL_STEPS];
static uint8_t g_visibleTraceCount[SCREEN_W];
static uint8_t g_visibleTraceBuilt[SCREEN_W];
static RC3D_WallDepthSpan g_wallDepthSpans[SCREEN_W][RC3D_MAX_WALL_SPANS_PER_COLUMN];
static uint8_t g_wallDepthSpanCount[SCREEN_W];

static inline int rc3dViewportScreenX(int sx){
    return g_viewport_left + sx;
}

static inline int rc3dViewportScreenY(int sy){
    return g_viewport_top + sy;
}

static inline uint8_t *rc3dViewportPixelPtr(int sx, int sy){
    return &fb[(rc3dViewportScreenY(sy) * SCREEN_W) + rc3dViewportScreenX(sx)];
}

static inline float rc3dViewportCameraX(int sx)
{
    if (g_viewport_width <= 1) {
        return 0.0f;
    }

    return -1.0f + (2.0f * ((float)sx / (float)(g_viewport_width - 1)));
}

static void rc3dBuildColumnRayCache(float dirX, float dirY, float planeX, float planeY)
{
    const float rayStepX = planeX * g_camStepConst;
    const float rayStepY = planeY * g_camStepConst;
    float rdx = dirX - planeX;
    float rdy = dirY - planeY;

    g_columnRayCacheCount = 0;

    if (g_viewport_width <= 0) {
        return;
    }

    if (g_viewport_width <= 1) {
        rdx = dirX;
        rdy = dirY;
    }

    for (int sx = 0; sx < g_viewport_width; ++sx) {
        RC3D_ColumnRayCache *cache = &g_columnRayCache[sx];

        cache->rdx = rdx;
        cache->rdy = rdy;
        cache->planeLightLen =
            sqrtf((rdx * rdx) + (rdy * rdy)) * RC3D_LIGHT_PLANE_DIST_SCALE;

        rdx += rayStepX;
        rdy += rayStepY;
    }

    g_columnRayCacheCount = g_viewport_width;
}

static inline void rc3dInvalidateVisibleTraceCache(void){
    memset(g_visibleTraceBuilt, 0, sizeof(g_visibleTraceBuilt));
}


/* ------------------------------------------------------------------------- */
/* forward decls                                                             */
/* ------------------------------------------------------------------------- */

static int tryMovePlayerSliding(float moveX, float moveY);
static void rc3dRefreshDynamicPortalCache(void);
static void rc3dRefreshSpritePlacement(RC3D_Sprite *sprite);
void screenupdate(void);

static inline RC3D_Fixed rc3dFloatToFixed(float v){
    if (v >= 0.0f) {
        return (RC3D_Fixed)(v * (float)RC3D_FIXED_ONE + 0.5f);
    }

    return (RC3D_Fixed)(v * (float)RC3D_FIXED_ONE - 0.5f);
}

static inline float rc3dFixedToFloat(RC3D_Fixed v){
    return (float)v / (float)RC3D_FIXED_ONE;
}

static inline RC3D_Fixed rc3dFixedMul(RC3D_Fixed a, RC3D_Fixed b){
    return (RC3D_Fixed)(((int64_t)a * (int64_t)b) >> RC3D_FIXED_SHIFT);
}

static inline int64_t rc3dFixedSq(RC3D_Fixed v){
    return (int64_t)v * (int64_t)v;
}

static inline void rc3dSyncPlayerFloatXY(void){
    g_player.x = rc3dFixedToFloat(g_player.xFixed);
    g_player.y = rc3dFixedToFloat(g_player.yFixed);
}

static inline void rc3dSetPlayerWorldXYFixed(RC3D_Fixed x, RC3D_Fixed y){
    g_player.xFixed = x;
    g_player.yFixed = y;
    rc3dSyncPlayerFloatXY();
}

static inline void rc3dSetSpriteWorldXYFixed(RC3D_Sprite *sprite, RC3D_Fixed x, RC3D_Fixed y){
    if (!sprite) return;

    sprite->xFixed = x;
    sprite->yFixed = y;
    sprite->x = rc3dFixedToFloat(x);
    sprite->y = rc3dFixedToFloat(y);
}

static inline int rc3dSpriteHandleValid(int spriteId){
    return (spriteId >= 0) &&
           (spriteId < RC3D_MAX_SPRITES) &&
           g_sprites[spriteId].inUse;
}

static void rc3dBuildTrigTables(void){
    if (g_trigLutInit)
        return;

    for (int i = 0; i < RC3D_TRIG_LUT_SIZE; ++i) {
        const float angle =
            ((float)i * (float)(M_PI * 2.0f)) / (float)RC3D_TRIG_LUT_SIZE;
        g_sinLut[i] = sinf(angle);
        g_cosLut[i] = cosf(angle);
    }

    g_halfFovRad = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    g_planeScaleConst = tanf(g_halfFovRad);
    g_projPlaneConst  = ((float)g_viewport_width * 0.5f) / g_planeScaleConst;
    g_camStepConst    = (g_viewport_width > 1) ? (2.0f / (float)(g_viewport_width - 1)) : 0.0f;
    g_angleToLutScale = (float)RC3D_TRIG_LUT_SIZE / (float)(M_PI * 2.0f);

    g_trigLutInit = 1;
}

static inline int rc3dAngleToLutIndex(float angle){
    const float scaled = angle * g_angleToLutScale;
    int idx = (int)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));

    idx %= RC3D_TRIG_LUT_SIZE;
    if (idx < 0) idx += RC3D_TRIG_LUT_SIZE;
    return idx & RC3D_TRIG_LUT_MASK;
}

static inline void rc3dLookupAngleTrig(float angle, float *outCos, float *outSin){
    const int idx = rc3dAngleToLutIndex(angle);

    if (outCos) *outCos = g_cosLut[idx];
    if (outSin) *outSin = g_sinLut[idx];
}

static inline uint16_t rc3dEncodeDepth(float dist){
    if (dist <= 0.0f)
        return 0;

    if (dist >= g_draw_distance)
        return UINT16_MAX - 1u;

    {
        const float scaled = dist * RC3D_DEPTH_SCALE;
        if (scaled >= (float)(UINT16_MAX - 1u)) {
            return UINT16_MAX - 1u;
        }

        return (uint16_t)(scaled + 0.5f);
    }
}

static inline void rc3dClearWallDepthSpans(void){
    memset(g_wallDepthSpanCount, 0, sizeof(g_wallDepthSpanCount));
}

static inline void rc3dRecordWallDepthSpan(int sx, int y0, int y1, float hitDist){
    uint8_t count;

    if ((unsigned)sx >= (unsigned)g_viewport_width) return;
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
    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return 0;
    }

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

static inline float rc3dSaturate(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline float rc3dSmoothStep01(float t)
{
    t = rc3dSaturate(t);
    return t * t * (3.0f - (2.0f * t));
}

static inline float rc3dMaxf(float a, float b)
{
    return (a > b) ? a : b;
}

static void rc3dBuildLightVariantTables(void)
{
    const int bankEnd = RC3D_LIGHT_BANK_START + (RC3D_LIGHT_BANK_SIZE * 3);

    for (int texel = 0; texel < 256; ++texel) {
        const uint8_t texel8 = (uint8_t)texel;

        g_lightVariantBright[texel] = texel8;
        g_lightVariantMid[texel] = texel8;
        g_lightVariantDark[texel] = texel8;
        g_lightVariantBlack[texel] = texel8;

        if (texel < RC3D_LIGHT_BANK_START || texel >= bankEnd) {
            continue;
        }

        {
            const uint8_t baseSlot = (uint8_t)((texel - RC3D_LIGHT_BANK_START) & (RC3D_LIGHT_BANK_SIZE - 1));
            const uint8_t bright = RC3D_LIGHT_BANK_START + baseSlot;
            const uint8_t mid = bright + RC3D_LIGHT_BANK_SIZE;
            const uint8_t dark = mid + RC3D_LIGHT_BANK_SIZE;

            g_lightVariantBright[texel] = bright;
            g_lightVariantMid[texel] = mid;
            g_lightVariantDark[texel] = dark;
            g_lightVariantBlack[texel] = RC3D_LIGHT_BLACK_INDEX;
        }
    }

    for (uint8_t glow = 0; glow <= RC3D_TEX_WALL_GLOW_MAX; ++glow) {
        if (glow >= RC3D_TEX_WALL_GLOW_MAX) {
            g_glowDistScale[glow] = 0.0f;
        } else {
            g_glowDistScale[glow] =
                1.0f - ((float)glow / (float)RC3D_TEX_WALL_GLOW_MAX);
        }
    }

    g_lightVariantTablesInit = 1;
}

static inline uint8_t rc3dClampGlowLevel(uint8_t glowLevel)
{
    if (glowLevel > RC3D_TEX_WALL_GLOW_MAX) {
        return RC3D_TEX_WALL_GLOW_MAX;
    }

    return glowLevel;
}

static inline uint8_t rc3dWallGlowFromFlags(uint32_t texture_flags)
{
    return rc3dClampGlowLevel(
        (uint8_t)((texture_flags & RC3D_TEX_WALL_GLOW_MASK) >> RC3D_TEX_WALL_GLOW_SHIFT));
}

static inline void rc3dBuildShadeProfile(
    float sampleDist,
    uint8_t glowLevel,
    RC3D_ShadeProfile *outProfile
)
{
    const float brightRange = g_lightBrightRange;
    const float midRange = g_lightMidRange;
    const float darkRange = g_lightDarkRange;
    const float brightEnd = brightRange;
    const float midEnd = brightEnd + midRange;
    const float darkEnd = midEnd + darkRange;

    if (!outProfile) {
        return;
    }

    glowLevel = rc3dClampGlowLevel(glowLevel);

    if (glowLevel > 0u) {
        sampleDist *= g_glowDistScale[glowLevel];
    }

    if (sampleDist <= 0.0f) {
        outProfile->mode = RC3D_SHADEMODE_BRIGHT;
        outProfile->threshold = 0u;
        return;
    }

    {
        const float brightBlendWidth =
            rc3dMaxf(brightRange * RC3D_LIGHT_BRIGHT_BLEND_RATIO, 0.0f);
        const float brightBlendStart = brightEnd - brightBlendWidth;

        if (sampleDist <= brightBlendStart) {
            outProfile->mode = RC3D_SHADEMODE_BRIGHT;
            outProfile->threshold = 0u;
            return;
        }

        if ((brightBlendWidth > RC3D_EPSILON) && (sampleDist < brightEnd)) {
            const float blend = rc3dSmoothStep01(
                (sampleDist - brightBlendStart) / brightBlendWidth);
            outProfile->mode = RC3D_SHADEMODE_DITHER_BRIGHT_MID;
            outProfile->threshold = (uint8_t)(blend * 255.0f);
            return;
        }
    }

    {
        const float midBlendWidth =
            rc3dMaxf(midRange * RC3D_LIGHT_BRIGHT_BLEND_RATIO, 0.0f);
        const float midBlendStart = midEnd - midBlendWidth;

        if (sampleDist <= midBlendStart) {
            outProfile->mode = RC3D_SHADEMODE_MID;
            outProfile->threshold = 0u;
            return;
        }

        if ((midBlendWidth > RC3D_EPSILON) && (sampleDist < midEnd)) {
            const float blend = rc3dSmoothStep01(
                (sampleDist - midBlendStart) / midBlendWidth);
            outProfile->mode = RC3D_SHADEMODE_DITHER_MID_DARK;
            outProfile->threshold = (uint8_t)(blend * 255.0f);
            return;
        }
    }

    if (darkRange <= RC3D_EPSILON) {
        outProfile->mode = RC3D_SHADEMODE_BLACK;
        outProfile->threshold = 0u;
        return;
    }

    if (sampleDist < darkEnd) {
        const float blend = rc3dSmoothStep01(
            (sampleDist - midEnd) / darkRange);
        outProfile->mode = RC3D_SHADEMODE_DITHER_DARK_BLACK;
        outProfile->threshold = (uint8_t)(blend * 255.0f);
        return;
    }

    outProfile->mode = RC3D_SHADEMODE_BLACK;
    outProfile->threshold = 0u;
}

static inline uint8_t rc3dLightNoise8(int a, int b, int c, int d)
{
    uint32_t n = (uint32_t)a;

    n *= 0x1f123bb5u;
    n += (uint32_t)b * 0x9e3779b9u;
    n ^= (uint32_t)c * 0x85ebca6bu;
    n += (uint32_t)d * 0xc2b2ae35u;
    n ^= n >> 15;
    n *= 0x85ebca6bu;
    n ^= n >> 13;
    n *= 0xc2b2ae35u;
    n ^= n >> 16;

    return (uint8_t)(n >> 24);
}

static inline uint8_t rc3dApplyShadeProfileToTexel(
    uint8_t texel,
    int sampleU,
    int sampleV,
    uint8_t texId,
    const RC3D_ShadeProfile *shadeProfile
)
{
    if (texel == RC3D_SPRITE_TEX_TRANSPARENT || !shadeProfile) {
        return texel;
    }

    switch ((RC3D_ShadeMode)shadeProfile->mode) {
        case RC3D_SHADEMODE_BRIGHT:
            return g_lightVariantBright[texel];

        case RC3D_SHADEMODE_MID:
            return g_lightVariantMid[texel];

        case RC3D_SHADEMODE_DARK:
            return g_lightVariantDark[texel];

        case RC3D_SHADEMODE_BLACK:
            return g_lightVariantBlack[texel];

        case RC3D_SHADEMODE_DITHER_BRIGHT_MID:
            return (rc3dLightNoise8(sampleU, sampleV, texId, 0) <= shadeProfile->threshold)
                ? g_lightVariantMid[texel]
                : g_lightVariantBright[texel];

        case RC3D_SHADEMODE_DITHER_MID_DARK:
            return (rc3dLightNoise8(sampleU, sampleV, texId, 1) <= shadeProfile->threshold)
                ? g_lightVariantDark[texel]
                : g_lightVariantMid[texel];

        case RC3D_SHADEMODE_DITHER_DARK_BLACK:
            return (rc3dLightNoise8(sampleU, sampleV, texId, 2) <= shadeProfile->threshold)
                ? g_lightVariantBlack[texel]
                : g_lightVariantDark[texel];

        default:
            return texel;
    }
}


extern uint8_t spr_oiiacat[];
extern uint8_t tex_notset[];
extern uint8_t tex_skybox[RC3D_SKYBOX_W * RC3D_SKYBOX_H];    // the skybox

static void rc3dBuildDefaultTextures(void)
{
    int x, y, i;
    int32_t tindex = 0;
    char filename[256];

    
    for (i = 0; i < 256; ++i) {
        tindex = 0;
        for (y = 0; y < RC3D_TEX_SIZE; ++y) {
            for (x = 0; x < RC3D_TEX_SIZE; ++x) {
                g_rc3dTextures[i].pix[(y * RC3D_TEX_SIZE) + x] = tex_notset[tindex++];
            }
        }
    }

    // one less as thats the sky box
    for(tindex = 0; tindex < 255; tindex++){
        snprintf(filename, sizeof(filename), "./textures/%02u.ppb", (unsigned)tindex);
        LoadPPB(filename, g_rc3dTextures[tindex].pix);
    }

    snprintf(filename, sizeof(filename), "./textures/255.ppb");
    LoadPPB(filename, tex_skybox);
    
    tindex = 0;
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            g_rc3dTextures[RC3D_TEXID_SPRITE_MAN].pix[tindex] = spr_oiiacat[tindex];
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
extern uint32_t clut[256];
extern uint32_t clut[256];

void rc3dPreparePalette(void)
{
    int p, step;
    const int cindex = 64;
    const float fade[2] = {
        RC3D_LIGHT_MID_BRIGHTNESS,
        RC3D_LIGHT_DARK_BRIGHTNESS
    };

    for (step = 0; step < 2; step++) {
        for (p = 0; p < 64; p++) {
            uint32_t src = clut[cindex + p];

            uint8_t a = 0xff;
            uint8_t r = (src >> 16) & 0xFF;
            uint8_t g = (src >> 8)  & 0xFF;
            uint8_t b = (src >> 0)  & 0xFF;

            r = (uint8_t)(r * fade[step]);
            g = (uint8_t)(g * fade[step]);
            b = (uint8_t)(b * fade[step]);

            clut[128 + (step * 64) + p] =
                ((uint32_t)a << 24) |
                ((uint32_t)r << 16) |
                ((uint32_t)g << 8)  |
                ((uint32_t)b << 0);
        }
    }

    if (!g_lightVariantTablesInit) {
        rc3dBuildLightVariantTables();
    }
}

void rc3dSetLightRange(float brightRange, float midRange, float darkRange)
{
    if (brightRange < 0.0f) brightRange = 0.0f;
    if (midRange < 0.0f) midRange = 0.0f;
    if (darkRange < 0.0f) darkRange = 0.0f;

    g_lightBrightRange = brightRange;
    g_lightMidRange = midRange;
    g_lightDarkRange = darkRange;
}

void rc3dLightRange(float brightRange, float midRange, float darkRange)
{
    rc3dSetLightRange(brightRange, midRange, darkRange);
}

void rc3dSetDrawDistance(float distance)
{
    if (distance < 0.5f) {
        distance = 0.5f;
    }

    if (distance > RC3D_MAX_RAY_DIST) {
        distance = RC3D_MAX_RAY_DIST;
    }

    g_draw_distance = distance;
}

void rc3dClearSprites(void)
{
    memset(g_sprites, 0, sizeof(g_sprites));
    memset(g_spriteOrder, 0, sizeof(g_spriteOrder));
    g_demoSpriteA = RC3D_INVALID_SPRITE;
    g_demoSpriteB = RC3D_INVALID_SPRITE;
}

int rc3dSpriteCreate(float x, float y, float width, float height, uint8_t texId)
{
    for (int i = 0; i < RC3D_MAX_SPRITES; ++i) {
        RC3D_Sprite *sprite = &g_sprites[i];

        if (sprite->inUse) {
            continue;
        }

        memset(sprite, 0, sizeof(*sprite));
        sprite->inUse = 1u;
        sprite->active = 1u;
        sprite->texId = texId;
        sprite->width =
            (width > RC3D_EPSILON) ? width : RC3D_TEST_SPRITE_WIDTH;
        sprite->height =
            (height > RC3D_EPSILON) ? height : RC3D_TEST_SPRITE_HEIGHT;

        rc3dSetSpriteWorldXYFixed(
            sprite,
            rc3dFloatToFixed(x),
            rc3dFloatToFixed(y));
        rc3dRefreshSpritePlacement(sprite);
        return i;
    }

    return RC3D_INVALID_SPRITE;
}

void rc3dSpriteDestroy(int spriteId)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    memset(&g_sprites[spriteId], 0, sizeof(g_sprites[spriteId]));
}

void rc3dSpriteSetActive(int spriteId, int active)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    g_sprites[spriteId].active = active ? 1u : 0u;
}

void rc3dSpriteSetPosition(int spriteId, float x, float y)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    rc3dSetSpriteWorldXYFixed(
        &g_sprites[spriteId],
        rc3dFloatToFixed(x),
        rc3dFloatToFixed(y));
    rc3dRefreshSpritePlacement(&g_sprites[spriteId]);
}

void rc3dSpriteSetPositionFixed(int spriteId, int32_t xFixed, int32_t yFixed)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    rc3dSetSpriteWorldXYFixed(
        &g_sprites[spriteId],
        (RC3D_Fixed)xFixed,
        (RC3D_Fixed)yFixed);
    rc3dRefreshSpritePlacement(&g_sprites[spriteId]);
}

void rc3dSpriteSetSize(int spriteId, float width, float height)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    if (width > RC3D_EPSILON) {
        g_sprites[spriteId].width = width;
    }

    if (height > RC3D_EPSILON) {
        g_sprites[spriteId].height = height;
    }
}

void rc3dSpriteSetTexture(int spriteId, uint8_t texId)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    g_sprites[spriteId].texId = texId;
}

void rc3dSpriteSetBaseZ(int spriteId, float baseZ)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    g_sprites[spriteId].baseZ = baseZ;
}

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

    g_sectorCache = (RC3D_SectorCache *)malloc(sizeof(RC3D_SectorCache) * (size_t)g_map->sectorCount);
    if (!g_sectorCache) {
        return 0;
    }

    g_sectorCacheCount = g_map->sectorCount;

    for (int i = 0; i < g_map->sectorCount; ++i) {
        const RC3D_Sector *sec = &g_map->sectors[i];
        RC3D_SectorCache *cache = &g_sectorCache[i];
        const int bboxStart = sec->wallStart;
        const int bboxEnd = bboxStart + ((sec->boundaryCount > 0) ? sec->boundaryCount : sec->wallCount);

        float floorScaleX = sec->floorTexScaleX;
        float floorScaleY = sec->floorTexScaleY;
        float ceilScaleX = sec->ceilTexScaleX;
        float ceilScaleY = sec->ceilTexScaleY;

        if (bboxStart < bboxEnd) {
            int bboxInit = 0;

            for (int wi = bboxStart; wi < bboxEnd; ++wi) {
                const RC3D_Wall *w = &g_map->walls[wi];
                RC3D_Fixed vx[2];
                RC3D_Fixed vy[2];

                if (g_fixedVerts &&
                    (unsigned)w->v0 < (unsigned)g_fixedVertCount &&
                    (unsigned)w->v1 < (unsigned)g_fixedVertCount)
                {
                    vx[0] = g_fixedVerts[w->v0].x;
                    vy[0] = g_fixedVerts[w->v0].y;
                    vx[1] = g_fixedVerts[w->v1].x;
                    vy[1] = g_fixedVerts[w->v1].y;
                } else {
                    vx[0] = rc3dFloatToFixed(g_map->verts[w->v0].x);
                    vy[0] = rc3dFloatToFixed(g_map->verts[w->v0].y);
                    vx[1] = rc3dFloatToFixed(g_map->verts[w->v1].x);
                    vy[1] = rc3dFloatToFixed(g_map->verts[w->v1].y);
                }

                for (int vi = 0; vi < 2; ++vi) {
                    if (!bboxInit) {
                        cache->minX = cache->maxX = vx[vi];
                        cache->minY = cache->maxY = vy[vi];
                        bboxInit = 1;
                    } else {
                        if (vx[vi] < cache->minX) cache->minX = vx[vi];
                        if (vx[vi] > cache->maxX) cache->maxX = vx[vi];
                        if (vy[vi] < cache->minY) cache->minY = vy[vi];
                        if (vy[vi] > cache->maxY) cache->maxY = vy[vi];
                    }
                }
            }
        } else {
            cache->minX = 0;
            cache->maxX = 0;
            cache->minY = 0;
            cache->maxY = 0;
        }

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
        const float texAngle = rc3dWallTexAngleFromFlags(w->texture_flags);

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

        {
            float texScaleX = w->texScaleX;
            float texScaleY = w->texScaleY;

            if (fabsf(texScaleX) < RC3D_EPSILON) texScaleX = 1.0f;
            if (fabsf(texScaleY) < RC3D_EPSILON) texScaleY = 1.0f;

            wc->texInvScaleX = 1.0f / texScaleX;
            wc->texInvScaleY = 1.0f / texScaleY;
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
            const int clampXL = (w->texture_flags & RC3D_TEX_FLAG_CLAMPXL) ? 1 : 0;
            const int clampXR = (w->texture_flags & RC3D_TEX_FLAG_CLAMPXR) ? 1 : 0;

            if (clampXL && clampXR) {
                wc->texXMode = RC3D_TEX_XMODE_STRETCH;
            } else if (clampXR && !clampXL) {
                wc->texXMode = RC3D_TEX_XMODE_CLAMP_RIGHT;
            } else {
                wc->texXMode = RC3D_TEX_XMODE_TILE;
            }
        }

        wc->ownerSector = -1;
        wc->backWallIndex = -1;
        wc->portalOpenBottom = 0.0f;
        wc->portalOpenTop = 0.0f;
        wc->portalHasUpper = 0u;
        wc->portalHasLower = 0u;
    }

    for (int s = 0; s < g_map->sectorCount; ++s) {
        const RC3D_Sector *sec = &g_map->sectors[s];
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        for (int wi = start; wi < end; ++wi) {
            if ((unsigned)wi < (unsigned)g_wallCacheCount) {
                g_wallCache[wi].ownerSector = s;
            }
        }
    }

    for (int i = 0; i < g_map->wallCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];
        if (w->neighbour < 0 || w->neighbour >= g_map->sectorCount) {
            continue;
        }

        const int thisSector = g_wallCache[i].ownerSector;
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

    rc3dRefreshDynamicPortalCache();
    return 1;
}

static void rc3dRefreshDynamicPortalCache(void)
{
    if (!g_map || !g_wallCache || g_wallCacheCount <= 0) {
        return;
    }

    for (int i = 0; i < g_wallCacheCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];
        RC3D_WallCache *wc = &g_wallCache[i];

        wc->portalOpenBottom = 0.0f;
        wc->portalOpenTop = 0.0f;
        wc->portalHasUpper = 0u;
        wc->portalHasLower = 0u;

        if (wc->wallClass != RC3D_WALLCLASS_PORTAL) {
            continue;
        }

        if ((unsigned)wc->ownerSector >= (unsigned)g_map->sectorCount ||
            (unsigned)w->neighbour >= (unsigned)g_map->sectorCount)
        {
            continue;
        }

        {
            const RC3D_Sector *sec = &g_map->sectors[wc->ownerSector];
            const RC3D_Sector *nextSec = &g_map->sectors[w->neighbour];
            float openTop = w->openTop;
            float openBottom = w->openBottom;

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

            wc->portalOpenTop = openTop;
            wc->portalOpenBottom = openBottom;
            wc->portalHasUpper =
                (sec->ceilHeight > (openTop + RC3D_EPSILON)) ? 1u : 0u;
            wc->portalHasLower =
                (sec->floorHeight < (openBottom - RC3D_EPSILON)) ? 1u : 0u;
        }
    }
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
    int mapVersion = 0;

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

    if (memcmp(magic, "RC3DMAP3", 8) == 0) {
        mapVersion = 3;
    } else if (memcmp(magic, "RC3DMAP4", 8) == 0) {
        mapVersion = 4;
    } else if (memcmp(magic, "RC3DMAP2", 8) == 0) {
        mapVersion = 2;
    } else if (memcmp(magic, "RC3DMAP1", 8) == 0) {
        mapVersion = 1;
    } else {
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

        if (mapVersion >= 3) {
            if (!readExact(f, &v0, sizeof(v0)) ||
                !readExact(f, &v1, sizeof(v1)) ||
                !readExact(f, &neighbour, sizeof(neighbour)) ||
                !readExact(f, &walls[i].openBottom, sizeof(float)) ||
                !readExact(f, &walls[i].openTop, sizeof(float)) ||
                !readExact(f, &walls[i].upperColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].midColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].lowerColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].flags, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].texture_flags, sizeof(uint32_t)) ||
                !readExact(f, &walls[i].texScaleX, sizeof(float)) ||
                !readExact(f, &walls[i].texScaleY, sizeof(float))) {
                fclose(f);
                free(verts);
                free(walls);
                free(sectors);
                return 0;
            }
        } else {
            if (!readExact(f, &v0, sizeof(v0)) ||
                !readExact(f, &v1, sizeof(v1)) ||
                !readExact(f, &neighbour, sizeof(neighbour)) ||
                !readExact(f, &walls[i].openBottom, sizeof(float)) ||
                !readExact(f, &walls[i].openTop, sizeof(float)) ||
                !readExact(f, &walls[i].upperColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].midColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].lowerColor, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].flags, sizeof(uint8_t)) ||
                !readExact(f, &walls[i].texture_flags, sizeof(uint32_t))) {
                fclose(f);
                free(verts);
                free(walls);
                free(sectors);
                return 0;
            }

            walls[i].texScaleX = 1.0f;
            walls[i].texScaleY = 1.0f;
        }

        walls[i].v0 = (int)v0;
        walls[i].v1 = (int)v1;
        walls[i].neighbour = (int)neighbour;
    }

    for (uint32_t i = 0; i < sectorCount; i++) {
        int32_t wallStart;
        int32_t wallCount_i;
        int32_t boundaryCount;

        if (mapVersion >= 4) {
            if (!readExact(f, &wallStart, sizeof(wallStart)) ||
                !readExact(f, &wallCount_i, sizeof(wallCount_i)) ||
                !readExact(f, &boundaryCount, sizeof(boundaryCount)) ||
                !readExact(f, &sectors[i].floorHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].ceilHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].floorColor, sizeof(uint8_t)) ||
                !readExact(f, &sectors[i].ceilColor, sizeof(uint8_t)) ||
                !readExact(f, &sectors[i].glowlevel, sizeof(uint8_t)) ||
                !readExact(f, &sectors[i].tagId, sizeof(int32_t)) ||
                !readExact(f, &sectors[i].stateFlags, sizeof(uint32_t)) ||
                !readExact(f, &sectors[i].floorMinHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].floorMaxHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].ceilMinHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].ceilMaxHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].floorFlowHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].ceilFlowHeight, sizeof(float)) ||
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
        } else if (mapVersion >= 2) {
            if (!readExact(f, &wallStart, sizeof(wallStart)) ||
                !readExact(f, &wallCount_i, sizeof(wallCount_i)) ||
                !readExact(f, &boundaryCount, sizeof(boundaryCount)) ||
                !readExact(f, &sectors[i].floorHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].ceilHeight, sizeof(float)) ||
                !readExact(f, &sectors[i].floorColor, sizeof(uint8_t)) ||
                !readExact(f, &sectors[i].ceilColor, sizeof(uint8_t)) ||
                !readExact(f, &sectors[i].glowlevel, sizeof(uint8_t)) ||
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

            sectors[i].tagId = 0;
            sectors[i].stateFlags = 0u;
            sectors[i].floorMinHeight = sectors[i].floorHeight;
            sectors[i].floorMaxHeight = sectors[i].floorHeight;
            sectors[i].ceilMinHeight = sectors[i].ceilHeight;
            sectors[i].ceilMaxHeight = sectors[i].ceilHeight;
            sectors[i].floorFlowHeight = 0.0f;
            sectors[i].ceilFlowHeight = 0.0f;
        } else {
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

            sectors[i].glowlevel = 0;
            sectors[i].tagId = 0;
            sectors[i].stateFlags = 0u;
            sectors[i].floorMinHeight = sectors[i].floorHeight;
            sectors[i].floorMaxHeight = sectors[i].floorHeight;
            sectors[i].ceilMinHeight = sectors[i].ceilHeight;
            sectors[i].ceilMaxHeight = sectors[i].ceilHeight;
            sectors[i].floorFlowHeight = 0.0f;
            sectors[i].ceilFlowHeight = 0.0f;
        }

        sectors[i].glowlevel = rc3dClampGlowLevel(sectors[i].glowlevel);
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

static inline int rc3dProjectTopPixel(int horizon, float worldZ, float eyeZ, float scale)
{
    const float y = (float)horizon - ((worldZ - eyeZ) * scale);
    return (int)ceilf(y - RC3D_EPSILON);
}

static inline int rc3dProjectBottomPixel(int horizon, float worldZ, float eyeZ, float scale)
{
    const float y = (float)horizon - ((worldZ - eyeZ) * scale);
    return (int)floorf(y + RC3D_EPSILON);
}

static void rc3dRefreshViewport(void)
{
    if (g_viewport_left < 0) {
        g_viewport_left = 0;
    }
    if (g_viewport_top < 0) {
        g_viewport_top = 0;
    }
    if (g_viewport_left >= SCREEN_W) {
        g_viewport_left = SCREEN_W - 1;
    }
    if (g_viewport_top >= SCREEN_H) {
        g_viewport_top = SCREEN_H - 1;
    }

    if (g_viewport_width < 1) {
        g_viewport_width = 1;
    }
    if (g_viewport_height < 1) {
        g_viewport_height = 1;
    }

    if (g_viewport_width > (SCREEN_W - g_viewport_left)) {
        g_viewport_width = SCREEN_W - g_viewport_left;
    }
    if (g_viewport_height > (SCREEN_H - g_viewport_top)) {
        g_viewport_height = SCREEN_H - g_viewport_top;
    }

    if (g_planeScaleConst > RC3D_EPSILON) {
        g_projPlaneConst = ((float)g_viewport_width * 0.5f) / g_planeScaleConst;
        g_camStepConst =
            (g_viewport_width > 1) ? (2.0f / (float)(g_viewport_width - 1)) : 0.0f;
    } else {
        g_projPlaneConst = 0.0f;
        g_camStepConst = 0.0f;
    }
}


static int pointInSectorFixed(RC3D_Fixed px, RC3D_Fixed py, int sectorIndex)
{
    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_SectorCache *secCache =
        (g_sectorCache && (unsigned)sectorIndex < (unsigned)g_sectorCacheCount)
            ? &g_sectorCache[sectorIndex] : NULL;
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_FixedVec2 *verts = g_fixedVerts;

    int inside = 0;
    const int start = sec->wallStart;
    const int end   = start + sec->boundaryCount;

    if (secCache) {
        if (px < secCache->minX || px > secCache->maxX ||
            py < secCache->minY || py > secCache->maxY)
        {
            return 0;
        }
    }

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

static int rc3dBuildPortalView(const RC3D_Wall *w, int sectorIndex, RC3D_PortalView *outView)
{
    const RC3D_Sector *sec;
    const RC3D_Sector *nextSec;
    ptrdiff_t wallIndex = -1;

    if (!outView) return 0;

    outView->openBottom = 0.0f;
    outView->openTop = 0.0f;
    outView->hasUpper = 0;
    outView->hasLower = 0;

    if (!g_map || !w || !(w->flags & RC3D_WALL_PORTAL)) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if ((unsigned)w->neighbour >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if (g_map->walls) {
        wallIndex = w - g_map->walls;
    }

    if (g_wallCache &&
        wallIndex >= 0 &&
        wallIndex < g_wallCacheCount &&
        g_wallCache[wallIndex].ownerSector == sectorIndex &&
        g_wallCache[wallIndex].wallClass == RC3D_WALLCLASS_PORTAL)
    {
        outView->openBottom = g_wallCache[wallIndex].portalOpenBottom;
        outView->openTop = g_wallCache[wallIndex].portalOpenTop;
        outView->hasUpper = g_wallCache[wallIndex].portalHasUpper;
        outView->hasLower = g_wallCache[wallIndex].portalHasLower;
        return 1;
    }

    sec = &g_map->sectors[sectorIndex];
    nextSec = &g_map->sectors[w->neighbour];

    outView->openBottom = w->openBottom;
    outView->openTop = w->openTop;

    if (sec->ceilHeight < outView->openTop) {
        outView->openTop = sec->ceilHeight;
    }
    if (nextSec->ceilHeight < outView->openTop) {
        outView->openTop = nextSec->ceilHeight;
    }

    if (sec->floorHeight > outView->openBottom) {
        outView->openBottom = sec->floorHeight;
    }
    if (nextSec->floorHeight > outView->openBottom) {
        outView->openBottom = nextSec->floorHeight;
    }

    outView->hasUpper = (sec->ceilHeight > (outView->openTop + RC3D_EPSILON)) ? 1u : 0u;
    outView->hasLower = (sec->floorHeight < (outView->openBottom - RC3D_EPSILON)) ? 1u : 0u;

    return 1;
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
    float openBottom;
    float openTop;
    RC3D_PortalView portalView;

    if (!rc3dBuildPortalView(w, sectorIndex, &portalView)) {
        return 0;
    }

    openBottom = portalView.openBottom;
    openTop = portalView.openTop;

    {
        const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
        const RC3D_Sector *nextSec = &g_map->sectors[w->neighbour];

        if (sec->floorHeight > transitionFeet) {
            transitionFeet = sec->floorHeight;
        }

        if (nextSec->floorHeight > transitionFeet) {
            transitionFeet = nextSec->floorHeight;
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
    uint8_t glowLevel,
    int horizon,
    const RC3D_Sector *sec,
    int isCeiling
){
    if (texId == RC3D_TEXID_SKYBOX) return;
    if (!sec) return;
    if ((unsigned)sx >= (unsigned)g_viewport_width) return;
    if (y0 > y1) return;

    if (y0 < 0) y0 = 0;
    if (y1 >= g_viewport_height) y1 = g_viewport_height - 1;
    if (y0 > y1) return;

    {
        const RC3D_SectorCache *secCache = NULL;
        const float eyeX = g_player.x;
        const float eyeY = g_player.y;
        const float eyeZ = g_renderEyeZ;
        const float planeFactor = (planeZ - eyeZ) * projPlaneGlobal;
        float rayLen =
            sqrtf((rayDirX * rayDirX) + (rayDirY * rayDirY)) * RC3D_LIGHT_PLANE_DIST_SCALE;
        const float *invTable = &g_invDTable[SCREEN_H];
        const uint8_t *texels = g_rc3dTextures[texId].pix;

        uint8_t *dst = rc3dViewportPixelPtr(sx, y0);

        if ((unsigned)sx < (unsigned)g_columnRayCacheCount) {
            rayLen = g_columnRayCache[sx].planeLightLen;
        }

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
                    RC3D_ShadeProfile shadeProfile;
                    const float t = planeFactor * invTable[d];
                    const float sampleDist = fabsf(t) * rayLen;
                    const int tx = (int)((eyeX + (rayDirX * t)) * (float)RC3D_TEX_SIZE);
                    const int ty = (int)((eyeY + (rayDirY * t)) * (float)RC3D_TEX_SIZE);
                    const uint8_t texel =
                        texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

                    rc3dBuildShadeProfile(sampleDist, glowLevel, &shadeProfile);
                    *dst = rc3dApplyShadeProfileToTexel(texel, tx, ty, texId, &shadeProfile);
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
                    RC3D_ShadeProfile shadeProfile;
                    const float t = planeFactor * invTable[d];
                    const float sampleDist = fabsf(t) * rayLen;
                    const float wx = eyeX + (rayDirX * t);
                    const float wy = eyeY + (rayDirY * t);

                    const float rx = ((wx * ca) + (wy * sa)) * invScaleX;
                    const float ry = (-(wx * sa) + (wy * ca)) * invScaleY;
                    const int tx = (int)(rx * (float)RC3D_TEX_SIZE);
                    const int ty = (int)(ry * (float)RC3D_TEX_SIZE);
                    const uint8_t texel =
                        texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

                    rc3dBuildShadeProfile(sampleDist, glowLevel, &shadeProfile);
                    *dst = rc3dApplyShadeProfileToTexel(texel, tx, ty, texId, &shadeProfile);
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
        const uint8_t wallGlow = rc3dWallGlowFromFlags(texFlags);

        if (worldSpan <= RC3D_EPSILON) return;

        {
            const float k = hitDist / projPlane;
            const float texPerWorld = (float)RC3D_TEX_SIZE * wallTexInvScaleYGlobal;
            const float baseWorldFromTop =
                vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            if (stretchY) {
                const float scale = texPerWorld / worldSpan;
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * scale;
                vStep   = k * scale;
            } else if (clampYB && !clampYT) {
                vLocal0 =
                    (((baseWorldFromTop + ((float)y0 * k)) - worldSpan) * texPerWorld) +
                    (float)(RC3D_TEX_SIZE - 1);
                vStep   = k * texPerWorld;
            } else {
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * texPerWorld;
                vStep   = k * texPerWorld;
            }

            {
                const uint8_t *texels = g_rc3dTextures[texId].pix;
                uint8_t *dst = rc3dViewportPixelPtr(sx, y0);
                const float shadeDist = hitDist * RC3D_LIGHT_WALL_DIST_SCALE;
                RC3D_ShadeProfile shadeProfile;

                rc3dBuildShadeProfile(shadeDist, wallGlow, &shadeProfile);

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
                        const int ty = (vFixed >> 16);
                        const uint8_t texel =
                            texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

                        *dst = rc3dApplyShadeProfileToTexel(texel, tx, ty, texId, &shadeProfile);
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
                        const int tx = (uRot >> 16);
                        const int ty = (vRot >> 16);
                        const uint8_t texel =
                            texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

                        *dst = rc3dApplyShadeProfileToTexel(texel, tx, ty, texId, &shadeProfile);
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
        const uint8_t wallGlow = rc3dWallGlowFromFlags(texFlags);

        if (worldSpan <= RC3D_EPSILON) return;

        {
            const float k = hitDist / projPlane;
            const float texPerWorld = (float)RC3D_TEX_SIZE * wallTexInvScaleYGlobal;
            const float baseWorldFromTop =
                vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            if (stretchY) {
                const float scale = texPerWorld / worldSpan;
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * scale;
                vStep   = k * scale;
            } else if (clampYB && !clampYT) {
                vLocal0 =
                    (((baseWorldFromTop + ((float)y0 * k)) - worldSpan) * texPerWorld) +
                    (float)(RC3D_TEX_SIZE - 1);
                vStep   = k * texPerWorld;
            } else {
                vLocal0 = (baseWorldFromTop + ((float)y0 * k)) * texPerWorld;
                vStep   = k * texPerWorld;
            }

            {
                const uint8_t *texels = g_rc3dTextures[texId].pix;
                uint8_t *dst = rc3dViewportPixelPtr(sx, y0);
                int opaqueSpanStart = -1;
                const float shadeDist = hitDist * RC3D_LIGHT_WALL_DIST_SCALE;
                RC3D_ShadeProfile shadeProfile;

                rc3dBuildShadeProfile(shadeDist, wallGlow, &shadeProfile);

                if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f) &&
                    (wallTexRotCosGlobal >  0.9999f)  && (wallTexRotCosGlobal < 1.0001f))
                {
                    const int32_t uFixed = (int32_t)(wallTexUBaseGlobal * 65536.0f);
                    int32_t vFixed = (int32_t)(vLocal0 * 65536.0f);
                    const int32_t vStepFixed = (int32_t)(vStep * 65536.0f);

                    const int tx = (uFixed >> 16);

                    for (int y = y0; y <= y1; ++y) {
                        const int ty = (vFixed >> 16);
                        const uint8_t texel =
                            rc3dApplyShadeProfileToTexel(
                                texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)],
                                tx,
                                ty,
                                texId,
                                &shadeProfile);

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
                        const int tx = (uRot >> 16);
                        const int ty = (vRot >> 16);
                        const uint8_t texel =
                            rc3dApplyShadeProfileToTexel(
                                texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)],
                                tx,
                                ty,
                                texId,
                                &shadeProfile);

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



static inline void rc3dTryNearestWallHit(
    RC3D_WallHit *hit,
    int wallIndex,
    float rox, float roy,
    float rdx, float rdy,
    float minT,
    const RC3D_Wall *walls,
    const RC3D_Vec2 *verts,
    const RC3D_WallCache *cache
){
    const RC3D_Wall *w = &walls[wallIndex];
    const RC3D_Vec2 *a = &verts[w->v0];
    float sx;
    float sy;

    if (cache) {
        const RC3D_WallCache *wc = &cache[wallIndex];
        sx = wc->dx;
        sy = wc->dy;
    } else {
        const RC3D_Vec2 *b = &verts[w->v1];
        sx = b->x - a->x;
        sy = b->y - a->y;
    }

    {
        const float denom = (rdx * sy) - (rdy * sx);

        if (denom >= -RC3D_EPSILON) {
            return;
        }

        {
            const float qpx = a->x - rox;
            const float qpy = a->y - roy;
            const float invDenom = 1.0f / denom;
            const float t = ((qpx * sy) - (qpy * sx)) * invDenom;

            if (t < minT || t >= hit->t) {
                return;
            }

            {
                const float u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
                if (u < 0.0f || u > 1.0f) {
                    return;
                }
            }

            hit->t = t;
            hit->wallIndex = wallIndex;
            hit->hit = 1;
        }
    }
}

static inline RC3D_WallHit findNearestWallInSector(
    int sectorIndex,
    float rox, float roy,
    float rdx, float rdy,
    int ignoreWallIndexA,
    int ignoreWallIndexB,
    float minT,
    int preferredWallIndex
){
    RC3D_WallHit hit;
    hit.t = g_draw_distance;
    hit.wallIndex = -1;
    hit.hit = 0;

    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_Vec2 *verts = g_map->verts;
    const RC3D_WallCache *cache = g_wallCache;

    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;
    const int usePreferredWall =
        (preferredWallIndex >= start) &&
        (preferredWallIndex < end) &&
        (preferredWallIndex != ignoreWallIndexA) &&
        (preferredWallIndex != ignoreWallIndexB);

    if (usePreferredWall) {
#if RC3D_DRAW_PROFILER
        g_profiler.wallTests++;
#endif
        rc3dTryNearestWallHit(
            &hit,
            preferredWallIndex,
            rox, roy,
            rdx, rdy,
            minT,
            walls,
            verts,
            cache
        );
    }

    for (int wallIndex = start; wallIndex < end; ++wallIndex) {
        if (wallIndex == preferredWallIndex) {
            continue;
        }

#if RC3D_DRAW_PROFILER
        g_profiler.wallTests++;
#endif
        if (wallIndex == ignoreWallIndexA || wallIndex == ignoreWallIndexB) {
            continue;
        }

        rc3dTryNearestWallHit(
            &hit,
            wallIndex,
            rox, roy,
            rdx, rdy,
            minT,
            walls,
            verts,
            cache
        );
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
                sec->glowlevel,
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
                sec->glowlevel,
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
    const int viewportHalfHeight = g_viewport_height / 2;

    if (g_viewport_width <= 0 || viewportHalfHeight <= 0) {
        return;
    }

    const float uStepRaw =
        (((g_halfFovRad * 2.0f) / (float)g_viewport_width) / fullTurn) * skyRepeat;

    float uStart = (leftAng / fullTurn) * skyRepeat;
    uStart -= floorf(uStart);

    {
        int32_t uFixed = (int32_t)(uStart * 65536.0f);
        const int32_t uStepFixed = (int32_t)(uStepRaw * 65536.0f);

        for (int x = 0; x < g_viewport_width; ++x) {
            g_skyXTable[x] = (uint16_t)(((uint32_t)uFixed * (uint32_t)RC3D_SKYBOX_W) >> 16);
            uFixed += uStepFixed;

            if (uFixed >= oneFixed) {
                uFixed -= oneFixed;
            } else if (uFixed < 0) {
                uFixed += oneFixed;
            }
        }
    }

    for (int y = 0; y < viewportHalfHeight; ++y) {
        const int ty = (y * RC3D_SKYBOX_H) / viewportHalfHeight;
        const uint8_t *srcRow = &tex_skybox[ty * RC3D_SKYBOX_W];
        uint8_t *dst = &fb[(rc3dViewportScreenY(y) * SCREEN_W) + g_viewport_left];

        for (int x = 0; x < g_viewport_width; ++x) {
            dst[x] = srcRow[g_skyXTable[x]];
        }
    }
}



static inline void renderColumnPortalTraceClipped(
    int sx,
    float rdx,
    float rdy,
    float projPlane,
    int horizon,
    int currentSector,
    int clipTop,
    int clipBottom,
    int ignoreWallIndexA,
    int ignoreWallIndexB,
    float rayMinT,
    int maskedDepth,
    int preferredWallIndex,
    int *outFirstHitWallIndex
){
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_renderEyeZ;

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) {
#if RC3D_DRAW_PROFILER
        g_profiler.portalSteps++;
#endif
        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return;
        }

        if (clipTop > clipBottom) {
            return;
        }

        {
            const RC3D_Sector *sec = &g_map->sectors[currentSector];
            const int stepPreferredWallIndex = (step == 0) ? preferredWallIndex : -1;

            const RC3D_WallHit hit = findNearestWallInSector(
                currentSector,
                playerX, playerY,
                rdx, rdy,
                ignoreWallIndexA,
                ignoreWallIndexB,
                rayMinT,
                stepPreferredWallIndex
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

            if (outFirstHitWallIndex && step == 0) {
                *outFirstHitWallIndex = hit.wallIndex;
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

                const float correctedDist = hit.t;

                if (correctedDist <= RC3D_EPSILON) {
                    return;
                }

                {
                    const float wallDx = wc ? wc->dx : (vb->x - va->x);
                    const float wallDy = wc ? wc->dy : (vb->y - va->y);
                    const float wallTexInvScaleX =
                        wc ? wc->texInvScaleX :
                        (1.0f / ((fabsf(w->texScaleX) < RC3D_EPSILON) ? 1.0f : w->texScaleX));
                    const float wallTexInvScaleY =
                        wc ? wc->texInvScaleY :
                        (1.0f / ((fabsf(w->texScaleY) < RC3D_EPSILON) ? 1.0f : w->texScaleY));

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
                        const float texAngle = rc3dWallTexAngleFromFlags(w->texture_flags);
                        if (fabsf(texAngle) > 0.0001f) {
                            wallTexRotCosGlobal = cosf(texAngle);
                            wallTexRotSinGlobal = sinf(texAngle);
                        } else {
                            wallTexRotCosGlobal = 1.0f;
                            wallTexRotSinGlobal = 0.0f;
                        }
                    }

                    wallTexInvScaleYGlobal = wallTexInvScaleY;

                    if (wc && wc->texXMode == RC3D_TEX_XMODE_STRETCH) {
                        if (uNorm < 0.0f) uNorm = 0.0f;
                        if (uNorm > 1.0f) uNorm = 1.0f;
                        wallTexUBaseGlobal = uNorm * (float)RC3D_TEX_SIZE;
                    } else if (wc && wc->texXMode == RC3D_TEX_XMODE_CLAMP_RIGHT) {
                        wallTexUBaseGlobal =
                            (distAlongWall * wallTexInvScaleX * (float)RC3D_TEX_SIZE) -
                            (wallLen * wallTexInvScaleX * (float)RC3D_TEX_SIZE);
                    } else {
                        wallTexUBaseGlobal = distAlongWall * wallTexInvScaleX * (float)RC3D_TEX_SIZE;
                    }
                }

                {
                    const float scale = projPlane / correctedDist;

                    const int secTop =
                        rc3dProjectTopPixel(horizon, sec->ceilHeight, playerZ, scale);
                    const int secBot =
                        rc3dProjectBottomPixel(horizon, sec->floorHeight, playerZ, scale);

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
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
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
                                            projPlane,
                                            horizon,
                                            w->neighbour,
                                            maskedClipTop,
                                            maskedClipBottom,
                                            hit.wallIndex,
                                            entryWallInNext,
                                            hit.t + RC3D_EPSILON,
                                            maskedDepth + 1,
                                            -1,
                                            NULL
                                        );
                                    } else {
                                        renderColumnPortalTraceClipped(
                                            sx,
                                            rdx,
                                            rdy,
                                            projPlane,
                                            horizon,
                                            currentSector,
                                            maskedClipTop,
                                            maskedClipBottom,
                                            hit.wallIndex,
                                            -1,
                                            hit.t + RC3D_EPSILON,
                                            maskedDepth + 1,
                                            -1,
                                            NULL
                                        );
                                    }
                                }

                                /* restore THIS wall's UV basis after recursive trace */
                                wallTexUBaseGlobal  = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;

                                renderMaskedTexturedBandIfVisible(
                                    sx, secTop, secBot,
                                    w->midColor,
                                    w->texture_flags,
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
                                    w->texture_flags,
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
                            const int midTopY =
                                rc3dProjectTopPixel(horizon, w->openTop, playerZ, scale);
                            const int midBotY =
                                rc3dProjectBottomPixel(horizon, w->openBottom, playerZ, scale);

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
                                        projPlane,
                                        horizon,
                                        currentSector,
                                        maskedClipTop,
                                        maskedClipBottom,
                                        hit.wallIndex,
                                        -1,
                                        hit.t + RC3D_EPSILON,
                                        maskedDepth + 1,
                                        -1,
                                        NULL
                                    );
                                }

                                renderMaskedTexturedBandIfVisible(
                                    sx, midTopY, midBotY,
                                    w->midColor,
                                    w->texture_flags,
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
                                    w->texture_flags,
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
                            const int openTopY =
                                rc3dProjectTopPixel(horizon, w->openTop, playerZ, scale);
                            const int openBotY =
                                rc3dProjectBottomPixel(horizon, w->openBottom, playerZ, scale);

                            fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);

                            if (w->flags & RC3D_WALL_UPPER) {
                                renderTexturedBandIfVisible(
                                    sx, secTop, openTopY - 1,
                                    w->upperColor,
                                    w->texture_flags,
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
                                    w->texture_flags,
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
                            RC3D_PortalView portalView;
                            int portalOpenTopY;
                            int portalOpenBotY;
                            int portalClipTop;
                            int portalClipBottom;

                            if ((unsigned)nextSectorIndex >= (unsigned)g_map->sectorCount) {
                                return;
                            }

                            if (!rc3dBuildPortalView(w, currentSector, &portalView)) {
                                return;
                            }

                            portalOpenTopY =
                                rc3dProjectTopPixel(horizon, portalView.openTop, playerZ, scale);
                            portalOpenBotY =
                                rc3dProjectBottomPixel(
                                    horizon, portalView.openBottom, playerZ, scale);

                            int openTop = secTop;
                            int openBot = secBot;

                            if (portalOpenTopY > openTop) openTop = portalOpenTopY;
                            if (clipTop > openTop)       openTop = clipTop;

                            if (portalOpenBotY < openBot) openBot = portalOpenBotY;
                            if (clipBottom < openBot)    openBot = clipBottom;

                            fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                            fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);

                            portalClipTop = openTop + (portalView.hasUpper ? 1 : 0);
                            portalClipBottom = openBot - (portalView.hasLower ? 1 : 0);

                            {
                                int entryWallInNext = -1;
                                const float savedWallTexUBase = wallTexUBaseGlobal;
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
                                const float savedWallTexRotCos = wallTexRotCosGlobal;
                                const float savedWallTexRotSin = wallTexRotSinGlobal;

                                if (wc) {
                                    entryWallInNext = wc->backWallIndex;
                                }

                                if (portalClipTop <= portalClipBottom) {
                                    renderColumnPortalTraceClipped(
                                        sx,
                                        rdx,
                                        rdy,
                                        projPlane,
                                        horizon,
                                        nextSectorIndex,
                                        portalClipTop,
                                        portalClipBottom,
                                        hit.wallIndex,
                                        entryWallInNext,
                                        hit.t,
                                        maskedDepth,
                                        -1,
                                        NULL
                                    );
                                }

                                wallTexUBaseGlobal = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;
                            }

                            if (portalView.hasUpper) {
                                renderTexturedBandIfVisible(
                                    sx, secTop, openTop,
                                    w->upperColor,
                                    w->texture_flags,
                                    sec->ceilHeight,
                                    portalView.openTop,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            if (portalView.hasLower) {
                                renderTexturedBandIfVisible(
                                    sx, openBot, secBot,
                                    w->lowerColor,
                                    w->texture_flags,
                                    portalView.openBottom,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom
                                );
                            }

                            return;
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
    float projPlane,
    int horizon,
    int preferredWallIndex,
    int *outFirstHitWallIndex
){
    renderColumnPortalTraceClipped(
        sx,
        rdx,
        rdy,
        projPlane,
        horizon,
        g_player.sector,
        0,
        g_viewport_height - 1,
        -1,
        -1,
        0.0f,
        0,
        preferredWallIndex,
        outFirstHitWallIndex
    );
}

static inline void rc3dRecordVisibleTraceSegment(
    int sx,
    int sector,
    int clipTop,
    int clipBottom,
    uint16_t depthLimit
){
    uint8_t count;
    RC3D_VisibleTraceSegment *segment;

    if ((unsigned)sx >= (unsigned)SCREEN_W) return;
    if (clipTop > clipBottom) return;

    count = g_visibleTraceCount[sx];
    if (count >= RC3D_MAX_PORTAL_STEPS) {
        return;
    }

    segment = &g_visibleTraceCache[sx][count];
    segment->sector = (int16_t)sector;
    segment->clipTop = (int16_t)clipTop;
    segment->clipBottom = (int16_t)clipBottom;
    segment->depthLimit = depthLimit;
    g_visibleTraceCount[sx] = (uint8_t)(count + 1);
}

static void rc3dBuildVisibleTraceCacheForColumn(int sx)
{
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_renderEyeZ;
    float rdx;
    float rdy;

    int currentSector = g_player.sector;
    int clipTop = 0;
    int clipBottom = g_viewport_height - 1;
    int ignoreWallIndexA = -1;
    int ignoreWallIndexB = -1;
    float rayMinT = 0.0f;

    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return;
    }

    if (g_visibleTraceBuilt[sx]) {
        return;
    }

    g_visibleTraceBuilt[sx] = 1u;
    g_visibleTraceCount[sx] = 0u;

    if ((unsigned)sx < (unsigned)g_columnRayCacheCount) {
        rdx = g_columnRayCache[sx].rdx;
        rdy = g_columnRayCache[sx].rdy;
    } else {
        float dirX = 1.0f;
        float dirY = 0.0f;
        const float cameraX = rc3dViewportCameraX(sx);
        float planeX;
        float planeY;

        rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);
        planeX = -dirY * g_planeScaleConst;
        planeY =  dirX * g_planeScaleConst;
        rdx = dirX + (planeX * cameraX);
        rdy = dirY + (planeY * cameraX);
    }

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) {
#if RC3D_DRAW_PROFILER
        g_profiler.portalSteps++;
#endif
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
            return;
        }

        if (clipTop > clipBottom) {
            return;
        }

        sec = &g_map->sectors[currentSector];

        hit = findNearestWallInSector(
            currentSector,
            playerX, playerY,
            rdx, rdy,
            ignoreWallIndexA,
            ignoreWallIndexB,
            rayMinT,
            -1
        );

        if (!hit.hit) {
            rc3dRecordVisibleTraceSegment(sx, currentSector, clipTop, clipBottom, UINT16_MAX);
            return;
        }

        correctedDist = hit.t;
        if (correctedDist <= RC3D_EPSILON) {
            return;
        }

        rc3dRecordVisibleTraceSegment(
            sx,
            currentSector,
            clipTop,
            clipBottom,
            rc3dEncodeDepth(correctedDist)
        );

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

        scale = projPlaneGlobal / correctedDist;
        secTop = rc3dProjectTopPixel(horizonGlobal, sec->ceilHeight, playerZ, scale);
        secBot = rc3dProjectBottomPixel(horizonGlobal, sec->floorHeight, playerZ, scale);

        wallMasked = ((w->flags & RC3D_WALL_TRANSPARENCY) != 0);
        flags = w->flags;

        if (wallClass == RC3D_WALLCLASS_PORTAL) {
            const int nextSectorIndex = w->neighbour;
            RC3D_PortalView portalView;
            int portalOpenTopY, portalOpenBotY;
            int openTop, openBot;
            int portalClipTop, portalClipBottom;
            int entryWallInNext = -1;

            if ((unsigned)nextSectorIndex >= (unsigned)g_map->sectorCount) {
                return;
            }

            if (!rc3dBuildPortalView(w, currentSector, &portalView)) {
                return;
            }

            portalOpenTopY =
                rc3dProjectTopPixel(horizonGlobal, portalView.openTop, playerZ, scale);
            portalOpenBotY =
                rc3dProjectBottomPixel(horizonGlobal, portalView.openBottom, playerZ, scale);

            openTop = secTop;
            openBot = secBot;

            if (portalOpenTopY > openTop) openTop = portalOpenTopY;
            if (clipTop > openTop)       openTop = clipTop;

            if (portalOpenBotY < openBot) openBot = portalOpenBotY;
            if (clipBottom < openBot)    openBot = clipBottom;

            portalClipTop = openTop + (portalView.hasUpper ? 1 : 0);
            portalClipBottom = openBot - (portalView.hasLower ? 1 : 0);

            if (portalClipTop > portalClipBottom) {
                return;
            }

            if (wc) {
                entryWallInNext = wc->backWallIndex;
            }

            currentSector = nextSectorIndex;
            clipTop = portalClipTop;
            clipBottom = portalClipBottom;
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
                if (clipTop > clipBottom) return;

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
                if (clipTop > clipBottom) return;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t + RC3D_EPSILON;
                continue;
            }

            if (wallClass == RC3D_WALLCLASS_MIDDLE) {
                const int midTopY =
                    rc3dProjectTopPixel(horizonGlobal, w->openTop, playerZ, scale);
                const int midBotY =
                    rc3dProjectBottomPixel(horizonGlobal, w->openBottom, playerZ, scale);

                if (midTopY > clipTop) clipTop = midTopY;
                if (midBotY < clipBottom) clipBottom = midBotY;
                if (clipTop > clipBottom) return;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t + RC3D_EPSILON;
                continue;
            }
        }

        return;
    }
}






static void renderCurrentSectorColumns(void)
{
    const float projPlane = g_projPlaneConst;
    const int horizon     = g_viewport_height / 2;

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

    rc3dBuildColumnRayCache(dirX, dirY, planeX, planeY);
    rc3dInvalidateVisibleTraceCache();

    {
        int preferredWallIndex = -1;

#if RC3D_DRAW_PROFILER
        g_profiler.rays = (g_viewport_width > 0) ? (uint32_t)g_viewport_width : 0u;
#endif

        for (int sx = 0; sx < g_viewport_width; ++sx) {
            const RC3D_ColumnRayCache *rayCache = &g_columnRayCache[sx];
            int firstHitWallIndex = -1;

            renderColumnPortalTrace(
                sx,
                rayCache->rdx,
                rayCache->rdy,
                projPlane,
                horizon,
                preferredWallIndex,
                &firstHitWallIndex
            );

            preferredWallIndex = firstHitWallIndex;
        }
    }
}

static int rc3dTraceVisibleSectorAtDepth(
    int sx,
    float targetDepth,
    int *outSector,
    int *outClipTop,
    int *outClipBottom
){
#if RC3D_DRAW_PROFILER
    g_profiler.spriteSectorTraces++;
#endif
    const uint16_t targetDepthCode = rc3dEncodeDepth(targetDepth);

    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return 0;
    }

    rc3dBuildVisibleTraceCacheForColumn(sx);

    for (uint8_t i = 0; i < g_visibleTraceCount[sx]; ++i) {
        const RC3D_VisibleTraceSegment *segment = &g_visibleTraceCache[sx][i];

        if (targetDepthCode > segment->depthLimit) {
            continue;
        }

        if (outSector) *outSector = (int)segment->sector;
        if (outClipTop) *outClipTop = (int)segment->clipTop;
        if (outClipBottom) *outClipBottom = (int)segment->clipBottom;
        return 1;
    }

    return 0;
}





static void rc3dRefreshSpritePlacement(RC3D_Sprite *sprite)
{
    int sector;

    if (!sprite || !sprite->inUse) return;

    sector = findSectorForSpritePosition(sprite->x, sprite->y, -1);
    if ((unsigned)sector >= (unsigned)g_map->sectorCount) {
        return;
    }

    sprite->baseZ = g_map->sectors[sector].floorHeight;
}




static void renderBillboardSprite(const RC3D_Sprite *sprite)
{
    const float screenCenterX = (float)g_viewport_width * 0.5f;
    const float eyeZ = g_renderEyeZ;
    const uint8_t *texels;

    float dirX = 1.0f;
    float dirY = 0.0f;
    float rightVecX;
    float rightVecY;

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

    if (!sprite || !sprite->inUse || !sprite->active) return;

    texels = g_rc3dTextures[sprite->texId].pix;
    rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

    rightVecX = -dirY;
    rightVecY =  dirX;

    toSpriteX = sprite->x - g_player.x;
    toSpriteY = sprite->y - g_player.y;

    camX     = (toSpriteX * -dirY) + (toSpriteY * dirX);
    camDepth = (toSpriteX *  dirX) + (toSpriteY * dirY);

    if (camDepth <= RC3D_EPSILON || camDepth >= g_draw_distance) {
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

    if (rightX < 0 || leftX >= g_viewport_width || bottomY < 0 || topY >= g_viewport_height) {
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
        const int drawRight =
            (rightX >= g_viewport_width) ? (g_viewport_width - 1) : rightX;

        int preferredSector = g_player.sector;

        for (int sx = drawLeft; sx <= drawRight; ++sx) {
#if RC3D_DRAW_PROFILER
            g_profiler.spriteColumns++;
#endif
            RC3D_ShadeProfile shadeProfile;
            const int tx = ((sx - leftX) * RC3D_TEX_SIZE) / unclampedWidth;
            uint8_t sectorGlow = 0u;

            int colTop    = (topY < 0) ? 0 : topY;
            int colBottom =
                (bottomY >= g_viewport_height) ? (g_viewport_height - 1) : bottomY;

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

                    sectorGlow = sec->glowlevel;

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
                int visClipBottom = g_viewport_height - 1;

                if (!rc3dTraceVisibleSectorAtDepth(
                        sx,
                        camDepth,
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

                    if (sectorGlow == 0u) {
                        sectorGlow = visSec->glowlevel;
                    }

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

            rc3dBuildShadeProfile(
                camDepth * RC3D_LIGHT_SPRITE_DIST_SCALE,
                sectorGlow,
                &shadeProfile);

            for (int sy = colTop; sy <= colBottom; ++sy) {
                const int ty = ((sy - topY) * RC3D_TEX_SIZE) / unclampedHeight;
                const uint8_t texel = texels[(ty * RC3D_TEX_SIZE) + tx];
                const uint8_t litTexel =
                    rc3dApplyShadeProfileToTexel(
                        texel,
                        tx,
                        ty,
                        sprite->texId,
                        &shadeProfile);

                if (litTexel == RC3D_SPRITE_TEX_TRANSPARENT) {
                    continue;
                }

                if (rc3dWallSpansBlockPixel(sx, sy, spriteDepth)) {
                    continue;
                }

                fb[(rc3dViewportScreenY(sy) * SCREEN_W) + rc3dViewportScreenX(sx)] = litTexel;
            }
        }
    }
}

static void renderSprites(void)
{
    float dirX = 1.0f;
    float dirY = 0.0f;
    int orderCount = 0;

    rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

    for (int i = 0; i < RC3D_MAX_SPRITES; ++i) {
        const RC3D_Sprite *sprite = &g_sprites[i];

        if (!sprite->inUse || !sprite->active) {
            continue;
        }

        {
            const float toSpriteX = sprite->x - g_player.x;
            const float toSpriteY = sprite->y - g_player.y;
            const float camDepth = (toSpriteX * dirX) + (toSpriteY * dirY);
            int insertAt = orderCount;

            if (camDepth <= RC3D_EPSILON || camDepth >= g_draw_distance) {
                continue;
            }

            while (insertAt > 0) {
                if (g_spriteOrder[insertAt - 1].camDepth >= camDepth) {
                    break;
                }

                g_spriteOrder[insertAt] = g_spriteOrder[insertAt - 1];
                insertAt--;
            }

            g_spriteOrder[insertAt].spriteId = i;
            g_spriteOrder[insertAt].camDepth = camDepth;
            orderCount++;
        }
    }

    for (int i = 0; i < orderCount; ++i) {
        renderBillboardSprite(&g_sprites[g_spriteOrder[i].spriteId]);
    }
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
    g_demoSpriteA = rc3dSpriteCreate(
        g_map->startX,
        g_map->startY,
        RC3D_TEST_SPRITE_WIDTH,
        RC3D_TEST_SPRITE_HEIGHT,
        RC3D_TEXID_SPRITE_MAN);

    g_demoSpriteB = rc3dSpriteCreate(
        g_map->startX,
        g_map->startY,
        RC3D_TEST_SPRITE_WIDTH,
        RC3D_TEST_SPRITE_HEIGHT,
        RC3D_TEXID_SPRITE_MAN);
}

static uint32_t rc3dSanitizeSectorStateFlags(uint32_t stateFlags)
{
    stateFlags &= (RC3D_SECTOR_STATE_RAISE_FLOOR |
                   RC3D_SECTOR_STATE_LOWER_FLOOR |
                   RC3D_SECTOR_STATE_LOWER_CEILING |
                   RC3D_SECTOR_STATE_RAISE_CEILING);

    if ((stateFlags & (RC3D_SECTOR_STATE_RAISE_FLOOR | RC3D_SECTOR_STATE_LOWER_FLOOR)) ==
        (RC3D_SECTOR_STATE_RAISE_FLOOR | RC3D_SECTOR_STATE_LOWER_FLOOR))
    {
        stateFlags &= ~(RC3D_SECTOR_STATE_RAISE_FLOOR | RC3D_SECTOR_STATE_LOWER_FLOOR);
    }

    if ((stateFlags & (RC3D_SECTOR_STATE_LOWER_CEILING | RC3D_SECTOR_STATE_RAISE_CEILING)) ==
        (RC3D_SECTOR_STATE_LOWER_CEILING | RC3D_SECTOR_STATE_RAISE_CEILING))
    {
        stateFlags &= ~(RC3D_SECTOR_STATE_LOWER_CEILING | RC3D_SECTOR_STATE_RAISE_CEILING);
    }

    return stateFlags;
}

static float rc3dMoveTowardsFloat(float current, float target, float maxDelta)
{
    if (maxDelta <= 0.0f) {
        return current;
    }

    if (current < target) {
        current += maxDelta;
        if (current > target) current = target;
    } else if (current > target) {
        current -= maxDelta;
        if (current < target) current = target;
    }

    return current;
}

static RC3D_Sector *rc3dMutableSectorData(void)
{
    if (!g_map || !g_map->sectors || g_map->sectorCount <= 0) {
        return NULL;
    }

    return (RC3D_Sector *)g_map->sectors;
}

static void rc3dClampPlayerIntoCurrentSector(void)
{
    if (!g_map) return;

    if ((unsigned)g_player.sector < (unsigned)g_map->sectorCount) {
        const RC3D_Sector *sec = &g_map->sectors[g_player.sector];
        const float minEyeZ = sec->floorHeight + RC3D_PLAYER_EYE_HEIGHT;
        const float maxEyeZ = sec->ceilHeight - (PLAYER_HEIGHT - RC3D_PLAYER_EYE_HEIGHT);

        if (g_player.z < minEyeZ) {
            g_player.z = minEyeZ;
            g_player.vz = 0.0f;
        }

        if (g_player.z > maxEyeZ) {
            g_player.z = maxEyeZ;
            g_player.vz = 0.0f;
        }
    } else if (g_player.z < RC3D_PLAYER_EYE_HEIGHT) {
        g_player.z = RC3D_PLAYER_EYE_HEIGHT;
        g_player.vz = 0.0f;
    }
}

static void rc3dUpdateSectorMotion(float dt)
{
    RC3D_Sector *sectors = rc3dMutableSectorData();
    int anySectorMoved = 0;

    if (!g_map || !sectors || dt <= 0.0f) {
        return;
    }

    for (int i = 0; i < g_map->sectorCount; ++i) {
        RC3D_Sector *sec = &sectors[i];
        uint32_t stateFlags = rc3dSanitizeSectorStateFlags(sec->stateFlags);
        const float oldFloorHeight = sec->floorHeight;
        const float oldCeilHeight = sec->ceilHeight;

        sec->stateFlags = stateFlags;

        if (stateFlags & RC3D_SECTOR_STATE_RAISE_FLOOR) {
            const float speed = fabsf(sec->floorFlowHeight);
            float target = sec->floorMaxHeight;
            const float maxFloor = sec->ceilHeight;

            if (target > maxFloor) target = maxFloor;
            if (target > (sec->floorHeight + RC3D_EPSILON)) {
                sec->floorHeight = rc3dMoveTowardsFloat(sec->floorHeight, target, speed * dt);
            }

            if (sec->floorHeight >= (target - RC3D_EPSILON)) {
                sec->floorHeight = target;
                stateFlags &= ~RC3D_SECTOR_STATE_RAISE_FLOOR;
            }

        } else if (stateFlags & RC3D_SECTOR_STATE_LOWER_FLOOR) {
            const float speed = fabsf(sec->floorFlowHeight);
            float target = sec->floorMinHeight;
            const float maxFloor = sec->ceilHeight;

            if (target > maxFloor) target = maxFloor;
            if (target < (sec->floorHeight - RC3D_EPSILON)) {
                sec->floorHeight = rc3dMoveTowardsFloat(sec->floorHeight, target, speed * dt);
            }

            if (sec->floorHeight <= (target + RC3D_EPSILON)) {
                sec->floorHeight = target;
                stateFlags &= ~RC3D_SECTOR_STATE_LOWER_FLOOR;
            }
        }

        if (stateFlags & RC3D_SECTOR_STATE_LOWER_CEILING) {
            const float speed = fabsf(sec->ceilFlowHeight);
            float target = sec->ceilMinHeight;
            const float minCeiling = sec->floorHeight;

            if (target < minCeiling) target = minCeiling;
            if (target < (sec->ceilHeight - RC3D_EPSILON)) {
                sec->ceilHeight = rc3dMoveTowardsFloat(sec->ceilHeight, target, speed * dt);
            }

            if (sec->ceilHeight <= (target + RC3D_EPSILON)) {
                sec->ceilHeight = target;
                stateFlags &= ~RC3D_SECTOR_STATE_LOWER_CEILING;
            }
        } else if (stateFlags & RC3D_SECTOR_STATE_RAISE_CEILING) {
            const float speed = fabsf(sec->ceilFlowHeight);
            float target = sec->ceilMaxHeight;
            const float minCeiling = sec->floorHeight;

            if (target < minCeiling) target = minCeiling;
            if (target > (sec->ceilHeight + RC3D_EPSILON)) {
                sec->ceilHeight = rc3dMoveTowardsFloat(sec->ceilHeight, target, speed * dt);
            }

            if (sec->ceilHeight >= (target - RC3D_EPSILON)) {
                sec->ceilHeight = target;
                stateFlags &= ~RC3D_SECTOR_STATE_RAISE_CEILING;
            }
        }

        sec->stateFlags = stateFlags;

        if (fabsf(sec->floorHeight - oldFloorHeight) > RC3D_EPSILON ||
            fabsf(sec->ceilHeight - oldCeilHeight) > RC3D_EPSILON)
        {
            anySectorMoved = 1;
        }

        if (i == g_player.sector) {
            const float playerFeet = g_player.z - RC3D_PLAYER_EYE_HEIGHT;

            if (playerFeet <= (oldFloorHeight + RC3D_EPSILON)) {
                g_player.z += (sec->floorHeight - oldFloorHeight);
            }

            if (sec->ceilHeight < oldCeilHeight) {
                rc3dClampPlayerIntoCurrentSector();
            }
        }
    }

    if (anySectorMoved) {
        rc3dRefreshDynamicPortalCache();
    }

    rc3dClampPlayerIntoCurrentSector();
}

void moveSprite(){
    if (rc3dSpriteHandleValid(g_demoSpriteB)) {
        rc3dSpriteSetPosition(g_demoSpriteB, g_player.x, g_player.y);
    }
}

/* ------------------------------------------------------------------------- */
/* public api                                                                */
/* ------------------------------------------------------------------------- */

void rc3dSetViewport(int left, int top, int width, int height)
{
    g_viewport_left = left;
    g_viewport_top = top;
    g_viewport_width = width;
    g_viewport_height = height;
    rc3dRefreshViewport();
}

void rc3dResetViewport(void)
{
    g_viewport_left = RC3D_VIEWPORT_LEFT;
    g_viewport_top = RC3D_VIEWPORT_TOP;
    g_viewport_width = RC3D_VIEWPORT_WIDTH;
    g_viewport_height = RC3D_VIEWPORT_HEIGHT;
    rc3dRefreshViewport();
}

void rc3dInit(void)
{
    if (!g_rc3dTexturesInit) rc3dBuildDefaultTextures();
    if (!g_invDTableInit)    rc3dBuildInvDTable();
    if (!g_lightVariantTablesInit) rc3dBuildLightVariantTables();
    rc3dBuildTrigTables();
    rc3dRefreshViewport();

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
    rc3dClearSprites();
    rc3dInitTestSprite();
}

int rc3dSetSectorStateByTag(int32_t tagId, uint32_t stateFlags)
{
    RC3D_Sector *sectors = rc3dMutableSectorData();
    int changedCount = 0;

    if (!g_map || !sectors) {
        return 0;
    }

    stateFlags = rc3dSanitizeSectorStateFlags(stateFlags);

    for (int i = 0; i < g_map->sectorCount; ++i) {
        if (sectors[i].tagId == tagId) {
            sectors[i].stateFlags = stateFlags;
            changedCount++;
        }
    }

    return changedCount;
}



void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx)
{
    float moveX = 0.0f;
    float moveY = 0.0f;
    int isMoving = 0;

    float forwardX = 1.0f;
    float forwardY = 0.0f;

    rc3dUpdateSectorMotion(dt);

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

    if (keys[SDL_SCANCODE_F1]) { bShowProfiler = 1; }
    if (keys[SDL_SCANCODE_F2]) { bShowProfiler = 0; }

    if (keys[SDL_SCANCODE_1]){ 
        rc3dSetSectorStateByTag(1, RC3D_SECTOR_STATE_RAISE_FLOOR | RC3D_SECTOR_STATE_LOWER_CEILING); 
    }
    if (keys[SDL_SCANCODE_2]){ 
        rc3dSetSectorStateByTag(1, RC3D_SECTOR_STATE_LOWER_FLOOR | RC3D_SECTOR_STATE_RAISE_CEILING); 
    }

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
#if RC3D_DRAW_PROFILER
    const Uint64 frameStart = SDL_GetPerformanceCounter();
    Uint64 sectionStart = frameStart;
#endif
    rc3dRefreshViewport();
    rc3dClearWallDepthSpans();
#if RC3D_DRAW_PROFILER
    rc3dProfilerBeginFrame();
    sectionStart = SDL_GetPerformanceCounter();
#endif
    drawBackground();
#if RC3D_DRAW_PROFILER
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        g_profiler.backgroundMs = rc3dProfilerTicksToMs(now - sectionStart);
        sectionStart = now;
    }
#endif
    renderCurrentSectorColumns();
#if RC3D_DRAW_PROFILER
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        g_profiler.wallsMs = rc3dProfilerTicksToMs(now - sectionStart);
        sectionStart = now;
    }
#endif
    renderSprites();
#if RC3D_DRAW_PROFILER
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        g_profiler.spritesMs = rc3dProfilerTicksToMs(now - sectionStart);
        sectionStart = now;
    }
#endif

#if RC3D_DRAW_MINIMAP
    drawMiniMap();
#endif

#if RC3D_DRAW_PROFILER
    {
        const Uint64 now = SDL_GetPerformanceCounter();
        g_profiler.minimapMs = rc3dProfilerTicksToMs(now - sectionStart);
        g_profiler.totalMs = rc3dProfilerTicksToMs(now - frameStart);
        rc3dProfilerBlendAverages();
    }
#endif

#if RC3D_DRAW_HUD
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "SECTOR %d", g_player.sector);
        drawText(8, 18, buf, 2);
    }
#endif

#if RC3D_DRAW_PROFILER
    if(bShowProfiler)
        rc3dDrawProfiler();
#endif
}
