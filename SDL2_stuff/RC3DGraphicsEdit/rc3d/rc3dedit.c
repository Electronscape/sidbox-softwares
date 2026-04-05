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

// tab toggles hi-res to low res
#define ED_GRID_STEP_TINY       0.1f
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







// GUI parts
#define ED_UI_TEXT_INFOTYPE         29
#define ED_UI_BTN_BG                13
#define ED_UI_BTN_BORDER            27
#define ED_UI_BTN_TEXT              2
#define ED_UI_BTN_HOVER             12

#define ED_UI_BTN_ACTIVE            19  // background colour
#define ED_UI_BTN_DISABLED          6   // background
#define ED_UI_BTN_TEXT_DISABLED     5
#define ED_UI_BTN_BORDER_DISABLED   16

#define ED_UI_ID_NONE           0

#define ED_BTN_W                96
#define ED_BTN_H                16
#define ED_BTN_GAP              4


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

    int splitPreviewValid;
    float splitPreviewX;
    float splitPreviewY;

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

    float currentGridStep;
    int tinyGridEnabled;

    int prevLeftDown;
    int prevRightDown;
    int prevMiddleDown;
    uint8_t prevKeys[SDL_NUM_SCANCODES];

    // GUI parts
    int uiMouseCaptured;
    int uiHotId;
    int uiActiveId;
} EditorState;



static EditorMap g_edMap;
static EditorState g_ed;



#define ED_HISTORY_MAX 100

typedef struct {
    EditorMap map;

    float camX;
    float camY;
    float zoom;

    int draftVertIndices[ED_MAX_DRAFT_POINTS];
    int draftCount;

    float sectorFloor;
    float sectorCeil;
    uint8_t sectorFloorColor;
    uint8_t sectorCeilColor;

    int selectedVert;
    int selectedWall;
    int selectedSector;
    EdSelectionType selectionType;

    float currentGridStep;
    int tinyGridEnabled;
} EditorSnapshot;

static EditorSnapshot g_undoStack[ED_HISTORY_MAX];
static int g_undoCount = 0;

static EditorSnapshot g_redoStack[ED_HISTORY_MAX];
static int g_redoCount = 0;


static void rebuildSectorWallLayout(void);
static void syncAllPortals(void);
static int findSectorForPoint(float x, float y);
static float draftSignedArea(void);
static void finalizeDraftSector(void);
static void finalizeDraftInnerSolid(void);
static int finalizeDraftSectorAttached(void);
static int getWallSplitPreviewPos(int wallIndex, float wx, float wy, float *outX, float *outY);

static void compactOrphanVertices(void);
static void cleanMapCompact(void);

/////// GUI PARTS
static int pointInRect(int px, int py, int x, int y, int w, int h);
static void drawUIButton(int x, int y, int w, int h, const char *text, int hot, int active, int disabled);
static int uiButtonLogic(int id, int x, int y, int w, int h, int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased, int disabled);
static int uiArrowPairFloat(int *idBase, int x, int y, const char *label, float *value, float step, float minVal, float maxVal,
                            int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased);
static void handleEditorUI(int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased, float worldX, float worldY);



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
    float step = g_ed.currentGridStep;

    if (step < ED_GRID_STEP_TINY) {
        step = ED_GRID_STEP_TINY;
    }

    return roundf(v / step) * step;
}


static void captureSnapshot(EditorSnapshot *s)
{
    s->map = g_edMap;

    s->camX = g_ed.camX;
    s->camY = g_ed.camY;
    s->zoom = g_ed.zoom;

    memcpy(s->draftVertIndices, g_ed.draftVertIndices, sizeof(g_ed.draftVertIndices));
    s->draftCount = g_ed.draftCount;

    s->sectorFloor = g_ed.sectorFloor;
    s->sectorCeil = g_ed.sectorCeil;
    s->sectorFloorColor = g_ed.sectorFloorColor;
    s->sectorCeilColor = g_ed.sectorCeilColor;

    s->selectedVert = g_ed.selectedVert;
    s->selectedWall = g_ed.selectedWall;
    s->selectedSector = g_ed.selectedSector;
    s->selectionType = g_ed.selectionType;

    s->currentGridStep = g_ed.currentGridStep;
    s->tinyGridEnabled = g_ed.tinyGridEnabled;
}

static void restoreSnapshot(const EditorSnapshot *s)
{
    g_edMap = s->map;

    g_ed.camX = s->camX;
    g_ed.camY = s->camY;
    g_ed.zoom = s->zoom;

    memcpy(g_ed.draftVertIndices, s->draftVertIndices, sizeof(g_ed.draftVertIndices));
    g_ed.draftCount = s->draftCount;

    g_ed.sectorFloor = s->sectorFloor;
    g_ed.sectorCeil = s->sectorCeil;
    g_ed.sectorFloorColor = s->sectorFloorColor;
    g_ed.sectorCeilColor = s->sectorCeilColor;

    g_ed.selectedVert = s->selectedVert;
    g_ed.selectedWall = s->selectedWall;
    g_ed.selectedSector = s->selectedSector;
    g_ed.selectionType = s->selectionType;

    g_ed.currentGridStep = s->currentGridStep;
    g_ed.tinyGridEnabled = s->tinyGridEnabled;

    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = -1;

    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.draggingPan = 0;

    g_ed.splitPreviewValid = 0;
}

static int keyPressedOnce(const uint8_t *keys, SDL_Scancode sc)
{
    return keys[sc] && !g_ed.prevKeys[sc];
}

static void pushUndoState(void)
{
    if (g_undoCount == ED_HISTORY_MAX) {
        memmove(&g_undoStack[0], &g_undoStack[1], sizeof(EditorSnapshot) * (ED_HISTORY_MAX - 1));
        g_undoCount = ED_HISTORY_MAX - 1;
    }

    captureSnapshot(&g_undoStack[g_undoCount]);
    g_undoCount++;

    /* any new edit kills redo history */
    g_redoCount = 0;
}

static void performUndo(void)
{
    if (g_undoCount <= 0) return;

    if (g_redoCount == ED_HISTORY_MAX) {
        memmove(&g_redoStack[0], &g_redoStack[1], sizeof(EditorSnapshot) * (ED_HISTORY_MAX - 1));
        g_redoCount = ED_HISTORY_MAX - 1;
    }

    captureSnapshot(&g_redoStack[g_redoCount]);
    g_redoCount++;

    g_undoCount--;
    restoreSnapshot(&g_undoStack[g_undoCount]);
}

