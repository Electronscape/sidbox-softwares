
#include "rc3d.h"

#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include "gfx.h"
#include "rc3d_map.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


int g_viewport_top    = RC3D_VIEWPORT_TOP;
int g_viewport_left   = RC3D_VIEWPORT_LEFT;
int g_viewport_width  = RC3D_VIEWPORT_WIDTH;
int g_viewport_height = RC3D_VIEWPORT_HEIGHT;

#define RC3D_DRAW_MINIMAP      1
#define RC3D_DRAW_HUD          1
#define RC3D_DRAW_PROFILER     1

#define RC3D_FOV_DEG           90.0f
#define RC3D_TURN_SPEED        2.4f
#define RC3D_MOVE_SPEED        2.0f
#define RC3D_MOUSE_SENS        0.0035f
#define RC3D_EPSILON           0.000000f
#define RC3D_COLLISION_SKIN    0.02f
#define RC3D_FIXED_SHIFT       16
#define RC3D_FIXED_ONE         (1 << RC3D_FIXED_SHIFT)
#define RC3D_TRIG_LUT_BITS     11
#define RC3D_TRIG_LUT_SIZE     (1 << RC3D_TRIG_LUT_BITS)
#define RC3D_TRIG_LUT_MASK     (RC3D_TRIG_LUT_SIZE - 1)
#define RC3D_MAX_RAY_DIST      40.0f
#define RC3D_MAX_PORTAL_STEPS  24
#define RC3D_MAX_MASKED_TRACE_DEPTH 8
#define RC3D_MAX_WALL_SPANS_PER_COLUMN 32
#define RC3D_DEPTH_SCALE       256.0f
#define RC3D_VISIBLE_TRACE_CACHE_SEGMENTS 21
#define RC3D_VISIBLE_TRACE_CLIP_BITS 10u
#define RC3D_VISIBLE_TRACE_CLIP_MASK ((1u << RC3D_VISIBLE_TRACE_CLIP_BITS) - 1u)
#define RC3D_VISIBLE_TRACE_SECTOR_BITS (32u - (RC3D_VISIBLE_TRACE_CLIP_BITS * 2u))
#define RC3D_VISIBLE_TRACE_SECTOR_MASK ((1u << RC3D_VISIBLE_TRACE_SECTOR_BITS) - 1u)

#if SCREEN_H > RC3D_VISIBLE_TRACE_CLIP_MASK
#error "Visible trace cache clip packing needs more bits for current SCREEN_H"
#endif

int bShowMiniMap = 1;
int bShowProfiler = 0;
float g_draw_distance = RC3D_MAX_RAY_DIST;

#define RC3D_PLAYER_EYE_HEIGHT 0.5f
#define RC3D_GRAVITY           18.0f
#define RC3D_STEP_SNAP_SPEED   114.0f

#define PLAYER_HEIGHT          0.6f
#define PLAYER_STEPUP          0.35f
#define PLAYER_RADIUS          0.40f


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

// i'll keep this i DO like the sunlight feature!
//#define RC3D_WALL_DIRLIGHT_ENABLE     1
//#define RC3D_WALL_DIRLIGHT_X         -0.70710678f
//#define RC3D_WALL_DIRLIGHT_Y          0.70710678f
//#define RC3D_WALL_DIRLIGHT_STRENGTH   2.25f

#define RC3D_WALL_POINTLIGHT_ENABLE    1
#define RC3D_WALL_POINTLIGHT_STRENGTH  1.25f

#define RC3D_TEXID_SPRITE_MAN       RC3D_SPRITE_TEX_MAN
#define RC3D_TEXID_SPRITE_GRICY     RC3D_SPRITE_TEX_GRICY
#define RC3D_TEXID_SKYBOX           255

#define RC3D_SPRITE_TEX_TRANSPARENT 0
#define RC3D_TEST_SPRITE_WIDTH      0.75f
#define RC3D_TEST_SPRITE_HEIGHT     0.75f

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


static float g_wallShadeBiasGlobal = 0.0f;


#if RC3D_DRAW_PROFILER
typedef struct {
double updateMs;
    double backgroundMs;
    double wallsMs;
    double spritesMs;
    double minimapMs;
    double totalMs;

    double avgUpdateMs;
    double avgBackgroundMs;
    double avgWallsMs;
    double avgSpritesMs;
    double avgMinimapMs;
    double avgTotalMs;

    double peakUpdateMs;
    double peakBackgroundMs;
    double peakWallsMs;
    double peakSpritesMs;
    double peakMinimapMs;
    double peakTotalMs;

    double fpsNow;
    double fpsAvg;

    uint32_t rays;
    uint32_t wallTests;
    uint32_t portalSteps;
    uint32_t spriteColumns;
    uint32_t spriteSectorTraces;

    uint32_t wallSpans;
    uint32_t visibleTraceSegments;
    uint32_t spritesVisible;
    uint32_t spritesDrawn;

    uint32_t objectsProcessed;
    uint32_t objectEdgeChanges;
    uint32_t sectorStateWrites;
    uint32_t sectorsMoved;
} RC3D_Profiler;

static RC3D_Profiler g_profiler;
static Uint64 g_profilerFreq = 0;
static int g_profilerHasHistory = 0;

static int g_profilerFrameActive = 0;

#define RC3D_PROFILER_DO(stmt) \
    do { if (g_profilerFrameActive) { stmt; } } while (0)

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
    g_profiler.updateMs = 0.0;
    g_profiler.backgroundMs = 0.0;
    g_profiler.wallsMs = 0.0;
    g_profiler.spritesMs = 0.0;
    g_profiler.minimapMs = 0.0;
    g_profiler.totalMs = 0.0;
    g_profiler.fpsNow = 0.0;

    g_profiler.rays = 0u;
    g_profiler.wallTests = 0u;
    g_profiler.portalSteps = 0u;
    g_profiler.spriteColumns = 0u;
    g_profiler.spriteSectorTraces = 0u;

    g_profiler.wallSpans = 0u;
    g_profiler.visibleTraceSegments = 0u;
    g_profiler.spritesVisible = 0u;
    g_profiler.spritesDrawn = 0u;

    g_profiler.objectsProcessed = 0u;
    g_profiler.objectEdgeChanges = 0u;
    g_profiler.sectorStateWrites = 0u;
    g_profiler.sectorsMoved = 0u;

    g_profilerFrameActive = 1;
}


static inline void rc3dProfilerBlendAverages(void)
{
    const double alphaMs  = 0.02;   /* smoother timing average */
    const double alphaFps = 0.02;   /* even smoother fps average */

    g_profiler.fpsNow = (g_profiler.totalMs > 0.0) ? (1000.0 / g_profiler.totalMs) : 0.0;

    if (!g_profilerHasHistory) {
        g_profiler.avgUpdateMs = g_profiler.updateMs;
        g_profiler.avgBackgroundMs = g_profiler.backgroundMs;
        g_profiler.avgWallsMs = g_profiler.wallsMs;
        g_profiler.avgSpritesMs = g_profiler.spritesMs;
        g_profiler.avgMinimapMs = g_profiler.minimapMs;
        g_profiler.avgTotalMs = g_profiler.totalMs;

        g_profiler.peakUpdateMs = g_profiler.updateMs;
        g_profiler.peakBackgroundMs = g_profiler.backgroundMs;
        g_profiler.peakWallsMs = g_profiler.wallsMs;
        g_profiler.peakSpritesMs = g_profiler.spritesMs;
        g_profiler.peakMinimapMs = g_profiler.minimapMs;
        g_profiler.peakTotalMs = g_profiler.totalMs;

        g_profiler.fpsAvg = g_profiler.fpsNow;

        g_profilerHasHistory = 1;
        return;
    }

    g_profiler.avgUpdateMs += (g_profiler.updateMs - g_profiler.avgUpdateMs) * alphaMs;
    g_profiler.avgBackgroundMs += (g_profiler.backgroundMs - g_profiler.avgBackgroundMs) * alphaMs;
    g_profiler.avgWallsMs += (g_profiler.wallsMs - g_profiler.avgWallsMs) * alphaMs;
    g_profiler.avgSpritesMs += (g_profiler.spritesMs - g_profiler.avgSpritesMs) * alphaMs;
    g_profiler.avgMinimapMs += (g_profiler.minimapMs - g_profiler.avgMinimapMs) * alphaMs;
    g_profiler.avgTotalMs += (g_profiler.totalMs - g_profiler.avgTotalMs) * alphaMs;

    g_profiler.fpsAvg += (g_profiler.fpsNow - g_profiler.fpsAvg) * alphaFps;

    if (g_profiler.updateMs > g_profiler.peakUpdateMs) g_profiler.peakUpdateMs = g_profiler.updateMs;
    if (g_profiler.backgroundMs > g_profiler.peakBackgroundMs) g_profiler.peakBackgroundMs = g_profiler.backgroundMs;
    if (g_profiler.wallsMs > g_profiler.peakWallsMs) g_profiler.peakWallsMs = g_profiler.wallsMs;
    if (g_profiler.spritesMs > g_profiler.peakSpritesMs) g_profiler.peakSpritesMs = g_profiler.spritesMs;
    if (g_profiler.minimapMs > g_profiler.peakMinimapMs) g_profiler.peakMinimapMs = g_profiler.minimapMs;
    if (g_profiler.totalMs > g_profiler.peakTotalMs) g_profiler.peakTotalMs = g_profiler.totalMs;
}

#define PROFILE_COLOUR      116
static void rc3dDrawProfiler(void)
{
    char buf[96];
    const int textW = 8;
    const int lineH = 8;
    const int panelX = 6;
    const int panelW = (34 * textW) + 8;
    const int panelH = (15 * lineH) + 8;
    int panelY = SCREEN_H - panelH - 6;
    int textY;
    double testsPerRay = 0.0;
    double portalsPerRay = 0.0;

    if (panelY < 6) {
        panelY = 6;
    }

    if (g_profiler.rays > 0u) {
        testsPerRay = (double)g_profiler.wallTests / (double)g_profiler.rays;
        portalsPerRay = (double)g_profiler.portalSteps / (double)g_profiler.rays;
    }

    drawRectSemi(panelX, panelY, panelW, panelH, 16);
    drawLine(panelX, panelY, panelX + panelW - 1, panelY, PROFILE_COLOUR);
    drawLine(panelX, panelY, panelX, panelY + panelH - 1, PROFILE_COLOUR);
    drawLine(panelX + panelW - 1, panelY, panelX + panelW - 1, panelY + panelH - 1, PROFILE_COLOUR);
    drawLine(panelX, panelY + panelH - 1, panelX + panelW - 1, panelY + panelH - 1, PROFILE_COLOUR);

    textY = panelY + 4;

    drawTextO(panelX + 4, textY, "PROFILER      NOW    AVG    PEAK", PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "FPS         %6.1f %6.1f", g_profiler.fpsNow, g_profiler.fpsAvg);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "FRAME       %6.2f %6.2f %6.2f", g_profiler.totalMs, g_profiler.avgTotalMs, g_profiler.peakTotalMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "UPDATE      %6.2f %6.2f %6.2f", g_profiler.updateMs, g_profiler.avgUpdateMs, g_profiler.peakUpdateMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "SKY/BG      %6.2f %6.2f %6.2f", g_profiler.backgroundMs, g_profiler.avgBackgroundMs, g_profiler.peakBackgroundMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "WALLS       %6.2f %6.2f %6.2f", g_profiler.wallsMs, g_profiler.avgWallsMs, g_profiler.peakWallsMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "SPRITES     %6.2f %6.2f %6.2f", g_profiler.spritesMs, g_profiler.avgSpritesMs, g_profiler.peakSpritesMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "MINIMAP     %6.2f %6.2f %6.2f", g_profiler.minimapMs, g_profiler.avgMinimapMs, g_profiler.peakMinimapMs);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "RAYS %u  WTEST %u  T/R %.1f", g_profiler.rays, g_profiler.wallTests, testsPerRay);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "PORT %u  VIS %u  P/R %.1f", g_profiler.portalSteps, g_profiler.visibleTraceSegments, portalsPerRay);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "SPAN %u  SCOL %u", g_profiler.wallSpans, g_profiler.spriteColumns);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "SPR V %u  D %u  TR %u", g_profiler.spritesVisible, g_profiler.spritesDrawn, g_profiler.spriteSectorTraces);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "OBJ P %u  E %u  W %u", g_profiler.objectsProcessed, g_profiler.objectEdgeChanges, g_profiler.sectorStateWrites);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
    textY += lineH;

    snprintf(buf, sizeof(buf), "SECT MOVED %2u  DD %.1f", g_profiler.sectorsMoved, g_draw_distance);
    drawTextO(panelX + 4, textY, buf, PROFILE_COLOUR);
}
#endif




static RC3D_Player g_player;
static RC3D_Sprite g_sprites[RC3D_MAX_SPRITES];
//static int g_demoSpriteA = RC3D_INVALID_SPRITE;
//static int g_demoSpriteB = RC3D_INVALID_SPRITE;

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
static uint64_t g_textureTransparentColumnMask[256];
static uint8_t g_textureHasTransparency[256];
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
    int32_t backWallIndex;

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
static RC3D_VisibleSprite g_visibleSprites[RC3D_MAX_SPRITES];
static int g_visibleSpriteCount = 0;
/*
    63,360 bytes total:
    - packed visible trace cache: 40,320 bytes
    - trace depth cache: 20,160 bytes
    - per-column trace counts/overflow: 960 bytes
    - per-column sprite coverage: 1,920 bytes
*/
static uint32_t g_cachedVisibleTracePacked[SCREEN_W][RC3D_VISIBLE_TRACE_CACHE_SEGMENTS];
static uint16_t g_cachedVisibleTraceDepth[SCREEN_W][RC3D_VISIBLE_TRACE_CACHE_SEGMENTS];
static uint8_t g_cachedVisibleTraceCount[SCREEN_W];
static uint8_t g_cachedVisibleTraceOverflow[SCREEN_W];
static int16_t g_spriteColumnFirstIndex[SCREEN_W];
static int16_t g_spriteColumnLastIndex[SCREEN_W];
static RC3D_ColumnContext g_columnContext = { 0 };
static int g_traceSuppressSectorPlanes = 0;

static int rc3dEnsurePlayerSectorValid(void);
static void rc3dBuildTextureTransparencyMasks(void);
static inline int rc3dTextureColumnNeedsMaskedTrace(uint8_t texId);
static int rc3dBuildVisibleSpriteList(float eyeZ, float dirX, float dirY);
static void rc3dBuildSpriteColumnCoverage(void);
static void rc3dRenderSpriteColumn(
    int sx,
    float eyeZ,
    const RC3D_VisibleTraceSegment *visibleTrace,
    uint8_t visibleTraceCount,
    const RC3D_WallDepthSpan *wallSpans,
    uint8_t wallSpanCount);

static int rc3dPositionBlockedInSectorFixed(
    RC3D_Fixed px,
    RC3D_Fixed py,
    RC3D_Fixed radius,
    int sectorIndex);







static uint32_t g_randState = 0x12345678u;

void randSeed(uint32_t seed)
{
    g_randState = seed ? seed : 0x12345678u;
}

uint32_t rand32(void)
{
    g_randState ^= g_randState << 13;
    g_randState ^= g_randState >> 17;
    g_randState ^= g_randState << 5;
    return g_randState;
}

uint32_t randRange(uint32_t min, uint32_t max)
{
    if (max <= min) return min;
    return min + (rand32() % (max - min + 1));
}

float randFloat(void)
{
    return (rand32() / 4294967295.0f);
}















static inline int rc3dViewportScreenX(int sx){
    return g_viewport_left + sx;
}

static inline int rc3dViewportScreenY(int sy){
    return g_viewport_top + sy;
}

static inline uint8_t *rc3dViewportPixelPtr(int sx, int sy){
    return &fb[(rc3dViewportScreenY(sy) * SCREEN_W) + rc3dViewportScreenX(sx)];
}

static inline float rc3dViewportCameraX(int sx){
    if (g_viewport_width <= 1) {
        return 0.0f;
    }
    return -1.0f + (2.0f * ((float)sx / (float)(g_viewport_width - 1)));
}

static void rc3dBuildColumnRayCache(float dirX, float dirY, float planeX, float planeY){
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
        cache->planeLightLen = sqrtf((rdx * rdx) + (rdy * rdy)) * RC3D_LIGHT_PLANE_DIST_SCALE;

        rdx += rayStepX;
        rdy += rayStepY;
    }

    g_columnRayCacheCount = g_viewport_width;
}

static inline void rc3dSetColumnContext(
    RC3D_VisibleTraceSegment *visibleTrace,
    uint8_t *visibleTraceCount,
    RC3D_WallDepthSpan *wallSpans,
    uint8_t *wallSpanCount)
{
    g_columnContext.visibleTrace = visibleTrace;
    g_columnContext.visibleTraceCount = visibleTraceCount;
    g_columnContext.wallSpans = wallSpans;
    g_columnContext.wallSpanCount = wallSpanCount;

    if (visibleTraceCount) {
        *visibleTraceCount = 0u;
    }

    if (wallSpanCount) {
        *wallSpanCount = 0u;
    }
}

/* ------------------------------------------------------------------------- */
/* forward decls                                                             */
/* ------------------------------------------------------------------------- */

static int tryMovePlayerSliding(float moveX, float moveY);
static void rc3dRefreshDynamicPortalCache(void);
static void rc3dRefreshSpritePlacement(RC3D_Sprite *sprite);
static void rc3dResolvePlayerPenetrationInSector(int sectorIndex);


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

static inline int rc3dSectorIndexValid(int sectorIndex){
    return g_map && ((unsigned)sectorIndex < (unsigned)g_map->sectorCount);
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
    return (spriteId >= 0) && (spriteId < RC3D_MAX_SPRITES) && g_sprites[spriteId].inUse;
}

static void rc3dBuildTrigTables(void){
    if (g_trigLutInit) {
        return;
    }

    for (int i = 0; i < RC3D_TRIG_LUT_SIZE; ++i) {
        const float angle = ((float)i * (float)(M_PI * 2.0f)) / (float)RC3D_TRIG_LUT_SIZE;
        g_sinLut[i] = sinf(angle);
        g_cosLut[i] = cosf(angle);
    }

    g_halfFovRad = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    g_planeScaleConst = tanf(g_halfFovRad);
    g_projPlaneConst = ((float)g_viewport_width * 0.5f) / g_planeScaleConst;
    g_camStepConst = (g_viewport_width > 1) ? (2.0f / (float)(g_viewport_width - 1)) : 0.0f;
    g_angleToLutScale = (float)RC3D_TRIG_LUT_SIZE / (float)(M_PI * 2.0f);

    g_trigLutInit = 1;
}

static void rc3dResetPlayerFromMapStart(void);

