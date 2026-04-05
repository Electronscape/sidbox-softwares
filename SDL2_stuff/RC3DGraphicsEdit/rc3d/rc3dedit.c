#include "rc3dedit.h"

#include <SDL2/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../gfx.h"
#include "rc3d_map.h"

#define RC3D_WALL_PORTAL   0x01
#define RC3D_WALL_UPPER    0x02
#define RC3D_WALL_MIDDLE   0x04
#define RC3D_WALL_LOWER    0x08
#define RC3D_WALL_SOLID    0x10

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define ED_MAX_VERTS            4096
#define ED_MAX_WALLS            4096
#define ED_MAX_SECTORS          512
#define ED_MAX_DRAFT_POINTS     128
#define ED_GRID_STEP            1.0f
#define ED_EPSILON              0.001f
#define ED_PICK_DIST_PX         10


#define ED_TEXT_COL             30
#define ED_UI_BG                22
#define ED_UI_BORDER            2

#define ED_GRID_MAJOR_COL       24  // blue
#define ED_GRID_MINOR_COL       22  // darker blue

#define ED_VERT_COL                 31
#define ED_WALL_COL                 2
#define ED_PORTAL_COL               27  // green
#define ED_COLOUR_SELECTED_SECTOR   29  // yellow

#define ED_COLOUR_HOVER_WALL        62  // dark magenta
#define ED_COLOUR_SELECTED_WALL     63  // magenta

#define ED_DRAFT_COL                26  // light gray

#define ED_CURSOR_COL           11

#define ED_START_COL            2   // white
#define ED_HOME_GRID_COL        40


typedef enum {
    ED_SEL_NONE = 0,
    ED_SEL_VERTEX,
    ED_SEL_WALL,
    ED_SEL_SECTOR
} EdSelectionType;


typedef struct {
    float x;
    float y;
} EdVec2;

typedef struct {
    int v0;
    int v1;
    int neighbour;
    float openBottom;
    float openTop;
    uint8_t upperColor;
    uint8_t midColor;
    uint8_t lowerColor;
    uint8_t flags;
} EdWall;

typedef struct {
    int wallStart;
    int wallCount;
    int boundaryCount;
    float floorHeight;
    float ceilHeight;
    uint8_t floorColor;
    uint8_t ceilColor;
} EdSector;

typedef struct {
    EdVec2 verts[ED_MAX_VERTS];
    int vertCount;

    EdWall walls[ED_MAX_WALLS];
    int wallCount;

    EdSector sectors[ED_MAX_SECTORS];
    int sectorCount;

    int startSector;
    float startX;
    float startY;
    float startAngle;
} EditorMap;

typedef struct {
    float camX;
    float camY;
    float zoom;
    int draggingPan;
    int lastMouseX;
    int lastMouseY;

    int draftVertIndices[ED_MAX_DRAFT_POINTS];
    int draftCount;

    float sectorFloor;
    float sectorCeil;
    uint8_t sectorFloorColor;
    uint8_t sectorCeilColor;

    int hoverVert;
    int hoverWall;
    int hoverSector;

    int selectedVert;
    int selectedWall;
    int selectedSector;
    int draggingVertex;
    int draggingWall;
    int draggingSector;

    /* vertex drag anchor */
    float dragStartWorldX;
    float dragStartWorldY;
    float dragVertexStartX;
    float dragVertexStartY;

    /* wall drag anchor */
    float dragWallStartWorldX;
    float dragWallStartWorldY;
    float dragWallV0StartX;
    float dragWallV0StartY;
    float dragWallV1StartX;
    float dragWallV1StartY;

    /* sector drag anchor */
    float dragSectorStartWorldX;
    float dragSectorStartWorldY;
    int   dragSectorVertCount;
    int   dragSectorVertIndices[ED_MAX_VERTS];
    float dragSectorVertStartX[ED_MAX_VERTS];
    float dragSectorVertStartY[ED_MAX_VERTS];

    EdSelectionType selectionType;

    int prevLeftDown;
    int prevRightDown;
    int prevMiddleDown;
    uint8_t prevKeys[SDL_NUM_SCANCODES];
} EditorState;



static EditorMap g_edMap;
static EditorState g_ed;

static float absf_local(float v)
{
    return (v < 0.0f) ? -v : v;
}

static int clampi_local(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float clampf_local(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float snapf(float v)
{
    const float inv = 1.0f / ED_GRID_STEP;
    return roundf(v * inv) / inv;
}

static int keyPressedOnce(const uint8_t *keys, SDL_Scancode sc)
{
    return keys[sc] && !g_ed.prevKeys[sc];
}

static void screenToWorld(int sx, int sy, float *wx, float *wy)
{
    *wx = g_ed.camX + ((float)sx - (SCREEN_W * 0.5f)) / g_ed.zoom;
    *wy = g_ed.camY + ((float)sy - (SCREEN_H * 0.5f)) / g_ed.zoom;
}

static void worldToScreen(float wx, float wy, int *sx, int *sy)
{
    *sx = (int)lroundf((wx - g_ed.camX) * g_ed.zoom + (SCREEN_W * 0.5f));
    *sy = (int)lroundf((wy - g_ed.camY) * g_ed.zoom + (SCREEN_H * 0.5f));
}

static int addVertex(float x, float y)
{
    if (g_edMap.vertCount >= ED_MAX_VERTS) {
        return -1;
    }
    g_edMap.verts[g_edMap.vertCount].x = x;
    g_edMap.verts[g_edMap.vertCount].y = y;
    g_edMap.vertCount++;
    return g_edMap.vertCount - 1;
}

static int findVertexExact(float x, float y)
{
    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (absf_local(g_edMap.verts[i].x - x) < ED_EPSILON &&
            absf_local(g_edMap.verts[i].y - y) < ED_EPSILON) {
            return i;
        }
    }
    return -1;
}

static int findOrAddVertex(float x, float y)
{
    const int idx = findVertexExact(x, y);
    if (idx >= 0) return idx;
    return addVertex(x, y);
}

static int pointInSector(float px, float py, int sectorIndex)
{
    const EdSector *sec = &g_edMap.sectors[sectorIndex];
    int inside = 0;

    for (int i = 0; i < sec->boundaryCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];

        const int condY = ((a->y > py) != (b->y > py));
        if (condY) {
            const float t = (py - a->y) / (b->y - a->y);
            const float xHit = a->x + ((b->x - a->x) * t);
            if (px < xHit) {
                inside ^= 1;
            }
        }
    }

    return inside;
}

static float sectorAreaAbs(int sectorIndex)
{
    const EdSector *sec = &g_edMap.sectors[sectorIndex];
    float area = 0.0f;

    for (int i = 0; i < sec->boundaryCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];

        area += (a->x * b->y) - (b->x * a->y);
    }

    if (area < 0.0f) area = -area;
    return area * 0.5f;
}

static int findSectorForPoint(float x, float y)
{
    int bestSector = -1;
    float bestArea = 99999999.0f;

    for (int i = 0; i < g_edMap.sectorCount; i++) {
        if (pointInSector(x, y, i)) {
            const float area = sectorAreaAbs(i);
            if (area < bestArea) {
                bestArea = area;
                bestSector = i;
            }
        }
    }

    return bestSector;
}

