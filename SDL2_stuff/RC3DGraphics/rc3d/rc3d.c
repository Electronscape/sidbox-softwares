#include "rc3d.h"

#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>

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

/* ------------------------------------------------------------------------- */
/* helpers                                                                   */
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

static int pointInSector(float px, float py, int sectorIndex)
{
    const RC3D_Sector *sec = &g_map->sectors[sectorIndex];
    const RC3D_Wall *walls = g_map->walls;
    const RC3D_Vec2 *verts = g_map->verts;

    int inside = 0;
    const int start = sec->wallStart;
    const int end   = start + sec->wallCount;

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

    for (int wallIndex = start; wallIndex < end; wallIndex++) {
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
        if ((nx * rdx) + (ny * rdy) >= 0.0f) {
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
    int horizon
){
    if (!sec) return;
    if (y0 > y1) return;

    if (y0 < horizon) {
        int topEnd = horizon - 1;
        if (topEnd > y1) topEnd = y1;
        if (y0 <= topEnd) {
            drawCeilSpan(sx, y0, topEnd, sec->ceilColor);
        }
    }

    if (y1 >= horizon) {
        int botBeg = horizon;
        if (botBeg < y0) botBeg = y0;
        if (botBeg <= y1) {
            drawFloorSpan(sx, botBeg, y1, sec->floorColor);
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

    fillSectorColumnSpan(sx, 0, SCREEN_H - 1, &g_map->sectors[currentSector], horizon);

    for (int step = 0; step < RC3D_MAX_PORTAL_STEPS; step++) {
        if ((unsigned)currentSector >= (unsigned)g_map->sectorCount) {
            return;
        }

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
            return;
        }

        const RC3D_Wall *w = &g_map->walls[hit.wallIndex];

        const float hitX = playerX + (rdx * hit.t);
        const float hitY = playerY + (rdy * hit.t);

        const float correctedDist = ((hitX - playerX) * dirX) + ((hitY - playerY) * dirY);
        if (correctedDist <= RC3D_EPSILON) {
            return;
        }

        const float scale = projPlane / correctedDist;

        const int secTop = (int)(horizon - ((sec->ceilHeight  - playerZ) * scale));
        const int secBot = (int)(horizon - ((sec->floorHeight - playerZ) * scale));

        const uint8_t flags = w->flags;

        if (flags & RC3D_WALL_SOLID) {
            renderBandIfVisible(sx, secTop, secBot, w->midColor, clipTop, clipBottom);
            return;
        }

        if ((flags & RC3D_WALL_MIDDLE) && !(flags & RC3D_WALL_PORTAL)) {
            const int midTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
            const int midBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

            renderBandIfVisible(sx, midTopY, midBotY, w->midColor, clipTop, clipBottom);
            return;
        }

        if ((flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
            !(flags & RC3D_WALL_PORTAL) &&
            !(flags & RC3D_WALL_MIDDLE)) {
            const int openTopY = (int)(horizon - ((w->openTop    - playerZ) * scale));
            const int openBotY = (int)(horizon - ((w->openBottom - playerZ) * scale));

            if (flags & RC3D_WALL_UPPER) {
                renderBandIfVisible(sx, secTop, openTopY - 1, w->upperColor, clipTop, clipBottom);
            }

            if (flags & RC3D_WALL_LOWER) {
                renderBandIfVisible(sx, openBotY + 1, secBot, w->lowerColor, clipTop, clipBottom);
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
                renderBandIfVisible(sx, secTop, openTop - 1, w->upperColor, clipTop, clipBottom);
            }

            if (flags & RC3D_WALL_LOWER) {
                renderBandIfVisible(sx, openBot + 1, secBot, w->lowerColor, clipTop, clipBottom);
            }

            if (openTop > openBot) {
                return;
            }

            fillSectorColumnSpan(sx, openTop, openBot, nextSec, horizon);

            clipTop = openTop;
            clipBottom = openBot;

            {
                const int prevSector = currentSector;
                int entryWallInNext = -1;

                const int nextStart = nextSec->wallStart;
                const int nextEnd   = nextStart + nextSec->wallCount;

                for (int testIndex = nextStart; testIndex < nextEnd; testIndex++) {
                    const RC3D_Wall *tw = &g_map->walls[testIndex];

                    if (tw->neighbour != prevSector) {
                        continue;
                    }

                    if ((tw->v0 == w->v1 && tw->v1 == w->v0) ||
                        (tw->v0 == w->v0 && tw->v1 == w->v1)) {
                        entryWallInNext = testIndex;
                        break;
                    }
                }

                currentSector = nextSectorIndex;
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
    const float halfFov = (RC3D_FOV_DEG * 0.5f) * (float)(M_PI / 180.0f);
    const float projPlane = (SCREEN_W * 0.5f) / tanf(halfFov);
    const int horizon = SCREEN_H / 2;

    const float dirX = cosf(g_player.angle);
    const float dirY = sinf(g_player.angle);

    const float planeScale = tanf(halfFov);
    const float planeX = -dirY * planeScale;
    const float planeY =  dirX * planeScale;

    const float camStep = 2.0f / (float)(SCREEN_W - 1);
    float camX = -1.0f;

    for (int sx = 0; sx < SCREEN_W; sx+=1) {
        const float rdx = dirX + (planeX * camX);
        const float rdy = dirY + (planeY * camX);

        renderColumnPortalTrace(sx, rdx, rdy, dirX, dirY, projPlane, horizon);

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