static void performRedo(void)
{
    if (g_redoCount <= 0) return;

    if (g_undoCount == ED_HISTORY_MAX) {
        memmove(&g_undoStack[0], &g_undoStack[1], sizeof(EditorSnapshot) * (ED_HISTORY_MAX - 1));
        g_undoCount = ED_HISTORY_MAX - 1;
    }

    captureSnapshot(&g_undoStack[g_undoCount]);
    g_undoCount++;

    g_redoCount--;
    restoreSnapshot(&g_redoStack[g_redoCount]);
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

static float cross2_local(float ax, float ay, float bx, float by)
{
    return (ax * by) - (ay * bx);
}

static int pointOnSegmentEps(float px, float py,
                             float ax, float ay,
                             float bx, float by,
                             float eps)
{
    const float abx = bx - ax;
    const float aby = by - ay;
    const float apx = px - ax;
    const float apy = py - ay;

    const float cross = cross2_local(abx, aby, apx, apy);
    if (absf_local(cross) > eps) return 0;

    const float dot = (apx * abx) + (apy * aby);
    if (dot < -eps) return 0;

    const float lenSq = (abx * abx) + (aby * aby);
    if (dot > lenSq + eps) return 0;

    return 1;
}

static int pointsSameEps(float ax, float ay, float bx, float by, float eps)
{
    return (absf_local(ax - bx) <= eps) && (absf_local(ay - by) <= eps);
}

static void insertWallsIntoSector(int sectorIndex, int insertLocalIndex, const EdWall *srcWalls, int insertCount)
{
    if (insertCount <= 0) return;
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;
    if ((g_edMap.wallCount + insertCount) > ED_MAX_WALLS) return;

    EdSector *sec = &g_edMap.sectors[sectorIndex];
    const int insertPos = sec->wallStart + insertLocalIndex;

    if (insertPos < g_edMap.wallCount) {
        memmove(&g_edMap.walls[insertPos + insertCount],
                &g_edMap.walls[insertPos],
                sizeof(EdWall) * (g_edMap.wallCount - insertPos));

        for (int s = 0; s < g_edMap.sectorCount; s++) {
            if (s == sectorIndex) continue;
            if (g_edMap.sectors[s].wallStart >= insertPos) {
                g_edMap.sectors[s].wallStart += insertCount;
            }
        }
    }

    for (int i = 0; i < insertCount; i++) {
        g_edMap.walls[insertPos + i] = srcWalls[i];
    }

    g_edMap.wallCount += insertCount;
    sec->wallCount += insertCount;
    if (insertLocalIndex < sec->boundaryCount) {
        sec->boundaryCount += insertCount;
    }
}

static void removeWallFromSector(int sectorIndex, int localWallIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    EdSector *sec = &g_edMap.sectors[sectorIndex];
    if (localWallIndex < 0 || localWallIndex >= sec->wallCount) return;

    const int wallIndex = sec->wallStart + localWallIndex;
    g_edMap.walls[wallIndex].v0 = -1;
    g_edMap.walls[wallIndex].v1 = -1;

    rebuildSectorWallLayout();
}

static int findDraftAttachWall(int *outSector,
                               int *outLocalWall,
                               int *outDraftEdge,
                               int *outOuterV0,
                               int *outOuterV1)
{
    const float eps = 0.01f;

    if (g_ed.draftCount < 3) return 0;

    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];

        for (int wi = 0; wi < sec->boundaryCount; wi++) {
            const EdWall *outer = &g_edMap.walls[sec->wallStart + wi];
            const EdVec2 *oa = &g_edMap.verts[outer->v0];
            const EdVec2 *ob = &g_edMap.verts[outer->v1];

            const float odx = ob->x - oa->x;
            const float ody = ob->y - oa->y;

            for (int di = 0; di < g_ed.draftCount; di++) {
                const int dv0 = g_ed.draftVertIndices[di];
                const int dv1 = g_ed.draftVertIndices[(di + 1) % g_ed.draftCount];

                const EdVec2 *da = &g_edMap.verts[dv0];
                const EdVec2 *db = &g_edMap.verts[dv1];

                const float ddx = db->x - da->x;
                const float ddy = db->y - da->y;

                /* parallel + collinear */
                if (absf_local(cross2_local(odx, ody, ddx, ddy)) > eps) continue;
                if (absf_local(cross2_local(odx, ody, da->x - oa->x, da->y - oa->y)) > eps) continue;

                /* draft edge endpoints must lie on outer wall segment */
                if (!pointOnSegmentEps(da->x, da->y, oa->x, oa->y, ob->x, ob->y, eps)) continue;
                if (!pointOnSegmentEps(db->x, db->y, oa->x, oa->y, ob->x, ob->y, eps)) continue;

                *outSector = s;
                *outLocalWall = wi;
                *outDraftEdge = di;
                *outOuterV0 = outer->v0;
                *outOuterV1 = outer->v1;
                return 1;
            }
        }
    }

    return 0;
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

    float sx, sy;
    if (!getWallSplitPreviewPos(wallIndex, wx, wy, &sx, &sy)) {
        return 0;
    }

    EdWall original = g_edMap.walls[wallIndex];

    int newVert = findVertexExact(sx, sy);
    if (newVert < 0) {
        newVert = addVertex(sx, sy);
        if (newVert < 0) return 0;
    }

    if (newVert == original.v0 || newVert == original.v1) return 0;

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

    /* auto-select new vertex and begin dragging immediately */
    g_ed.selectionType = ED_SEL_VERTEX;
    g_ed.selectedVert = newVert;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    g_ed.hoverVert = newVert;

    g_ed.draggingVertex = 1;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;

    g_ed.dragStartWorldX = wx;
    g_ed.dragStartWorldY = wy;
    g_ed.dragVertexStartX = g_edMap.verts[newVert].x;
    g_ed.dragVertexStartY = g_edMap.verts[newVert].y;

    return 1;
}