static float distPointSegSq(float px, float py, float ax, float ay, float bx, float by)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float apx = px - ax;
    const float apy = py - ay;
    const float abLenSq = (abx * abx) + (aby * aby);

    if (abLenSq <= ED_EPSILON) {
        const float dx = px - ax;
        const float dy = py - ay;
        return (dx * dx) + (dy * dy);
    }

    float t = ((apx * abx) + (apy * aby)) / abLenSq;
    t = clampf_local(t, 0.0f, 1.0f);

    const float cx = ax + (abx * t);
    const float cy = ay + (aby * t);
    const float dx = px - cx;
    const float dy = py - cy;
    return (dx * dx) + (dy * dy);
}

static int findReversedWall(int v0, int v1)
{
    for (int i = 0; i < g_edMap.wallCount; i++) {
        if (g_edMap.walls[i].v0 == v1 && g_edMap.walls[i].v1 == v0) {
            return i;
        }
    }
    return -1;
}

static void setPortalPair(int wallA, int sectorA, int wallB, int sectorB)
{
    EdWall *a = &g_edMap.walls[wallA];
    EdWall *b = &g_edMap.walls[wallB];
    const EdSector *sa = &g_edMap.sectors[sectorA];
    const EdSector *sb = &g_edMap.sectors[sectorB];

    const float openBottom = (sa->floorHeight > sb->floorHeight) ? sa->floorHeight : sb->floorHeight;
    const float openTop    = (sa->ceilHeight  < sb->ceilHeight)  ? sa->ceilHeight  : sb->ceilHeight;

    a->neighbour = sectorB;
    b->neighbour = sectorA;

    a->openBottom = openBottom;
    a->openTop = openTop;
    b->openBottom = openBottom;
    b->openTop = openTop;

    a->upperColor = a->midColor;
    a->lowerColor = a->midColor;
    b->upperColor = b->midColor;
    b->lowerColor = b->midColor;

    a->flags = RC3D_WALL_PORTAL;
    b->flags = RC3D_WALL_PORTAL;

    if (sb->ceilHeight < sa->ceilHeight) a->flags |= RC3D_WALL_UPPER;
    if (sb->floorHeight > sa->floorHeight) a->flags |= RC3D_WALL_LOWER;
    if (sa->ceilHeight < sb->ceilHeight) b->flags |= RC3D_WALL_UPPER;
    if (sa->floorHeight > sb->floorHeight) b->flags |= RC3D_WALL_LOWER;

    if (openTop <= openBottom) {
        a->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        b->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        a->neighbour = -1;
        b->neighbour = -1;
        a->openBottom = 0.0f;
        a->openTop = 0.0f;
        b->openBottom = 0.0f;
        b->openTop = 0.0f;
    }
}

static void syncAllPortals(void)
{
    for (int wi = 0; wi < g_edMap.wallCount; wi++) {
        EdWall *w = &g_edMap.walls[wi];
        if (w->neighbour >= 0) {
            w->neighbour = -1;
            w->openBottom = 0.0f;
            w->openTop = 0.0f;
            w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        }
    }

    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];
        for (int i = 0; i < sec->boundaryCount; i++) {
            const int wi = sec->wallStart + i;
            EdWall *w = &g_edMap.walls[wi];
            const int other = findReversedWall(w->v0, w->v1);
            if (other >= 0 && other != wi) {
                int otherSector = -1;
                for (int ss = 0; ss < g_edMap.sectorCount; ss++) {
                    const EdSector *s2 = &g_edMap.sectors[ss];
                    if (other >= s2->wallStart && other < (s2->wallStart + s2->boundaryCount)) {
                        otherSector = ss;
                        break;
                    }
                }
                if (otherSector >= 0) {
                    setPortalPair(wi, s, other, otherSector);
                }
            }
        }
    }
}




static void rebuildSectorBoundaryCounts(void)
{
    for (int s = 0; s < g_edMap.sectorCount; s++) {
        int count = 0;
        EdSector *sec = &g_edMap.sectors[s];

        for (int i = 0; i < sec->wallCount; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            if (w->v0 >= 0 && w->v1 >= 0) {
                count++;
            } else {
                break;
            }
        }

        sec->boundaryCount = count;
    }
}

static int sectorCollectUniqueVerts(int sectorIndex, int *outIndices, float *outX, float *outY, int maxOut)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return 0;

    const EdSector *sec = &g_edMap.sectors[sectorIndex];
    int count = 0;

    for (int i = 0; i < sec->wallCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        const int verts[2] = { w->v0, w->v1 };

        for (int k = 0; k < 2; k++) {
            const int vi = verts[k];
            int exists = 0;

            for (int j = 0; j < count; j++) {
                if (outIndices[j] == vi) {
                    exists = 1;
                    break;
                }
            }

            if (!exists) {
                if (count >= maxOut) return count;
                outIndices[count] = vi;
                outX[count] = g_edMap.verts[vi].x;
                outY[count] = g_edMap.verts[vi].y;
                count++;
            }
        }
    }

    return count;
}

static void beginSectorDrag(int sectorIndex, float worldX, float worldY)
{
    g_ed.draggingSector = 0;
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    g_ed.dragSectorStartWorldX = worldX;
    g_ed.dragSectorStartWorldY = worldY;

    g_ed.dragSectorVertCount = sectorCollectUniqueVerts(
        sectorIndex,
        g_ed.dragSectorVertIndices,
        g_ed.dragSectorVertStartX,
        g_ed.dragSectorVertStartY,
        ED_MAX_VERTS
    );

    if (g_ed.dragSectorVertCount > 0) {
        g_ed.draggingSector = 1;
    }
}

static void dragSelectedSectorTo(float worldX, float worldY)
{
    if (!g_ed.draggingSector || g_ed.selectedSector < 0) return;

    const float dx = snapf(worldX - g_ed.dragSectorStartWorldX);
    const float dy = snapf(worldY - g_ed.dragSectorStartWorldY);

    for (int i = 0; i < g_ed.dragSectorVertCount; i++) {
        const int vi = g_ed.dragSectorVertIndices[i];
        g_edMap.verts[vi].x = snapf(g_ed.dragSectorVertStartX[i] + dx);
        g_edMap.verts[vi].y = snapf(g_ed.dragSectorVertStartY[i] + dy);
    }

    syncAllPortals();
}

static void compactDeletedWalls(void)
{
    int write = 0;

    for (int read = 0; read < g_edMap.wallCount; read++) {
        if (g_edMap.walls[read].v0 < 0 || g_edMap.walls[read].v1 < 0) {
            continue;
        }

        if (write != read) {
            g_edMap.walls[write] = g_edMap.walls[read];
        }
        write++;
    }

    g_edMap.wallCount = write;
}

static void rebuildSectorWallLayout(void)
{
    EdWall tempWalls[ED_MAX_WALLS];
    int tempCount = 0;

    for (int s = 0; s < g_edMap.sectorCount; s++) {
        EdSector *sec = &g_edMap.sectors[s];
        const int oldStart = sec->wallStart;
        const int oldCount = sec->wallCount;

        sec->wallStart = tempCount;
        sec->wallCount = 0;
        sec->boundaryCount = 0;

        for (int i = 0; i < oldCount; i++) {
            const EdWall *w = &g_edMap.walls[oldStart + i];
            if (w->v0 < 0 || w->v1 < 0) {
                continue;
            }

            if (tempCount >= ED_MAX_WALLS) {
                break;
            }

            tempWalls[tempCount++] = *w;
            sec->wallCount++;
            sec->boundaryCount++;
        }
    }

    for (int i = 0; i < tempCount; i++) {
        g_edMap.walls[i] = tempWalls[i];
    }
    g_edMap.wallCount = tempCount;
}