static inline int rc3dAngleToLutIndex(float angle)
{
    const float scaled = angle * g_angleToLutScale;
    int idx = (int)((scaled >= 0.0f) ? (scaled + 0.5f) : (scaled - 0.5f));

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

    if (dist >= g_draw_distance) {
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

static inline void rc3dRecordWallDepthSpan(int sx, int y0, int y1, float hitDist)
{
    uint8_t count;
    (void)sx;

    if (y0 > y1) return;
    if (!g_columnContext.wallSpans || !g_columnContext.wallSpanCount) return;

    count = *g_columnContext.wallSpanCount;
    if (count >= RC3D_MAX_WALL_SPANS_PER_COLUMN) {
        return;
    }

    g_columnContext.wallSpans[count].y0 = (int16_t)y0;
    g_columnContext.wallSpans[count].y1 = (int16_t)y1;
    g_columnContext.wallSpans[count].depth = rc3dEncodeDepth(hitDist);
    *g_columnContext.wallSpanCount = (uint8_t)(count + 1);

    RC3D_PROFILER_DO(g_profiler.wallSpans++);
}

static inline void rc3dRecordVisibleTraceSegment(
    int sector,
    int clipTop,
    int clipBottom,
    uint16_t depthLimit)
{
    uint8_t count;

    if (!g_columnContext.visibleTrace || !g_columnContext.visibleTraceCount) {
        return;
    }

    if (clipTop > clipBottom) {
        return;
    }

    count = *g_columnContext.visibleTraceCount;
    if (count >= RC3D_MAX_PORTAL_STEPS) {
        return;
    }

    g_columnContext.visibleTrace[count].sector = (int16_t)sector;
    g_columnContext.visibleTrace[count].clipTop = (int16_t)clipTop;
    g_columnContext.visibleTrace[count].clipBottom = (int16_t)clipBottom;
    g_columnContext.visibleTrace[count].depthLimit = depthLimit;
    *g_columnContext.visibleTraceCount = (uint8_t)(count + 1);

    RC3D_PROFILER_DO(g_profiler.visibleTraceSegments++);
}

static inline void rc3dRecordCachedVisibleTraceSegment(
    int sx,
    int sector,
    int clipTop,
    int clipBottom,
    uint16_t depthLimit)
{
    uint8_t count;

    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return;
    }

    if (g_spriteColumnFirstIndex[sx] < 0) {
        return;
    }

    if (clipTop > clipBottom) {
        return;
    }

    if ((unsigned)sector > RC3D_VISIBLE_TRACE_SECTOR_MASK ||
        (unsigned)clipTop > RC3D_VISIBLE_TRACE_CLIP_MASK ||
        (unsigned)clipBottom > RC3D_VISIBLE_TRACE_CLIP_MASK)
    {
        g_cachedVisibleTraceOverflow[sx] = 1u;
        return;
    }

    count = g_cachedVisibleTraceCount[sx];
    if (count >= RC3D_VISIBLE_TRACE_CACHE_SEGMENTS) {
        g_cachedVisibleTraceOverflow[sx] = 1u;
        return;
    }

    g_cachedVisibleTracePacked[sx][count] =
        (((uint32_t)sector & RC3D_VISIBLE_TRACE_SECTOR_MASK) << (RC3D_VISIBLE_TRACE_CLIP_BITS * 2u)) |
        (((uint32_t)clipTop & RC3D_VISIBLE_TRACE_CLIP_MASK) << RC3D_VISIBLE_TRACE_CLIP_BITS) |
        ((uint32_t)clipBottom & RC3D_VISIBLE_TRACE_CLIP_MASK);
    g_cachedVisibleTraceDepth[sx][count] = depthLimit;
    g_cachedVisibleTraceCount[sx] = (uint8_t)(count + 1u);

    RC3D_PROFILER_DO(g_profiler.visibleTraceSegments++);
}

static inline int rc3dWallSpansBlockPixel(
    const RC3D_WallDepthSpan *wallSpans,
    uint8_t wallSpanCount,
    int y,
    uint16_t spriteDepth)
{
    if (!wallSpans || wallSpanCount == 0u) {
        return 0;
    }

    for (uint8_t i = 0; i < wallSpanCount; ++i) {
        const RC3D_WallDepthSpan *span = &wallSpans[i];

        if (y >= span->y0 && y <= span->y1 && span->depth <= spriteDepth) {
            return 1;
        }
    }

    return 0;
}

static inline int rc3dTraceVisibleSectorAtDepthCached(
    int sx,
    uint16_t targetDepthCode,
    int *outSector,
    int *outClipTop,
    int *outClipBottom)
{
    const uint8_t visibleTraceCount =
        ((unsigned)sx < (unsigned)g_viewport_width) ? g_cachedVisibleTraceCount[sx] : 0u;

    RC3D_PROFILER_DO(g_profiler.spriteSectorTraces++);

    if (visibleTraceCount == 0u) {
        return 0;
    }

    for (uint8_t i = 0; i < visibleTraceCount; ++i) {
        const uint16_t depthLimit = g_cachedVisibleTraceDepth[sx][i];
        const uint32_t packed = g_cachedVisibleTracePacked[sx][i];

        if (targetDepthCode > depthLimit) {
            continue;
        }

        if (outSector) {
            *outSector = (int)(packed >> (RC3D_VISIBLE_TRACE_CLIP_BITS * 2u));
        }

        if (outClipTop) {
            *outClipTop = (int)((packed >> RC3D_VISIBLE_TRACE_CLIP_BITS) & RC3D_VISIBLE_TRACE_CLIP_MASK);
        }

        if (outClipBottom) {
            *outClipBottom = (int)(packed & RC3D_VISIBLE_TRACE_CLIP_MASK);
        }

        return 1;
    }

    return 0;
}

static inline int rc3dTraceVisibleSectorAtDepth(
    const RC3D_VisibleTraceSegment *visibleTrace,
    uint8_t visibleTraceCount,
    uint16_t targetDepthCode,
    int *outSector,
    int *outClipTop,
    int *outClipBottom)
{
    RC3D_PROFILER_DO(g_profiler.spriteSectorTraces++);

    if (!visibleTrace || visibleTraceCount == 0u) {
        return 0;
    }

    for (uint8_t i = 0; i < visibleTraceCount; ++i) {
        const RC3D_VisibleTraceSegment *segment = &visibleTrace[i];

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



static inline float rc3dComputeWallShadeBias(
    float wallDx, float wallDy,
    float hitX, float hitY,
    float lightX, float lightY)
{
#if RC3D_WALL_POINTLIGHT_ENABLE
    float wallLen;
    float nx, ny;
    float lx, ly;
    float lightLen;
    float facing;

    wallLen = sqrtf((wallDx * wallDx) + (wallDy * wallDy));
    if (wallLen <= RC3D_EPSILON) {
        return 0.0f;
    }

    /* wall normal */
    nx = -wallDy / wallLen;
    ny =  wallDx / wallLen;

    /* vector from wall hit point toward the light */
    lx = lightX - hitX;
    ly = lightY - hitY;

    lightLen = sqrtf((lx * lx) + (ly * ly));
    if (lightLen <= RC3D_EPSILON) {
        return 0.0f;
    }

    lx /= lightLen;
    ly /= lightLen;

    /*
        abs() avoids winding flipping the result.
        We only want "how aligned is this wall with the light",
        not "did this map wall happen to be authored backwards".
    */
    facing = fabsf((nx * lx) + (ny * ly));

    if (facing < 0.0f) facing = 0.0f;
    if (facing > 1.0f) facing = 1.0f;

    /* facing light -> less extra darkening, grazing -> more */
    return (1.0f - facing) * RC3D_WALL_POINTLIGHT_STRENGTH;
#else
    (void)wallDx;
    (void)wallDy;
    (void)hitX;
    (void)hitY;
    (void)lightX;
    (void)lightY;
    return 0.0f;
#endif
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
            g_glowDistScale[glow] = 1.0f - ((float)glow / (float)RC3D_TEX_WALL_GLOW_MAX);
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
    RC3D_ShadeProfile *outProfile)
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
        const float brightBlendWidth = rc3dMaxf(brightRange * RC3D_LIGHT_BRIGHT_BLEND_RATIO, 0.0f);
        const float brightBlendStart = brightEnd - brightBlendWidth;

        if (sampleDist <= brightBlendStart) {
            outProfile->mode = RC3D_SHADEMODE_BRIGHT;
            outProfile->threshold = 0u;
            return;
        }

        if ((brightBlendWidth > RC3D_EPSILON) && (sampleDist < brightEnd)) {
            const float blend = rc3dSmoothStep01((sampleDist - brightBlendStart) / brightBlendWidth);
            outProfile->mode = RC3D_SHADEMODE_DITHER_BRIGHT_MID;
            outProfile->threshold = (uint8_t)(blend * 255.0f);
            return;
        }
    }

    {
        const float midBlendWidth = rc3dMaxf(midRange * RC3D_LIGHT_BRIGHT_BLEND_RATIO, 0.0f);
        const float midBlendStart = midEnd - midBlendWidth;

        if (sampleDist <= midBlendStart) {
            outProfile->mode = RC3D_SHADEMODE_MID;
            outProfile->threshold = 0u;
            return;
        }

        if ((midBlendWidth > RC3D_EPSILON) && (sampleDist < midEnd)) {
            const float blend = rc3dSmoothStep01((sampleDist - midBlendStart) / midBlendWidth);
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
        const float blend = rc3dSmoothStep01((sampleDist - midEnd) / darkRange);
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
    const RC3D_ShadeProfile *shadeProfile)
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
extern uint8_t spr_gricy1[];
extern uint8_t tex_skybox[RC3D_SKYBOX_W * RC3D_SKYBOX_H];

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

    for (tindex = 0; tindex < 255; tindex++) {
        snprintf(filename, sizeof(filename), "./textures/%02u.ppb", (unsigned)tindex);
        LoadPPB(filename, g_rc3dTextures[tindex].pix);
    }

    snprintf(filename, sizeof(filename), "./textures/255.ppb");
    LoadPPB(filename, tex_skybox);

    tindex = 0;
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            g_rc3dTextures[RC3D_TEXID_SPRITE_MAN].pix[tindex] = spr_oiiacat[tindex];
            g_rc3dTextures[RC3D_TEXID_SPRITE_GRICY].pix[tindex] = spr_gricy1[tindex];
            tindex++;
        }
    }

    rc3dBuildTextureTransparencyMasks();
    g_rc3dTexturesInit = 1;
}

uint8_t *rc3d_GetTexturePtr(uint8_t textureindex){
    return g_rc3dTextures[textureindex].pix;
}

void copyTextureToTexture(uint8_t *from, uint8_t *to, int sizex, int sizey){
    for(int p = 0; p < (sizex * sizey); p++){
        *to++ = *from++;
    }
}




void shiftTexture(uint8_t texIndex, int8_t dir)
{
    uint8_t *tex = rc3d_GetTexturePtr(texIndex);
    if (!tex) return;

    switch (dir) {
        case TEXSHIFT_LEFT: /* left */
            for (int y = 0; y < 64; y++) {
                const int row = y * 64;
                uint8_t first = tex[row];

                for (int x = 0; x < 63; x++) {
                    tex[row + x] = tex[row + x + 1];
                }

                tex[row + 63] = first;
            }
            break;

        case TEXSHIFT_RIGHT: /* right */
            for (int y = 0; y < 64; y++) {
                const int row = y * 64;
                uint8_t last = tex[row + 63];

                for (int x = 63; x > 0; x--) {
                    tex[row + x] = tex[row + x - 1];
                }

                tex[row] = last;
            }
            break;

        case TEXSHIFT_UP: /* up */
            for (int x = 0; x < 64; x++) {
                uint8_t first = tex[x];

                for (int y = 0; y < 63; y++) {
                    tex[(y * 64) + x] = tex[((y + 1) * 64) + x];
                }

                tex[(63 * 64) + x] = first;
            }
            break;

        case TEXSHIFT_DOWN: /* down */
            for (int x = 0; x < 64; x++) {
                uint8_t last = tex[(63 * 64) + x];

                for (int y = 63; y > 0; y--) {
                    tex[(y * 64) + x] = tex[((y - 1) * 64) + x];
                }

                tex[x] = last;
            }
            break;

        default:
            break;
    }
}
static void shiftTexturePixelsX(uint8_t *tex, int dx)
{
    if (!tex) return;

    dx %= 64;
    if (dx < 0) dx += 64;
    if (dx == 0) return;

    for (int y = 0; y < 64; y++) {
        uint8_t row[64];
        const int base = y * 64;

        for (int x = 0; x < 64; x++) {
            row[x] = tex[base + x];
        }

        for (int x = 0; x < 64; x++) {
            tex[base + ((x + dx) & 63)] = row[x];
        }
    }
}

static void shiftTexturePixelsY(uint8_t *tex, int dy)
{
    if (!tex) return;

    dy %= 64;
    if (dy < 0) dy += 64;
    if (dy == 0) return;

    for (int x = 0; x < 64; x++) {
        uint8_t col[64];

        for (int y = 0; y < 64; y++) {
            col[y] = tex[(y * 64) + x];
        }

        for (int y = 0; y < 64; y++) {
            tex[(((y + dy) & 63) * 64) + x] = col[y];
        }
    }
}

void shiftTextureFX(
    uint8_t texIndex,
    uint8_t flags,
    float speedscalex,
    float speedscaley,
    float timescalex,
    float timescaley,
    float frametime)
{
    uint8_t *tex = rc3d_GetTexturePtr(texIndex);
    if (!tex) return;

    if (speedscalex < 0.0f) speedscalex = 0.0f;
    if (speedscaley < 0.0f) speedscaley = 0.0f;

    {
        static float phaseX[256]  = {0.0f};
        static float phaseY[256]  = {0.0f};
        static float scrollX[256] = {0.0f};
        static float scrollY[256] = {0.0f};
        static int   lastX[256]   = {0};
        static int   lastY[256]   = {0};

        float wantedX;
        float wantedY;
        int newX;
        int newY;
        int dx;
        int dy;

        /* tune these */
        const float waveSpeedX = timescalex;   /* radians/sec */
        const float waveSpeedY = timescaley;   /* radians/sec */

        /* advance wave time independently of amplitude */
        phaseX[texIndex] += frametime * waveSpeedX;
        phaseY[texIndex] += frametime * waveSpeedY;

        if (phaseX[texIndex] >= (float)(M_PI * 2.0))
            phaseX[texIndex] -= (float)(M_PI * 2.0);

        if (phaseY[texIndex] >= (float)(M_PI * 2.0))
            phaseY[texIndex] -= (float)(M_PI * 2.0);

        /* continuous scroll accumulation */
        if (flags & TEXSHIFT_LEFT)  scrollX[texIndex] -= speedscalex * frametime;
        if (flags & TEXSHIFT_RIGHT) scrollX[texIndex] += speedscalex * frametime;
        if (flags & TEXSHIFT_UP)    scrollY[texIndex] -= speedscaley * frametime;
        if (flags & TEXSHIFT_DOWN)  scrollY[texIndex] += speedscaley * frametime;

        wantedX = scrollX[texIndex];
        wantedY = scrollY[texIndex];

        /* smooth whole-texture oscillation */
        if (flags & TEXSHIFT_SINOUSSX) wantedX += sinf(phaseX[texIndex]) * speedscalex;
        if (flags & TEXSHIFT_SINOUSSY) wantedY += sinf(phaseY[texIndex]) * speedscaley;

        if (flags & TEXSHIFT_SINOUSCX) wantedX += cosf(phaseX[texIndex]) * speedscalex;
        if (flags & TEXSHIFT_SINOUSCY) wantedY += cosf(phaseY[texIndex]) * speedscaley;

        /* use nearest whole pixel, not truncation */
        newX = (int)((wantedX >= 0.0f) ? (wantedX + 0.5f) : (wantedX - 0.5f));
        newY = (int)((wantedY >= 0.0f) ? (wantedY + 0.5f) : (wantedY - 0.5f));

        dx = newX - lastX[texIndex];
        dy = newY - lastY[texIndex];

        if (dx) shiftTexturePixelsX(tex, dx);
        if (dy) shiftTexturePixelsY(tex, dy);

        lastX[texIndex] = newX;
        lastY[texIndex] = newY;
    }
}


static void rc3dBuildTextureTransparencyMasks(void)
{
    for (int texId = 0; texId < 256; ++texId) {
        uint64_t columnMask = 0u;
        int hasTransparency = 0;

        for (int x = 0; x < RC3D_TEX_SIZE; ++x) {
            for (int y = 0; y < RC3D_TEX_SIZE; ++y) {
                if (g_rc3dTextures[texId].pix[(y * RC3D_TEX_SIZE) + x] == RC3D_SPRITE_TEX_TRANSPARENT) {
                    columnMask |= (1ULL << x);
                    hasTransparency = 1;
                    break;
                }
            }
        }

        g_textureTransparentColumnMask[texId] = columnMask;
        g_textureHasTransparency[texId] = (uint8_t)hasTransparency;
    }
}

static inline int rc3dTextureColumnNeedsMaskedTrace(uint8_t texId)
{
    if (!g_textureHasTransparency[texId]) {
        return 0;
    }

    if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f)) {
        const float texX = wallTexUBaseGlobal * wallTexRotCosGlobal;
        const int tx = (((int32_t)(texX * 65536.0f)) >> 16) & RC3D_TEX_MASK;
        return (int)((g_textureTransparentColumnMask[texId] >> tx) & 1ULL);
    }

    return 1;
}

#define RC3D_TEX_WALL_ANGLE_SHIFT   4u
#define RC3D_TEX_WALL_ANGLE_MASK    (0xFFFFu << RC3D_TEX_WALL_ANGLE_SHIFT)

static inline float rc3dWallTexAngleFromFlags(uint32_t texture_flags)
{
    const uint32_t packed = (texture_flags & RC3D_TEX_WALL_ANGLE_MASK) >> RC3D_TEX_WALL_ANGLE_SHIFT;
    return (float)packed * ((float)(M_PI * 2.0) / 65536.0f);
}

/* ------------------------------------------------------------------------- */
/* binary map loading                                                        */
/* ------------------------------------------------------------------------- */

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
            uint8_t g = (src >> 8) & 0xFF;
            uint8_t b = (src >> 0) & 0xFF;

            r = (uint8_t)(r * fade[step]);
            g = (uint8_t)(g * fade[step]);
            b = (uint8_t)(b * fade[step]);

            clut[128 + (step * 64) + p] =
                ((uint32_t)a << 24) |
                ((uint32_t)r << 16) |
                ((uint32_t)g << 8) |
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
    memset(g_visibleSprites, 0, sizeof(g_visibleSprites));
    g_visibleSpriteCount = 0;
}