static int getWallSplitPreviewPos(int wallIndex, float wx, float wy, float *outX, float *outY)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return 0;

    const EdWall *w = &g_edMap.walls[wallIndex];
    const EdVec2 *a = &g_edMap.verts[w->v0];
    const EdVec2 *b = &g_edMap.verts[w->v1];

    const float abx = b->x - a->x;
    const float aby = b->y - a->y;
    const float lenSq = (abx * abx) + (aby * aby);

    if (lenSq <= ED_EPSILON) return 0;

    float t = (((wx - a->x) * abx) + ((wy - a->y) * aby)) / lenSq;
    t = clampf_local(t, 0.0f, 1.0f);

    float sx = a->x + (abx * t);
    float sy = a->y + (aby * t);

    sx = snapf(sx);
    sy = snapf(sy);

    if (pointsSameEps(sx, sy, a->x, a->y, 0.01f)) return 0;
    if (pointsSameEps(sx, sy, b->x, b->y, 0.01f)) return 0;

    *outX = sx;
    *outY = sy;
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

    g_ed.currentGridStep = ED_GRID_STEP;
    g_ed.tinyGridEnabled = 0;

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

    g_ed.splitPreviewValid = 0;
    g_ed.splitPreviewX = 0.0f;
    g_ed.splitPreviewY = 0.0f;

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

static int finalizeDraftSectorAttached(void)
{
    int outerSectorIndex, outerLocalWall, draftEdgeIndex;
    int outerV0, outerV1;

    if (g_ed.draftCount < 3) return 0;
    if (g_edMap.sectorCount >= ED_MAX_SECTORS) return 0;

    if (!findDraftAttachWall(&outerSectorIndex,
                             &outerLocalWall,
                             &draftEdgeIndex,
                             &outerV0,
                             &outerV1)) {
        return 0;
    }

    const EdVec2 *outerA = &g_edMap.verts[outerV0];
    const EdVec2 *outerB = &g_edMap.verts[outerV1];

    const int draftV0 = g_ed.draftVertIndices[draftEdgeIndex];
    const int draftV1 = g_ed.draftVertIndices[(draftEdgeIndex + 1) % g_ed.draftCount];

    const EdVec2 *draftA = &g_edMap.verts[draftV0];
    const EdVec2 *draftB = &g_edMap.verts[draftV1];

    const float odx = outerB->x - outerA->x;
    const float ody = outerB->y - outerA->y;

    const float lenSq = (odx * odx) + (ody * ody);
    if (lenSq <= ED_EPSILON) return 0;

    /* project both draft points onto the outer wall so we can order them */
    const float ta = (((draftA->x - outerA->x) * odx) + ((draftA->y - outerA->y) * ody)) / lenSq;
    const float tb = (((draftB->x - outerA->x) * odx) + ((draftB->y - outerA->y) * ody)) / lenSq;

    int splitV0, splitV1;

    if (ta <= tb) {
        splitV0 = findOrAddVertex(draftA->x, draftA->y);
        splitV1 = findOrAddVertex(draftB->x, draftB->y);
    } else {
        splitV0 = findOrAddVertex(draftB->x, draftB->y);
        splitV1 = findOrAddVertex(draftA->x, draftA->y);
    }

    if (splitV0 < 0 || splitV1 < 0) return 0;

    /* create the new sector walls first */
    const int newSectorIndex = g_edMap.sectorCount;
    const int newWallStart = g_edMap.wallCount;

    if ((g_edMap.wallCount + g_ed.draftCount) > ED_MAX_WALLS) return 0;

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
        w->midColor = 10 + (newSectorIndex % 22);
        w->lowerColor = 0;
        w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    }

    g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
    g_edMap.sectors[newSectorIndex].wallCount = g_ed.draftCount;
    g_edMap.sectors[newSectorIndex].boundaryCount = g_ed.draftCount;
    g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
    g_edMap.sectors[newSectorIndex].ceilHeight = g_ed.sectorCeil;
    g_edMap.sectors[newSectorIndex].floorColor = g_ed.sectorFloorColor;
    g_edMap.sectors[newSectorIndex].ceilColor = g_ed.sectorCeilColor;
    g_edMap.sectorCount++;

    /* make the matching draft edge a portal candidate */
    {
        EdWall *newShared = &g_edMap.walls[newWallStart + draftEdgeIndex];
        newShared->v0 = splitV0;
        newShared->v1 = splitV1;
    }

    /* split outer wall into up to 3 pieces */
    {
        EdWall outerPieces[3];
        int pieceCount = 0;
        EdWall templateWall = g_edMap.walls[g_edMap.sectors[outerSectorIndex].wallStart + outerLocalWall];

        if (!pointsSameEps(outerA->x, outerA->y, draftA->x, draftA->y, 0.01f)) {
            outerPieces[pieceCount] = templateWall;
            outerPieces[pieceCount].v0 = outerV0;
            outerPieces[pieceCount].v1 = splitV0;
            outerPieces[pieceCount].neighbour = -1;
            outerPieces[pieceCount].openBottom = 0.0f;
            outerPieces[pieceCount].openTop = 0.0f;
            outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            pieceCount++;
        }

        outerPieces[pieceCount] = templateWall;
        outerPieces[pieceCount].v0 = splitV0;
        outerPieces[pieceCount].v1 = splitV1;
        outerPieces[pieceCount].neighbour = -1;
        outerPieces[pieceCount].openBottom = 0.0f;
        outerPieces[pieceCount].openTop = 0.0f;
        outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        pieceCount++;

        if (!pointsSameEps(draftB->x, draftB->y, outerB->x, outerB->y, 0.01f)) {
            outerPieces[pieceCount] = templateWall;
            outerPieces[pieceCount].v0 = splitV1;
            outerPieces[pieceCount].v1 = outerV1;
            outerPieces[pieceCount].neighbour = -1;
            outerPieces[pieceCount].openBottom = 0.0f;
            outerPieces[pieceCount].openTop = 0.0f;
            outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            pieceCount++;
        }

        removeWallFromSector(outerSectorIndex, outerLocalWall);

        /* after rebuild, sector local index still points at insertion place */
        insertWallsIntoSector(outerSectorIndex, outerLocalWall, outerPieces, pieceCount);
    }

    syncAllPortals();
    clearDraft();
    return 1;
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