static void deleteWallByIndex(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return;
    }

    g_edMap.walls[wallIndex].v0 = -1;
    g_edMap.walls[wallIndex].v1 = -1;

    rebuildSectorWallLayout();
    syncAllPortals();

    g_ed.hoverWall = -1;
    g_ed.selectedWall = -1;
}

static void deleteVertexByIndex(int vertIndex)
{
    if (vertIndex < 0 || vertIndex >= g_edMap.vertCount) {
        return;
    }

    /* mark any wall using this vertex as deleted */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];
        if (w->v0 == vertIndex || w->v1 == vertIndex) {
            w->v0 = -1;
            w->v1 = -1;
        }
    }

    /* shift vertices down */
    for (int i = vertIndex; i < (g_edMap.vertCount - 1); i++) {
        g_edMap.verts[i] = g_edMap.verts[i + 1];
    }
    g_edMap.vertCount--;

    /* fix wall vertex indices */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];

        if (w->v0 > vertIndex) w->v0--;
        if (w->v1 > vertIndex) w->v1--;
    }

    /* fix draft indices */
    for (int i = 0; i < g_ed.draftCount; ) {
        if (g_ed.draftVertIndices[i] == vertIndex) {
            for (int j = i; j < (g_ed.draftCount - 1); j++) {
                g_ed.draftVertIndices[j] = g_ed.draftVertIndices[j + 1];
            }
            g_ed.draftCount--;
            continue;
        }

        if (g_ed.draftVertIndices[i] > vertIndex) {
            g_ed.draftVertIndices[i]--;
        }
        i++;
    }

    rebuildSectorWallLayout();
    syncAllPortals();

    g_ed.hoverVert = -1;
    g_ed.selectedVert = -1;
    g_ed.hoverWall = -1;
    g_ed.selectedWall = -1;
}

static int findSectorOwningWall(int wallIndex)
{
    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];
        if (wallIndex >= sec->wallStart && wallIndex < (sec->wallStart + sec->wallCount)) {
            return s;
        }
    }
    return -1;
}

static void makeWallSolid(int wallIndex, uint8_t midColor)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];
    w->neighbour = -1;
    w->openBottom = 0.0f;
    w->openTop = 0.0f;
    w->upperColor = 0;
    w->midColor = midColor;
    w->lowerColor = 0;
    w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
}

static void makeWallWindow(int wallIndex, float openBottom, float openTop,
                           uint8_t upperColor, uint8_t lowerColor)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];
    w->neighbour = -1;
    w->openBottom = openBottom;
    w->openTop = openTop;
    w->upperColor = upperColor;
    w->midColor = 0;
    w->lowerColor = lowerColor;
    w->flags = RC3D_WALL_UPPER | RC3D_WALL_LOWER;
}

static void makeWallDoor(int wallIndex, float openBottom, float openTop, uint8_t midColor)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];
    w->neighbour = -1;
    w->openBottom = openBottom;
    w->openTop = openTop;
    w->upperColor = 0;
    w->midColor = midColor;
    w->lowerColor = 0;
    w->flags = RC3D_WALL_MIDDLE;
}

static void refreshSelectedPortalFromReverse(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];
    const int other = findReversedWall(w->v0, w->v1);
    const int sectorA = findSectorOwningWall(wallIndex);

    if (other < 0 || sectorA < 0) {
        return;
    }

    const int sectorB = findSectorOwningWall(other);
    if (sectorB < 0 || sectorA == sectorB) {
        return;
    }

    setPortalPair(wallIndex, sectorA, other, sectorB);
}

static void tryMakeWallPortal(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];
    const int other = findReversedWall(w->v0, w->v1);
    if (other < 0) {
        return;
    }

    const int sectorA = findSectorOwningWall(wallIndex);
    const int sectorB = findSectorOwningWall(other);
    if (sectorA < 0 || sectorB < 0 || sectorA == sectorB) {
        return;
    }

    setPortalPair(wallIndex, sectorA, other, sectorB);
}

static int mergeVertexInto(int srcVert, int dstVert)
{
    if (srcVert < 0 || dstVert < 0) return 0;
    if (srcVert == dstVert) return 0;
    if (srcVert >= g_edMap.vertCount || dstVert >= g_edMap.vertCount) return 0;

    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];

        if (w->v0 == srcVert) w->v0 = dstVert;
        if (w->v1 == srcVert) w->v1 = dstVert;

        if (w->v0 > srcVert) w->v0--;
        if (w->v1 > srcVert) w->v1--;
    }

    for (int i = srcVert; i < (g_edMap.vertCount - 1); i++) {
        g_edMap.verts[i] = g_edMap.verts[i + 1];
    }
    g_edMap.vertCount--;

    for (int i = 0; i < g_ed.draftCount; i++) {
        if (g_ed.draftVertIndices[i] == srcVert) g_ed.draftVertIndices[i] = dstVert;
        if (g_ed.draftVertIndices[i] > srcVert) g_ed.draftVertIndices[i]--;
    }

    syncAllPortals();

    g_ed.selectedVert = -1;
    g_ed.hoverVert = -1;
    return 1;
}

static int findVertexNearWorld(float wx, float wy, float distWorld, int ignoreIndex)
{
    const float distSq = distWorld * distWorld;
    int best = -1;
    float bestD2 = distSq;

    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (i == ignoreIndex) continue;

        const float dx = g_edMap.verts[i].x - wx;
        const float dy = g_edMap.verts[i].y - wy;
        const float d2 = (dx * dx) + (dy * dy);

        if (d2 <= bestD2) {
            bestD2 = d2;
            best = i;
        }
    }

    return best;
}

static int splitWallAtSelected(int wallIndex, float wx, float wy)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return 0;
    if (g_edMap.wallCount >= ED_MAX_WALLS) return 0;
    if (g_edMap.vertCount >= ED_MAX_VERTS) return 0;

    const int sectorIndex = findSectorOwningWall(wallIndex);
    if (sectorIndex < 0) return 0;

    EdSector *sec = &g_edMap.sectors[sectorIndex];
    const int localIndex = wallIndex - sec->wallStart;
    if (localIndex < 0 || localIndex >= sec->wallCount) return 0;

    const float sx = snapf(wx);
    const float sy = snapf(wy);
    const int newVert = findOrAddVertex(sx, sy);
    if (newVert < 0) return 0;

    EdWall original = g_edMap.walls[wallIndex];

    for (int i = g_edMap.wallCount; i > wallIndex + 1; i--) {
        g_edMap.walls[i] = g_edMap.walls[i - 1];
    }
    g_edMap.wallCount++;

    g_edMap.walls[wallIndex].v1 = newVert;
    g_edMap.walls[wallIndex + 1] = original;
    g_edMap.walls[wallIndex + 1].v0 = newVert;

    sec->wallCount++;
    if (localIndex < sec->boundaryCount) {
        sec->boundaryCount++;
    }

    for (int s = sectorIndex + 1; s < g_edMap.sectorCount; s++) {
        g_edMap.sectors[s].wallStart++;
    }

    syncAllPortals();
    return 1;
}



