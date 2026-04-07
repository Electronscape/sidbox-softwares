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

#define RC3D_DRAW_MINIMAP  1
#define RC3D_DRAW_HUD      1

#define RC3D_FOV_DEG         75.0f
#define RC3D_TURN_SPEED      2.4f
#define RC3D_MOVE_SPEED      3.0f
#define RC3D_MOUSE_SENS      0.0035f
#define RC3D_EPSILON         0.0001f
#define RC3D_MAX_RAY_DIST    50.0f
#define RC3D_MAX_PORTAL_STEPS 12
//#define RC3D_EYE_Z           0.5f

#define RC3D_PLAYER_EYE_HEIGHT  0.5f
#define RC3D_GRAVITY            18.0f
#define RC3D_STEP_SNAP_SPEED    24.0f

typedef struct {
    float x;
    float y;
    float z;
    float vz;
    float angle;
    int sector;
} RC3D_Player;

typedef struct {
    float t;
    int wallIndex;
    int hit;
} RC3D_WallHit;

static RC3D_Player g_player;

static const RC3D_Map *g_map = &g_rc3dDemoMap;
static RC3D_Map g_loadedMap;
static int g_loadedMapValid = 0;
static int horizonGlobal = 0;
static int wallTexXGlobal = 0;


/* ------------------------------------------------------------------------- */
/* helpers                                                                   */
/* ------------------------------------------------------------------------- */
#define RC3D_TEX_SIZE 64
#define RC3D_TEX_MASK (RC3D_TEX_SIZE - 1)

typedef struct {
    uint8_t pix[RC3D_TEX_SIZE * RC3D_TEX_SIZE];
} RC3D_Texture;

static RC3D_Texture g_rc3dTextures[256];
static int g_rc3dTexturesInit = 0;
static float projPlaneGlobal = 0.0f;
static float g_invRowDist[SCREEN_H];
static float g_rayDirLocalX[SCREEN_W];
static float g_rayDirLocalY[SCREEN_W];
static float g_rayCosFix[SCREEN_W];
static int g_rayTableInit = 0;
static int g_planeTablesInit = 0;

static void rc3dBuildPlaneTables(void)
{
    for (int y = 0; y < SCREEN_H; ++y) {
        const int d = (SCREEN_H / 2) - y;

        if (d == 0) {
            g_invRowDist[y] = 0.0f;
        } else {
            g_invRowDist[y] = 1.0f / (float)d;
        }
    }

    g_planeTablesInit = 1;
}

static void rc3dBuildRayTable(void)
{
    const float halfFov = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    const float step = (halfFov * 2.0f) / (float)(SCREEN_W - 1);

    for (int sx = 0; sx < SCREEN_W; ++sx) {
        const float a = -halfFov + ((float)sx * step);

        g_rayDirLocalX[sx] = cosf(a);
        g_rayDirLocalY[sx] = sinf(a);
        g_rayCosFix[sx]    = cosf(a);
    }

    g_rayTableInit = 1;
}

static inline uint8_t texelFetch(uint8_t texId, int tx, int ty)
{
    const RC3D_Texture *t = &g_rc3dTextures[texId];
    tx &= RC3D_TEX_MASK;
    ty &= RC3D_TEX_MASK;
    return t->pix[(ty * RC3D_TEX_SIZE) + tx];
}