static void cleanMapCompact(void)
{
    int sectorRemap[ED_MAX_SECTORS];
    EdSector newSectors[ED_MAX_SECTORS];
    int newSectorCount = 0;

    for (int i = 0; i < ED_MAX_SECTORS; i++) {
        sectorRemap[i] = -1;
    }

    /* first rebuild walls cleanly so sector wall ranges are sane */
    rebuildSectorWallLayout();

    /* remove sectors that have no valid walls */
    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];

        if (sec->wallCount <= 0 || sec->boundaryCount <= 0) {
            continue;
        }

        if (sec->wallStart < 0 || sec->wallStart >= g_edMap.wallCount) {
            continue;
        }

        if ((sec->wallStart + sec->wallCount) > g_edMap.wallCount) {
            continue;
        }

        sectorRemap[s] = newSectorCount;
        newSectors[newSectorCount++] = *sec;
    }

    for (int i = 0; i < newSectorCount; i++) {
        g_edMap.sectors[i] = newSectors[i];
    }
    g_edMap.sectorCount = newSectorCount;

    /* fix wall neighbour sector indices */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];

        if (w->neighbour >= 0 && w->neighbour < ED_MAX_SECTORS) {
            w->neighbour = sectorRemap[w->neighbour];
        } else {
            w->neighbour = -1;
        }
    }

    /* fix start sector */
    if (g_edMap.startSector >= 0 && g_edMap.startSector < ED_MAX_SECTORS) {
        g_edMap.startSector = sectorRemap[g_edMap.startSector];
    } else {
        g_edMap.startSector = -1;
    }

    if (g_edMap.startSector < 0) {
        g_edMap.startSector = (g_edMap.sectorCount > 0) ? 0 : -1;
    }

    /* fix selected / hovered sectors */
    if (g_ed.selectedSector >= 0 && g_ed.selectedSector < ED_MAX_SECTORS) {
        g_ed.selectedSector = sectorRemap[g_ed.selectedSector];
    } else {
        g_ed.selectedSector = -1;
    }

    if (g_ed.hoverSector >= 0 && g_ed.hoverSector < ED_MAX_SECTORS) {
        g_ed.hoverSector = sectorRemap[g_ed.hoverSector];
    } else {
        g_ed.hoverSector = -1;
    }

    /* wall selection safety */
    if (g_ed.selectedWall >= g_edMap.wallCount) g_ed.selectedWall = -1;
    if (g_ed.hoverWall >= g_edMap.wallCount) g_ed.hoverWall = -1;

    /* vertex cleanup */
    compactOrphanVertices();

    /* if no sectors remain, make start sane */
    if (g_edMap.sectorCount <= 0) {
        g_edMap.startSector = -1;
    }

    /* selection mode safety */
    if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert < 0) {
        g_ed.selectionType = ED_SEL_NONE;
    }
    if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall < 0) {
        g_ed.selectionType = ED_SEL_NONE;
    }
    if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector < 0) {
        g_ed.selectionType = ED_SEL_NONE;
    }

    /* drag states off */
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.splitPreviewValid = 0;

    /* final portal rebuild */
    syncAllPortals();
}

static void compactOrphanVertices(void)
{
    int used[ED_MAX_VERTS];
    int remap[ED_MAX_VERTS];
    int newCount = 0;

    for (int i = 0; i < ED_MAX_VERTS; i++) {
        used[i] = 0;
        remap[i] = -1;
    }

    /* mark vertices referenced by walls */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];

        if (w->v0 >= 0 && w->v0 < g_edMap.vertCount) used[w->v0] = 1;
        if (w->v1 >= 0 && w->v1 < g_edMap.vertCount) used[w->v1] = 1;
    }

    /* build remap */
    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (used[i]) {
            remap[i] = newCount++;
        }
    }

    /* compact verts */
    if (newCount < g_edMap.vertCount) {
        EdVec2 newVerts[ED_MAX_VERTS];

        for (int i = 0; i < g_edMap.vertCount; i++) {
            if (remap[i] >= 0) {
                newVerts[remap[i]] = g_edMap.verts[i];
            }
        }

        for (int i = 0; i < newCount; i++) {
            g_edMap.verts[i] = newVerts[i];
        }

        g_edMap.vertCount = newCount;
    }

    /* remap wall vertex indices */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];

        if (w->v0 >= 0) w->v0 = remap[w->v0];
        if (w->v1 >= 0) w->v1 = remap[w->v1];
    }

    /* remap draft vertex indices, removing invalid ones */
    for (int i = 0; i < g_ed.draftCount; ) {
        const int oldIdx = g_ed.draftVertIndices[i];
        const int newIdx = (oldIdx >= 0 && oldIdx < ED_MAX_VERTS) ? remap[oldIdx] : -1;

        if (newIdx < 0) {
            for (int j = i; j < g_ed.draftCount - 1; j++) {
                g_ed.draftVertIndices[j] = g_ed.draftVertIndices[j + 1];
            }
            g_ed.draftCount--;
            continue;
        }

        g_ed.draftVertIndices[i] = newIdx;
        i++;
    }

    /* fix selected / hovered vertex */
    if (g_ed.selectedVert >= 0 && g_ed.selectedVert < ED_MAX_VERTS) {
        g_ed.selectedVert = remap[g_ed.selectedVert];
    } else {
        g_ed.selectedVert = -1;
    }

    if (g_ed.hoverVert >= 0 && g_ed.hoverVert < ED_MAX_VERTS) {
        g_ed.hoverVert = remap[g_ed.hoverVert];
    } else {
        g_ed.hoverVert = -1;
    }
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

    clearScreen(16);

    /* ------------------------------------------------------------ */
    /* Base grid: always draw the normal 1.0 grid                   */
    /* ------------------------------------------------------------ */

     /* ------------------------------------------------------------ */
    /* Tiny overlay: only when tiny mode is enabled                 */
    /* ------------------------------------------------------------ */
    if (g_ed.tinyGridEnabled) {
        const float step = ED_GRID_STEP_TINY;

        const int xCount0 = (int)floorf(leftW / step);
        const int xCount1 = (int)ceilf(rightW / step);
        const int yCount0 = (int)floorf(topW / step);
        const int yCount1 = (int)ceilf(bottomW / step);

        for (int gx = xCount0; gx <= xCount1; gx++) {
            const float x = (float)gx * step;

            /* skip lines that already belong to the 1.0 grid */
            const float baseT = x / ED_GRID_STEP;
            if (absf_local(baseT - roundf(baseT)) < 0.0001f) {
                continue;
            }

            int sx0, sy0, sx1, sy1;
            worldToScreen(x, topW, &sx0, &sy0);
            worldToScreen(x, bottomW, &sx1, &sy1);

            drawLine(sx0, sy0, sx1, sy1, 20);
        }

        for (int gy = yCount0; gy <= yCount1; gy++) {
            const float y = (float)gy * step;

            /* skip lines that already belong to the 1.0 grid */
            const float baseT = y / ED_GRID_STEP;
            if (absf_local(baseT - roundf(baseT)) < 0.0001f) {
                continue;
            }

            int sx0, sy0, sx1, sy1;
            worldToScreen(leftW, y, &sx0, &sy0);
            worldToScreen(rightW, y, &sx1, &sy1);

            drawLine(sx0, sy0, sx1, sy1, 20);
        }
    }

    {
        const float step = ED_GRID_STEP;
        const float majorStep = step * 4.0f;

        const int xCount0 = (int)floorf(leftW / step);
        const int xCount1 = (int)ceilf(rightW / step);
        const int yCount0 = (int)floorf(topW / step);
        const int yCount1 = (int)ceilf(bottomW / step);

        for (int gx = xCount0; gx <= xCount1; gx++) {
            const float x = (float)gx * step;
            int sx0, sy0, sx1, sy1;
            worldToScreen(x, topW, &sx0, &sy0);
            worldToScreen(x, bottomW, &sx1, &sy1);

            uint8_t col = ED_GRID_MINOR_COL;

            if (absf_local(x) < 0.0001f) {
                col = ED_HOME_GRID_COL;
            } else {
                const float majorT = x / majorStep;
                const float majorRounded = roundf(majorT);
                if (absf_local(majorT - majorRounded) < 0.0001f) {
                    col = ED_GRID_MAJOR_COL;
                }
            }

            drawLine(sx0, sy0, sx1, sy1, col);
        }

        for (int gy = yCount0; gy <= yCount1; gy++) {
            const float y = (float)gy * step;
            int sx0, sy0, sx1, sy1;
            worldToScreen(leftW, y, &sx0, &sy0);
            worldToScreen(rightW, y, &sx1, &sy1);

            uint8_t col = ED_GRID_MINOR_COL;

            if (absf_local(y) < 0.0001f) {
                col = ED_HOME_GRID_COL;
            } else {
                const float majorT = y / majorStep;
                const float majorRounded = roundf(majorT);
                if (absf_local(majorT - majorRounded) < 0.0001f) {
                    col = ED_GRID_MAJOR_COL;
                }
            }

            drawLine(sx0, sy0, sx1, sy1, col);
        }
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
        drawText(px + 8, py + 106, "Drop onto another vertex to merge (hold shift)", ED_TEXT_COL);
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
        drawText(px + 8, py + 162, "[space] split wall", ED_TEXT_COL);
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





//// GUI PARTS ////
static int pointInRect(int px, int py, int x, int y, int w, int h)
{
    return (px >= x && px < (x + w) && py >= y && py < (y + h));
}

static void drawUIButton(int x, int y, int w, int h, const char *text, int hot, int active, int disabled)
{
    uint8_t bg = ED_UI_BTN_BG;
    uint8_t border = ED_UI_BTN_BORDER;
    uint8_t textCol = ED_UI_BTN_TEXT;

    if (disabled) {
        bg = ED_UI_BTN_DISABLED;
        border = ED_UI_BTN_BORDER_DISABLED;
        textCol = ED_UI_BTN_TEXT_DISABLED;
    } else if (active) {
        bg = ED_UI_BTN_ACTIVE;
    } else if (hot) {
        bg = ED_UI_BTN_HOVER;
    }

    drawRect(x, y, w, h, bg);
    drawLine(x, y, x + w - 1, y, border);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, border);
    drawLine(x, y, x, y + h - 1, border);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, border);

    drawText(x + 4, y + 4, text, textCol);
}