static void selectBestHoverTarget(const uint8_t *keys, float worldX, float worldY)
{
    (void)worldX;
    (void)worldY;

    const int wantVertex = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    const int wantWall   = keys[SDL_SCANCODE_LCTRL]  || keys[SDL_SCANCODE_RCTRL];
    const int wantSector = keys[SDL_SCANCODE_LALT]   || keys[SDL_SCANCODE_RALT];

    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    g_ed.selectionType = ED_SEL_NONE;

    if (wantVertex) {
        if (g_ed.hoverVert >= 0) {
            g_ed.selectionType = ED_SEL_VERTEX;
            g_ed.selectedVert = g_ed.hoverVert;
            return;
        }
    }

    if (wantWall) {
        if (g_ed.hoverWall >= 0) {
            g_ed.selectionType = ED_SEL_WALL;
            g_ed.selectedWall = g_ed.hoverWall;
            return;
        }
    }

    if (wantSector) {
        if (g_ed.hoverSector >= 0) {
            g_ed.selectionType = ED_SEL_SECTOR;
            g_ed.selectedSector = g_ed.hoverSector;
            return;
        }
    }

    /* default behaviour if no modifier is held:
       vertex first, then wall, then sector */
    if (g_ed.hoverVert >= 0) {
        g_ed.selectionType = ED_SEL_VERTEX;
        g_ed.selectedVert = g_ed.hoverVert;
        return;
    }

    if (g_ed.hoverWall >= 0) {
        g_ed.selectionType = ED_SEL_WALL;
        g_ed.selectedWall = g_ed.hoverWall;
        return;
    }

    if (g_ed.hoverSector >= 0) {
        g_ed.selectionType = ED_SEL_SECTOR;
        g_ed.selectedSector = g_ed.hoverSector;
        return;
    }
}


static void clearDraft(void)
{
    g_ed.draftCount = 0;
}

static void beginNewMap(void)
{
    memset(&g_edMap, 0, sizeof(g_edMap));
    g_edMap.startSector = 0;
    g_edMap.startX = 0.0f;
    g_edMap.startY = 0.0f;
    g_edMap.startAngle = 0.0f;

    // id like to not change the location of the camera or zoom on load - maybe at start up
    g_ed.camX = 0.0f;
    g_ed.camY = 0.0f;
    g_ed.zoom = 32.0f;    

    g_ed.draggingPan = 0;
    g_ed.lastMouseX = 0;
    g_ed.lastMouseY = 0;
    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = -1;

    g_ed.sectorFloor = 0.0f;
    g_ed.sectorCeil = 2.0f;
    g_ed.sectorFloorColor = 173;
    g_ed.sectorCeilColor = 79;

    g_ed.selectionType = ED_SEL_NONE;
    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;

    clearDraft();
    memset(g_ed.prevKeys, 0, sizeof(g_ed.prevKeys));
    g_ed.prevLeftDown = 0;
    g_ed.prevRightDown = 0;
    g_ed.prevMiddleDown = 0;
}

static int saveTextMap(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "MAPEDIT1\n");
    fprintf(f, "START %.6f %.6f %.6f %d\n",
            g_edMap.startX, g_edMap.startY, g_edMap.startAngle, g_edMap.startSector);

    fprintf(f, "VERTS %d\n", g_edMap.vertCount);
    for (int i = 0; i < g_edMap.vertCount; i++) {
        fprintf(f, "%.6f %.6f\n", g_edMap.verts[i].x, g_edMap.verts[i].y);
    }

    fprintf(f, "WALLS %d\n", g_edMap.wallCount);
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        fprintf(f, "%d %d %d %.6f %.6f %u %u %u %u\n",
                w->v0, w->v1, w->neighbour,
                w->openBottom, w->openTop,
                (unsigned)w->upperColor,
                (unsigned)w->midColor,
                (unsigned)w->lowerColor,
                (unsigned)w->flags);
    }

    fprintf(f, "SECTORS %d\n", g_edMap.sectorCount);
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f, "%d %d %d %.6f %.6f %u %u\n",
                s->wallStart, s->wallCount, s->boundaryCount,
                s->floorHeight, s->ceilHeight,
                (unsigned)s->floorColor,
                (unsigned)s->ceilColor);
    }

    fclose(f);
    return 1;
}

static int loadTextMap(const char *path)
{
    FILE *f = fopen(path, "r");
    char tag[64];

    if (!f) return 0;
    beginNewMap();

    if (fscanf(f, "%63s", tag) != 1 || strcmp(tag, "MAPEDIT1") != 0) {
        fclose(f);
        return 0;
    }

    if (fscanf(f, "%63s", tag) != 1 || strcmp(tag, "START") != 0) {
        fclose(f);
        return 0;
    }
    fscanf(f, "%f %f %f %d",
           &g_edMap.startX, &g_edMap.startY, &g_edMap.startAngle, &g_edMap.startSector);

    int count = 0;

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "VERTS") != 0) {
        fclose(f);
        return 0;
    }
    if (count > ED_MAX_VERTS) {
        fclose(f);
        return 0;
    }
    g_edMap.vertCount = count;
    for (int i = 0; i < count; i++) {
        fscanf(f, "%f %f", &g_edMap.verts[i].x, &g_edMap.verts[i].y);
    }

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "WALLS") != 0) {
        fclose(f);
        return 0;
    }
    if (count > ED_MAX_WALLS) {
        fclose(f);
        return 0;
    }
    g_edMap.wallCount = count;
    for (int i = 0; i < count; i++) {
        unsigned uc, mc, lc, flags;
        fscanf(f, "%d %d %d %f %f %u %u %u %u",
               &g_edMap.walls[i].v0,
               &g_edMap.walls[i].v1,
               &g_edMap.walls[i].neighbour,
               &g_edMap.walls[i].openBottom,
               &g_edMap.walls[i].openTop,
               &uc, &mc, &lc, &flags);
        g_edMap.walls[i].upperColor = (uint8_t)uc;
        g_edMap.walls[i].midColor   = (uint8_t)mc;
        g_edMap.walls[i].lowerColor = (uint8_t)lc;
        g_edMap.walls[i].flags      = (uint8_t)flags;
    }

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "SECTORS") != 0) {
        fclose(f);
        return 0;
    }
    if (count > ED_MAX_SECTORS) {
        fclose(f);
        return 0;
    }
    g_edMap.sectorCount = count;
    for (int i = 0; i < count; i++) {
        unsigned fc, cc;
        fscanf(f, "%d %d %d %f %f %u %u",
               &g_edMap.sectors[i].wallStart,
               &g_edMap.sectors[i].wallCount,
               &g_edMap.sectors[i].boundaryCount,
               &g_edMap.sectors[i].floorHeight,
               &g_edMap.sectors[i].ceilHeight,
               &fc, &cc);
        g_edMap.sectors[i].floorColor = (uint8_t)fc;
        g_edMap.sectors[i].ceilColor  = (uint8_t)cc;
    }

    fclose(f);
    clearDraft();
    return 1;
}