static void rc3dBuildDefaultTextures(void)
{
    int x, y, i;

    /* default: flat fallback */
    for (i = 0; i < 256; ++i) {
        for (y = 0; y < RC3D_TEX_SIZE; ++y) {
            for (x = 0; x < RC3D_TEX_SIZE; ++x) {
                g_rc3dTextures[i].pix[(y * RC3D_TEX_SIZE) + x] = (uint8_t)i;
            }
        }
    }

    /* --------------------------------------------------------- */
    /* wall textures used by your map: 10..16                    */
    /* --------------------------------------------------------- */

    /* 10 = red brick */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = 4;
            if ((y & 15) == 0) c = 1;
            if ((x & 15) == (((y >> 4) & 1) ? 8 : 0)) c = 15;
            g_rc3dTextures[10].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 11 = brown block */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = ((x ^ y) & 4) ? 6 : 7;
            if ((x & 15) == 0 || (y & 15) == 0) c = 15;
            g_rc3dTextures[11].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 12 = blue panels */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = 24 + ((x + y) & 3);
            if ((x & 15) == 0 || (y & 15) == 0) c = 31;
            if (((x - 8) & 15) == 0 && ((y - 8) & 15) == 0) c = 27;
            g_rc3dTextures[12].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 13 = green metal */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = ((x + y) & 8) ? 34 : 35;
            if ((x & 7) == 0) c = 31;
            g_rc3dTextures[13].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 14 = grey tech wall */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = ((x ^ y) & 8) ? 18 : 20;
            if ((x & 15) == 0 || (y & 15) == 0) c = 31;
            if ((x > 20 && x < 44) && (y > 20 && y < 44)) c = 22;
            g_rc3dTextures[14].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 15 = dark stone */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = ((x + (y * 3)) & 4) ? 8 : 9;
            if ((x & 15) == 0 || (y & 15) == 0) c = 15;
            g_rc3dTextures[15].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 16 = light stone */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = ((x ^ (y * 2)) & 4) ? 19 : 21;
            if ((x & 15) == 0 || (y & 15) == 0) c = 31;
            g_rc3dTextures[16].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* --------------------------------------------------------- */
    /* floor / ceiling textures used by your sectors             */
    /* --------------------------------------------------------- */

    /* 170 = floor checker */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = (((x >> 4) ^ (y >> 4)) & 1) ? 10 : 12;
            g_rc3dTextures[170].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 173 = darker floor checker */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = (((x >> 4) ^ (y >> 4)) & 1) ? 6 : 7;
            g_rc3dTextures[173].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 75 = ceiling tile */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = 18;
            if ((x & 15) == 0 || (y & 15) == 0) c = 20;
            if (((x - 8) & 15) == 0 && ((y - 8) & 15) == 0) c = 31;
            g_rc3dTextures[75].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    /* 79 = darker ceiling */
    for (y = 0; y < RC3D_TEX_SIZE; ++y) {
        for (x = 0; x < RC3D_TEX_SIZE; ++x) {
            uint8_t c = 17;
            if ((x & 15) == 0 || (y & 15) == 0) c = 19;
            if (((x - 8) & 15) == 0 && ((y - 8) & 15) == 0) c = 31;
            g_rc3dTextures[79].pix[(y * RC3D_TEX_SIZE) + x] = c;
        }
    }

    g_rc3dTexturesInit = 1;
}




static int readExact(FILE *f, void *dst, size_t size)
{
    return fread(dst, 1, size, f) == size;
}

void rc3dMapFreeBinary(RC3D_Map *map)
{
    if (!map) return;

    if (map->verts) {
        free((void *)map->verts);
    }
    if (map->walls) {
        free((void *)map->walls);
    }
    if (map->sectors) {
        free((void *)map->sectors);
    }

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
    if (!f) {
        return 0;
    }

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

    verts = (RC3D_Vec2 *)malloc(sizeof(RC3D_Vec2) * vertCount);
    walls = (RC3D_Wall *)malloc(sizeof(RC3D_Wall) * wallCount);
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
            !readExact(f, &walls[i].flags, sizeof(uint8_t))) {
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
            !readExact(f, &sectors[i].ceilColor, sizeof(uint8_t))) {
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

    /* basic validation */
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
    return 1;
}

void rc3dUnloadMapBinary(void)
{
    if (g_loadedMapValid) {
        rc3dMapFreeBinary(&g_loadedMap);
        g_loadedMapValid = 0;
    }

    g_map = &g_rc3dDemoMap;
}















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