static int uiButtonLogic(int id, int x, int y, int w, int h,
                         int mouseX, int mouseY,
                         int leftDown, int leftPressed, int leftReleased,
                         int disabled)
{
    const int hot = !disabled && pointInRect(mouseX, mouseY, x, y, w, h);

    if (hot) {
        g_ed.uiMouseCaptured = 1;
        g_ed.uiHotId = id;
    }

    if (disabled) {
        if (g_ed.uiActiveId == id && !leftDown) {
            g_ed.uiActiveId = ED_UI_ID_NONE;
        }
        drawUIButton(x, y, w, h, "", hot, 0, 1);
        return 0;
    }

    if (hot && leftPressed) {
        g_ed.uiActiveId = id;
    }

    if (leftReleased) {
        if (g_ed.uiActiveId == id) {
            g_ed.uiActiveId = ED_UI_ID_NONE;
            if (hot) {
                return 1;
            }
        }
    }

    return 0;
}

static int uiArrowPairFloat(int *idBase,
                            int x, int y,
                            const char *label,
                            float *value,
                            float step,
                            float minVal,
                            float maxVal,
                            int mouseX, int mouseY,
                            int leftDown, int leftPressed, int leftReleased)
{
    char buf[64];
    int changed = 0;

    drawText(x, y + 4, label, ED_TEXT_COL);

    {
        const int minusId = (*idBase)++;
        const int plusId  = (*idBase)++;

        const int bx = x + 84;
        const int by = y;

        drawUIButton(bx, by, 16, 16, "-", g_ed.uiHotId == minusId, g_ed.uiActiveId == minusId, 0);
        drawUIButton(bx + 20, by, 16, 16, "+", g_ed.uiHotId == plusId, g_ed.uiActiveId == plusId, 0);

        snprintf(buf, sizeof(buf), "%.2f", *value);
        drawText(bx + 44, by + 4, buf, ED_TEXT_COL);

        if (uiButtonLogic(minusId, bx, by, 16, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            *value -= step;
            if (*value < minVal) *value = minVal;
            changed = 1;
        }

        if (uiButtonLogic(plusId, bx + 20, by, 16, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            *value += step;
            if (*value > maxVal) *value = maxVal;
            changed = 1;
        }
    }

    return changed;
}




static void drawEditorUI(void)
{
    char buf[256];

    drawRect(6, 6, 760, 238, ED_UI_BG);
    drawLine(6, 6, 765, 6, ED_UI_BORDER);
    drawLine(6, 243, 765, 243, ED_UI_BORDER);
    drawLine(6, 6, 6, 243, ED_UI_BORDER);
    drawLine(765, 6, 765, 243, ED_UI_BORDER);

    drawText(12, 10,  "RC3D EDITOR", 31);
    drawText(12, 24,  "LMB select/drag   RMB add draft point   MMB pan   Wheel zoom", ED_TEXT_COL);
    drawText(12, 36,  "ENTER finish draft   ESC clear draft   DEL delete hovered", ED_TEXT_COL);
    drawText(12, 48,  "CTRL+Z undo, CTRL+Y redo, TAB grid 1.0/0.1, F6 clean map, SHIFT drop = merge", ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Draft: %d   SignedArea: %.2f   Grid: %.1f",
             g_ed.draftCount,
             draftSignedArea(),
             g_ed.currentGridStep);
    drawText(12, 62, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Hover V:%d W:%d S:%d   Selected V:%d W:%d S:%d   Verts:%d Walls:%d Sectors:%d",
             g_ed.hoverVert, g_ed.hoverWall, g_ed.hoverSector,
             g_ed.selectedVert, g_ed.selectedWall, g_ed.selectedSector,
             g_edMap.vertCount, g_edMap.wallCount, g_edMap.sectorCount);
    drawText(12, 74, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Start: %.2f %.2f ang %.2f sector %d",
             g_edMap.startX, g_edMap.startY, g_edMap.startAngle, g_edMap.startSector);
    drawText(12, 86, buf, ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "New sector defaults: floor %.2f  ceil %.2f  floorCol %u  ceilCol %u",
             g_ed.sectorFloor, g_ed.sectorCeil,
             (unsigned)g_ed.sectorFloorColor,
             (unsigned)g_ed.sectorCeilColor);
    drawText(12, 98, buf, ED_TEXT_COL);

    drawText(12, 120, "Buttons below are clickable UI. Keyboard shortcuts still work.", ED_UI_TEXT_INFOTYPE);
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

    if (g_ed.splitPreviewValid && g_ed.selectionType == ED_SEL_WALL) {
        int sx, sy;
        worldToScreen(g_ed.splitPreviewX, g_ed.splitPreviewY, &sx, &sy);

        drawRect(sx - 3, sy - 3, 7, 7, 15);
        drawRect(sx - 1, sy - 1, 3, 3, 63);
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

static void handleEditorUI(int mouseX, int mouseY,
                           int leftDown, int leftPressed, int leftReleased,
                           float worldX, float worldY)
{
    int id = 1000;

    g_ed.uiMouseCaptured = 0;
    g_ed.uiHotId = ED_UI_ID_NONE;

    /* -------------------------------------------------- */
    /* top toolbar                                         */
    /* -------------------------------------------------- */
    {
        const int x = 12;
        const int y = 138;

        int cx = x;



        /* Undo */
        drawUIButton(cx, y, 56, 16, "Undo", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_undoCount <= 0));
        if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, (g_undoCount <= 0))) {
            performUndo();
        }
        cx += 60;

        /* Redo */
        drawUIButton(cx, y, 56, 16, "Redo", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_redoCount <= 0));
        if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, (g_redoCount <= 0))) {
            performRedo();
        }
        cx += 60;

        /* Save */
        drawUIButton(cx, y, 56, 16, "Save", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
        if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            saveTextMap("rc3d_map.txt");
        }
        cx += 60;

        /* Load */
        drawUIButton(cx, y, 56, 16, "Load", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
        if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            loadTextMap("rc3d_map.txt");
        }
        cx += 60;

        /* Export */
        drawUIButton(cx, y, 64, 16, "Export", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
        if (uiButtonLogic(id++, cx, y, 64, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            exportCStringMap("rc3d_map_export.c");
        }
        cx += 68;

        /* Grid */
        drawUIButton(cx, y, 72, 16,
                     g_ed.tinyGridEnabled ? "Grid 0.1" : "Grid 1.0",
                     g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
        if (uiButtonLogic(id++, cx, y, 68, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            g_ed.tinyGridEnabled = !g_ed.tinyGridEnabled;
            g_ed.currentGridStep = g_ed.tinyGridEnabled ? ED_GRID_STEP_TINY : ED_GRID_STEP;
        }
        cx += 76;

        /* Finish draft */
        drawUIButton(cx, y, 72, 16, "Finish", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_ed.draftCount < 3));
        if (uiButtonLogic(id++, cx, y, 72, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, (g_ed.draftCount < 3))) {
            const float area = draftSignedArea();

            pushUndoState();

            if (!finalizeDraftSectorAttached()) {
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
        cx += 76;

        /* Clear draft */
        drawUIButton(cx, y, 72, 16, "ClrDraft", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_ed.draftCount <= 0));
        if (uiButtonLogic(id++, cx, y, 72, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, (g_ed.draftCount <= 0))) {
            clearDraft();
        }

        cx += 76;
        /* Clean map */
        drawUIButton(cx, y, 76, 16, "CleanMap", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
        if (uiButtonLogic(id++, cx, y, 76, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
            pushUndoState();
            cleanMapCompact();
        }

    }

    /* -------------------------------------------------- */
    /* wall tools                                           */
    /* -------------------------------------------------- */
    {
        const int x = 12;
        const int y = 160;

        drawText(x, y + 4, "Wall:", ED_TEXT_COL);

        if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
            EdWall *w = &g_edMap.walls[g_ed.selectedWall];
            int cx = x + 40;

            drawUIButton(cx, y, 52, 16, "Solid", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
            if (uiButtonLogic(id++, cx, y, 52, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
                pushUndoState();
                makeWallSolid(g_ed.selectedWall, (w->midColor == 0) ? 14 : w->midColor);
            }
            cx += 56;

            drawUIButton(cx, y, 56, 16, "Portal", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
            if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
                pushUndoState();
                tryMakeWallPortal(g_ed.selectedWall);
            }
            cx += 60;

            drawUIButton(cx, y, 56, 16, "Window", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
            if (uiButtonLogic(id++, cx, y, 56, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
                float ob = w->openBottom;
                float ot = w->openTop;
                if (ot <= ob) {
                    ob = 0.5f;
                    ot = 1.4f;
                }
                pushUndoState();
                makeWallWindow(g_ed.selectedWall, ob, ot,
                               (w->upperColor == 0) ? 14 : w->upperColor,
                               (w->lowerColor == 0) ? 14 : w->lowerColor);
            }
            cx += 60;

            drawUIButton(cx, y, 48, 16, "Door", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0);
            if (uiButtonLogic(id++, cx, y, 48, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, 0)) {
                float ob = w->openBottom;
                float ot = w->openTop;
                if (ot <= ob) {
                    ob = 0.0f;
                    ot = 1.6f;
                }
                pushUndoState();
                makeWallDoor(g_ed.selectedWall, ob, ot, (w->midColor == 0) ? 14 : w->midColor);
            }
            cx += 52;

            drawUIButton(cx, y, 48, 16, "Split", g_ed.uiHotId == id, g_ed.uiActiveId == id, !g_ed.splitPreviewValid);
            if (uiButtonLogic(id++, cx, y, 48, 16, mouseX, mouseY, leftDown, leftPressed, leftReleased, !g_ed.splitPreviewValid)) {
                pushUndoState();
                splitWallAtSelected(g_ed.selectedWall, worldX, worldY);
            }
        } else {
            drawText(x + 40, y + 4, "No wall selected", 24);
            id += 5;
        }
    }

    /* -------------------------------------------------- */
    /* selected sector controls                             */
    /* -------------------------------------------------- */
    {
        const int x = 12;
        const int y = 182;

        if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
            EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];

            drawText(x, y + 4, "Sector:", ED_TEXT_COL);

            {
                float tempFloor = sec->floorHeight;
                float tempCeil  = sec->ceilHeight;

                if (uiArrowPairFloat(&id, x + 52, y, "Floor", &tempFloor, 0.1f, -128.0f, 128.0f,
                                     mouseX, mouseY, leftDown, leftPressed, leftReleased)) {
                    pushUndoState();
                    sec->floorHeight = tempFloor;
                    if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                        sec->ceilHeight = sec->floorHeight + 0.1f;
                    }
                    syncAllPortals();
                }

                if (uiArrowPairFloat(&id, x + 200, y, "Ceil", &tempCeil, 0.1f, -128.0f, 128.0f,
                                     mouseX, mouseY, leftDown, leftPressed, leftReleased)) {
                    pushUndoState();
                    sec->ceilHeight = tempCeil;
                    if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                        sec->ceilHeight = sec->floorHeight + 0.1f;
                    }
                    syncAllPortals();
                }
            }
        } else {
            drawText(x, y + 4, "Sector: none selected", 24);
        }
    }

    /* -------------------------------------------------- */
    /* selected wall float controls                         */
    /* -------------------------------------------------- */
    {
        const int x = 12;
        const int y = 204;

        if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
            EdWall *w = &g_edMap.walls[g_ed.selectedWall];

            drawText(x, y + 4, "Open:", ED_TEXT_COL);

            {
                float tempBot = w->openBottom;
                float tempTop = w->openTop;

                if (uiArrowPairFloat(&id, x + 40, y, "Bot", &tempBot, 0.1f, -128.0f, 128.0f,
                                     mouseX, mouseY, leftDown, leftPressed, leftReleased)) {
                    pushUndoState();
                    w->openBottom = tempBot;
                    if (w->openTop < w->openBottom) {
                        float t = w->openTop;
                        w->openTop = w->openBottom;
                        w->openBottom = t;
                    }
                }

                if (uiArrowPairFloat(&id, x + 180, y, "Top", &tempTop, 0.1f, -128.0f, 128.0f,
                                     mouseX, mouseY, leftDown, leftPressed, leftReleased)) {
                    pushUndoState();
                    w->openTop = tempTop;
                    if (w->openTop < w->openBottom) {
                        float t = w->openTop;
                        w->openTop = w->openBottom;
                        w->openBottom = t;
                    }
                }
            }
        } else {
            drawText(x, y + 4, "Open: no wall selected", 24);
        }
    }
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

    const int leftPressed  = leftDown && !g_ed.prevLeftDown;
    const int leftReleased = !leftDown && g_ed.prevLeftDown;




    float worldX, worldY;
    screenToWorld(mouseX, mouseY, &worldX, &worldY);

    if (mouseWheelY != 0) {
        const float beforeX = worldX;
        const float beforeY = worldY;

        g_ed.zoom *= (mouseWheelY > 0) ? 1.15f : (1.0f / 1.15f);
        g_ed.zoom = clampf_local(g_ed.zoom, 4.0f, 512.0f);

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
    handleEditorUI(mouseX, mouseY, leftDown, leftPressed, leftReleased, worldX, worldY);




    g_ed.splitPreviewValid = 0;

    {
        int wallToPreview = -1;

        if (g_ed.hoverWall >= 0) {
            wallToPreview = g_ed.hoverWall;
        } else if (g_ed.selectedWall >= 0) {
            wallToPreview = g_ed.selectedWall;
        }

        if (wallToPreview >= 0) {
            if (getWallSplitPreviewPos(wallToPreview, worldX, worldY,
                                       &g_ed.splitPreviewX, &g_ed.splitPreviewY)) {
                g_ed.splitPreviewValid = 1;
            }
        }
    }




    /* right click = add draft point */
    if (rightDown && !g_ed.prevRightDown) {
        addDraftPoint(worldX, worldY);
    }

    //if (leftDown && !g_ed.prevLeftDown) {
    if (leftDown && !g_ed.prevLeftDown && !g_ed.uiMouseCaptured) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;

        selectBestHoverTarget(keys, worldX, worldY);

        if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert >= 0) {
            pushUndoState();

            g_ed.draggingVertex = 1;

            g_ed.dragStartWorldX = worldX;
            g_ed.dragStartWorldY = worldY;
            g_ed.dragVertexStartX = g_edMap.verts[g_ed.selectedVert].x;
            g_ed.dragVertexStartY = g_edMap.verts[g_ed.selectedVert].y;
        }
        else if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
            EdWall *w = &g_edMap.walls[g_ed.selectedWall];
            pushUndoState();
            g_ed.draggingWall = 1;

            g_ed.dragWallStartWorldX = worldX;
            g_ed.dragWallStartWorldY = worldY;

            g_ed.dragWallV0StartX = g_edMap.verts[w->v0].x;
            g_ed.dragWallV0StartY = g_edMap.verts[w->v0].y;
            g_ed.dragWallV1StartX = g_edMap.verts[w->v1].x;
            g_ed.dragWallV1StartY = g_edMap.verts[w->v1].y;
        }
        else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
            pushUndoState();
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
        if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
            float mergeDist = g_ed.currentGridStep * 0.45f;

            if (mergeDist < 0.05f) mergeDist = 0.05f;
            if (mergeDist > 0.45f) mergeDist = 0.45f;

            const int other = findVertexNearWorld(
                g_edMap.verts[g_ed.selectedVert].x,
                g_edMap.verts[g_ed.selectedVert].y,
                mergeDist,
                g_ed.selectedVert
            );

            if (other >= 0) {
                mergeVertexInto(g_ed.selectedVert, other);
            }
        }
    }

    if (!leftDown) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_DELETE)) {
        if (g_ed.hoverVert >= 0) {
            pushUndoState();
            deleteVertexByIndex(g_ed.hoverVert);
        } else if (g_ed.hoverWall >= 0) {
            pushUndoState();
            deleteWallByIndex(g_ed.hoverWall);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_BACKSPACE)) {
        if (g_ed.draftCount > 0) {
            g_ed.draftCount--;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F6)) {
        pushUndoState();
        cleanMapCompact();
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_ESCAPE)) {
        clearDraft();
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) &&
        keyPressedOnce(keys, SDL_SCANCODE_Z)) {
        performUndo();
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) &&
        keyPressedOnce(keys, SDL_SCANCODE_Y)) {
        performRedo();
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_TAB)) {
        g_ed.tinyGridEnabled = !g_ed.tinyGridEnabled;
        g_ed.currentGridStep = g_ed.tinyGridEnabled ? ED_GRID_STEP_TINY : ED_GRID_STEP;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_RETURN)) {
        const float area = draftSignedArea();

        if (g_ed.draftCount >= 3) {
            /* first try boundary attach */
            if (finalizeDraftSectorAttached()) {
                /* done */
            }
            else {
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

        if (keyPressedOnce(keys, SDL_SCANCODE_SPACE)) {
            int wallToSplit = -1;

            if (g_ed.hoverWall >= 0) {
                wallToSplit = g_ed.hoverWall;
            } else if (g_ed.selectedWall >= 0) {
                wallToSplit = g_ed.selectedWall;
            }

            if (wallToSplit >= 0) {
                pushUndoState();
                splitWallAtSelected(wallToSplit, worldX, worldY);
            }
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
        pushUndoState();
        cleanMapCompact();
        exportCStringMap("rc3d_map_export.c");
    }

    memcpy(g_ed.prevKeys, keys, SDL_NUM_SCANCODES);
    g_ed.prevLeftDown = leftDown;
    g_ed.prevRightDown = rightDown;
    g_ed.prevMiddleDown = middleDown;

    g_ed.lastMouseX = mouseX;
    g_ed.lastMouseY = mouseY;
}



static void drawEditorButtons(void)
{
    int id = 1000;

    {
        const int x = 12;
        const int y = 138;
        int cx = x;

        drawUIButton(cx, y, 56, 16, "Undo", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_undoCount <= 0)); id++; cx += 60;
        drawUIButton(cx, y, 56, 16, "Redo", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_redoCount <= 0)); id++; cx += 60;
        drawUIButton(cx, y, 56, 16, "Save", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 60;
        drawUIButton(cx, y, 56, 16, "Load", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 60;
        drawUIButton(cx, y, 64, 16, "Export", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 68;
        drawUIButton(cx, y, 72, 16, g_ed.tinyGridEnabled ? "Grid 0.1" : "Grid 1.0", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 76;
        drawUIButton(cx, y, 72, 16, "Finish", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_ed.draftCount < 3)); id++; cx += 76;
        drawUIButton(cx, y, 72, 16, "ClrDraft", g_ed.uiHotId == id, g_ed.uiActiveId == id, (g_ed.draftCount <= 0)); id++; cx += 76;
        drawUIButton(cx, y, 76, 16, "CleanMap", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 80;
    }

    {
        const int x = 12;
        const int y = 160;

        drawText(x, y + 4, "Wall:", ED_TEXT_COL);

        if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
            int cx = x + 40;
            drawUIButton(cx, y, 52, 16, "Solid",  g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 56;
            drawUIButton(cx, y, 56, 16, "Portal", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 60;
            drawUIButton(cx, y, 56, 16, "Window", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 60;
            drawUIButton(cx, y, 48, 16, "Door",   g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++; cx += 52;
            drawUIButton(cx, y, 48, 16, "Split",  g_ed.uiHotId == id, g_ed.uiActiveId == id, !g_ed.splitPreviewValid); id++;
        } else {
            drawText(x + 40, y + 4, "No wall selected", 11);
            id += 5;
        }
    }

    {
        const int x = 12;
        const int y = 182;

        if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
            char buf[64];
            const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];

            drawText(x, y + 4, "Sector:", ED_TEXT_COL);

            drawUIButton(x + 136, y, 16, 16, "-", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            drawUIButton(x + 156, y, 16, 16, "+", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            snprintf(buf, sizeof(buf), "Floor %.2f", sec->floorHeight);
            drawText(x + 52, y + 4, buf, ED_TEXT_COL);

            drawUIButton(x + 284, y, 16, 16, "-", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            drawUIButton(x + 304, y, 16, 16, "+", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            snprintf(buf, sizeof(buf), "Ceil %.2f", sec->ceilHeight);
            drawText(x + 200, y + 4, buf, ED_TEXT_COL);
        } else {
            drawText(x, y + 4, "Sector: none selected", 11);
            id += 4;
        }
    }

    {
        const int x = 12;
        const int y = 204;

        if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
            char buf[64];
            const EdWall *w = &g_edMap.walls[g_ed.selectedWall];

            drawText(x, y + 4, "Open:", ED_TEXT_COL);

            drawUIButton(x + 124, y, 16, 16, "-", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            drawUIButton(x + 144, y, 16, 16, "+", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            snprintf(buf, sizeof(buf), "Bot %.2f", w->openBottom);
            drawText(x + 40, y + 4, buf, ED_TEXT_COL);

            drawUIButton(x + 264, y, 16, 16, "-", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            drawUIButton(x + 284, y, 16, 16, "+", g_ed.uiHotId == id, g_ed.uiActiveId == id, 0); id++;
            snprintf(buf, sizeof(buf), "Top %.2f", w->openTop);
            drawText(x + 180, y + 4, buf, ED_TEXT_COL);
        } else {
            drawText(x, y + 4, "Open: no wall selected", 11);
            id += 4;
        }
    }
}






void rc3dEditRender(void)
{
    drawGrid();
    drawMapGeometry();
    drawStartMarker();
    drawEditorUI();
    drawEditorButtons();
    drawInspectorPanel();
}