static int exportCStringMap(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "#include \"rc3d_map.h\"\n\n");

    fprintf(f, "static const RC3D_Vec2 g_verts[] = {\n");
    for (int i = 0; i < g_edMap.vertCount; i++) {
        fprintf(f, "    { %.6ff, %.6ff },\n", g_edMap.verts[i].x, g_edMap.verts[i].y);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const RC3D_Wall g_walls[] = {\n");
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        fprintf(f,
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u, %u, %u },\n",
            w->v0, w->v1, w->neighbour, w->openBottom, w->openTop,
            (unsigned)w->upperColor,
            (unsigned)w->midColor,
            (unsigned)w->lowerColor,
            (unsigned)w->flags);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const RC3D_Sector g_sectors[] = {\n");
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f,
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u },\n",
            s->wallStart, s->wallCount, s->boundaryCount,
            s->floorHeight, s->ceilHeight,
            (unsigned)s->floorColor, (unsigned)s->ceilColor);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "const RC3D_Map g_rc3dDemoMap = {\n");
    fprintf(f, "    g_verts,\n");
    fprintf(f, "    (int)(sizeof(g_verts) / sizeof(g_verts[0])),\n\n");
    fprintf(f, "    g_walls,\n");
    fprintf(f, "    (int)(sizeof(g_walls) / sizeof(g_walls[0])),\n\n");
    fprintf(f, "    g_sectors,\n");
    fprintf(f, "    (int)(sizeof(g_sectors) / sizeof(g_sectors[0])),\n\n");
    fprintf(f, "    %d,\n", g_edMap.startSector);
    fprintf(f, "    %.6ff,\n", g_edMap.startX);
    fprintf(f, "    %.6ff,\n", g_edMap.startY);
    fprintf(f, "    %.6ff\n", g_edMap.startAngle);
    fprintf(f, "};\n");

    fclose(f);
    return 1;
}

static void addDraftPoint(float wx, float wy)
{
    if (g_ed.draftCount >= ED_MAX_DRAFT_POINTS) return;

    wx = snapf(wx);
    wy = snapf(wy);

    const int v = findOrAddVertex(wx, wy);
    if (v < 0) return;

    if (g_ed.draftCount > 0 && g_ed.draftVertIndices[g_ed.draftCount - 1] == v) {
        return;
    }

    g_ed.draftVertIndices[g_ed.draftCount++] = v;
}

static void finalizeDraftSector(void)
{
    if (g_ed.draftCount < 3) return;
    if (g_edMap.sectorCount >= ED_MAX_SECTORS) return;
    if ((g_edMap.wallCount + g_ed.draftCount) > ED_MAX_WALLS) return;

    const int sectorIndex = g_edMap.sectorCount;
    const int wallStart = g_edMap.wallCount;

    for (int i = 0; i < g_ed.draftCount; i++) {
        const int v0 = g_ed.draftVertIndices[i];
        const int v1 = g_ed.draftVertIndices[(i + 1) % g_ed.draftCount];
        EdWall *w = &g_edMap.walls[g_edMap.wallCount++];

        w->v0 = v0;
        w->v1 = v1;
        w->neighbour = -1;
        w->openBottom = 0.0f;
        w->openTop = 0.0f;
        w->upperColor = 0;
        w->midColor = 10 + (sectorIndex % 22);
        w->lowerColor = 0;
        w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    }

    g_edMap.sectors[g_edMap.sectorCount].wallStart = wallStart;
    g_edMap.sectors[g_edMap.sectorCount].wallCount = g_ed.draftCount;
    g_edMap.sectors[g_edMap.sectorCount].boundaryCount = g_ed.draftCount;
    g_edMap.sectors[g_edMap.sectorCount].floorHeight = g_ed.sectorFloor;
    g_edMap.sectors[g_edMap.sectorCount].ceilHeight = g_ed.sectorCeil;
    g_edMap.sectors[g_edMap.sectorCount].floorColor = g_ed.sectorFloorColor;
    g_edMap.sectors[g_edMap.sectorCount].ceilColor = g_ed.sectorCeilColor;
    g_edMap.sectorCount++;

    syncAllPortals();
    clearDraft();
}


static float draftSignedArea(void)
{
    float area = 0.0f;

    if (g_ed.draftCount < 3) {
        return 0.0f;
    }

    for (int i = 0; i < g_ed.draftCount; i++) {
        const int i0 = g_ed.draftVertIndices[i];
        const int i1 = g_ed.draftVertIndices[(i + 1) % g_ed.draftCount];

        const EdVec2 *a = &g_edMap.verts[i0];
        const EdVec2 *b = &g_edMap.verts[i1];

        area += (a->x * b->y) - (b->x * a->y);
    }

    return area * 0.5f;
}

static void finalizeDraftInnerSolid(void)
{
    if (g_ed.draftCount < 3) return;
    if (g_ed.selectedSector < 0 || g_ed.selectedSector >= g_edMap.sectorCount) return;
    if ((g_edMap.wallCount + g_ed.draftCount) > ED_MAX_WALLS) return;

    EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];

    const int insertPos = sec->wallStart + sec->wallCount;

    /* make room if this sector is not the last wall block */
    if (insertPos < g_edMap.wallCount) {
        const int moveCount = g_edMap.wallCount - insertPos;
        memmove(&g_edMap.walls[insertPos + g_ed.draftCount],
                &g_edMap.walls[insertPos],
                sizeof(EdWall) * moveCount);

        for (int s = 0; s < g_edMap.sectorCount; s++) {
            if (s == g_ed.selectedSector) continue;
            if (g_edMap.sectors[s].wallStart >= insertPos) {
                g_edMap.sectors[s].wallStart += g_ed.draftCount;
            }
        }
    }

    for (int i = 0; i < g_ed.draftCount; i++) {
        const int v0 = g_ed.draftVertIndices[i];
        const int v1 = g_ed.draftVertIndices[(i + 1) % g_ed.draftCount];
        EdWall *w = &g_edMap.walls[insertPos + i];

        w->v0 = v0;
        w->v1 = v1;
        w->neighbour = -1;
        w->openBottom = 0.0f;
        w->openTop = 0.0f;
        w->upperColor = 0;
        w->midColor = 31;
        w->lowerColor = 0;
        w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    }

    g_edMap.wallCount += g_ed.draftCount;
    sec->wallCount += g_ed.draftCount;

    /* DO NOT increase boundaryCount */
    syncAllPortals();
    clearDraft();
}




static void updateHover(float worldX, float worldY, int mouseX, int mouseY)
{
    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = findSectorForPoint(worldX, worldY);

    float bestVertDistSq = 99999999.0f;
    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;
        worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);

        const int dx = sx - mouseX;
        const int dy = sy - mouseY;
        const float d2 = (float)(dx * dx + dy * dy);

        if (d2 < (float)(ED_PICK_DIST_PX * ED_PICK_DIST_PX) && d2 < bestVertDistSq) {
            bestVertDistSq = d2;
            g_ed.hoverVert = i;
        }
    }

    float bestWallDistSq = 99999999.0f;
    const float worldPick = (float)ED_PICK_DIST_PX / g_ed.zoom;
    const float worldPickSq = worldPick * worldPick;

    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];
        const float d2 = distPointSegSq(worldX, worldY, a->x, a->y, b->x, b->y);

        if (d2 < worldPickSq && d2 < bestWallDistSq) {
            bestWallDistSq = d2;
            g_ed.hoverWall = i;
        }
    }
}









static void drawGrid(void)
{
    const float leftW   = g_ed.camX - (SCREEN_W * 0.5f) / g_ed.zoom;
    const float rightW  = g_ed.camX + (SCREEN_W * 0.5f) / g_ed.zoom;
    const float topW    = g_ed.camY - (SCREEN_H * 0.5f) / g_ed.zoom;
    const float bottomW = g_ed.camY + (SCREEN_H * 0.5f) / g_ed.zoom;

    const int x0 = (int)floorf(leftW);
    const int x1 = (int)ceilf(rightW);
    const int y0 = (int)floorf(topW);
    const int y1 = (int)ceilf(bottomW);

    clearScreen(16);

    for (int gx = x0; gx <= x1; gx++) {
        int sx0, sy0, sx1, sy1;
        worldToScreen((float)gx, topW, &sx0, &sy0);
        worldToScreen((float)gx, bottomW, &sx1, &sy1);
        drawLine(sx0, sy0, sx1, sy1,
                 (gx == 0) ? ED_HOME_GRID_COL : ((gx % 4 == 0) ? ED_GRID_MAJOR_COL : ED_GRID_MINOR_COL));
    }

    for (int gy = y0; gy <= y1; gy++) {
        int sx0, sy0, sx1, sy1;
        worldToScreen(leftW, (float)gy, &sx0, &sy0);
        worldToScreen(rightW, (float)gy, &sx1, &sy1);
        drawLine(sx0, sy0, sx1, sy1,
                 (gy == 0) ? ED_HOME_GRID_COL : ((gy % 4 == 0) ? ED_GRID_MAJOR_COL : ED_GRID_MINOR_COL));
    }
}