static int pointInSector(float px, float py, int sectorIndex)
{
    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_Vec2 *verts = g_map->verts;

    int inside = 0;
    const int start = sec->wallStart;
    //const int end   = start + sec->wallCount;
    const int end   = start + sec->boundaryCount;

    for (int wi = start; wi < end; wi++) {
        const RC3D_Wall *w = &walls[wi];
        const RC3D_Vec2 *a = &verts[w->v0];
        const RC3D_Vec2 *b = &verts[w->v1];

        if ((a->y > py) != (b->y > py)) {
            const float xHit = a->x + ((py - a->y) * (b->x - a->x)) / (b->y - a->y);
            if (px < xHit) {
                inside ^= 1;
            }
        }
    }

    return inside;
}


static int findSectorForPoint(float x, float y)
{
    //const RC3D_Sector *secs = g_map->sectors; // <<--- why add this?? (AI said to add it, for... by why!??)
    for (int i = 0; i < g_map->sectorCount; i++) {
        if (pointInSector(x, y, i)) {
            return i;
        }
    }
    return -1;
}

static float pointSegmentDistSq(float px, float py, float ax, float ay, float bx, float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float apx = px - ax;
    const float apy = py - ay;
    const float abLenSq = (abx * abx) + (aby * aby);

    if (abLenSq <= RC3D_EPSILON) {
        const float dx = px - ax;
        const float dy = py - ay;
        return (dx * dx) + (dy * dy);
    }

    float t = ((apx * abx) + (apy * aby)) / abLenSq;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    const float cx = ax + (abx * t);
    const float cy = ay + (aby * t);
    const float dx = px - cx;
    const float dy = py - cy;
    return (dx * dx) + (dy * dy);
}

static inline int wallBlocksMovement(const RC3D_Wall *w)
{
    return (w->flags & (RC3D_WALL_SOLID | RC3D_WALL_MIDDLE)) != 0;
}

static int positionHitsSolidWallsInSector(float px, float py, float radius, int sectorIndex)
{
    if ((unsigned)sectorIndex >= (unsigned)g_map->sectorCount) {
        return 1;
    }

    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_Vec2 *verts = g_map->verts;
    const float radiusSq = radius * radius;

    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;

    for (int wi = start; wi < end; wi++) {
        const RC3D_Wall *w = &walls[wi];
        if ((w->flags & (RC3D_WALL_SOLID | RC3D_WALL_MIDDLE)) == 0) {
            continue;
        }

        const RC3D_Vec2 *a = &verts[w->v0];
        const RC3D_Vec2 *b = &verts[w->v1];

        if (pointSegmentDistSq(px, py, a->x, a->y, b->x, b->y) < radiusSq) {
            return 1;
        }
    }

    return 0;
}


static int canMoveToPosition(float px, float py, int newSector)
{
    const float playerRadius = 0.35f;

    if (newSector < 0) {
        return 0;
    }

    if (positionHitsSolidWallsInSector(px, py, playerRadius, g_player.sector)) {
        return 0;
    }

    if (newSector != g_player.sector) {
        if (positionHitsSolidWallsInSector(px, py, playerRadius, newSector)) {
            return 0;
        }
    }

    return 1;
}

static int movePlayerWithSectorTest(float newX, float newY)
{
    const int newSector = findSectorForPoint(newX, newY);

    if (!canMoveToPosition(newX, newY, newSector)) {
        return 0;
    }

    g_player.x = newX;
    g_player.y = newY;
    g_player.sector = newSector;
    return 1;
}


void screenupdate();

static inline void drawVerticalSpanFast(int x, int y0, int y1, uint8_t col)
{
    if ((unsigned)x >= SCREEN_W) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    if (y0 > y1) return;

    uint8_t *dst = &fb[(y0 * SCREEN_W) + x];
    const int count = y1 - y0 + 1;
    for (int i = 0; i < count; i++) {
        //if(col)
        *dst = col; // colour 0 is transparency
        dst += SCREEN_W;
        //screenupdate(); // debug - will use this to see per pixel draw, this is a very very crude way of showing whats happening <this is NOT to be left at final code>
    }
    
}