int rc3dSpriteCreate(float x, float y, float z, float width, float height, uint8_t texId)
{
    for (int i = 0; i < RC3D_MAX_SPRITES; ++i) {
        RC3D_Sprite *sprite = &g_sprites[i];

        if (sprite->inUse) {
            continue;
        }

        memset(sprite, 0, sizeof(*sprite));
        sprite->sector = -1;
        sprite->inUse = 1u;
        sprite->active = 1u;
        sprite->texId = texId;
        
        sprite->width = (width > RC3D_EPSILON) ? width : RC3D_EPSILON;
        sprite->height = (height > RC3D_EPSILON) ? height : RC3D_EPSILON;

        rc3dSetSpriteWorldXYFixed(sprite, rc3dFloatToFixed(x), rc3dFloatToFixed(y));
        rc3dRefreshSpritePlacement(sprite); // usually just used to put the sprite on the floor, Comment out if not wanted later
        sprite->baseZ = z;
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

    rc3dSetSpriteWorldXYFixed(&g_sprites[spriteId], rc3dFloatToFixed(x), rc3dFloatToFixed(y));
    rc3dRefreshSpritePlacement(&g_sprites[spriteId]);
}

void rc3dSpriteSetPositionFixed(int spriteId, int32_t xFixed, int32_t yFixed)
{
    if (!rc3dSpriteHandleValid(spriteId)) {
        return;
    }

    rc3dSetSpriteWorldXYFixed(&g_sprites[spriteId], (RC3D_Fixed)xFixed, (RC3D_Fixed)yFixed);
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

    g_fixedVerts = (RC3D_FixedVec2 *)malloc(sizeof(RC3D_FixedVec2) * (size_t)g_map->vertCount);
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


static inline uint8_t rc3dClassifyWallFlags(uint8_t flags)
{
    if (flags & RC3D_WALL_SOLID) {
        return RC3D_WALLCLASS_SOLID;
    }

    if ((flags & RC3D_WALL_MIDDLE) && !(flags & RC3D_WALL_PORTAL)) {
        return RC3D_WALLCLASS_MIDDLE;
    }

    if (flags & RC3D_WALL_PORTAL) {
        return RC3D_WALLCLASS_PORTAL;
    }

    if ((flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
        !(flags & RC3D_WALL_MIDDLE)) {
        return RC3D_WALLCLASS_UPPER_LOWER;
    }

    return RC3D_WALLCLASS_NONE;
}


static inline int rc3dGetWallFixedEndpoints(
    const RC3D_Wall *w,
    RC3D_Fixed *ax, RC3D_Fixed *ay,
    RC3D_Fixed *bx, RC3D_Fixed *by)
{
    if (!w || !g_fixedVerts || g_fixedVertCount <= 0) {
        return 0;
    }

    if ((unsigned)w->v0 >= (unsigned)g_fixedVertCount ||
        (unsigned)w->v1 >= (unsigned)g_fixedVertCount) {
        return 0;
    }

    *ax = g_fixedVerts[w->v0].x;
    *ay = g_fixedVerts[w->v0].y;
    *bx = g_fixedVerts[w->v1].x;
    *by = g_fixedVerts[w->v1].y;
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
                RC3D_Fixed vx0, vy0, vx1, vy1;
                RC3D_Fixed vx[2];
                RC3D_Fixed vy[2];

                if ((unsigned)wi >= (unsigned)g_map->wallCount) {
                    continue;
                }

                {
                    const RC3D_Wall *w = &g_map->walls[wi];

                    if (!rc3dGetWallFixedEndpoints(w, &vx0, &vy0, &vx1, &vy1)) {
                        continue;
                    }
                }

                vx[0] = vx0;
                vy[0] = vy0;
                vx[1] = vx1;
                vy[1] = vy1;

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

            if (!bboxInit) {
                cache->minX = 0;
                cache->maxX = 0;
                cache->minY = 0;
                cache->maxY = 0;
            }
        } else {
            cache->minX = 0;
            cache->maxX = 0;
            cache->minY = 0;
            cache->maxY = 0;
        }

        if (fabsf(floorScaleX) < RC3D_EPSILON) floorScaleX = 1.0f;
        if (fabsf(floorScaleY) < RC3D_EPSILON) floorScaleY = 1.0f;
        if (fabsf(ceilScaleX) < RC3D_EPSILON) ceilScaleX = 1.0f;
        if (fabsf(ceilScaleY) < RC3D_EPSILON) ceilScaleY = 1.0f;

        cache->floorInvScaleX = 1.0f / floorScaleX;
        cache->floorInvScaleY = 1.0f / floorScaleY;
        cache->ceilInvScaleX = 1.0f / ceilScaleX;
        cache->ceilInvScaleY = 1.0f / ceilScaleY;

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

    if (!g_map || !g_map->walls || !g_map->verts || g_map->wallCount <= 0) {
        return 1;
    }

    g_wallCache = (RC3D_WallCache *)malloc(sizeof(RC3D_WallCache) * (size_t)g_map->wallCount);
    if (!g_wallCache) {
        return 0;
    }

    g_wallCacheCount = g_map->wallCount;

    for (int i = 0; i < g_map->wallCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];
        RC3D_WallCache *wc = &g_wallCache[i];

        memset(wc, 0, sizeof(*wc));
        wc->ownerSector = -1;
        wc->backWallIndex = -1;

        if ((unsigned)w->v0 >= (unsigned)g_map->vertCount ||
            (unsigned)w->v1 >= (unsigned)g_map->vertCount) {
            continue;
        }

        {
            const RC3D_Vec2 *a = &g_map->verts[w->v0];
            const RC3D_Vec2 *b = &g_map->verts[w->v1];
            const float texAngle = rc3dWallTexAngleFromFlags(w->texture_flags);

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
                wc->hasTexRotate = 1u;
            } else {
                wc->texCos = 1.0f;
                wc->texSin = 0.0f;
                wc->hasTexRotate = 0u;
            }

            {
                wc->wallClass = rc3dClassifyWallFlags(w->flags);
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
        }
    }

    for (int s = 0; s < g_map->sectorCount; ++s) {
        const RC3D_Sector *sec = &g_map->sectors[s];
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        if (start < 0 || end < start || end > g_map->wallCount) {
            continue;
        }

        for (int wi = start; wi < end; ++wi) {
            g_wallCache[wi].ownerSector = s;
        }
    }

    for (int i = 0; i < g_map->wallCount; ++i) {
        const RC3D_Wall *w = &g_map->walls[i];

        if (w->neighbour < 0 || w->neighbour >= g_map->sectorCount) {
            continue;
        }

        if (g_wallCache[i].ownerSector < 0) {
            continue;
        }

        {
            const int thisSector = g_wallCache[i].ownerSector;
            const RC3D_Sector *nextSec = &g_map->sectors[w->neighbour];
            const int nextStart = nextSec->wallStart;
            const int nextEnd = nextStart + nextSec->wallCount;

            if (nextStart < 0 || nextEnd < nextStart || nextEnd > g_map->wallCount) {
                continue;
            }

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
    }

    rc3dRefreshDynamicPortalCache();
    return 1;
}


static int rc3dRebuildCurrentMapCaches(void)
{
    if (!rc3dBuildFixedVertCacheForCurrentMap()) {
        rc3dFreeFixedVertCache();
        rc3dFreeWallCache();
        rc3dFreeSectorCache();
        return 0;
    }

    if (!rc3dBuildWallCacheForCurrentMap()) {
        rc3dFreeFixedVertCache();
        rc3dFreeWallCache();
        rc3dFreeSectorCache();
        return 0;
    }

    if (!rc3dBuildSectorCacheForCurrentMap()) {
        rc3dFreeFixedVertCache();
        rc3dFreeWallCache();
        rc3dFreeSectorCache();
        return 0;
    }

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
            wc->portalHasUpper = (sec->ceilHeight > (openTop + RC3D_EPSILON)) ? 1u : 0u;
            wc->portalHasLower = (sec->floorHeight < (openBottom - RC3D_EPSILON)) ? 1u : 0u;
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

    if (map->verts) free((void *)map->verts);
    if (map->walls) free((void *)map->walls);
    if (map->sectors) free((void *)map->sectors);
    if (map->objects) free((void *)map->objects);

    map->verts = NULL;
    map->walls = NULL;
    map->sectors = NULL;
    map->objects = NULL;

    map->vertCount = 0;
    map->wallCount = 0;
    map->sectorCount = 0;
    map->objectCount = 0;

    map->startSector = -1;
    map->startX = 0.0f;
    map->startY = 0.0f;
    map->startAngle = 0.0f;
}



int rc3dMapLoadBinary(const char *path, RC3D_Map *outMap)
{
    FILE *f;
    char magic[8];

    uint32_t vertCount = 0;
    uint32_t wallCount = 0;
    uint32_t sectorCount = 0;
    uint32_t objectCount = 0;

    int32_t startSector;
    float startX;
    float startY;
    float startAngle;

    RC3D_Vec2 *verts = NULL;
    RC3D_Wall *walls = NULL;
    RC3D_Sector *sectors = NULL;
    RC3D_Object *objects = NULL;

    if (!path || !outMap) return 0;

    rc3dMapFreeBinary(outMap);

    f = fopen(path, "rb");
    if (!f) return 0;

    if (!readExact(f, magic, sizeof(magic))) {
        fclose(f);
        return 0;
    }

    // only map version 5 no backwards compatibility wanted
    if (memcmp(magic, "RC3DMAP5", 8) == 0) { } else {
        fclose(f);
        return 0;
    }

    if (!readExact(f, &vertCount, sizeof(vertCount)) ||
        !readExact(f, &wallCount, sizeof(wallCount)) ||
        !readExact(f, &sectorCount, sizeof(sectorCount)) ||
        !readExact(f, &objectCount, sizeof(objectCount))) {
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

    verts = vertCount ? (RC3D_Vec2 *)malloc(sizeof(RC3D_Vec2) * vertCount) : NULL;
    walls = wallCount ? (RC3D_Wall *)malloc(sizeof(RC3D_Wall) * wallCount) : NULL;
    sectors = sectorCount ? (RC3D_Sector *)malloc(sizeof(RC3D_Sector) * sectorCount) : NULL;
    //objects = objectCount ? (RC3D_Object *)malloc(sizeof(RC3D_Object) * objectCount) : NULL;
    objects = objectCount ? (RC3D_Object *)calloc(objectCount, sizeof(RC3D_Object)) : NULL;

    if ((vertCount && !verts) ||
        (wallCount && !walls) ||
        (sectorCount && !sectors) ||
        (objectCount && !objects)) {
        fclose(f);
        free(verts);
        free(walls);
        free(sectors);
        free(objects);
        return 0;
    }

    for (uint32_t i = 0; i < vertCount; i++) {
        if (!readExact(f, &verts[i].x, sizeof(float)) ||
            !readExact(f, &verts[i].y, sizeof(float))) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
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
            !readExact(f, &walls[i].texture_flags, sizeof(uint32_t)) ||
            !readExact(f, &walls[i].texScaleX, sizeof(float)) ||
            !readExact(f, &walls[i].texScaleY, sizeof(float))) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
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
            !readExact(f, &sectors[i].glowlevel, sizeof(uint8_t)) ||
            !readExact(f, &sectors[i].texFlags, sizeof(uint8_t)) ||
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
            !readExact(f, &sectors[i].ceilTexAngle, sizeof(float))  ||
            !readExact(f, &sectors[i].sectorFlags, sizeof(uint32_t))
        ) {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
            return 0;
        }
        sectors[i].glowlevel = rc3dClampGlowLevel(sectors[i].glowlevel);
        sectors[i].wallStart = (int)wallStart;
        sectors[i].wallCount = (int)wallCount_i;
        sectors[i].boundaryCount = (int)boundaryCount;
        sectors[i].originalLightLevel = sectors[i].glowlevel;
        sectors[i].PulsatingLightTimeDir = 1;
        //sectors[i].texFlags = 0xf;  // everything is clamped for now ;)
        
    }

    for (uint32_t i = 0; i < objectCount; i++) {
        int32_t tagId;
        int32_t targTagId;
        uint32_t oflags;
        uint32_t otype;

        if (!readExact(f, &objects[i].x, sizeof(float)) ||
            !readExact(f, &objects[i].y, sizeof(float)) ||
            !readExact(f, &objects[i].z, sizeof(float)) ||
            !readExact(f, &tagId, sizeof(int32_t)) ||
            !readExact(f, &targTagId, sizeof(int32_t)) ||
            !readExact(f, &oflags, sizeof(uint32_t)) ||
            !readExact(f, &otype, sizeof(uint32_t)) ||
            !readExact(f, &objects[i].radius, sizeof(float)) ||
            !readExact(f, &objects[i].textureId, sizeof(uint8_t)) ||
            !readExact(f, &objects[i].inFlag, sizeof(uint8_t)) ||
            !readExact(f, &objects[i].outFlag, sizeof(uint8_t)) ||
            !readExact(f, &objects[i].scalex, sizeof(float)) || 
            !readExact(f, &objects[i].scaley, sizeof(float)) 
        )
        {
            fclose(f);
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
            return 0;
        }

        objects[i].tagId = (int)tagId;

        /* only keep these if RC3D_Object actually has them */
        objects[i].targetTagId = (int)targTagId;
        objects[i].flags = oflags;
        objects[i].type = otype;

        objects[i].trigger = 0;

        if (objects[i].radius < 0.01f) {
            objects[i].radius = 0.25f;
        }
    }

    fclose(f);

    if (startSector < -1 || startSector >= (int32_t)sectorCount) {
        free(verts);
        free(walls);
        free(sectors);
        free(objects);
        return 0;
    }

    for (uint32_t i = 0; i < wallCount; i++) {
        if (walls[i].v0 < 0 || walls[i].v0 >= (int)vertCount ||
            walls[i].v1 < 0 || walls[i].v1 >= (int)vertCount) {
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
            return 0;
        }

        if (walls[i].neighbour < -1 || walls[i].neighbour >= (int)sectorCount) {
            free(verts);
            free(walls);
            free(sectors);
            free(objects);
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
            free(objects);
            return 0;
        }
    }

    outMap->verts = verts;
    outMap->vertCount = (int)vertCount;
    outMap->walls = walls;
    outMap->wallCount = (int)wallCount;
    outMap->sectors = sectors;
    outMap->sectorCount = (int)sectorCount;
    outMap->objects = objects;
    outMap->objectCount = (int)objectCount;
    outMap->startSector = (int)startSector;
    outMap->startX = startX;
    outMap->startY = startY;
    outMap->startAngle = startAngle;

    return 1;
}




int rc3dLoadMapBinary(const char *path)
{
    RC3D_Map newMap;
    memset(&newMap, 0, sizeof(newMap));

    newMap.startSector = -1;

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

    if (!rc3dRebuildCurrentMapCaches()) {
        rc3dMapFreeBinary(&g_loadedMap);
        g_loadedMapValid = 0;
        g_map = &g_rc3dDemoMap;

        if (!rc3dRebuildCurrentMapCaches()) {
            fprintf(stderr, "rc3dLoadMapBinary: failed to rebuild caches for loaded map and fallback map\n");
        }

        return 0;
    }

    rc3dResetPlayerFromMapStart();
    rc3dClearSprites();
    //rc3dInitTestSprite();




    return 1;
}

void rc3dUnloadMapBinary(void)
{
    if (g_loadedMapValid) {
        rc3dMapFreeBinary(&g_loadedMap);
        g_loadedMapValid = 0;
    }

    g_map = &g_rc3dDemoMap;
    if (!rc3dRebuildCurrentMapCaches()) {
        fprintf(stderr, "rc3dUnloadMapBinary: failed to rebuild demo map caches\n");
        return;
    }

    rc3dResetPlayerFromMapStart();
    rc3dClearSprites();
    //rc3dInitTestSprite();
}

/* ------------------------------------------------------------------------- */
/* movement / collision                                                      */
/* ------------------------------------------------------------------------- */

static float wrapAngle(float a)
{
    while (a < -(float)M_PI) a += (float)(M_PI * 2.0f);
    while (a > (float)M_PI) a -= (float)(M_PI * 2.0f);
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
        g_camStepConst = (g_viewport_width > 1) ? (2.0f / (float)(g_viewport_width - 1)) : 0.0f;
    } else {
        g_projPlaneConst = 0.0f;
        g_camStepConst = 0.0f;
    }
}