static void drawInspectorPanel(void)
{
    char buf[256];
    const int px = SCREEN_W - 460;
    const int py = 8;
    const int pw = 450;
    const int ph = 210;

    drawRect(px, py, pw, ph, ED_UI_BG);
    drawLine(px, py, px + pw - 1, py, ED_UI_BORDER);
    drawLine(px, py + ph - 1, px + pw - 1, py + ph - 1, ED_UI_BORDER);
    drawLine(px, py, px, py + ph - 1, ED_UI_BORDER);
    drawLine(px + pw - 1, py, px + pw - 1, py + ph - 1, ED_UI_BORDER);

    drawText(px + 8, py + 8, "INSPECTOR", ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Selection mode: %s",
             (g_ed.selectionType == ED_SEL_VERTEX) ? "VERTEX" :
             (g_ed.selectionType == ED_SEL_WALL)   ? "WALL" :
             (g_ed.selectionType == ED_SEL_SECTOR) ? "SECTOR" : "NONE");
    drawText(px + 8, py + 22, buf, ED_TEXT_COL);

    if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert >= 0) {
        const EdVec2 *v = &g_edMap.verts[g_ed.selectedVert];
        snprintf(buf, sizeof(buf), "Vertex %d", g_ed.selectedVert);
        drawText(px + 8, py + 42, buf, 31);

        snprintf(buf, sizeof(buf), "x: %.2f", v->x);
        drawText(px + 8, py + 58, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "y: %.2f", v->y);
        drawText(px + 8, py + 72, buf, ED_TEXT_COL);

        drawText(px + 8, py + 92, "LMB drag vertex", ED_TEXT_COL);
        drawText(px + 8, py + 106, "Drop onto another vertex to merge", ED_TEXT_COL);
    }
    else if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
        const EdWall *w = &g_edMap.walls[g_ed.selectedWall];
        snprintf(buf, sizeof(buf), "Wall %d", g_ed.selectedWall);
        drawText(px + 8, py + 42, buf, 31);

        snprintf(buf, sizeof(buf), "v0: %d   v1: %d   neigh: %d", w->v0, w->v1, w->neighbour);
        drawText(px + 8, py + 58, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "openBottom: %.2f", w->openBottom);
        drawText(px + 8, py + 72, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "openTop: %.2f", w->openTop);
        drawText(px + 8, py + 86, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "upper: %u   mid: %u   lower: %u",
                 (unsigned)w->upperColor, (unsigned)w->midColor, (unsigned)w->lowerColor);
        drawText(px + 8, py + 100, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "flags: %u", (unsigned)w->flags);
        drawText(px + 8, py + 114, buf, ED_TEXT_COL);

        drawText(px + 8, py + 134, "1 solid   2 portal   3 window   4 door", ED_TEXT_COL);
        drawText(px + 8, py + 148, "R/T bot  Y/U top  A/S upper  D/F mid  H/J lower", ED_TEXT_COL);
        drawText(px + 8, py + 162, "I split wall", ED_TEXT_COL);
    }
    else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        snprintf(buf, sizeof(buf), "Sector %d", g_ed.selectedSector);
        drawText(px + 8, py + 42, buf, 31);

        snprintf(buf, sizeof(buf), "wallStart: %d", sec->wallStart);
        drawText(px + 8, py + 58, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "wallCount: %d   boundaryCount: %d",
                 sec->wallCount, sec->boundaryCount);
        drawText(px + 8, py + 72, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "floor: %.2f   ceil: %.2f",
                 sec->floorHeight, sec->ceilHeight);
        drawText(px + 8, py + 86, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "floorCol: %u   ceilCol: %u",
                 (unsigned)sec->floorColor, (unsigned)sec->ceilColor);
        drawText(px + 8, py + 100, buf, ED_TEXT_COL);

        drawText(px + 8, py + 120, "SHIFT+F/G floor", ED_TEXT_COL);
        drawText(px + 8, py + 134, "SHIFT+C/V ceil", ED_TEXT_COL);
        drawText(px + 8, py + 148, "SHIFT+J/K floor color", ED_TEXT_COL);
        drawText(px + 8, py + 162, "SHIFT+N/M ceil color", ED_TEXT_COL);


    }
    else {
        drawText(px + 8, py + 42, "Nothing selected", ED_TEXT_COL);
    }
}

static void drawEditorUI(void)
{
    char buf[256];

    drawRect(6, 6, 1260, 118, ED_UI_BG);
    drawLine(6, 6, 1265, 6, ED_UI_BORDER);
    drawLine(6, 123, 1265, 123, ED_UI_BORDER);
    drawLine(6, 6, 6, 123, ED_UI_BORDER);
    drawLine(1265, 6, 1265, 123, ED_UI_BORDER);

    snprintf(buf, sizeof(buf),
             "RMB add point  ENTER finish sector  LMB select/drag vertex  DEL delete hovered  ESC cancel draft");



    drawText(12, 10, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "MMB pan  wheel zoom  X set start  Q/E start angle  I split wall  drag vertex onto another = merge");
    drawText(12, 22, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "NEW sector defaults: floor %.2f  ceil %.2f  floorCol %u  ceilCol %u",
             g_ed.sectorFloor, g_ed.sectorCeil,
             (unsigned)g_ed.sectorFloorColor,
             (unsigned)g_ed.sectorCeilColor);
    drawText(12, 34, buf, ED_TEXT_COL);

    if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        snprintf(buf, sizeof(buf),
                 "SELECTED sector %d: floor %.2f  ceil %.2f  floorCol %u  ceilCol %u   (SHIFT+F/G, SHIFT+C/V, SHIFT+J/K, SHIFT+N/M)",
                 g_ed.selectedSector,
                 sec->floorHeight, sec->ceilHeight,
                 (unsigned)sec->floorColor,
                 (unsigned)sec->ceilColor);
    } else {
        snprintf(buf, sizeof(buf),
                 "SELECTED sector: none");
    }
    drawText(12, 46, buf, ED_TEXT_COL);

    if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
        const EdWall *w = &g_edMap.walls[g_ed.selectedWall];
        snprintf(buf, sizeof(buf),
                 "SELECTED wall %d: flags %u  neigh %d  openBot %.2f  openTop %.2f  upper %u  mid %u  lower %u",
                 g_ed.selectedWall,
                 (unsigned)w->flags,
                 w->neighbour,
                 w->openBottom, w->openTop,
                 (unsigned)w->upperColor,
                 (unsigned)w->midColor,
                 (unsigned)w->lowerColor);
    } else {
        snprintf(buf, sizeof(buf),
                 "SELECTED wall: none");
    }
    drawText(12, 58, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Wall edit: 1 solid  2 portal  3 window  4 door  R/T bot -/+  Y/U top -/+  A/S upper  D/F mid  H/J lower");
    drawText(12, 70, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "hoverV %d hoverW %d hoverS %d   selV %d selW %d selS %d   verts %d walls %d sectors %d",
             g_ed.hoverVert, g_ed.hoverWall, g_ed.hoverSector,
             g_ed.selectedVert, g_ed.selectedWall, g_ed.selectedSector,
             g_edMap.vertCount, g_edMap.wallCount, g_edMap.sectorCount);
    drawText(12, 82, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "F2 save txt  F3 load txt  F5 export C");
    drawText(12, 94, buf, ED_TEXT_COL);


    snprintf(buf, sizeof(buf),
         "ENTER finish draft  CCW=new sector  CW=inner solid in selected sector");
    //drawText(12, 106, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
         "draftCount %d  signedArea %.2f : ENTER finish draft  CCW=new sector  CW=inner solid in selected sector",
         g_ed.draftCount,
         draftSignedArea());
    drawText(12, 106, buf, ED_TEXT_COL);
}