static inline void drawCeilSpan(int x, int y0, int y1, uint8_t col)
{
    drawVerticalSpanFast(x, y0, y1, col);
}

static inline void drawFloorSpan(int x, int y0, int y1, uint8_t col)
{
    drawVerticalSpanFast(x, y0, y1, col);
}

static inline void drawVerticalColumn(int x, int y0, int y1, uint8_t col)
{
    drawVerticalSpanFast(x, y0, y1, col);
}

static inline void drawTexturedVerticalSpan(
    int x,
    int y0,
    int y1,
    uint8_t texId,
    int texX,
    int texY0,
    int texY1
){
    if ((unsigned)x >= SCREEN_W) return;
    if (y0 > y1) return;

    if (y0 < 0) {
        if (y1 == y0) return;
        texY0 += ((texY1 - texY0) * (0 - y0)) / (y1 - y0);
        y0 = 0;
    }

    if (y1 >= SCREEN_H) {
        if (y1 == y0) return;
        texY1 = texY0 + ((texY1 - texY0) * ((SCREEN_H - 1) - y0)) / (y1 - y0);
        y1 = SCREEN_H - 1;
    }

    if (y0 > y1) return;

    uint8_t *dst = &fb[(y0 * SCREEN_W) + x];
    const int h = y1 - y0 + 1;

    if (h <= 1) {
        *dst = texelFetch(texId, texX, texY0);
        return;
    }

    const int dty = texY1 - texY0;
    for (int i = 0; i < h; ++i) {
        const int ty = texY0 + (dty * i) / (h - 1);
        *dst = texelFetch(texId, texX, ty);
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
    int horizon
){
    if ((unsigned)sx >= SCREEN_W) return;
    if (y0 > y1) return;

    if (y0 < 0) y0 = 0;
    if (y1 >= SCREEN_H) y1 = SCREEN_H - 1;
    if (y0 > y1) return;

    uint8_t *dst = &fb[(y0 * SCREEN_W) + sx];
    const float dz = planeZ - g_player.z;

    for (int y = y0; y <= y1; ++y) {
        const float inv = g_invRowDist[y];
        if (inv == 0.0f) {
            dst += SCREEN_W;
            continue;
        }

        const float t = dz * projPlaneGlobal * inv;

        const float wx = g_player.x + (rayDirX * t);
        const float wy = g_player.y + (rayDirY * t);

        const int tx = (int)(wx * (float)RC3D_TEX_SIZE);
        const int ty = (int)(wy * (float)RC3D_TEX_SIZE);

        *dst = texelFetch(texId, tx, ty);
        dst += SCREEN_W;
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

    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;

    for (int wallIndex = start; wallIndex < end; ++wallIndex) {
        if (wallIndex == ignoreWallIndexA || wallIndex == ignoreWallIndexB) {
            continue;
        }

        const RC3D_Wall *w = &walls[wallIndex];
        const RC3D_Vec2 *a = &verts[w->v0];
        const RC3D_Vec2 *b = &verts[w->v1];

        const float ax = a->x;
        const float ay = a->y;
        const float sx = b->x - ax;
        const float sy = b->y - ay;

        const float nx = sy;
        const float ny = -sx;
        if (((nx * rdx) + (ny * rdy)) >= 0.0f) {
            continue;
        }

        const float denom = (rdx * sy) - (rdy * sx);
        if (denom > -RC3D_EPSILON && denom < RC3D_EPSILON) {
            continue;
        }

        const float qpx = ax - rox;
        const float qpy = ay - roy;
        const float invDenom = 1.0f / denom;

        const float t = ((qpx * sy) - (qpy * sx)) * invDenom;
        if (t < minT || t >= hit.t) {
            continue;
        }

        const float u = ((qpx * rdy) - (qpy * rdx)) * invDenom;
        if (u < 0.0f || u > 1.0f) {
            continue;
        }

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
                horizon
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
                horizon
            );
        }
    }
}