static int pointInSectorFixed(RC3D_Fixed px, RC3D_Fixed py, int sectorIndex)
{
    if (!g_map || !g_map->sectors || !g_map->walls || !g_map->verts) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if (!g_fixedVerts || g_fixedVertCount != g_map->vertCount) {
        return 0;
    }

    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_SectorCache *secCache =
        (g_sectorCache && (unsigned)sectorIndex < (unsigned)g_sectorCacheCount)
            ? &g_sectorCache[sectorIndex] : NULL;
    const RC3D_Wall *walls = g_map->walls;

    int inside = 0;
    const int start = sec->wallStart;
    const int end = start + sec->boundaryCount;

    if (start < 0 || end < start || end > g_map->wallCount) {
        return 0;
    }

    if (secCache) {
        if (px < secCache->minX || px > secCache->maxX ||
            py < secCache->minY || py > secCache->maxY)
        {
            return 0;
        }
    }

    for (int wi = start; wi < end; ++wi) {
        const RC3D_Wall *w = &walls[wi];

        if ((unsigned)w->v0 >= (unsigned)g_fixedVertCount ||
            (unsigned)w->v1 >= (unsigned)g_fixedVertCount) {
            continue;
        }

        const RC3D_Fixed ax = g_fixedVerts[w->v0].x;
        const RC3D_Fixed ay = g_fixedVerts[w->v0].y;
        const RC3D_Fixed bx = g_fixedVerts[w->v1].x;
        const RC3D_Fixed by = g_fixedVerts[w->v1].y;

        if ((ay > py) != (by > py)) {
            const int64_t dy = (int64_t)(by - ay);
            const int64_t xHit = (int64_t)ax + (((int64_t)(py - ay) * (int64_t)(bx - ax)) / dy);

            if ((int64_t)px < xHit) {
                inside ^= 1;
            }
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

    outView->hasUpper = (sec->ceilHeight >  (outView->openTop + RC3D_EPSILON)) ? 1u : 0u;
    outView->hasLower = (sec->floorHeight < (outView->openBottom - RC3D_EPSILON)) ? 1u : 0u;

    return 1;
}

static int findSectorForPointFixed(RC3D_Fixed x, RC3D_Fixed y)
{
    const float playerFeet = g_player.z - RC3D_PLAYER_EYE_HEIGHT;
    const float playerHeight = PLAYER_HEIGHT;
    const float playerHead = playerFeet + playerHeight;
    const float maxStepUp = PLAYER_STEPUP;

    int bestSector = -1;
    float bestScore = 1e30f;

    if (rc3dSectorIndexValid(g_player.sector)) {
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
    RC3D_Fixed *outClosestY)
{
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
    RC3D_BlockingContact *outContact)
{
    if (!g_map || !g_map->sectors || !g_map->walls) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    {
        const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
        const RC3D_Wall *walls = g_map->walls;
        const int64_t radiusSq = rc3dFixedSq(radius);
        const float radiusF = rc3dFixedToFloat(radius);
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        RC3D_BlockingContact best;
        int64_t bestDistSq = radiusSq;

        if (start < 0 || end < start || end > g_map->wallCount) {
            return 0;
        }

        best.hit = 0;
        best.wallIndex = -1;
        best.normalX = fallbackNX;
        best.normalY = fallbackNY;
        best.penetration = 0.0f;

        for (int wi = start; wi < end; ++wi) {
            const RC3D_Wall *w = &walls[wi];
            RC3D_Fixed ax, ay, bx, by;

            if (!wallBlocksPlayerMovement(w, sectorIndex)) {
                continue;
            }

            if (!rc3dGetWallFixedEndpoints(w, &ax, &ay, &bx, &by)) {
                continue;
            }

            {
                const RC3D_Fixed abx = bx - ax;
                const RC3D_Fixed aby = by - ay;
                const int64_t abLenSq = rc3dFixedSq(abx) + rc3dFixedSq(aby);
                RC3D_Fixed cx = ax;
                RC3D_Fixed cy = ay;
                RC3D_Fixed dx;
                RC3D_Fixed dy;
                int64_t distSq;

                distSq = pointSegmentDistSqFixed(px, py, ax, ay, bx, by, &cx, &cy);

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
                        const float invWallLen = 1.0f / sqrtf((abxF * abxF) + (abyF * abyF));
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
    RC3D_BlockingContact *outContact)
{
    RC3D_BlockingContact best;
    int found = 0;
    float bestPenetration = -1.0f;

    best.hit = 0;
    best.wallIndex = -1;
    best.normalX = fallbackNX;
    best.normalY = fallbackNY;
    best.penetration = 0.0f;

    if (findBlockingContactInSectorFixed(px, py, radius, currentSector, fallbackNX, fallbackNY, &best)) {
        found = 1;
        bestPenetration = best.penetration;
    }

    if (newSector != currentSector && newSector >= 0) {
        RC3D_BlockingContact other;

        if (findBlockingContactInSectorFixed(px, py, radius, newSector, fallbackNX, fallbackNY, &other) &&
            (!found || other.penetration > bestPenetration)) {
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

static int rc3dPositionFitsInSectorFixed(RC3D_Fixed px, RC3D_Fixed py, int sectorIndex)
{
    const RC3D_Fixed radius = rc3dFloatToFixed(PLAYER_RADIUS);

    if (!g_map) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if (!pointInSectorFixed(px, py, sectorIndex)) {
        return 0;
    }

    if (rc3dPositionBlockedInSectorFixed(px, py, radius, sectorIndex)) {
        return 0;
    }

    return 1;
}

static int rc3dTryOrbitalUnstuckInSector(int sectorIndex)
{
    const RC3D_Fixed startX = g_player.xFixed;
    const RC3D_Fixed startY = g_player.yFixed;
    const float stepDist = 0.05f;
    const float maxDist = PLAYER_RADIUS;// + 0.30f;
    const int samples = 24;

    float baseAngle = 0.0f;

    if (!g_map) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    if (rc3dPositionFitsInSectorFixed(startX, startY, sectorIndex)) {
        return 1;
    }

    {
        RC3D_BlockingContact contact;

        if (findBlockingContactInSectorFixed(
                startX,
                startY,
                rc3dFloatToFixed(PLAYER_RADIUS),
                sectorIndex,
                0.0f,
                0.0f,
                &contact))
        {
            baseAngle = atan2f(contact.normalY, contact.normalX);
        }
    }

    for (float dist = stepDist; dist <= (maxDist + 0.0001f); dist += stepDist) {
        for (int i = 0; i < samples; ++i) {
            float angle;

            if (i == 0) {
                angle = baseAngle;
            } else {
                const int half = (i + 1) >> 1;
                const float sign = (i & 1) ? 1.0f : -1.0f;
                angle = baseAngle + (sign * (((float)half / (float)samples) * (float)(M_PI * 2.0f)));
            }

            {
                const float dx = cosf(angle) * dist;
                const float dy = sinf(angle) * dist;
                const RC3D_Fixed testX = startX + rc3dFloatToFixed(dx);
                const RC3D_Fixed testY = startY + rc3dFloatToFixed(dy);

                if (!rc3dPositionFitsInSectorFixed(testX, testY, sectorIndex)) {
                    continue;
                }

                rc3dSetPlayerWorldXYFixed(testX, testY);
                g_player.sector = sectorIndex;
                return 1;
            }
        }
    }

    return 0;
}

static void rc3dResolvePlayerPenetrationInSector(int sectorIndex)
{
    const RC3D_Fixed radius = rc3dFloatToFixed(PLAYER_RADIUS);

    if (!g_map) {
        return;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return;
    }

    for (int iter = 0; iter < 4; ++iter) {
        RC3D_BlockingContact contact;

        if (rc3dPositionFitsInSectorFixed(g_player.xFixed, g_player.yFixed, sectorIndex)) {
            return;
        }

        if (!findBlockingContactInSectorFixed(
                g_player.xFixed,
                g_player.yFixed,
                radius,
                sectorIndex,
                0.0f,
                0.0f,
                &contact))
        {
            break;
        }

        if (contact.penetration <= 0.0f) {
            break;
        }

        {
            const float pushDist = contact.penetration + RC3D_COLLISION_SKIN + RC3D_EPSILON;
            const RC3D_Fixed pushX = rc3dFloatToFixed(contact.normalX * pushDist);
            const RC3D_Fixed pushY = rc3dFloatToFixed(contact.normalY * pushDist);

            if ((pushX == 0) && (pushY == 0)) {
                break;
            }

            rc3dSetPlayerWorldXYFixed(g_player.xFixed + pushX, g_player.yFixed + pushY);
        }
    }

    if (!rc3dPositionFitsInSectorFixed(g_player.xFixed, g_player.yFixed, sectorIndex)) {
        rc3dTryOrbitalUnstuckInSector(sectorIndex);
    }
}

static int rc3dSectorNeedsDropResolve(int oldSector, int newSector)
{
    if (!rc3dSectorIndexValid(oldSector) || !rc3dSectorIndexValid(newSector)) {
        return 0;
    }

    return g_map->sectors[newSector].floorHeight <
           (g_map->sectors[oldSector].floorHeight - RC3D_EPSILON);
}

static int rc3dCommitPlayerMoveFixed(RC3D_Fixed newX, RC3D_Fixed newY, int newSector, int oldSector)
{
    if (newSector < 0) {
        return 0;
    }

    rc3dSetPlayerWorldXYFixed(newX, newY);
    g_player.sector = newSector;

    if (rc3dSectorNeedsDropResolve(oldSector, newSector)) {
        rc3dResolvePlayerPenetrationInSector(newSector);
    }

    return 1;
}





static int rc3dPositionBlockedInSectorFixed(
    RC3D_Fixed px,
    RC3D_Fixed py,
    RC3D_Fixed radius,
    int sectorIndex)
{
    if (!g_map || !g_map->sectors || !g_map->walls) {
        return 0;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 0;
    }

    {
        const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
        const RC3D_Wall *walls = g_map->walls;
        const int64_t radiusSq = rc3dFixedSq(radius);
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        if (start < 0 || end < start || end > g_map->wallCount) {
            return 0;
        }

        for (int wi = start; wi < end; ++wi) {
            const RC3D_Wall *w = &walls[wi];
            RC3D_Fixed ax, ay, bx, by;

            if (!wallBlocksPlayerMovement(w, sectorIndex)) {
                continue;
            }

            if (!rc3dGetWallFixedEndpoints(w, &ax, &ay, &bx, &by)) {
                continue;
            }

            if (pointSegmentDistSqFixed(px, py, ax, ay, bx, by, NULL, NULL) < radiusSq) {
                return 1;
            }
        }
    }

    return 0;
}

static int canMoveToPositionFixed(RC3D_Fixed px, RC3D_Fixed py, int newSector)
{
    const RC3D_Fixed radius = rc3dFloatToFixed(PLAYER_RADIUS);
    int skipNewSectorWallCheck = 0;
    int blocked = 0;

    if (newSector < 0) {
        return 0;
    }

    if (newSector != g_player.sector) {
        if (rc3dSectorIndexValid(g_player.sector) && rc3dSectorIndexValid(newSector)) {
            const float fromFloor = g_map->sectors[g_player.sector].floorHeight;
            const float toFloor = g_map->sectors[newSector].floorHeight;

            if (toFloor < (fromFloor - RC3D_EPSILON)) {
                skipNewSectorWallCheck = 1;
            }
        }
    }

    if (rc3dPositionBlockedInSectorFixed(px, py, radius, g_player.sector)) {
        blocked = 1;
    }

    if (!skipNewSectorWallCheck && newSector != g_player.sector) {
        if (rc3dPositionBlockedInSectorFixed(px, py, radius, newSector)) {
            blocked = 1;
        }
    }

    if (!blocked) {
        return 1;
    }

    {
        RC3D_BlockingContact currentContact;
        RC3D_BlockingContact nextContact;

        const int currentHit = findBlockingContactAtPositionFixed(
            g_player.xFixed,
            g_player.yFixed,
            radius,
            g_player.sector,
            g_player.sector,
            0.0f,
            0.0f,
            &currentContact);

        const int nextHit = findBlockingContactAtPositionFixed(
            px,
            py,
            radius,
            g_player.sector,
            skipNewSectorWallCheck ? g_player.sector : newSector,
            0.0f,
            0.0f,
            &nextContact);

        if (currentHit) {
            if (!nextHit) {
                return 1;
            }

            if (nextContact.penetration < (currentContact.penetration - 0.0025f)) {
                return 1;
            }
        }
    }

    return 0;
}

static int tryMovePlayerSliding(float moveX, float moveY)
{
    const int oldSector = g_player.sector;
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

    {
        const RC3D_Fixed tryX = startX + moveXFixed;
        const RC3D_Fixed tryY = startY + moveYFixed;
        const int newSector = findSectorForPointFixed(tryX, tryY);

        if (canMoveToPositionFixed(tryX, tryY, newSector)) {
            return rc3dCommitPlayerMoveFixed(tryX, tryY, newSector, oldSector);
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
                    const RC3D_Fixed slideTryX = rc3dFloatToFixed(startXFloat + slideX + nudgeX);
                    const RC3D_Fixed slideTryY = rc3dFloatToFixed(startYFloat + slideY + nudgeY);
                    const int slideSector = findSectorForPointFixed(slideTryX, slideTryY);

                    if (canMoveToPositionFixed(slideTryX, slideTryY, slideSector)) {
                        return rc3dCommitPlayerMoveFixed(slideTryX, slideTryY, slideSector, oldSector);
                    }
                }
            }
        }
    }

    {
        const RC3D_Fixed xOnlyX = startX + moveXFixed;
        const RC3D_Fixed xOnlyY = startY;
        const int xSector = findSectorForPointFixed(xOnlyX, xOnlyY);

        if (canMoveToPositionFixed(xOnlyX, xOnlyY, xSector)) {
            return rc3dCommitPlayerMoveFixed(xOnlyX, xOnlyY, xSector, oldSector);
        }
    }

    {
        const RC3D_Fixed yOnlyX = startX;
        const RC3D_Fixed yOnlyY = startY + moveYFixed;
        const int ySector = findSectorForPointFixed(yOnlyX, yOnlyY);

        if (canMoveToPositionFixed(yOnlyX, yOnlyY, ySector)) {
            return rc3dCommitPlayerMoveFixed(yOnlyX, yOnlyY, ySector, oldSector);
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
/* drawing helpers                                                           */
/* ------------------------------------------------------------------------- */
static inline int rc3dPlaneResolveAxisTexel(
    float coord,
    float minCoord,
    float maxCoord,
    int clampMin,
    int clampMax)
{
    if (clampMin && clampMax) {
        const float span = maxCoord - minCoord;
        float t;

        if (fabsf(span) <= RC3D_EPSILON) {
            return 0;
        }

        t = (coord - minCoord) / span;

        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        return (int)(t * (float)(RC3D_TEX_SIZE - 1) + 0.5f);
    }

    if (clampMin) {
        const int s = (int)floorf((coord - minCoord) * (float)RC3D_TEX_SIZE);
        if (s < 0) return 0;
        return (s & RC3D_TEX_MASK);
    }

    if (clampMax) {
        const int s = (int)floorf((coord - maxCoord) * (float)RC3D_TEX_SIZE) + (RC3D_TEX_SIZE - 1);
        if (s >= RC3D_TEX_SIZE) return RC3D_TEX_SIZE - 1;
        return (s & RC3D_TEX_MASK);
    }

    return ((int)floorf(coord * (float)RC3D_TEX_SIZE) & RC3D_TEX_MASK);
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
    int isCeiling)
{
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
        const float *invTable = &g_invDTable[SCREEN_H];
        const uint8_t *texels = g_rc3dTextures[texId].pix;
        const uint8_t texFlags = sec->texFlags;
        uint8_t *dst = rc3dViewportPixelPtr(sx, y0);

        float rayLen = sqrtf((rayDirX * rayDirX) + (rayDirY * rayDirY)) * RC3D_LIGHT_PLANE_DIST_SCALE;

        if ((unsigned)sx < (unsigned)g_columnRayCacheCount) {
            rayLen = g_columnRayCache[sx].planeLightLen;
        }

        if (g_sectorCache && sec >= g_map->sectors) {
            const ptrdiff_t secIndex = sec - g_map->sectors;
            if (secIndex >= 0 && secIndex < g_sectorCacheCount) {
                secCache = &g_sectorCache[secIndex];
            }
        }

        {
            const int clampX1 = (texFlags & RC3D_SECTORTEX_CLAMPX1) ? 1 : 0;
            const int clampX2 = (texFlags & RC3D_SECTORTEX_CLAMPX2) ? 1 : 0;
            const int clampY1 = (texFlags & RC3D_SECTORTEX_CLAMPY1) ? 1 : 0;
            const int clampY2 = (texFlags & RC3D_SECTORTEX_CLAMPY2) ? 1 : 0;
            const int useClamp = clampX1 || clampX2 || clampY1 || clampY2;

            if (secCache && (isCeiling ? secCache->ceilSimple : secCache->floorSimple)) {
                const float minU = rc3dFixedToFloat(secCache->minX);
                const float maxU = rc3dFixedToFloat(secCache->maxX);
                const float minV = rc3dFixedToFloat(secCache->minY);
                const float maxV = rc3dFixedToFloat(secCache->maxY);

                RC3D_ShadeProfile shadeProfile;
                float lastSampleDist = -999999.0f;

                for (int y = y0; y <= y1; ++y) {
                    const int d = horizon - y;

                    if (d != 0) {
                        const float t = planeFactor * invTable[d];
                        const float sampleDist = fabsf(t) * rayLen;

                        const float uCoord = eyeX + (rayDirX * t);
                        const float vCoord = eyeY + (rayDirY * t);

                        int tx, ty;

                        if (useClamp) {
                            tx = rc3dPlaneResolveAxisTexel(uCoord, minU, maxU, clampX1, clampX2);
                            ty = rc3dPlaneResolveAxisTexel(vCoord, minV, maxV, clampY1, clampY2);
                        } else {
                            tx = ((int)floorf(uCoord * (float)RC3D_TEX_SIZE) & RC3D_TEX_MASK);
                            ty = ((int)floorf(vCoord * (float)RC3D_TEX_SIZE) & RC3D_TEX_MASK);
                        }

                        if (fabsf(sampleDist - lastSampleDist) > 0.25f) {
                            rc3dBuildShadeProfile(sampleDist, glowLevel, &shadeProfile);
                            lastSampleDist = sampleDist;
                        }

                        *dst = rc3dApplyShadeProfileToTexel(
                            texels[(ty * RC3D_TEX_SIZE) + tx],
                            tx,
                            ty,
                            texId,
                            &shadeProfile);
                    }

                    dst += SCREEN_W;
                }

                return;
            }

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

                float minU = 0.0f, maxU = 0.0f, minV = 0.0f, maxV = 0.0f;

                if (secCache && useClamp) {
                    const float x0 = rc3dFixedToFloat(secCache->minX);
                    const float x1 = rc3dFixedToFloat(secCache->maxX);
                    const float y0b = rc3dFixedToFloat(secCache->minY);
                    const float y1b = rc3dFixedToFloat(secCache->maxY);

                    const float u0 = ((x0 * ca) + (y0b * sa)) * invScaleX;
                    const float v0 = (-(x0 * sa) + (y0b * ca)) * invScaleY;

                    const float u1 = ((x1 * ca) + (y0b * sa)) * invScaleX;
                    const float v1 = (-(x1 * sa) + (y0b * ca)) * invScaleY;

                    const float u2 = ((x0 * ca) + (y1b * sa)) * invScaleX;
                    const float v2 = (-(x0 * sa) + (y1b * ca)) * invScaleY;

                    const float u3 = ((x1 * ca) + (y1b * sa)) * invScaleX;
                    const float v3 = (-(x1 * sa) + (y1b * ca)) * invScaleY;

                    minU = u0; maxU = u0;
                    minV = v0; maxV = v0;

                    if (u1 < minU) minU = u1; if (u1 > maxU) maxU = u1;
                    if (u2 < minU) minU = u2; if (u2 > maxU) maxU = u2;
                    if (u3 < minU) minU = u3; if (u3 > maxU) maxU = u3;

                    if (v1 < minV) minV = v1; if (v1 > maxV) maxV = v1;
                    if (v2 < minV) minV = v2; if (v2 > maxV) maxV = v2;
                    if (v3 < minV) minV = v3; if (v3 > maxV) maxV = v3;
                }

                RC3D_ShadeProfile shadeProfile;
                float lastSampleDist = -999999.0f;

                for (int y = y0; y <= y1; ++y) {
                    const int d = horizon - y;

                    if (d != 0) {
                        const float t = planeFactor * invTable[d];
                        const float sampleDist = fabsf(t) * rayLen;
                        const float wx = eyeX + (rayDirX * t);
                        const float wy = eyeY + (rayDirY * t);

                        const float uCoord = ((wx * ca) + (wy * sa)) * invScaleX;
                        const float vCoord = (-(wx * sa) + (wy * ca)) * invScaleY;

                        int tx, ty;

                        if (useClamp) {
                            tx = rc3dPlaneResolveAxisTexel(uCoord, minU, maxU, clampX1, clampX2);
                            ty = rc3dPlaneResolveAxisTexel(vCoord, minV, maxV, clampY1, clampY2);
                        } else {
                            tx = ((int)floorf(uCoord * (float)RC3D_TEX_SIZE) & RC3D_TEX_MASK);
                            ty = ((int)floorf(vCoord * (float)RC3D_TEX_SIZE) & RC3D_TEX_MASK);
                        }

                        if (fabsf(sampleDist - lastSampleDist) > 0.25f) {
                            rc3dBuildShadeProfile(sampleDist, glowLevel, &shadeProfile);
                            lastSampleDist = sampleDist;
                        }

                        *dst = rc3dApplyShadeProfileToTexel(
                            texels[(ty * RC3D_TEX_SIZE) + tx],
                            tx,
                            ty,
                            texId,
                            &shadeProfile);
                    }

                    dst += SCREEN_W;
                }
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
    int clipBottom)
{
    const int origY0 = y0;
    const int origY1 = y1;

    const int clampYT = (texFlags & RC3D_TEX_FLAG_CLAMPYT) ? 1 : 0;
    const int clampYB = (texFlags & RC3D_TEX_FLAG_CLAMPYB) ? 1 : 0;
    const int stretchY = (clampYT && clampYB) ? 1 : 0;
    const int flipY = (texFlags & RC3D_TEX_FLAG_FLIPY) ? 1 : 0;

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
            const float baseWorldFromTop = vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            {
                const float sample0 = baseWorldFromTop + ((float)y0 * k);

                if (stretchY) {
                    const float scale = texPerWorld / worldSpan;

                    if (flipY) {
                        vLocal0 = ((worldSpan - sample0) * scale);
                        vStep = -(k * scale);
                    } else {
                        vLocal0 = sample0 * scale;
                        vStep = k * scale;
                    }

                } else if (clampYB && !clampYT) {
                    if (flipY) {
                        vLocal0 = (worldSpan - sample0) * texPerWorld;
                        vStep = -(k * texPerWorld);
                    } else {
                        vLocal0 = ((sample0 - worldSpan) * texPerWorld) +
                                (float)(RC3D_TEX_SIZE - 1);
                        vStep = k * texPerWorld;
                    }

                } else {
                    if (flipY) {
                        vLocal0 = -sample0 * texPerWorld;
                        vStep = -(k * texPerWorld);
                    } else {
                        vLocal0 = sample0 * texPerWorld;
                        vStep = k * texPerWorld;
                    }
                }
            }

            {
                const uint8_t *texels = g_rc3dTextures[texId].pix;
                uint8_t *dst = rc3dViewportPixelPtr(sx, y0);
                //const float shadeDist = hitDist * RC3D_LIGHT_WALL_DIST_SCALE;
                const float shadeDist = (hitDist * RC3D_LIGHT_WALL_DIST_SCALE) + g_wallShadeBiasGlobal;
                RC3D_ShadeProfile shadeProfile;

                rc3dBuildShadeProfile(shadeDist, wallGlow, &shadeProfile);

                if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f) &&
                    (wallTexRotCosGlobal > 0.9999f) && (wallTexRotCosGlobal < 1.0001f))
                {
                    const int32_t uFixed = (int32_t)(wallTexUBaseGlobal * 65536.0f);
                    int32_t vFixed = (int32_t)(vLocal0 * 65536.0f);
                    const int32_t vStepFixed = (int32_t)(vStep * 65536.0f);
                    const int tx = (uFixed >> 16);

                    for (int y = y0; y <= y1; ++y) {
                        const int ty = (vFixed >> 16);
                        const uint8_t texel = texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

                        *dst = rc3dApplyShadeProfileToTexel(texel, tx, ty, texId, &shadeProfile);
                        dst += SCREEN_W;
                        vFixed += vStepFixed;
                    }
                } else {
                    float uRotF = (wallTexUBaseGlobal * wallTexRotCosGlobal) - (vLocal0 * wallTexRotSinGlobal);
                    float vRotF = (wallTexUBaseGlobal * wallTexRotSinGlobal) + (vLocal0 * wallTexRotCosGlobal);

                    const float uStepF = -vStep * wallTexRotSinGlobal;
                    const float vStepF = vStep * wallTexRotCosGlobal;

                    int32_t uRot = (int32_t)(uRotF * 65536.0f);
                    int32_t vRot = (int32_t)(vRotF * 65536.0f);
                    const int32_t uStep = (int32_t)(uStepF * 65536.0f);
                    const int32_t vStepRot = (int32_t)(vStepF * 65536.0f);

                    for (int y = y0; y <= y1; ++y) {
                        const int tx = (uRot >> 16);
                        const int ty = (vRot >> 16);
                        const uint8_t texel = texels[((ty & RC3D_TEX_MASK) * RC3D_TEX_SIZE) + (tx & RC3D_TEX_MASK)];

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
    int clipBottom)
{
    const int origY0 = y0;
    const int origY1 = y1;

    const int clampYT = (texFlags & RC3D_TEX_FLAG_CLAMPYT) ? 1 : 0;
    const int clampYB = (texFlags & RC3D_TEX_FLAG_CLAMPYB) ? 1 : 0;
    const int stretchY = (clampYT && clampYB) ? 1 : 0;
    const int flipY = (texFlags & RC3D_TEX_FLAG_FLIPY) ? 1 : 0;

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
            const float baseWorldFromTop = vTopWorld - eyeZ - ((float)horizonGlobal * k);

            float vLocal0;
            float vStep;

            {
                const float sample0 = baseWorldFromTop + ((float)y0 * k);

                if (stretchY) {
                    const float scale = texPerWorld / worldSpan;

                    if (flipY) {
                        vLocal0 = ((worldSpan - sample0) * scale);
                        vStep = -(k * scale);
                    } else {
                        vLocal0 = sample0 * scale;
                        vStep = k * scale;
                    }

                } else if (clampYB && !clampYT) {
                    if (flipY) {
                        vLocal0 = (worldSpan - sample0) * texPerWorld;
                        vStep = -(k * texPerWorld);
                    } else {
                        vLocal0 = ((sample0 - worldSpan) * texPerWorld) +
                                (float)(RC3D_TEX_SIZE - 1);
                        vStep = k * texPerWorld;
                    }

                } else {
                    if (flipY) {
                        vLocal0 = -sample0 * texPerWorld;
                        vStep = -(k * texPerWorld);
                    } else {
                        vLocal0 = sample0 * texPerWorld;
                        vStep = k * texPerWorld;
                    }
                }
            }

            {
                const uint8_t *texels = g_rc3dTextures[texId].pix;
                uint8_t *dst = rc3dViewportPixelPtr(sx, y0);
                int opaqueSpanStart = -1;
                //const float shadeDist = hitDist * RC3D_LIGHT_WALL_DIST_SCALE;
                const float shadeDist = (hitDist * RC3D_LIGHT_WALL_DIST_SCALE) + g_wallShadeBiasGlobal;
                RC3D_ShadeProfile shadeProfile;

                rc3dBuildShadeProfile(shadeDist, wallGlow, &shadeProfile);

                if ((wallTexRotSinGlobal > -0.0001f) && (wallTexRotSinGlobal < 0.0001f) &&
                    (wallTexRotCosGlobal > 0.9999f) && (wallTexRotCosGlobal < 1.0001f))
                {
                    const int32_t uFixed = (int32_t)(wallTexUBaseGlobal * 65536.0f);
                    int32_t vFixed = (int32_t)(vLocal0 * 65536.0f);
                    const int32_t vStepFixed = (int32_t)(vStep * 65536.0f);
                    const int tx = (uFixed >> 16);

                    for (int y = y0; y <= y1; ++y) {
                        const int ty = (vFixed >> 16);
                        const uint8_t texel = rc3dApplyShadeProfileToTexel(
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
                } else {
                    float uRotF = (wallTexUBaseGlobal * wallTexRotCosGlobal) - (vLocal0 * wallTexRotSinGlobal);
                    float vRotF = (wallTexUBaseGlobal * wallTexRotSinGlobal) + (vLocal0 * wallTexRotCosGlobal);

                    const float uStepF = -vStep * wallTexRotSinGlobal;
                    const float vStepF = vStep * wallTexRotCosGlobal;

                    int32_t uRot = (int32_t)(uRotF * 65536.0f);
                    int32_t vRot = (int32_t)(vRotF * 65536.0f);
                    const int32_t uStep = (int32_t)(uStepF * 65536.0f);
                    const int32_t vStepRot = (int32_t)(vStepF * 65536.0f);

                    for (int y = y0; y <= y1; ++y) {
                        const int tx = (uRot >> 16);
                        const int ty = (vRot >> 16);
                        const uint8_t texel = rc3dApplyShadeProfileToTexel(
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




static inline RC3D_WallHit findNearestWallInSector(
    int sectorIndex,
    float rox, float roy,
    float rdx, float rdy,
    int ignoreWallIndexA,
    int ignoreWallIndexB,
    float minT,
    int preferredWallIndex)
{
    RC3D_WallHit hit;
    const RC3D_Sector *sec;
    const RC3D_Wall *walls;
    const RC3D_Vec2 *verts;
    const RC3D_WallCache *cache;
    int start, end;

    hit.t = g_draw_distance;
    hit.wallIndex = -1;
    hit.hit = 0;

    if (!g_map || !g_map->sectors || !g_map->walls || !g_map->verts) {
        return hit;
    }

    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return hit;
    }

    sec = &g_map->sectors[sectorIndex];
    walls = g_map->walls;
    verts = g_map->verts;
    cache = g_wallCache;

    start = sec->wallStart;
    end   = start + sec->wallCount;

    if (start < 0 || end < start || end > g_map->wallCount) {
        return hit;
    }

    if ((preferredWallIndex >= start) &&
        (preferredWallIndex < end) &&
        (preferredWallIndex != ignoreWallIndexA) &&
        (preferredWallIndex != ignoreWallIndexB))
    {
        const RC3D_Wall *w = &walls[preferredWallIndex];
        const RC3D_Vec2 *a = &verts[w->v0];
        float sx, sy;
        float denom, qpx, qpy, invDenom, t, u;
        int doubleSided = 0;

        if (cache) {
            sx = cache[preferredWallIndex].dx;
            sy = cache[preferredWallIndex].dy;
        } else {
            const RC3D_Vec2 *b = &verts[w->v1];
            sx = b->x - a->x;
            sy = b->y - a->y;
        }

        /* portals must be traceable from either side */
        if (w->flags & RC3D_WALL_PORTAL) {
            doubleSided = 1;
        }

        /* optional explicit double-sided flag */
        if (w->flags & RC3D_WALL_DOUBLESIDED) {
            doubleSided = 1;
        }

        RC3D_PROFILER_DO(g_profiler.wallTests++);

        denom = (rdx * sy) - (rdy * sx);

        if (doubleSided) {
            if (fabsf(denom) <= RC3D_EPSILON) {
                goto skip_preferred_wall;
            }
        } else {
            if (denom >= -RC3D_EPSILON) {
                goto skip_preferred_wall;
            }
        }

        qpx = a->x - rox;
        qpy = a->y - roy;
        invDenom = 1.0f / denom;
        t = ((qpx * sy) - (qpy * sx)) * invDenom;

        if (t > minT && t < hit.t) {
            u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
            if (u >= 0.0f && u <= 1.0f) {
                hit.t = t;
                hit.wallIndex = preferredWallIndex;
                hit.hit = 1;
            }
        }
    }

skip_preferred_wall:
    for (int wallIndex = start; wallIndex < end; ++wallIndex) {
        const RC3D_Wall *w;
        const RC3D_Vec2 *a;
        float sx, sy;
        float denom, qpx, qpy, invDenom, t, u;
        int doubleSided = 0;

        if (wallIndex == preferredWallIndex) continue;
        if (wallIndex == ignoreWallIndexA || wallIndex == ignoreWallIndexB) continue;

        w = &walls[wallIndex];
        a = &verts[w->v0];

        if (cache) {
            sx = cache[wallIndex].dx;
            sy = cache[wallIndex].dy;
        } else {
            const RC3D_Vec2 *b = &verts[w->v1];
            sx = b->x - a->x;
            sy = b->y - a->y;
        }

        /* portals must be traceable from either side */
        if (w->flags & RC3D_WALL_PORTAL) {
            doubleSided = 1;
        }

        /* optional explicit double-sided flag */
        if (w->flags & RC3D_WALL_DOUBLESIDED) {
            doubleSided = 1;
        }

        RC3D_PROFILER_DO(g_profiler.wallTests++);

        denom = (rdx * sy) - (rdy * sx);

        if (doubleSided) {
            if (fabsf(denom) <= RC3D_EPSILON) continue;
        } else {
            if (denom >= -RC3D_EPSILON) continue;
        }

        qpx = a->x - rox;
        qpy = a->y - roy;
        invDenom = 1.0f / denom;
        t = ((qpx * sy) - (qpy * sx)) * invDenom;

        if (t <= minT || t >= hit.t) continue;

        u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
        if (u < 0.0f || u > 1.0f) continue;

        hit.t = t;
        hit.wallIndex = wallIndex;
        hit.hit = 1;
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
    float rayDirY)
{
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
                1);
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
                0);
        }
    }
}

static void drawBackground(void)
{
    const float fullTurn = (float)(M_PI * 2.0f);
    const float leftAng = g_player.angle - g_halfFovRad;
    const float skyRepeat = 2.0f;
    const int32_t oneFixed = 1 << 16;
    const int viewportHalfHeight = g_viewport_height / 1.50;

    if (g_viewport_width <= 0 || viewportHalfHeight <= 0) {
        return;
    }

    const float uStepRaw = (((g_halfFovRad * 2.0f) / (float)g_viewport_width) / fullTurn) * skyRepeat;

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


static int g_traceAllowBackSectorPlaneFills = 0;
//recursive function, CEARLY with the STACK!! GOBBLE GOBBLE
static inline void renderColumnTrace(
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
    int *outFirstHitWallIndex)
{
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_renderEyeZ;

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) 
    {
        RC3D_PROFILER_DO(g_profiler.portalSteps++);

        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return;
        }

        if (clipTop > clipBottom) {
            return;
        }

        {
            const RC3D_Sector *sec = &g_map->sectors[currentSector];
            const int stepPreferredWallIndex = (step == 0) ? preferredWallIndex : -1;
            const int allowNormalPlaneFills = !g_traceSuppressSectorPlanes;

            if (g_traceSuppressSectorPlanes && g_traceAllowBackSectorPlaneFills) {
                fillSectorColumnSpan(sx, clipTop, clipBottom, sec, horizon, rdx, rdy);
            }
            const RC3D_WallHit hit = findNearestWallInSector(
                currentSector,
                playerX, playerY,
                rdx, rdy,
                ignoreWallIndexA,
                ignoreWallIndexB,
                rayMinT,
                stepPreferredWallIndex);

            if (!hit.hit) {
                rc3dRecordCachedVisibleTraceSegment(sx, currentSector, clipTop, clipBottom, UINT16_MAX);

                if (allowNormalPlaneFills) {
                    fillSectorColumnSpan(sx, clipTop, clipBottom, sec, horizon, rdx, rdy);
                }
                return;
            }

            if (outFirstHitWallIndex && step == 0) {
                *outFirstHitWallIndex = hit.wallIndex;
            }

            {
                const RC3D_Wall *w = &g_map->walls[hit.wallIndex];
                const RC3D_WallCache *wc = (g_wallCache && hit.wallIndex >= 0 && hit.wallIndex < g_wallCacheCount) ? &g_wallCache[hit.wallIndex] : NULL;

                const RC3D_Vec2 *va = &g_map->verts[w->v0];
                const RC3D_Vec2 *vb = &g_map->verts[w->v1];

                const float hitX = playerX + (rdx * hit.t);
                const float hitY = playerY + (rdy * hit.t);
                const float correctedDist = hit.t;

                if (correctedDist <= RC3D_EPSILON) {
                    return;
                }

                rc3dRecordCachedVisibleTraceSegment(
                    sx,
                    currentSector,
                    clipTop,
                    clipBottom,
                    rc3dEncodeDepth(correctedDist));

                {
                    const float wallDx = wc ? wc->dx : (vb->x - va->x);
                    const float wallDy = wc ? wc->dy : (vb->y - va->y);
                    float wallTexInvScaleX = wc ? wc->texInvScaleX : (1.0f / ((fabsf(w->texScaleX) < RC3D_EPSILON) ? 1.0f : w->texScaleX));
                    float wallTexInvScaleY = wc ? wc->texInvScaleY : (1.0f / ((fabsf(w->texScaleY) < RC3D_EPSILON) ? 1.0f : w->texScaleY));

                    //wallTexInvScaleX *= -1.0f;

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

                    {
                        const int flipX = (w->texture_flags & RC3D_TEX_FLAG_FLIPX) ? 1 : 0;

                        if (wc && wc->texXMode == RC3D_TEX_XMODE_STRETCH) {
                            if (uNorm < 0.0f) uNorm = 0.0f;
                            if (uNorm > 1.0f) uNorm = 1.0f;

                            if (flipX) {
                                uNorm = 1.0f - uNorm;
                            }

                            wallTexUBaseGlobal = uNorm * (float)RC3D_TEX_SIZE;

                        } else if (wc && wc->texXMode == RC3D_TEX_XMODE_CLAMP_RIGHT) {
                            if (flipX) {
                                wallTexUBaseGlobal =
                                    ((wallLen - distAlongWall) * wallTexInvScaleX * (float)RC3D_TEX_SIZE) -
                                    (wallLen * wallTexInvScaleX * (float)RC3D_TEX_SIZE);
                            } else {
                                wallTexUBaseGlobal =
                                    (distAlongWall * wallTexInvScaleX * (float)RC3D_TEX_SIZE) -
                                    (wallLen * wallTexInvScaleX * (float)RC3D_TEX_SIZE);
                            }

                        } else {
                            if (flipX) {
                                wallTexUBaseGlobal =
                                    (wallLen - distAlongWall) * wallTexInvScaleX * (float)RC3D_TEX_SIZE;
                            } else {
                                wallTexUBaseGlobal =
                                    distAlongWall * wallTexInvScaleX * (float)RC3D_TEX_SIZE;
                            }
                        }
                    }

                    {
                        const float wallMidX = (va->x + vb->x) * 0.5f;
                        const float wallMidY = (va->y + vb->y) * 0.5f;

                        g_wallShadeBiasGlobal = rc3dComputeWallShadeBias(
                            wallDx, wallDy,
                            wallMidX, wallMidY,
                            playerX, playerY);
                    }
                }

                {
                    const float scale = projPlane / correctedDist;
                    const int secTop = rc3dProjectTopPixel(horizon, sec->ceilHeight, playerZ, scale);
                    const int secBot = rc3dProjectBottomPixel(horizon, sec->floorHeight, playerZ, scale);

                    //const uint8_t wallClass = wc ? wc->wallClass : RC3D_WALLCLASS_NONE;
                    const uint8_t wallClass = wc ? wc->wallClass : rc3dClassifyWallFlags(w->flags);
                    const int wallTransparent = ((w->flags & RC3D_WALL_TRANSPARENCY) != 0);

                    switch (wallClass) {
                        case RC3D_WALLCLASS_SOLID:
                        {
                            const int midMasked = wallTransparent && rc3dTextureColumnNeedsMaskedTrace(w->midColor);

                            if (allowNormalPlaneFills) {
                                fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                                fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);
                            }

                            if (midMasked && maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH) {
                                const float savedWallTexUBase = wallTexUBaseGlobal;
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
                                const float savedWallTexRotCos = wallTexRotCosGlobal;
                                const float savedWallTexRotSin = wallTexRotSinGlobal;
                                const float savedWallShadeBias = g_wallShadeBiasGlobal;

                                const int maskedClipTop = clampi(secTop, clipTop, clipBottom);
                                const int maskedClipBottom = clampi(secBot, clipTop, clipBottom);

                                if (maskedClipTop <= maskedClipBottom) {
                                    if ((w->flags & RC3D_WALL_PORTAL) &&
                                        ((unsigned)w->neighbour < (unsigned)g_map->sectorCount))
                                    {
                                        int entryWallInNext = -1;

                                        if (wc) {
                                            entryWallInNext = wc->backWallIndex;
                                        }

                                        renderColumnTrace(
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
                                            hit.t,
                                            maskedDepth + 1,
                                            -1,
                                            NULL);
                                    } else {
                                        renderColumnTrace(
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
                                            hit.t,
                                            maskedDepth + 1,
                                            -1,
                                            NULL);
                                    }
                                }

                                wallTexUBaseGlobal = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;
                                g_wallShadeBiasGlobal = savedWallShadeBias;

                                renderMaskedTexturedBandIfVisible(
                                    sx, secTop, secBot,
                                    w->midColor,
                                    w->texture_flags,
                                    sec->ceilHeight,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom);
                            } else {
                                renderTexturedBandIfVisible(
                                    sx, secTop, secBot,
                                    w->midColor,
                                    w->texture_flags,
                                    sec->ceilHeight,
                                    sec->floorHeight,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom);
                            }

                            return;
                        }

                        case RC3D_WALLCLASS_MIDDLE:
                        {
                            const int midTopY = rc3dProjectTopPixel(horizon, w->openTop, playerZ, scale);
                            const int midBotY = rc3dProjectBottomPixel(horizon, w->openBottom, playerZ, scale);
                            const int midMasked = wallTransparent && rc3dTextureColumnNeedsMaskedTrace(w->midColor);

                            if (allowNormalPlaneFills) {
                                fillSectorColumnSpan(sx, clipTop, midTopY - 1, sec, horizon, rdx, rdy);
                                fillSectorColumnSpan(sx, midBotY + 1, clipBottom, sec, horizon, rdx, rdy);
                            }

                            if (midMasked && maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH) {
                                int maskedClipTop = clipTop;
                                int maskedClipBottom = clipBottom;

                                const float savedWallTexUBase = wallTexUBaseGlobal;
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
                                const float savedWallTexRotCos = wallTexRotCosGlobal;
                                const float savedWallTexRotSin = wallTexRotSinGlobal;
                                const float savedWallShadeBias = g_wallShadeBiasGlobal;

                                if (midTopY > maskedClipTop) maskedClipTop = midTopY;
                                if (midBotY < maskedClipBottom) maskedClipBottom = midBotY;

                                if (maskedClipTop <= maskedClipBottom) {
                                    renderColumnTrace(
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
                                        hit.t,
                                        maskedDepth + 1,
                                        -1,
                                        NULL);
                                }

                                wallTexUBaseGlobal = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;
                                g_wallShadeBiasGlobal = savedWallShadeBias;

                                renderMaskedTexturedBandIfVisible(
                                    sx, midTopY, midBotY,
                                    w->midColor,
                                    w->texture_flags,
                                    w->openTop,
                                    w->openBottom,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom);
                            } else {
                                renderTexturedBandIfVisible(
                                    sx, midTopY, midBotY,
                                    w->midColor,
                                    w->texture_flags,
                                    w->openTop,
                                    w->openBottom,
                                    correctedDist,
                                    projPlane,
                                    clipTop, clipBottom);
                            }
                            return;
                        }

                        case RC3D_WALLCLASS_UPPER_LOWER:
                        {
                            const int openTopY = rc3dProjectTopPixel(horizon, w->openTop, playerZ, scale);
                            const int openBotY = rc3dProjectBottomPixel(horizon, w->openBottom, playerZ, scale);
                            const int upperMasked =
                                ((w->flags & RC3D_WALL_UPPER) != 0) &&
                                wallTransparent &&
                                rc3dTextureColumnNeedsMaskedTrace(w->upperColor);
                            const int lowerMasked =
                                ((w->flags & RC3D_WALL_LOWER) != 0) &&
                                wallTransparent &&
                                rc3dTextureColumnNeedsMaskedTrace(w->lowerColor);

                            if (allowNormalPlaneFills) {
                                fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                                fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);
                            }

                            if ((upperMasked || lowerMasked) && maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH) {
                                const int maskedClipTop = clampi(secTop, clipTop, clipBottom);
                                const int maskedClipBottom = clampi(secBot, clipTop, clipBottom);
                                const float savedWallTexUBase = wallTexUBaseGlobal;
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
                                const float savedWallTexRotCos = wallTexRotCosGlobal;
                                const float savedWallTexRotSin = wallTexRotSinGlobal;
                                const float savedWallShadeBias = g_wallShadeBiasGlobal;

                                if (maskedClipTop <= maskedClipBottom) {
                                    renderColumnTrace(
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
                                        NULL);
                                }

                                wallTexUBaseGlobal = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;
                                g_wallShadeBiasGlobal = savedWallShadeBias;
                            }

                            if (w->flags & RC3D_WALL_UPPER) {
                                if (upperMasked) {
                                    renderMaskedTexturedBandIfVisible(
                                        sx, secTop, openTopY - 1,
                                        w->upperColor,
                                        w->texture_flags,
                                        sec->ceilHeight,
                                        w->openTop,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                } else {
                                    renderTexturedBandIfVisible(
                                        sx, secTop, openTopY - 1,
                                        w->upperColor,
                                        w->texture_flags,
                                        sec->ceilHeight,
                                        w->openTop,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                }
                            }

                            if (w->flags & RC3D_WALL_LOWER) {
                                if (lowerMasked) {
                                    renderMaskedTexturedBandIfVisible(
                                        sx, openBotY + 1, secBot,
                                        w->lowerColor,
                                        w->texture_flags,
                                        w->openBottom,
                                        sec->floorHeight,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                } else {
                                    renderTexturedBandIfVisible(
                                        sx, openBotY + 1, secBot,
                                        w->lowerColor,
                                        w->texture_flags,
                                        w->openBottom,
                                        sec->floorHeight,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                }
                            }

                            return;
                        }

                        case RC3D_WALLCLASS_PORTAL:
{
                            const int nextSectorIndex = w->neighbour;
                            RC3D_PortalView portalView;
                            int portalOpenTopY;
                            int portalOpenBotY;
                            int openTop;
                            int openBot;
                            int portalClipTop;
                            int portalClipBottom;

                            if ((unsigned)nextSectorIndex >= (unsigned)g_map->sectorCount) {
                                return;
                            }

                            if (!rc3dBuildPortalView(w, currentSector, &portalView)) {
                                return;
                            }

                            portalOpenTopY = rc3dProjectTopPixel(horizon, portalView.openTop, playerZ, scale);
                            portalOpenBotY = rc3dProjectBottomPixel(horizon, portalView.openBottom, playerZ, scale);

                            openTop = portalOpenTopY;
                            if (openTop < secTop) openTop = secTop;
                            if (openTop < clipTop) openTop = clipTop;

                            openBot = portalOpenBotY;
                            if (openBot > secBot) openBot = secBot;
                            if (openBot > clipBottom) openBot = clipBottom;

                            if (allowNormalPlaneFills) {
                                fillSectorColumnSpan(sx, clipTop, secTop - 1, sec, horizon, rdx, rdy);
                                fillSectorColumnSpan(sx, secBot + 1, clipBottom, sec, horizon, rdx, rdy);
                            }

                            const int upperMasked =
                                portalView.hasUpper &&
                                wallTransparent &&
                                rc3dTextureColumnNeedsMaskedTrace(w->upperColor);
                            const int lowerMasked =
                                portalView.hasLower &&
                                wallTransparent &&
                                rc3dTextureColumnNeedsMaskedTrace(w->lowerColor);
                            const int midMasked =
                                ((w->flags & RC3D_WALL_MIDDLE) != 0) &&
                                wallTransparent &&
                                rc3dTextureColumnNeedsMaskedTrace(w->midColor);
                            const int fullMaskedPortalTrace =
                                (upperMasked || lowerMasked) &&
                                (maskedDepth < RC3D_MAX_MASKED_TRACE_DEPTH);

                            /*
                                Opening owns [openTop .. openBot]
                                Upper wall owns [secTop .. openTop-1]
                                Lower wall owns [openBot+1 .. secBot]
                            */
                            if (fullMaskedPortalTrace) {
                                portalClipTop = secTop;
                                if (portalClipTop < clipTop) portalClipTop = clipTop;

                                portalClipBottom = secBot;
                                if (portalClipBottom > clipBottom) portalClipBottom = clipBottom;
                            } else if (g_traceSuppressSectorPlanes) {
                                portalClipTop = clipTop;
                                portalClipBottom = clipBottom;
                            } else {
                                portalClipTop = openTop;
                                portalClipBottom = openBot;
                            }

                            {
                                int entryWallInNext = -1;
                                const float savedWallTexUBase = wallTexUBaseGlobal;
                                const float savedWallTexInvScaleY = wallTexInvScaleYGlobal;
                                const float savedWallTexRotCos = wallTexRotCosGlobal;
                                const float savedWallTexRotSin = wallTexRotSinGlobal;
                                const float savedWallShadeBias = g_wallShadeBiasGlobal;
                                const int savedSuppressSectorPlanes = g_traceSuppressSectorPlanes;
                                const int savedAllowBackSectorPlaneFills = g_traceAllowBackSectorPlaneFills;

                                if (wc) {
                                    entryWallInNext = wc->backWallIndex;
                                }

                                if (fullMaskedPortalTrace) {
                                    g_traceSuppressSectorPlanes = 1;
                                    g_traceAllowBackSectorPlaneFills = 0;
                                } else if (g_traceSuppressSectorPlanes) {
                                    g_traceSuppressSectorPlanes = 0;
                                    g_traceAllowBackSectorPlaneFills = 0;
                                }

                                if (portalClipTop <= portalClipBottom) {
                                    renderColumnTrace(
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
                                        hit.t + RC3D_EPSILON,
                                        maskedDepth,
                                        -1,
                                        NULL);
                                }

                                g_traceSuppressSectorPlanes = savedSuppressSectorPlanes;
                                g_traceAllowBackSectorPlaneFills = savedAllowBackSectorPlaneFills;
                                wallTexUBaseGlobal = savedWallTexUBase;
                                wallTexInvScaleYGlobal = savedWallTexInvScaleY;
                                wallTexRotCosGlobal = savedWallTexRotCos;
                                wallTexRotSinGlobal = savedWallTexRotSin;
                                g_wallShadeBiasGlobal = savedWallShadeBias;

                            }

                            if (portalView.hasUpper) {
                                if (upperMasked) {
                                    renderMaskedTexturedBandIfVisible(
                                        sx, secTop, openTop - 1,
                                        w->upperColor,
                                        w->texture_flags,
                                        sec->ceilHeight,
                                        portalView.openTop,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                } else {
                                    renderTexturedBandIfVisible(
                                        sx, secTop, openTop - 1,
                                        w->upperColor,
                                        w->texture_flags,
                                        sec->ceilHeight,
                                        portalView.openTop,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                }
                            }

                            if (portalView.hasLower) {
                                if (lowerMasked) {
                                    renderMaskedTexturedBandIfVisible(
                                        sx, openBot + 1, secBot,
                                        w->lowerColor,
                                        w->texture_flags,
                                        portalView.openBottom,
                                        sec->floorHeight,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                } else {
                                    renderTexturedBandIfVisible(
                                        sx, openBot + 1, secBot,
                                        w->lowerColor,
                                        w->texture_flags,
                                        portalView.openBottom,
                                        sec->floorHeight,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                }
                            }

                            if (w->flags & RC3D_WALL_MIDDLE) {
                                if (midMasked) {
                                    renderMaskedTexturedBandIfVisible(
                                        sx, openTop, openBot,
                                        w->midColor,
                                        w->texture_flags,
                                        portalView.openTop,
                                        portalView.openBottom,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                } else {
                                    renderTexturedBandIfVisible(
                                        sx, openTop, openBot,
                                        w->midColor,
                                        w->texture_flags,
                                        portalView.openTop,
                                        portalView.openBottom,
                                        correctedDist,
                                        projPlane,
                                        clipTop, clipBottom);
                                }
                            }

                            return;
                        }

                        default:
                        {
                            if (allowNormalPlaneFills) {
                                fillSectorColumnSpan(sx, clipTop, clipBottom, sec, horizon, rdx, rdy);
                            }
                            return;
                        }
                    }
                }
            }
        }
    }
}


static void rc3dBuildVisibleTraceForColumn(int sx)
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

    if (!g_columnContext.visibleTrace || !g_columnContext.visibleTraceCount) {
        return;
    }

    *g_columnContext.visibleTraceCount = 0u;

    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return;
    }

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
        planeY = dirX * g_planeScaleConst;
        rdx = dirX + (planeX * cameraX);
        rdy = dirY + (planeY * cameraX);
    }

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step)
    {
        RC3D_PROFILER_DO(g_profiler.portalSteps++);
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
            -1);

        if (!hit.hit) {
            rc3dRecordVisibleTraceSegment(currentSector, clipTop, clipBottom, UINT16_MAX);
            return;
        }

        correctedDist = hit.t;
        if (correctedDist <= RC3D_EPSILON) {
            return;
        }

        rc3dRecordVisibleTraceSegment(
            currentSector,
            clipTop,
            clipBottom,
            rc3dEncodeDepth(correctedDist));

        w = &g_map->walls[hit.wallIndex];

        if (g_wallCache && hit.wallIndex >= 0 && hit.wallIndex < g_wallCacheCount) {
            wc = &g_wallCache[hit.wallIndex];
            wallClass = wc->wallClass;
        } else {
            wallClass = rc3dClassifyWallFlags(w->flags);
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

            portalOpenTopY = rc3dProjectTopPixel(horizonGlobal, portalView.openTop, playerZ, scale);
            portalOpenBotY = rc3dProjectBottomPixel(horizonGlobal, portalView.openBottom, playerZ, scale);

            openTop = portalOpenTopY;
            if (openTop < secTop) openTop = secTop;
            if (openTop < clipTop) openTop = clipTop;

            openBot = portalOpenBotY;
            if (openBot > secBot) openBot = secBot;
            if (openBot > clipBottom) openBot = clipBottom;

            portalClipTop = openTop;
            portalClipBottom = openBot;

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
                rayMinT = hit.t;
                continue;
            }

            if (wallClass == RC3D_WALLCLASS_SOLID) {
                if (secTop > clipTop) clipTop = secTop;
                if (secBot < clipBottom) clipBottom = secBot;
                if (clipTop > clipBottom) return;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t;
                continue;
            }

            if (wallClass == RC3D_WALLCLASS_MIDDLE) {
                const int midTopY = rc3dProjectTopPixel(horizonGlobal, w->openTop, playerZ, scale);
                const int midBotY = rc3dProjectBottomPixel(horizonGlobal, w->openBottom, playerZ, scale);

                if (midTopY > clipTop) clipTop = midTopY;
                if (midBotY < clipBottom) clipBottom = midBotY;
                if (clipTop > clipBottom) return;

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = -1;
                rayMinT = hit.t;
                continue;
            }
        }

        return;
    }
}


static int rc3dBuildVisibleSpriteList(float eyeZ, float dirX, float dirY)
{
    const float screenCenterX = (float)g_viewport_width * 0.5f;
    int visibleCount = 0;

    g_visibleSpriteCount = 0;

    for (int i = 0; i < RC3D_MAX_SPRITES; ++i) {
        const RC3D_Sprite *sprite = &g_sprites[i];
        const RC3D_Sector *sec = NULL;
        const uint8_t *texels;
        const float toSpriteX = sprite->x - g_player.x;
        const float toSpriteY = sprite->y - g_player.y;
        const float camX = (toSpriteX * -dirY) + (toSpriteY * dirX);
        const float camDepth = (toSpriteX * dirX) + (toSpriteY * dirY);
        float scale;
        int leftX;
        int rightX;
        int topY;
        int bottomY;
        int unclampedWidth;
        int unclampedHeight;
        int spriteClipTop = 0;
        int spriteClipBottom = g_viewport_height - 1;
        uint8_t spriteGlow = 0u;
        int insertAt;

        if (!sprite->inUse || !sprite->active) {
            continue;
        }

        if (camDepth <= RC3D_EPSILON || camDepth >= g_draw_distance) {
            continue;
        }

        RC3D_PROFILER_DO(g_profiler.spritesVisible++);

        scale = g_projPlaneConst / camDepth;
        leftX = (int)(screenCenterX + ((camX - (sprite->width * 0.5f)) * scale));
        rightX = (int)(screenCenterX + ((camX + (sprite->width * 0.5f)) * scale));
        topY = (int)(horizonGlobal - (((sprite->baseZ + sprite->height) - eyeZ) * scale));
        bottomY = (int)(horizonGlobal - ((sprite->baseZ - eyeZ) * scale));

        if (leftX > rightX || topY > bottomY) {
            continue;
        }

        if (rightX < 0 || leftX >= g_viewport_width || bottomY < 0 || topY >= g_viewport_height) {
            continue;
        }

        unclampedWidth = (rightX - leftX) + 1;
        unclampedHeight = (bottomY - topY) + 1;

        if (unclampedWidth <= 0 || unclampedHeight <= 0) {
            continue;
        }

        if ((unsigned)sprite->sector < (unsigned)g_map->sectorCount) {
            sec = &g_map->sectors[sprite->sector];
            spriteClipTop = (int)(horizonGlobal - ((sec->ceilHeight - eyeZ) * scale));
            spriteClipBottom = (int)(horizonGlobal - ((sec->floorHeight - eyeZ) * scale));
            spriteGlow = sec->glowlevel;
        }

        if (visibleCount >= RC3D_MAX_SPRITES) {
            break;
        }

        texels = g_rc3dTextures[sprite->texId].pix;
        insertAt = visibleCount;

        while (insertAt > 0) {
            if (g_visibleSprites[insertAt - 1].camDepth >= camDepth) {
                break;
            }

            g_visibleSprites[insertAt] = g_visibleSprites[insertAt - 1];
            insertAt--;
        }

        g_visibleSprites[insertAt].sprite = sprite;
        g_visibleSprites[insertAt].texels = texels;
        g_visibleSprites[insertAt].camDepth = camDepth;
        g_visibleSprites[insertAt].scale = scale;
        g_visibleSprites[insertAt].leftX = leftX;
        g_visibleSprites[insertAt].rightX = rightX;
        g_visibleSprites[insertAt].topY = topY;
        g_visibleSprites[insertAt].bottomY = bottomY;
        g_visibleSprites[insertAt].unclampedWidth = unclampedWidth;
        g_visibleSprites[insertAt].unclampedHeight = unclampedHeight;
        g_visibleSprites[insertAt].spriteClipTop = spriteClipTop;
        g_visibleSprites[insertAt].spriteClipBottom = spriteClipBottom;
        g_visibleSprites[insertAt].spriteGlow = spriteGlow;
        g_visibleSprites[insertAt].depthCode = rc3dEncodeDepth(camDepth);

        visibleCount++;
        RC3D_PROFILER_DO(g_profiler.spritesDrawn++);
    }

    g_visibleSpriteCount = visibleCount;
    return visibleCount;
}

static void rc3dBuildSpriteColumnCoverage(void)
{
    for (int sx = 0; sx < g_viewport_width; ++sx) {
        g_spriteColumnFirstIndex[sx] = -1;
        g_spriteColumnLastIndex[sx] = -1;
        g_cachedVisibleTraceCount[sx] = 0u;
        g_cachedVisibleTraceOverflow[sx] = 0u;
    }

    for (int i = 0; i < g_visibleSpriteCount; ++i) {
        int leftX = g_visibleSprites[i].leftX;
        int rightX = g_visibleSprites[i].rightX;

        if (leftX < 0) {
            leftX = 0;
        }

        if (rightX >= g_viewport_width) {
            rightX = g_viewport_width - 1;
        }

        if (leftX > rightX) {
            continue;
        }

        for (int sx = leftX; sx <= rightX; ++sx) {
            if (g_spriteColumnFirstIndex[sx] < 0) {
                g_spriteColumnFirstIndex[sx] = (int16_t)i;
            }

            g_spriteColumnLastIndex[sx] = (int16_t)i;
        }
    }
}

static void rc3dRefreshSpritePlacement(RC3D_Sprite *sprite)
{
    int sector;

    if (!sprite || !sprite->inUse) return;

    sector = findSectorForSpritePosition(sprite->x, sprite->y, sprite->sector);

    if ((unsigned)sector < (unsigned)g_map->sectorCount) {
        sprite->sector = (int16_t)sector;
        sprite->baseZ = g_map->sectors[sector].floorHeight;
    } else {
        sprite->sector = -1;
    }
}

static void rc3dRenderSpriteColumn(
    int sx,
    float eyeZ,
    const RC3D_VisibleTraceSegment *visibleTrace,
    uint8_t visibleTraceCount,
    const RC3D_WallDepthSpan *wallSpans,
    uint8_t wallSpanCount)
{
    int spriteStart;
    int spriteEnd;

    if ((unsigned)sx >= (unsigned)g_viewport_width) {
        return;
    }

    spriteStart = g_spriteColumnFirstIndex[sx];
    spriteEnd = g_spriteColumnLastIndex[sx];

    if (spriteStart < 0 || spriteEnd < spriteStart) {
        return;
    }

    for (int i = spriteStart; i <= spriteEnd; ++i) {
        const RC3D_VisibleSprite *visibleSprite = &g_visibleSprites[i];
        const RC3D_Sprite *sprite = visibleSprite->sprite;
        int visSector = -1;
        int visClipTop = 0;
        int visClipBottom = g_viewport_height - 1;
        int colTop;
        int colBottom;
        int tx;
        uint8_t spriteGlow;
        RC3D_ShadeProfile shadeProfile;

        if (sx < visibleSprite->leftX || sx > visibleSprite->rightX) {
            continue;
        }

        colTop = visibleSprite->topY;
        colBottom = visibleSprite->bottomY;
        tx = ((sx - visibleSprite->leftX) * RC3D_TEX_SIZE) / visibleSprite->unclampedWidth;
        spriteGlow = visibleSprite->spriteGlow;

        if (tx < 0) {
            tx = 0;
        } else if (tx >= RC3D_TEX_SIZE) {
            tx = RC3D_TEX_SIZE - 1;
        }

        if (colTop < 0) {
            colTop = 0;
        }

        if (colBottom >= g_viewport_height) {
            colBottom = g_viewport_height - 1;
        }

        if (colTop < visibleSprite->spriteClipTop) {
            colTop = visibleSprite->spriteClipTop;
        }

        if (colBottom > visibleSprite->spriteClipBottom) {
            colBottom = visibleSprite->spriteClipBottom;
        }

        if (colTop > colBottom) {
            continue;
        }

        if (!(visibleTrace
                ? rc3dTraceVisibleSectorAtDepth(
                    visibleTrace,
                    visibleTraceCount,
                    visibleSprite->depthCode,
                    &visSector,
                    &visClipTop,
                    &visClipBottom)
                : rc3dTraceVisibleSectorAtDepthCached(
                    sx,
                    visibleSprite->depthCode,
                    &visSector,
                    &visClipTop,
                    &visClipBottom)))
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
            const int visSecTopY = (int)(horizonGlobal - ((visSec->ceilHeight - eyeZ) * visibleSprite->scale));
            const int visSecBotY = (int)(horizonGlobal - ((visSec->floorHeight - eyeZ) * visibleSprite->scale));

            if (colTop < visSecTopY) {
                colTop = visSecTopY;
            }

            if (colBottom > visSecBotY) {
                colBottom = visSecBotY;
            }

            if (spriteGlow == 0u) {
                spriteGlow = visSec->glowlevel;
            }
        }

        if (colTop > colBottom) {
            continue;
        }

        RC3D_PROFILER_DO(g_profiler.spriteColumns++);

        rc3dBuildShadeProfile(
            visibleSprite->camDepth * RC3D_LIGHT_SPRITE_DIST_SCALE,
            spriteGlow,
            &shadeProfile);

        {
            uint8_t *dst = rc3dViewportPixelPtr(sx, colTop);

            for (int sy = colTop; sy <= colBottom; ++sy) {
                int ty = ((sy - visibleSprite->topY) * RC3D_TEX_SIZE) / visibleSprite->unclampedHeight;
                uint8_t texel;
                uint8_t litTexel;

                if (ty < 0) {
                    ty = 0;
                } else if (ty >= RC3D_TEX_SIZE) {
                    ty = RC3D_TEX_SIZE - 1;
                }

                texel = visibleSprite->texels[(ty * RC3D_TEX_SIZE) + tx];
                litTexel = rc3dApplyShadeProfileToTexel(
                    texel,
                    tx,
                    ty,
                    sprite->texId,
                    &shadeProfile);

                if (litTexel != RC3D_SPRITE_TEX_TRANSPARENT &&
                    !rc3dWallSpansBlockPixel(wallSpans, wallSpanCount, sy, visibleSprite->depthCode))
                {
                    *dst = litTexel;
                }

                dst += SCREEN_W;
            }
        }
    }
}

static void renderCurrentSectorColumns(Uint64 *outWallTicks, Uint64 *outSpriteTicks)
{
    const float projPlane = g_projPlaneConst;
    const int horizon = g_viewport_height / 2;
    const float bobZ = sinf(g_player.tHeadbob) * 0.075f;
    float dirX = 1.0f;
    float dirY = 0.0f;
    const float planeScale = g_planeScaleConst;
    Uint64 wallTicks = 0;
    Uint64 spriteTicks = 0;

    g_renderEyeZ = g_player.z + fabsf(bobZ);
    projPlaneGlobal = projPlane;
    horizonGlobal = horizon;

    rc3dLookupAngleTrig(g_player.angle, &dirX, &dirY);

    rc3dBuildColumnRayCache(
        dirX,
        dirY,
        -dirY * planeScale,
        dirX * planeScale);

    rc3dBuildVisibleSpriteList(g_renderEyeZ, dirX, dirY);
    rc3dBuildSpriteColumnCoverage();
    RC3D_PROFILER_DO(g_profiler.rays = (g_viewport_width > 0) ? (uint32_t)g_viewport_width : 0u;);

    {
        int preferredWallIndex = -1;

        for (int sx = 0; sx < g_viewport_width; ++sx) {
            RC3D_VisibleTraceSegment visibleTrace[RC3D_MAX_PORTAL_STEPS];
            RC3D_WallDepthSpan wallSpans[RC3D_MAX_WALL_SPANS_PER_COLUMN];
            uint8_t visibleTraceCount = 0u;
            uint8_t wallSpanCount = 0u;
            const RC3D_ColumnRayCache *rayCache = &g_columnRayCache[sx];
            int firstHitWallIndex = -1;
            const int needsSpriteColumn = (g_spriteColumnFirstIndex[sx] >= 0);

            rc3dSetColumnContext(visibleTrace, &visibleTraceCount, wallSpans, &wallSpanCount);

#if RC3D_DRAW_PROFILER
            if (g_profilerFrameActive) {
                const Uint64 start = SDL_GetPerformanceCounter();

                renderColumnTrace(
                    sx,
                    rayCache->rdx,
                    rayCache->rdy,
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
                    &firstHitWallIndex);

                wallTicks += SDL_GetPerformanceCounter() - start;
            } else
#endif
            {
                renderColumnTrace(
                    sx,
                    rayCache->rdx,
                    rayCache->rdy,
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
                    &firstHitWallIndex);
            }

            preferredWallIndex = firstHitWallIndex;

            if (!needsSpriteColumn) {
                continue;
            }

#if RC3D_DRAW_PROFILER
            if (g_profilerFrameActive) {
                const Uint64 start = SDL_GetPerformanceCounter();

                if (g_cachedVisibleTraceOverflow[sx]) {
                    rc3dBuildVisibleTraceForColumn(sx);
                    rc3dRenderSpriteColumn(
                        sx,
                        g_renderEyeZ,
                        visibleTrace,
                        visibleTraceCount,
                        wallSpans,
                        wallSpanCount);
                } else {
                    rc3dRenderSpriteColumn(
                        sx,
                        g_renderEyeZ,
                        NULL,
                        0u,
                        wallSpans,
                        wallSpanCount);
                }

                spriteTicks += SDL_GetPerformanceCounter() - start;
            } else
#endif
            {
                if (g_cachedVisibleTraceOverflow[sx]) {
                    rc3dBuildVisibleTraceForColumn(sx);
                    rc3dRenderSpriteColumn(
                        sx,
                        g_renderEyeZ,
                        visibleTrace,
                        visibleTraceCount,
                        wallSpans,
                        wallSpanCount);
                } else {
                    rc3dRenderSpriteColumn(
                        sx,
                        g_renderEyeZ,
                        NULL,
                        0u,
                        wallSpans,
                        wallSpanCount);
                }
            }
        }
    }

    rc3dSetColumnContext(NULL, NULL, NULL, NULL);

    if (outWallTicks) {
        *outWallTicks = wallTicks;
    }

    if (outSpriteTicks) {
        *outSpriteTicks = spriteTicks;
    }
}

/* ------------------------------------------------------------------------- */
/* minimap                                                                   */
/* ------------------------------------------------------------------------- */

static int minimapOutCode(int x, int y, int left, int top, int right, int bottom)
{
    int code = 0;
    if (x < left) code |= 1;
    if (x > right) code |= 2;
    if (y < top) code |= 4;
    if (y > bottom) code |= 8;
    return code;
}

static int clipLineToRect(
    int *x0, int *y0,
    int *x1, int *y1,
    int left, int top,
    int right, int bottom)
{
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

    const int left = mapX + 1;
    const int top = mapY + 1;
    const int right = mapX + mapW - 2;
    const int bottom = mapY + mapH - 2;

    const int centerX = mapX + (mapW / 2);
    const int centerY = mapY + (mapH / 2);

    const float scale = 4.0f;

    drawRectSemi(mapX, mapY, mapW, mapH, 16);

    drawLine(mapX, mapY, mapX + mapW - 1, mapY, 15);
    drawLine(mapX, mapY, mapX, mapY + mapH - 1, 15);
    drawLine(mapX + mapW - 1, mapY, mapX + mapW - 1, mapY + mapH - 1, 15);
    drawLine(mapX, mapY + mapH - 1, mapX + mapW - 1, mapY + mapH - 1, 15);

    {
        const RC3D_Wall *walls = g_map->walls;
        const RC3D_Vec2 *verts = g_map->verts;

        for (int s = 0; s < g_map->sectorCount; s++) {
            const RC3D_Sector *sec = &g_map->sectors[s];
            const int start = sec->wallStart;
            const int end = start + sec->wallCount;

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

static RC3D_Wall *rc3dMutableWallData(void)
{
    if (!g_map || !g_map->walls || g_map->wallCount <= 0) {
        return NULL;
    }

    return (RC3D_Wall *)g_map->walls;
}

static inline uint32_t rc3dWallTextureFlagsWithGlow(uint32_t textureFlags, uint8_t glowLevel)
{
    const uint32_t packedGlow =
        (((uint32_t)rc3dClampGlowLevel(glowLevel)) & RC3D_TEX_WALL_GLOW_MAX) << RC3D_TEX_WALL_GLOW_SHIFT;

    return (textureFlags & ~RC3D_TEX_WALL_GLOW_MASK) | packedGlow;
}

static void rc3dSyncSectorWallGlow(int sectorId, uint8_t glowLevel)
{
    RC3D_Sector *sectors = rc3dMutableSectorData();
    RC3D_Wall *walls = rc3dMutableWallData();

    if (!g_map || !sectors || !walls || (unsigned)sectorId >= (unsigned)g_map->sectorCount) {
        return;
    }

    {
        const RC3D_Sector *sec = &sectors[sectorId];
        const int start = sec->wallStart;
        const int end = start + sec->wallCount;

        if (!(sec->sectorFlags & RC3D_SECTOR_FLAGS_EFFECTWALLS)) {
            return;
        }

        if (start < 0 || end < start || end > g_map->wallCount) {
            return;
        }

        glowLevel = rc3dClampGlowLevel(glowLevel);

        for (int wi = start; wi < end; ++wi) {
            walls[wi].texture_flags = rc3dWallTextureFlagsWithGlow(
                walls[wi].texture_flags,
                glowLevel);
        }
    }
}

static void rc3dClampPlayerIntoCurrentSector(void)
{
    if (!g_map) return;

    if ((unsigned)g_player.sector < (unsigned)g_map->sectorCount) {
        const RC3D_Sector *sec = &g_map->sectors[g_player.sector];
        const float minEyeZ = sec->floorHeight + RC3D_PLAYER_EYE_HEIGHT;
        const float maxEyeZ = sec->ceilHeight - (PLAYER_HEIGHT - RC3D_PLAYER_EYE_HEIGHT);

        if (g_player.z < (minEyeZ - PLAYER_STEPUP - RC3D_EPSILON)) {
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

void rc3dSetSectorLightLevel(int sectorId, uint8_t level){
    RC3D_Sector *sectors = rc3dMutableSectorData();
    if (!g_map || !sectors || (unsigned)sectorId >= (unsigned)g_map->sectorCount) {
        return;
    }

    level = rc3dClampGlowLevel(level);
    sectors[sectorId].glowlevel = level;
    rc3dSyncSectorWallGlow(sectorId, level);

}

static int lightPulsationTimer = 0;
static int rc3dShouldAdvancePulsatingLights(void)
{
    lightPulsationTimer++;
    if (lightPulsationTimer > 4) {
        lightPulsationTimer = 0;
        return 1;
    }

    return 0;
}

static void rc3dAdvanceSectorPulse(int sectorId, RC3D_Sector *sec, uint32_t sectorFlags)
{
    int glow;
    int dir;
    int targetGlow;

    if (!sec) {
        return;
    }

    glow = (int)rc3dClampGlowLevel(sec->glowlevel);
    dir = (sec->PulsatingLightTimeDir < 0) ? -1 : 1;
    targetGlow = (sectorFlags & RC3D_SECTOR_FLAGS_FULLBRIGHT) ? 7 : (int)sec->originalLightLevel;
    if (targetGlow < 0) targetGlow = 0;
    if (targetGlow > (int)RC3D_TEX_WALL_GLOW_MAX) targetGlow = (int)RC3D_TEX_WALL_GLOW_MAX;

    if (targetGlow <= 0) {
        sec->PulsatingLightTimeDir = 1;
        rc3dSetSectorLightLevel(sectorId, 0u);
        return;
    }

    if (dir > 0) {
        if (glow >= targetGlow) {
            dir = -1;
            glow = targetGlow - 1;
        } else {
            glow++;
            if (glow >= targetGlow) {
                glow = targetGlow;
                dir = -1;
            }
        }
    } else {
        if (glow <= 0) {
            dir = 1;
            glow = 1;
        } else {
            glow--;
            if (glow <= 0) {
                glow = 0;
                dir = 1;
            }
        }
    }

    if (glow < 0) glow = 0;
    if (glow > targetGlow) glow = targetGlow;

    sec->PulsatingLightTimeDir = (int8_t)dir;
    rc3dSetSectorLightLevel(sectorId, (uint8_t)glow);
}

static void rc3dUpdateSectorMotion(float dt)
{
    RC3D_Sector *sectors = rc3dMutableSectorData();
    int anySectorMoved = 0;
    int advancePulsatingLights = 0;

    if (!g_map || !sectors || dt <= 0.0f) {
        return;
    }

    advancePulsatingLights = rc3dShouldAdvancePulsatingLights();

    int secFlicker = randRange(1, 8);
    for (int i = 0; i < g_map->sectorCount; ++i) {
        RC3D_Sector *sec = &sectors[i];
        uint32_t stateFlags = rc3dSanitizeSectorStateFlags(sec->stateFlags);
        uint32_t sectorFlags = sec->sectorFlags;
        const float oldFloorHeight = sec->floorHeight;
        const float oldCeilHeight = sec->ceilHeight;

        sec->stateFlags = stateFlags;

        // do the sectorFlags
        if (sectorFlags & RC3D_SECTOR_FLAGS_FLICKERING_LIGHTS){
            
            // sec->PulsatingLightTimeDir;
            if(secFlicker == 2) rc3dSetSectorLightLevel(i, (sectorFlags & RC3D_SECTOR_FLAGS_FULLBRIGHT) ? 7 : sec->originalLightLevel);
            if(secFlicker == 5) rc3dSetSectorLightLevel(i, 0);
        }
        
        if ((sectorFlags & RC3D_SECTOR_FLAGS_PULSATING_LIGHT) && advancePulsatingLights){
            rc3dAdvanceSectorPulse(i, sec, sectorFlags);
        }

        if (sectorFlags & RC3D_SECTOR_FLAGS_EFFECTWALLS) {
            rc3dSyncSectorWallGlow(i, sec->glowlevel);
        }

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
            RC3D_PROFILER_DO(g_profiler.sectorsMoved++);
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

void moveSprite(void)
{
    //if (rc3dSpriteHandleValid(g_demoSpriteB)) {
        //rc3dSpriteSetPosition(g_demoSpriteB, g_player.x, g_player.y);
    //}
}

static float rc3dGetTargetEyeZ(void)
{
    if (rc3dSectorIndexValid(g_player.sector)) {
        return g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;
    }

    return RC3D_PLAYER_EYE_HEIGHT;
}

static void rc3dUpdateHeadbob(float dt, int isMoving)
{
    if (isMoving) {
        g_player.tHeadbob += dt * 7.0f;
        if (g_player.tHeadbob > (float)(M_PI)) {
            g_player.tHeadbob -= (float)(M_PI);
        }
    } else {
        if (g_player.tHeadbob > 0.0f) {
            g_player.tHeadbob += dt * 12.0f;
            if (g_player.tHeadbob > (float)(M_PI)) {
                g_player.tHeadbob = 0.0f;
            }
        }
    }
}

static void rc3dUpdatePlayerVertical(float dt)
{
    const float targetZ = rc3dGetTargetEyeZ();

    if (g_player.z > targetZ) {
        g_player.vz -= RC3D_GRAVITY * dt;
        g_player.z += g_player.vz * dt;

        if (g_player.z <= targetZ) {
            g_player.z = targetZ;
            g_player.vz = 0.0f;
        }
    } else {
        const float dz = targetZ - g_player.z;

        if (dz > 0.0f) {
            const float riseSpeed = dz * 10.0f;
            const float maxRise = RC3D_STEP_SNAP_SPEED * dt;
            float stepUp = riseSpeed * dt;

            if (stepUp > maxRise) stepUp = maxRise;
            if (stepUp > dz) stepUp = dz;

            g_player.z += stepUp;
        }

        g_player.vz = 0.0f;
    }
}



static void rc3dResetPlayerFromMapStart(void)
{
    rc3dSetPlayerWorldXYFixed(
        rc3dFloatToFixed(g_map->startX),
        rc3dFloatToFixed(g_map->startY)
    );
    
    g_player.angle = g_map->startAngle;
    g_player.sector = g_map->startSector;
    g_player.tHeadbob = 0.0f;

    if (rc3dSectorIndexValid(g_player.sector)) {
        g_player.z = g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;
    } else {
        g_player.z = RC3D_PLAYER_EYE_HEIGHT;
    }

    g_player.vz = 0.0f;
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
    if (!g_invDTableInit) rc3dBuildInvDTable();
    if (!g_lightVariantTablesInit) rc3dBuildLightVariantTables();
    rc3dBuildTrigTables();
    rc3dRefreshViewport();

    if (!rc3dRebuildCurrentMapCaches()) {
        fprintf(stderr, "rc3dInit: failed to build map caches\n");
        return;
    }

    rc3dResetPlayerFromMapStart();
    rc3dClearSprites();
    //rc3dInitTestSprite();
    int spriteTmp = 0;

    for(int o = 0; o < g_map->objectCount; o++){
        if(g_map->objects[o].type == OBJECT_TYPE_BASIC_SPRITE){    // basic Sprites
            //spriteTmp = rc3dSpriteCreate(g_map->objects[o].x, g_map->objects[o].y, g_map->objects[o].scalex, g_map->objects[o].scalex, g_map->objects[o].textureId);
            rc3dSpriteCreate(g_map->objects[o].x, g_map->objects[o].y, g_map->objects[o].z, 
                g_map->objects[o].scalex, 
                g_map->objects[o].scaley, 
                g_map->objects[o].textureId
            );
        }
    }
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

static RC3D_Object *rc3dMutableObjectData(void)
{
    if (!g_map || !g_map->objects || g_map->objectCount <= 0) {
        return NULL;
    }

    return (RC3D_Object *)g_map->objects;
}

int rc3dSetObjectStateByTag(int32_t tagId, uint32_t stateFlags){
    int changedCount = 0;
    RC3D_Object *objs = rc3dMutableObjectData();

    if (!objs) {
        return 0;
    }
   
    for (int i = 0; i < g_map->objectCount; ++i){
        if (objs[i].tagId == tagId) {
            objs[i].flags = stateFlags;
            changedCount++; 
        }
    }
    return changedCount;
}



int rc3dGetSectorByTag(int32_t tagId){
    RC3D_Sector *sectors = rc3dMutableSectorData();

    if (!g_map || !sectors) {
        return 0;
    }
    for (int i = 0; i < g_map->sectorCount; ++i) {
        if (sectors[i].tagId == tagId) {
            return i;
        }
    }
    return 0;
}



static inline float distance3D(float x1, float y1, float z1,   float x2, float y2, float z2){
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float dz = z2 - z1;
    return sqrtf((dx * dx) + (dy * dy) + (dz * dz));
}

static inline float distance2D(float x1, float y1,   float x2, float y2){
    const float dx = x2 - x1;
    const float dy = y2 - y1;
    return (dx * dx) + (dy * dy);
}

static inline int rc3dObjectMatchesPlayerElevation(const RC3D_Object *obj)
{
    const float playerFeet = g_player.z - RC3D_PLAYER_EYE_HEIGHT;
    const float playerHead = playerFeet + PLAYER_HEIGHT;

    if (!obj) {
        return 0;
    }

    return (obj->z >= (playerFeet - RC3D_EPSILON)) &&
           (obj->z <= (playerHead + RC3D_EPSILON));
}

#define RC3D_PROFILE_OBJ_LINES 16
static float obj1dist[RC3D_PROFILE_OBJ_LINES] = {0.0f};



// API CALL
void enableObject(int tagId, uint8_t en){
    RC3D_Object *objs = rc3dMutableObjectData();
    int objCnt;

    if (!objs) {
        return;
    }

    objCnt = g_map->objectCount;
    for (int i = 0; i < objCnt; i++) {
        RC3D_Object *obj = &objs[i];
        if(tagId == obj->tagId){
            if(en)
                obj->flags |= OBJECT_GENERAL_FLAGS_ENABLE;
            else
                obj->flags &= ~OBJECT_GENERAL_FLAGS_ENABLE;
        }
    }
}

// an api for just reading weather a player is with in the object range or not
int getObjectState(int pTagId){
    int objCnt;
    const float px = g_player.x;
    const float py = g_player.y;

    if (!g_map || !g_map->objects) {
        return -1;
    }
    objCnt = g_map->objectCount;
    if (objCnt <= 0) {
        return 0;
    }

    for (int i = 0; i < objCnt; i++) {
        const RC3D_Object *obj = &g_map->objects[i];
        const float distSq = distance2D(px, py, obj->x, obj->y);
        if((obj->tagId == pTagId) &&
           (obj->flags & OBJECT_GENERAL_FLAGS_ENABLE) &&
           rc3dObjectMatchesPlayerElevation(obj)){
            const float radiusSq = obj->radius * obj->radius;
            return (distSq < radiusSq) ? 1 : 0;
        }
    }

    return 0;
}

// API CALLER
#define OBJTRIG_TYPE_SECTORS    1   // just a local non magic number
#define OBJTRIG_TYPE_OBJECTS    2   // to other objects
void processObjects(int pTagId, int pType)  // engine internal systems
{
    RC3D_Object *objs = rc3dMutableObjectData();
    int objCnt;
    const float px = g_player.x;
    const float py = g_player.y;

    if (!rc3dEnsurePlayerSectorValid()) {
        return;
    }

    if (!objs) {
        return;
    }

    objCnt = g_map->objectCount;
    if (objCnt <= 0) {
        return;
    }

    for (int i = 0; i < objCnt; i++) {
        RC3D_Object *obj = &objs[i];

        // Only trigger objects that overlap the player's current vertical span.
        if (!rc3dObjectMatchesPlayerElevation(obj)) {
            continue;
        }

        const float distSq = distance2D(px, py, obj->x, obj->y);
        const float radiusSq = obj->radius * obj->radius;
        const int inside = (distSq < radiusSq) ? 1 : 0;
        const int wasInside = (obj->trigger != 0) ? 1 : 0;
        uint8_t flagToSet = 0;

        RC3D_PROFILER_DO(g_profiler.objectsProcessed++);

        // for debug information only
        if (i < RC3D_PROFILE_OBJ_LINES) {
            obj1dist[i] = sqrtf(distSq);
        }

        if (inside == wasInside) {
            continue;
        }

        if (!(obj->flags & OBJECT_GENERAL_FLAGS_ENABLE)) continue;  // enabled bit
        if (pType  != 0 && obj->type  != (uint32_t)pType)  continue;
        if (pTagId != 0 && obj->tagId != pTagId) continue;


        RC3D_PROFILER_DO(g_profiler.objectEdgeChanges++);

        obj->trigger = (uint8_t)inside;
        
        int procType = 0;

        switch (obj->type) {
            // general sprite, just decorative
            case RC3D_OBJTYPE_SPRITE:
                break;

            // triggers for sectors!
            case RC3D_OBJTYPE_SECTOR_TRIGGER_INOUT:
                procType = OBJTRIG_TYPE_SECTORS;
                flagToSet = inside ? obj->inFlag : obj->outFlag;
                break;

            case RC3D_OBJTYPE_SECTOR_TRIGGER_ENTER:
                if (inside) {
                    procType = OBJTRIG_TYPE_SECTORS;
                    flagToSet = obj->inFlag;
                }
                break;

            case RC3D_OBJTYPE_SECTOR_TRIGGER_EXIT:
                if (!inside) {
                    procType = OBJTRIG_TYPE_SECTORS;
                    flagToSet = obj->outFlag;
                }
                break;

            // triggers for OTHER objects
            case RC3D_OBJTYPE_OBJECT_TRIGGER_INOUT:
                procType = OBJTRIG_TYPE_OBJECTS;
                flagToSet = inside ? obj->inFlag : obj->outFlag;
                break;

            case RC3D_OBJTYPE_OBJECT_TRIGGER_ENTER:
                if (inside) {
                    procType = OBJTRIG_TYPE_OBJECTS;
                    flagToSet = obj->inFlag;
                }
                break;
            case RC3D_OBJTYPE_OBJECT_TRIGGER_EXIT:
                if (!inside) {
                    procType = OBJTRIG_TYPE_OBJECTS;
                    flagToSet = obj->outFlag;
                }
                break;
            default:
                break;
        }

        if (procType == OBJTRIG_TYPE_SECTORS){
            if (obj->targetTagId != 0) {
                const int changed = rc3dSetSectorStateByTag(obj->targetTagId, flagToSet);
                RC3D_PROFILER_DO(
                    if (changed > 0) {
                        g_profiler.sectorStateWrites += (uint32_t)changed;
                    }
                );
            }
        }
        if (procType == OBJTRIG_TYPE_OBJECTS){
            if (obj->targetTagId != 0) {
                const int changed = rc3dSetObjectStateByTag(obj->targetTagId, flagToSet);
                RC3D_PROFILER_DO(
                    if (changed > 0) {
                        g_profiler.sectorStateWrites += (uint32_t)changed;
                    }
                );
            }
        }

    }
}

int scancode_f1 = 0;
int scancode_m = 0;

void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx)
{
#if RC3D_DRAW_PROFILER
    const Uint64 updateStart = SDL_GetPerformanceCounter();
    rc3dProfilerBeginFrame();
#endif

    float moveX = 0.0f;
    float moveY = 0.0f;
    int isMoving = 0;

    float forwardX = 1.0f;
    float forwardY = 0.0f;

    rc3dUpdateSectorMotion(dt);
    rc3dLookupAngleTrig(g_player.angle, &forwardX, &forwardY);

    const float rightX = -forwardY;
    const float rightY = forwardX;

    if (keys[SDL_SCANCODE_Q]) g_player.angle -= RC3D_TURN_SPEED * dt;
    if (keys[SDL_SCANCODE_E]) g_player.angle += RC3D_TURN_SPEED * dt;

    g_player.angle += (float)mouseDx * RC3D_MOUSE_SENS;
    g_player.angle = wrapAngle(g_player.angle);

    if (keys[SDL_SCANCODE_W]) { moveX += forwardX; moveY += forwardY; }
    if (keys[SDL_SCANCODE_S]) { moveX -= forwardX; moveY -= forwardY; }
    if (keys[SDL_SCANCODE_A]) { moveX -= rightX; moveY -= rightY; }
    if (keys[SDL_SCANCODE_D]) { moveX += rightX; moveY += rightY; }

    if (keys[SDL_SCANCODE_F1]) { 
        if(scancode_f1 == 0){
            scancode_f1 = 1;
            bShowProfiler = 1 - bShowProfiler; 
        }
    }
    else
        scancode_f1 = 0;

    if (keys[SDL_SCANCODE_M]) { 
        if(scancode_m == 0){
            scancode_m = 1;
            bShowMiniMap = 1 - bShowMiniMap;
        }
    }
    else
        scancode_m = 0;

    if (keys[SDL_SCANCODE_1]) {
        rc3dSetSectorStateByTag(1, RC3D_SECTOR_STATE_RAISE_FLOOR | RC3D_SECTOR_STATE_LOWER_CEILING);
    }
    if (keys[SDL_SCANCODE_2]) {
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

    static int gamelogictime = 0;
    
    gamelogictime++;
    if(gamelogictime> 8){
        gamelogictime = 0;
        // standard object(triggers)
        processObjects(0, RC3D_OBJTYPE_SECTOR_TRIGGER_INOUT);
        processObjects(0, RC3D_OBJTYPE_SECTOR_TRIGGER_ENTER);
        processObjects(0, RC3D_OBJTYPE_SECTOR_TRIGGER_EXIT);
        processObjects(0, RC3D_OBJTYPE_OBJECT_TRIGGER_INOUT);
        processObjects(0, RC3D_OBJTYPE_OBJECT_TRIGGER_ENTER);
        processObjects(0, RC3D_OBJTYPE_OBJECT_TRIGGER_EXIT);
    }

    rc3dUpdateHeadbob(dt, isMoving);
    rc3dUpdatePlayerVertical(dt);

#if RC3D_DRAW_PROFILER
    g_profiler.updateMs = rc3dProfilerTicksToMs(SDL_GetPerformanceCounter() - updateStart);
#endif
}


static int rc3dEnsurePlayerSectorValid(void)
{
    if (!g_map || !g_map->sectors || g_map->sectorCount <= 0) {
        return 0;
    }

    if (rc3dSectorIndexValid(g_player.sector)) {
        if (pointInSectorFixed(g_player.xFixed, g_player.yFixed, g_player.sector)) {
            rc3dClampPlayerIntoCurrentSector();
            return 1;
        }
    }

    g_player.sector = findSectorForPointFixed(g_player.xFixed, g_player.yFixed);

    if (rc3dSectorIndexValid(g_player.sector)) {
        rc3dClampPlayerIntoCurrentSector();
        return 1;
    }

    if ((unsigned)g_map->startSector < (unsigned)g_map->sectorCount) {
        rc3dResetPlayerFromMapStart();
        return 1;
    }

    return 0;
}


void rc3dRender(void)
{
    if (!rc3dEnsurePlayerSectorValid()) {
        return;
    }

#if RC3D_DRAW_PROFILER
    const int profileThisPass = g_profilerFrameActive;
    const Uint64 frameStart = profileThisPass ? SDL_GetPerformanceCounter() : 0;
    Uint64 sectionStart = frameStart;
    Uint64 wallTicks = 0;
    Uint64 spriteTicks = 0;
#endif

    rc3dRefreshViewport();

#if RC3D_DRAW_PROFILER
    if (profileThisPass) {
        sectionStart = SDL_GetPerformanceCounter();
    }
#endif

    drawBackground();

#if RC3D_DRAW_PROFILER
    if (profileThisPass) {
        const Uint64 now = SDL_GetPerformanceCounter();
        g_profiler.backgroundMs = rc3dProfilerTicksToMs(now - sectionStart);
        sectionStart = now;
    }
#endif

#if RC3D_DRAW_PROFILER
    renderCurrentSectorColumns(
        profileThisPass ? &wallTicks : NULL,
        profileThisPass ? &spriteTicks : NULL);
#else
    renderCurrentSectorColumns(NULL, NULL);
#endif

#if RC3D_DRAW_PROFILER
    if (profileThisPass) {
        g_profiler.wallsMs = rc3dProfilerTicksToMs(wallTicks);
        g_profiler.spritesMs = rc3dProfilerTicksToMs(spriteTicks);
        sectionStart = SDL_GetPerformanceCounter();
    }
#endif

#if RC3D_DRAW_MINIMAP
    if (bShowMiniMap) {
        drawMiniMap();
    }
#endif

#if RC3D_DRAW_PROFILER
    if (profileThisPass) {
        const Uint64 now = SDL_GetPerformanceCounter();
        const double renderMs = rc3dProfilerTicksToMs(now - frameStart);

        g_profiler.minimapMs = rc3dProfilerTicksToMs(now - sectionStart);
        g_profiler.totalMs = g_profiler.updateMs + renderMs;
        rc3dProfilerBlendAverages();

        g_profilerFrameActive = 0;
    }
#endif

#if RC3D_DRAW_PROFILER
    if (bShowProfiler) {
        char buf[64];
        const int dbgObjectCount =
            (g_map->objectCount < RC3D_PROFILE_OBJ_LINES) ? g_map->objectCount : RC3D_PROFILE_OBJ_LINES;


        snprintf(buf, sizeof(buf), "X:%.3f, Y:%.03f, Z:%.03f", g_player.x, g_player.y, g_player.z - RC3D_PLAYER_EYE_HEIGHT);
        drawTextO(8, 8, buf, 92);

        snprintf(buf, sizeof(buf), "SECTOR %d", g_player.sector);
        drawTextO(8, 16, buf, 92);

        if (g_map->objects) {
            for (int oi = 0; oi < dbgObjectCount; oi++) {
                snprintf(buf, sizeof(buf), "OBJ:%.1f (%.1f), trgd:%d, type:%d",
                         obj1dist[oi],
                         g_map->objects[oi].radius,
                         g_map->objects[oi].trigger,
                         g_map->objects[oi].type);
                drawTextO(8, 30 + (oi * 8), buf, 2);
            }

            if (g_map->objectCount > dbgObjectCount) {
                drawTextO(8, 38 + (dbgObjectCount * 16), "...", 2);
            }
        }

        rc3dDrawProfiler();
    }
#endif
}