static void drawMapGeometry(void)
{
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];
        int x0, y0, x1, y1;

        worldToScreen(a->x, a->y, &x0, &y0);
        worldToScreen(b->x, b->y, &x1, &y1);

        uint8_t c = ED_WALL_COL;

        if (w->flags & RC3D_WALL_PORTAL) c = ED_PORTAL_COL;
        if (w->flags & RC3D_WALL_MIDDLE) c = 14;
        if ((w->flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) &&
            !(w->flags & RC3D_WALL_MIDDLE) &&
            !(w->flags & RC3D_WALL_PORTAL)) {
            c = 30;
        }

        //if (i == g_ed.hoverWall) c = ED_COLOUR_HOVER_WALL;
        //if (i == g_ed.selectedWall) c = ED_COLOUR_SELECTED_WALL;
        if (i == g_ed.hoverWall) c = ED_COLOUR_HOVER_WALL;
        if (g_ed.selectionType == ED_SEL_WALL && i == g_ed.selectedWall) c = ED_COLOUR_SELECTED_WALL;

        drawLine(x0, y0, x1, y1, c);
    }

    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;
        uint8_t c = ED_VERT_COL;

        if (i == g_ed.hoverVert) c = 15;
        if (g_ed.selectionType == ED_SEL_VERTEX && i == g_ed.selectedVert) c = 27;

        worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);
        drawRect(sx - 2, sy - 2, 5, 5, c);
    }

    for (int i = 0; i < g_ed.draftCount; i++) {
        const EdVec2 *a = &g_edMap.verts[g_ed.draftVertIndices[i]];
        int sx, sy;
        worldToScreen(a->x, a->y, &sx, &sy);
        drawRect(sx - 2, sy - 2, 5, 5, ED_DRAFT_COL);

        if (i > 0) {
            const EdVec2 *b = &g_edMap.verts[g_ed.draftVertIndices[i - 1]];
            int x0, y0, x1, y1;
            worldToScreen(b->x, b->y, &x0, &y0);
            worldToScreen(a->x, a->y, &x1, &y1);
            drawLine(x0, y0, x1, y1, ED_DRAFT_COL);
        }
    }

        if (g_ed.selectionType == ED_SEL_SECTOR &&
        g_ed.selectedSector >= 0 &&
        g_ed.selectedSector < g_edMap.sectorCount) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        for (int i = 0; i < sec->boundaryCount; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            const EdVec2 *a = &g_edMap.verts[w->v0];
            const EdVec2 *b = &g_edMap.verts[w->v1];
            int x0, y0, x1, y1;
            worldToScreen(a->x, a->y, &x0, &y0);
            worldToScreen(b->x, b->y, &x1, &y1);

            drawLine(x0, y0, x1, y1, ED_COLOUR_SELECTED_SECTOR);
        }
    }
}

static void drawStartMarker(void)
{
    int sx, sy, fx, fy;
    worldToScreen(g_edMap.startX, g_edMap.startY, &sx, &sy);
    fx = sx + (int)(cosf(g_edMap.startAngle) * 16.0f);
    fy = sy + (int)(sinf(g_edMap.startAngle) * 16.0f);
    drawRect(sx - 2, sy - 2, 5, 5, ED_START_COL);
    drawLine(sx, sy, fx, fy, ED_START_COL);
}

void rc3dEditInit(void)
{
    beginNewMap();
}