static void drawBackground(void)
{
    /* no clear needed */
}

static inline void renderBandIfVisible(int sx, int y0, int y1, uint8_t color, int clipTop, int clipBottom)
{
    y0 = clampi(y0, clipTop, clipBottom);
    y1 = clampi(y1, clipTop, clipBottom);
    if (y0 > y1) return;

    drawVerticalColumn(sx, y0, y1, color);
    // black line for seems testing
    //fb[(y0 * SCREEN_W) + sx] = 16;
    //fb[(y1 * SCREEN_W) + sx] = 16;
}


static inline void renderTexturedBandIfVisible(
    int sx,
    int y0,
    int y1,
    uint8_t texId,
    float vTopWorld,
    float vBotWorld,
    float hitDist,
    float projPlane,
    int clipTop,
    int clipBottom
){
    const int origY0 = y0;
    const int origY1 = y1;

    if (origY0 > origY1) return;

    y0 = clampi(y0, clipTop, clipBottom);
    y1 = clampi(y1, clipTop, clipBottom);
    if (y0 > y1) return;

    const float worldSpan = vTopWorld - vBotWorld;
    if (worldSpan <= RC3D_EPSILON) return;

    /* texture Y at the TRUE unclipped wall endpoints */
    const float sampleTopWorld =
        g_player.z + (((float)(horizonGlobal - origY0) * hitDist) / projPlane);
    const float sampleBotWorld =
        g_player.z + (((float)(horizonGlobal - origY1) * hitDist) / projPlane);

    const int texYTop =
        (int)(((vTopWorld - sampleTopWorld) * (float)RC3D_TEX_SIZE) / worldSpan);
    const int texYBot =
        (int)(((vTopWorld - sampleBotWorld) * (float)RC3D_TEX_SIZE) / worldSpan);

    /* now map clipped screen y back into that original tex range */
    uint8_t *dst = &fb[(y0 * SCREEN_W) + sx];

    if (origY1 == origY0) {
        *dst = texelFetch(texId, wallTexXGlobal, texYTop);
        return;
    }

    const int fullH = origY1 - origY0;
    const int dty   = texYBot - texYTop;

    for (int y = y0; y <= y1; ++y) {
        const int rel = y - origY0;
        const int ty  = texYTop + (dty * rel) / fullH;
        *dst = texelFetch(texId, wallTexXGlobal, ty);
        dst += SCREEN_W;
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
    const float playerX = g_player.x;
    const float playerY = g_player.y;
    const float playerZ = g_player.z;

    int currentSector = g_player.sector;
    int clipTop = 0;
    int clipBottom = SCREEN_H - 1;

    int ignoreWallIndexA = -1;
    int ignoreWallIndexB = -1;

    float rayMinT = 0.0f;

    horizonGlobal = horizon;

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; ++step) {
        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return;
        }

        const RC3D_Sector *sec = &g_map->sectors[currentSector];

        if (clipTop <= clipBottom) {
            fillSectorColumnSpan(
                sx,
                clipTop,
                clipBottom,
                sec,
                horizon,
                rdx,
                rdy
            );
        } else {
            return;
        }

        const RC3D_WallHit hit = findNearestWallInSector(
            currentSector,
            playerX, playerY,
            rdx, rdy,
            ignoreWallIndexA,
            ignoreWallIndexB,
            rayMinT
        );

        if (!hit.hit) {
            return;
        }

        const RC3D_Wall *w = &g_map->walls[hit.wallIndex];
        const RC3D_Vec2 *va = &g_map->verts[w->v0];
        const RC3D_Vec2 *vb = &g_map->verts[w->v1];

        const float hitX = playerX + (rdx * hit.t);
        const float hitY = playerY + (rdy * hit.t);

        /* correct projected depth for perspective */
        const float correctedDist =
            ((hitX - playerX) * dirX) +
            ((hitY - playerY) * dirY);

        if (correctedDist <= RC3D_EPSILON) {
            return;
        }

        {
            const float wallDx = vb->x - va->x;
            const float wallDy = vb->y - va->y;
            const float wallLenSq = (wallDx * wallDx) + (wallDy * wallDy);

            float u = 0.0f;
            if (wallLenSq > RC3D_EPSILON) {
                u = (((hitX - va->x) * wallDx) + ((hitY - va->y) * wallDy)) / wallLenSq;
            }

            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;

            wallTexXGlobal = (int)(u * (float)RC3D_TEX_SIZE);
        }

        const float scale = projPlane / correctedDist;

        const int secTop = (int)(horizon - ((sec->ceilHeight  - playerZ) * scale));
        const int secBot = (int)(horizon - ((sec->floorHeight - playerZ) * scale));

        const uint8_t flags = w->flags;

        if (flags & RC3D_WALL_SOLID) {
            renderTexturedBandIfVisible(
                sx, secTop, secBot,
                w->midColor,
                sec->ceilHeight,
                sec->floorHeight,
                correctedDist,
                projPlane,
                clipTop, clipBottom
            );
            return;
        }

        if ((flags & RC3D_WALL_MIDDLE) && !(flags & RC3D_WALL_PORTAL)) {
            const int midTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
            const int midBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

            renderTexturedBandIfVisible(
                sx, midTopY, midBotY,
                w->midColor,
                w->openTop,
                w->openBottom,
                correctedDist,
                projPlane,
                clipTop, clipBottom
            );
            return;
        }

        if ((flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
            !(flags & RC3D_WALL_PORTAL) &&
            !(flags & RC3D_WALL_MIDDLE))
        {
            const int openTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
            const int openBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

            if (flags & RC3D_WALL_UPPER) {
                renderTexturedBandIfVisible(
                    sx, secTop, openTopY - 1,
                    w->upperColor,
                    sec->ceilHeight,
                    w->openTop,
                    correctedDist,
                    projPlane,
                    clipTop, clipBottom
                );
            }

            if (flags & RC3D_WALL_LOWER) {
                renderTexturedBandIfVisible(
                    sx, openBotY + 1, secBot,
                    w->lowerColor,
                    w->openBottom,
                    sec->floorHeight,
                    correctedDist,
                    projPlane,
                    clipTop, clipBottom
                );
            }

            return;
        }

        if (flags & RC3D_WALL_PORTAL) {
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

            if (flags & RC3D_WALL_UPPER) {
                renderTexturedBandIfVisible(
                    sx, secTop, openTop - 1,
                    w->upperColor,
                    sec->ceilHeight,
                    w->openTop,
                    correctedDist,
                    projPlane,
                    clipTop, clipBottom
                );
            }

            if (flags & RC3D_WALL_LOWER) {
                renderTexturedBandIfVisible(
                    sx, openBot + 1, secBot,
                    w->lowerColor,
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
                const int prevSector = (int)(sec - g_map->sectors);
                int entryWallInNext = -1;

                const int nextStart = nextSec->wallStart;
                const int nextEnd   = nextStart + nextSec->wallCount;

                for (int testIndex = nextStart; testIndex < nextEnd; ++testIndex) {
                    const RC3D_Wall *tw = &g_map->walls[testIndex];

                    if (tw->neighbour != prevSector) {
                        continue;
                    }

                    if ((tw->v0 == w->v1 && tw->v1 == w->v0) ||
                        (tw->v0 == w->v0 && tw->v1 == w->v1))
                    {
                        entryWallInNext = testIndex;
                        break;
                    }
                }

                ignoreWallIndexA = hit.wallIndex;
                ignoreWallIndexB = entryWallInNext;
                rayMinT = hit.t;
            }

            continue;
        }

        return;
    }
}



static void renderCurrentSectorColumns(void)
{
    const float halfFov   = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    const float projPlane = (SCREEN_W * 0.5f) / tanf(halfFov);
    const int horizon     = SCREEN_H / 2;

    projPlaneGlobal = projPlane;
    horizonGlobal = horizon;

    const float dirX = cosf(g_player.angle);
    const float dirY = sinf(g_player.angle);

    const float planeScale = tanf(halfFov);
    const float planeX = -dirY * planeScale;
    const float planeY =  dirX * planeScale;

    const float camStep = 2.0f / (float)(SCREEN_W - 1);
    float camX = -1.0f;

    for (int sx = 0; sx < SCREEN_W; ++sx) {
        const float rdx = dirX + (planeX * camX);
        const float rdy = dirY + (planeY * camX);

        renderColumnPortalTrace(
            sx,
            rdx,
            rdy,
            dirX,
            dirY,
            projPlane,
            horizon
        );

        camX += camStep;
    }
}



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

    drawLine(mapX,             mapY,             mapX + mapW - 1, mapY,              15);
    drawLine(mapX,             mapY,             mapX,            mapY + mapH - 1,   15);
    drawLine(mapX + mapW - 1,  mapY,             mapX + mapW - 1, mapY + mapH - 1,   15);
    drawLine(mapX,             mapY + mapH - 1,  mapX + mapW - 1, mapY + mapH - 1,   15);

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

    if (centerX >= left && centerX <= right && centerY >= top && centerY <= bottom) {
        drawRect(centerX - 1, centerY - 1, 3, 3, 15);
    }

    {
        int x0 = centerX;
        int y0 = centerY;
        int x1 = centerX + (int)(cosf(g_player.angle) * 12.0f);
        int y1 = centerY + (int)(sinf(g_player.angle) * 12.0f);

        if (clipLineToRect(&x0, &y0, &x1, &y1, left, top, right, bottom)) {
            drawLine(x0, y0, x1, y1, 31);
        }
    }
}