void rc3dEditUpdate(float dt,
                    const uint8_t *keys,
                    int mouseX,
                    int mouseY,
                    uint32_t mouseButtons,
                    int mouseWheelY)
{
    (void)dt;

    const int prevMouseX = g_ed.lastMouseX;
    const int prevMouseY = g_ed.lastMouseY;

    const int leftDown   = (mouseButtons & SDL_BUTTON_LMASK) != 0;
    const int rightDown  = (mouseButtons & SDL_BUTTON_RMASK) != 0;
    const int middleDown = (mouseButtons & SDL_BUTTON_MMASK) != 0;

    float worldX, worldY;
    screenToWorld(mouseX, mouseY, &worldX, &worldY);

    if (mouseWheelY != 0) {
        const float beforeX = worldX;
        const float beforeY = worldY;

        g_ed.zoom *= (mouseWheelY > 0) ? 1.15f : (1.0f / 1.15f);
        g_ed.zoom = clampf_local(g_ed.zoom, 4.0f, 128.0f);

        {
            float afterX, afterY;
            screenToWorld(mouseX, mouseY, &afterX, &afterY);
            g_ed.camX += beforeX - afterX;
            g_ed.camY += beforeY - afterY;
        }
    }

    if (middleDown && !g_ed.prevMiddleDown) {
        g_ed.draggingPan = 1;
    }
    if (!middleDown) {
        g_ed.draggingPan = 0;
    }

    if (g_ed.draggingPan) {
        const int dx = mouseX - prevMouseX;
        const int dy = mouseY - prevMouseY;
        g_ed.camX -= (float)dx / g_ed.zoom;
        g_ed.camY -= (float)dy / g_ed.zoom;
    }

    screenToWorld(mouseX, mouseY, &worldX, &worldY);
    updateHover(worldX, worldY, mouseX, mouseY);
    /* right click = add draft point */
    if (rightDown && !g_ed.prevRightDown) {
        addDraftPoint(worldX, worldY);
    }

    if (leftDown && !g_ed.prevLeftDown) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;

        selectBestHoverTarget(keys, worldX, worldY);

        if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert >= 0) {
            g_ed.draggingVertex = 1;

            g_ed.dragStartWorldX = worldX;
            g_ed.dragStartWorldY = worldY;
            g_ed.dragVertexStartX = g_edMap.verts[g_ed.selectedVert].x;
            g_ed.dragVertexStartY = g_edMap.verts[g_ed.selectedVert].y;
        }
        else if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
            EdWall *w = &g_edMap.walls[g_ed.selectedWall];

            g_ed.draggingWall = 1;

            g_ed.dragWallStartWorldX = worldX;
            g_ed.dragWallStartWorldY = worldY;

            g_ed.dragWallV0StartX = g_edMap.verts[w->v0].x;
            g_ed.dragWallV0StartY = g_edMap.verts[w->v0].y;
            g_ed.dragWallV1StartX = g_edMap.verts[w->v1].x;
            g_ed.dragWallV1StartY = g_edMap.verts[w->v1].y;
        }
        else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
            beginSectorDrag(g_ed.selectedSector, worldX, worldY);
        }
    }

    /* drag selected vertex */
    if (leftDown && g_ed.draggingVertex && g_ed.selectedVert >= 0) {
        const float dx = snapf(worldX - g_ed.dragStartWorldX);
        const float dy = snapf(worldY - g_ed.dragStartWorldY);

        g_edMap.verts[g_ed.selectedVert].x = snapf(g_ed.dragVertexStartX + dx);
        g_edMap.verts[g_ed.selectedVert].y = snapf(g_ed.dragVertexStartY + dy);

        syncAllPortals();
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    /* drag selected wall */
    if (leftDown && g_ed.draggingWall && g_ed.selectedWall >= 0) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        const float dx = snapf(worldX - g_ed.dragWallStartWorldX);
        const float dy = snapf(worldY - g_ed.dragWallStartWorldY);

        g_edMap.verts[w->v0].x = snapf(g_ed.dragWallV0StartX + dx);
        g_edMap.verts[w->v0].y = snapf(g_ed.dragWallV0StartY + dy);

        if (w->v1 != w->v0) {
            g_edMap.verts[w->v1].x = snapf(g_ed.dragWallV1StartX + dx);
            g_edMap.verts[w->v1].y = snapf(g_ed.dragWallV1StartY + dy);
        }

        syncAllPortals();
        updateHover(worldX, worldY, mouseX, mouseY);
    }


    if (leftDown && g_ed.draggingSector && g_ed.selectedSector >= 0) {
        dragSelectedSectorTo(worldX, worldY);
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    /* release drag -> merge if dropped onto another vertex */
    if (!leftDown && g_ed.prevLeftDown && g_ed.draggingVertex && g_ed.selectedVert >= 0) {
        const int other = findVertexNearWorld(
            g_edMap.verts[g_ed.selectedVert].x,
            g_edMap.verts[g_ed.selectedVert].y,
            0.25f,
            g_ed.selectedVert
        );

        if (other >= 0) {
            mergeVertexInto(g_ed.selectedVert, other);
        }
    }

    if (!leftDown) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_DELETE)) {
        if (g_ed.hoverVert >= 0) {
            deleteVertexByIndex(g_ed.hoverVert);
        } else if (g_ed.hoverWall >= 0) {
            deleteWallByIndex(g_ed.hoverWall);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_BACKSPACE)) {
        if (g_ed.draftCount > 0) {
            g_ed.draftCount--;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_ESCAPE)) {
        clearDraft();
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_RETURN)) {
        const float area = draftSignedArea();

        if (g_ed.draftCount >= 3) {
            /* In this editor, Y grows downward, so winding is flipped */
            if (area < 0.0f) {
                finalizeDraftSector();
            } else if (area > 0.0f) {
                if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
                    finalizeDraftInnerSolid();
                } else {
                    finalizeDraftSector();
                }
            }
        }
    }
    /* sector defaults for new sectors */
    if (keyPressedOnce(keys, SDL_SCANCODE_F)) g_ed.sectorFloor -= 0.1f;
    if (keyPressedOnce(keys, SDL_SCANCODE_G)) g_ed.sectorFloor += 0.1f;
    if (keyPressedOnce(keys, SDL_SCANCODE_C)) g_ed.sectorCeil  -= 0.1f;
    if (keyPressedOnce(keys, SDL_SCANCODE_V)) g_ed.sectorCeil  += 0.1f;

    if (g_ed.sectorCeil < g_ed.sectorFloor + 0.1f) {
        g_ed.sectorCeil = g_ed.sectorFloor + 0.1f;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_J)) g_ed.sectorFloorColor--;
    if (keyPressedOnce(keys, SDL_SCANCODE_K)) g_ed.sectorFloorColor++;
    if (keyPressedOnce(keys, SDL_SCANCODE_N)) g_ed.sectorCeilColor--;
    if (keyPressedOnce(keys, SDL_SCANCODE_M)) g_ed.sectorCeilColor++;

    /* selected sector editing */
    if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
        EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];

        if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
            int changed = 0;

            if (keyPressedOnce(keys, SDL_SCANCODE_F)) { sec->floorHeight -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_G)) { sec->floorHeight += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_C)) { sec->ceilHeight  -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_V)) { sec->ceilHeight  += 0.1f; changed = 1; }

            if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                sec->ceilHeight = sec->floorHeight + 0.1f;
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_J)) { sec->floorColor--; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_K)) { sec->floorColor++; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_N)) { sec->ceilColor--;  changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_M)) { sec->ceilColor++;  changed = 1; }

            if (changed) {
                syncAllPortals();
            }
        }
    }

    /* selected wall editing */
    if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        if (keyPressedOnce(keys, SDL_SCANCODE_1)) {
            makeWallSolid(g_ed.selectedWall, (w->midColor == 0) ? 14 : w->midColor);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_2)) {
            tryMakeWallPortal(g_ed.selectedWall);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_3)) {
            float ob = w->openBottom;
            float ot = w->openTop;
            if (ot <= ob) {
                ob = 0.5f;
                ot = 1.4f;
            }
            makeWallWindow(g_ed.selectedWall, ob, ot,
                           (w->upperColor == 0) ? 14 : w->upperColor,
                           (w->lowerColor == 0) ? 14 : w->lowerColor);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_4)) {
            float ob = w->openBottom;
            float ot = w->openTop;
            if (ot <= ob) {
                ob = 0.0f;
                ot = 1.6f;
            }
            makeWallDoor(g_ed.selectedWall, ob, ot,
                         (w->midColor == 0) ? 14 : w->midColor);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_R)) w->openBottom -= 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_T)) w->openBottom += 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_Y)) w->openTop    -= 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_U)) w->openTop    += 0.1f;

        if (w->openTop < w->openBottom) {
            const float tmp = w->openTop;
            w->openTop = w->openBottom;
            w->openBottom = tmp;
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_A)) w->upperColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_S)) w->upperColor++;
        if (keyPressedOnce(keys, SDL_SCANCODE_D)) w->midColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_F)) w->midColor++;
        if (keyPressedOnce(keys, SDL_SCANCODE_H)) w->lowerColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_J)) w->lowerColor++;

        if (keyPressedOnce(keys, SDL_SCANCODE_I)) {
            splitWallAtSelected(g_ed.selectedWall, worldX, worldY);
        }

        if (w->flags & RC3D_WALL_PORTAL) {
            refreshSelectedPortalFromReverse(g_ed.selectedWall);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_X)) {
        const int s = findSectorForPoint(worldX, worldY);
        if (s >= 0) {
            g_edMap.startX = snapf(worldX);
            g_edMap.startY = snapf(worldY);
            g_edMap.startSector = s;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_Q)) {
        g_edMap.startAngle -= 0.1f;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_E)) {
        g_edMap.startAngle += 0.1f;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F2)) {
        saveTextMap("rc3d_map.txt");
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_F3)) {
        loadTextMap("rc3d_map.txt");
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_F5)) {
        exportCStringMap("rc3d_map_export.c");
    }

    memcpy(g_ed.prevKeys, keys, SDL_NUM_SCANCODES);
    g_ed.prevLeftDown = leftDown;
    g_ed.prevRightDown = rightDown;
    g_ed.prevMiddleDown = middleDown;

    g_ed.lastMouseX = mouseX;
    g_ed.lastMouseY = mouseY;
}

void rc3dEditRender(void)
{
    drawGrid();
    drawMapGeometry();
    drawStartMarker();
    drawEditorUI();
    drawInspectorPanel();
}