void rc3dInit(void)
{
    if (!g_rc3dTexturesInit) {
        rc3dBuildDefaultTextures();
    }

    if (!g_planeTablesInit) {
        rc3dBuildPlaneTables();
    }

    if (!g_rayTableInit) {
        rc3dBuildRayTable();
    }

    g_player.x = g_map->startX;
    g_player.y = g_map->startY;
    g_player.angle = g_map->startAngle;
    g_player.sector = g_map->startSector;

    if (g_player.sector >= 0 && g_player.sector < g_map->sectorCount) {
        g_player.z = g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;
    } else {
        g_player.z = RC3D_PLAYER_EYE_HEIGHT;
    }

    g_player.vz = 0.0f;
}

void rc3dUpdate(float dt, const uint8_t *keys, int mouseDx)
{
    float moveX = 0.0f;
    float moveY = 0.0f;

    const float forwardX = cosf(g_player.angle);
    const float forwardY = sinf(g_player.angle);
    const float rightX   = -sinf(g_player.angle);
    const float rightY   =  cosf(g_player.angle);

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

        if (!movePlayerWithSectorTest(g_player.x + moveX, g_player.y + moveY)) {
            if (!movePlayerWithSectorTest(g_player.x + moveX, g_player.y)) {
                movePlayerWithSectorTest(g_player.x, g_player.y + moveY);
            }
        }
    }

    if (g_player.sector >= 0 && g_player.sector < g_map->sectorCount) {
        const float targetZ = g_map->sectors[g_player.sector].floorHeight + RC3D_PLAYER_EYE_HEIGHT;

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
                const float riseSpeed = dz * 10.0f;   /* bigger = snappier */
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
    drawBackground();
    renderCurrentSectorColumns();

#if RC3D_DRAW_MINIMAP
    drawMiniMap();
#endif

#if RC3D_DRAW_HUD
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "SECTOR %d", g_player.sector);
        drawText(8, 8, buf, 15);
    }
#endif
}