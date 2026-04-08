// file: rc3edit.c

#include "rc3dedit.h"

#include <SDL2/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

#define ED_COLOUR_HOVER_WALL        30  // dark magenta
#define ED_COLOUR_SELECTED_WALL     135  // magenta

#define ED_DRAFT_COL                26  // light gray

#define ED_CURSOR_COL           11

#define ED_START_COL            2   // white
#define ED_HOME_GRID_COL        5


#define RC3D_EXPORT_MODE_C      0
#define RC3D_EXPORT_MODE_BINARY 1


//// GUI system settings
#define ED_UI_PAD               6
#define ED_FONT_W               8
#define ED_FONT_H               16
#define ED_ROW_STEP             (ED_FONT_H + ED_UI_PAD)

#define ED_TOPBAR_X             6
#define ED_TOPBAR_Y             6
#define ED_TOPBAR_W             890
#define ED_TOPBAR_H             (ED_FONT_H + (ED_UI_PAD * 2))

#define ED_PANEL_X              6
#define ED_PANEL_Y              (ED_TOPBAR_Y + ED_TOPBAR_H + 4)
#define ED_PANEL_W              782
#define ED_PANEL_H              280



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
#define ED_BTN_H                20
#define ED_BTN_GAP              4


#define ED_INSPECTOR_PANEL          450
#define ED_HOVER_FOCUS_INFO_PANEL   26
#define EDIT_VIEW_PORT_WIDTH    (SCREEN_W - ED_INSPECTOR_PANEL)
#define EDIT_VIEW_PORT_HEIGHT   (SCREEN_H - ED_HOVER_FOCUS_INFO_PANEL)



enum
{
    GUI_BTN_UNDO = 100,
    GUI_BTN_REDO,
    GUI_BTN_QUIT,
    GUI_BTN_HELP,
    GUI_BTN_NEWMAP,
    GUI_BTN_LOAD,
    GUI_BTN_SAVE,
    GUI_BTN_EXPORT,
    GUI_BTN_GRID,
    GUI_BTN_FINISH,
    GUI_BTN_CLRDRAFT,
    GUI_BTN_CLEANMAP,

    GUI_BTN_WALL_SOLID,
    GUI_BTN_WALL_PORTAL,
    GUI_BTN_WALL_WINDOW,
    GUI_BTN_WALL_DOOR,
    GUI_BTN_WALL_SPLIT,

    GUI_BTN_SECTOR_FLOOR_MINUS,
    GUI_BTN_SECTOR_FLOOR_PLUS,
    GUI_BTN_SECTOR_CEIL_MINUS,
    GUI_BTN_SECTOR_CEIL_PLUS,

    GUI_BTN_WALL_OPENBOT_MINUS,
    GUI_BTN_WALL_OPENBOT_PLUS,
    GUI_BTN_WALL_OPENTOP_MINUS,
    GUI_BTN_WALL_OPENTOP_PLUS,

    GUI_BTN_CONFIRM_YES,
    GUI_BTN_CONFIRM_NO
};

typedef enum {
    ED_CONFIRM_NONE = 0,
    ED_CONFIRM_NEW_MAP
} EdConfirmAction;


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

    // drafting
    uint8_t newWallUpperColor;
    uint8_t newWallMidColor;
    uint8_t newWallLowerColor;



    float copiedSectorFloor;
    float copiedSectorCeil;
    int   hasCopiedSectorFloor;
    int   hasCopiedSectorCeil;

    uint8_t copiedSectorFloorColor;
    uint8_t copiedSectorCeilColor;
    int     hasCopiedSectorFloorColor;
    int     hasCopiedSectorCeilColor;


    int hoverVert;
    int hoverWall;
    int hoverSector;

    int splitPreviewValid;
    float splitPreviewX;
    float splitPreviewY;

    int selectedVert;
    int selectedWall;
    int selectedSector;

    /* multi-vertex selection */
    uint8_t selectedVerts[ED_MAX_VERTS];
    int selectedVertCount;

    /* box select */
    int boxSelecting;
    int boxStartMouseX;
    int boxStartMouseY;
    int boxEndMouseX;
    int boxEndMouseY;

    /* multi-vertex drag */
    int draggingMultiVertex;
    float dragMultiStartWorldX;
    float dragMultiStartWorldY;
    int   dragMultiVertCount;
    int   dragMultiVertIndices[ED_MAX_VERTS];
    float dragMultiVertStartX[ED_MAX_VERTS];
    float dragMultiVertStartY[ED_MAX_VERTS];

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
    uint8_t ui_menu_visable;

    char statusText[256];
    float statusTimer;

    int confirmVisible;
    EdConfirmAction confirmAction;
    char confirmText[256];

} EditorState;



static EditorMap g_edMap;
static EditorState g_ed;

static char g_mapDialogDir[1024];
static char g_exportDialogDir[1024];
static int g_dialogDirsInit = 0;

static int mergeCloseVertices(float epsilon);
static void repairMapTopology(void);


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

    // draft
    uint8_t newWallUpperColor;
    uint8_t newWallMidColor;
    uint8_t newWallLowerColor;

    float copiedSectorFloor;
    float copiedSectorCeil;
    int   hasCopiedSectorFloor;
    int   hasCopiedSectorCeil;

    uint8_t copiedSectorFloorColor;
    uint8_t copiedSectorCeilColor;
    int     hasCopiedSectorFloorColor;
    int     hasCopiedSectorCeilColor;

    int selectedVert;
    int selectedWall;
    int selectedSector;

    uint8_t selectedVerts[ED_MAX_VERTS];
    int selectedVertCount;

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
static void resetWallToSolidFromOwnColour(int wallIndex);
static int findSectorOwningWall(int wallIndex);
static void compactOrphanVertices(void);
static void cleanMapCompact(void);
static int findBoundaryVertexIndexInSector(int sectorIndex, int vertIndex);
static int splitSelectedSectorByDraftLine(void);

static void clearAllSelections(void);
static void clearMultiVertexSelection(void);
static int findBoundaryWallNearPointInSector(int sectorIndex, float wx, float wy);
static int splitBoundaryWallAtPoint(int sectorIndex, int localWallIndex, float wx, float wy);
static float distPointSegSq(float px, float py, float ax, float ay, float bx, float by);
static void cleanMapCompactWithReport(int *removedVerts, int *removedWalls, int *removedSectors);
static void drawStatusPopup(void);
static void acceptConfirmDialog(void);
static void closeConfirmDialog(void);
static void openConfirmDialog(EdConfirmAction action, const char *text);
static int buildInnerSectorsFromSelectedSector(void);
static float innerLoopSignedAreaFromWalls(const int *wallIndices, int count);

static void pathDirnameFromFile(char *outDir, size_t outDirSize, const char *path);
static void initRememberedDialogDirs(void);



/////// GUI PARTS
static void handleEditorUI(int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased, float worldX, float worldY);

int g_dirtyGui = 0;

void rc3dGuiDirty(){
    g_dirtyGui = 1;
}

int rc3dGuiCheckDirty(){
    int g_dirtyGuiTmp;

    g_dirtyGuiTmp = g_dirtyGui;
    g_dirtyGui = 0;
    return g_dirtyGuiTmp;
}

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

static float snapDeltaf(float v)
{
    float step = g_ed.currentGridStep;

    if (step < ED_GRID_STEP_TINY) {
        step = ED_GRID_STEP_TINY;
    }

    return roundf(v / step) * step;
}

static float snapf(float v)
{
    float step = g_ed.currentGridStep;

    if (step < ED_GRID_STEP_TINY) {
        step = ED_GRID_STEP_TINY;
    }

    return roundf(v / step) * step;
}

static void setEditorStatus(const char *text)
{
    if (!text) {
        g_ed.statusText[0] = '\0';
        g_ed.statusTimer = 0.0f;
        return;
    }

    snprintf(g_ed.statusText, sizeof(g_ed.statusText), "%s", text);
    g_ed.statusTimer = 2.5f;// / 8.0f; // 8ms timeout pause in the main loop
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

    s->newWallUpperColor = g_ed.newWallUpperColor;
    s->newWallMidColor   = g_ed.newWallMidColor;
    s->newWallLowerColor = g_ed.newWallLowerColor;

    s->copiedSectorFloor = g_ed.copiedSectorFloor;
    s->copiedSectorCeil = g_ed.copiedSectorCeil;
    s->hasCopiedSectorFloor = g_ed.hasCopiedSectorFloor;
    s->hasCopiedSectorCeil = g_ed.hasCopiedSectorCeil;

    s->copiedSectorFloorColor = g_ed.copiedSectorFloorColor;
    s->copiedSectorCeilColor = g_ed.copiedSectorCeilColor;
    s->hasCopiedSectorFloorColor = g_ed.hasCopiedSectorFloorColor;
    s->hasCopiedSectorCeilColor = g_ed.hasCopiedSectorCeilColor;

    s->selectedVert = g_ed.selectedVert;
    s->selectedWall = g_ed.selectedWall;
    s->selectedSector = g_ed.selectedSector;

    memcpy(s->selectedVerts, g_ed.selectedVerts, sizeof(g_ed.selectedVerts));
    s->selectedVertCount = g_ed.selectedVertCount;

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

    g_ed.newWallUpperColor = s->newWallUpperColor;
    g_ed.newWallMidColor   = s->newWallMidColor;
    g_ed.newWallLowerColor = s->newWallLowerColor;

    g_ed.copiedSectorFloor = s->copiedSectorFloor;
    g_ed.copiedSectorCeil = s->copiedSectorCeil;
    g_ed.hasCopiedSectorFloor = s->hasCopiedSectorFloor;
    g_ed.hasCopiedSectorCeil = s->hasCopiedSectorCeil;

    g_ed.copiedSectorFloorColor = s->copiedSectorFloorColor;
    g_ed.copiedSectorCeilColor = s->copiedSectorCeilColor;
    g_ed.hasCopiedSectorFloorColor = s->hasCopiedSectorFloorColor;
    g_ed.hasCopiedSectorCeilColor = s->hasCopiedSectorCeilColor;

    g_ed.selectedVert = s->selectedVert;
    g_ed.selectedWall = s->selectedWall;
    g_ed.selectedSector = s->selectedSector;

    memcpy(g_ed.selectedVerts, s->selectedVerts, sizeof(g_ed.selectedVerts));
    g_ed.selectedVertCount = s->selectedVertCount;

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

    g_ed.boxSelecting = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiVertCount = 0;

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
    *wx = g_ed.camX + ((float)sx - (EDIT_VIEW_PORT_WIDTH * 0.5f)) / g_ed.zoom;
    *wy = g_ed.camY + ((float)sy - (EDIT_VIEW_PORT_HEIGHT * 0.5f)) / g_ed.zoom;
}

static void worldToScreen(float wx, float wy, int *sx, int *sy)
{
    *sx = (int)lroundf((wx - g_ed.camX) * g_ed.zoom + (EDIT_VIEW_PORT_WIDTH * 0.5f));
    *sy = (int)lroundf((wy - g_ed.camY) * g_ed.zoom + (EDIT_VIEW_PORT_HEIGHT * 0.5f));
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



static int findBoundaryWallNearPointInSector(int sectorIndex, float wx, float wy)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return -1;

    const EdSector *sec = &g_edMap.sectors[sectorIndex];
    const float worldPick = (float)ED_PICK_DIST_PX / g_ed.zoom;
    const float worldPickSq = worldPick * worldPick;

    int bestLocal = -1;
    float bestD2 = worldPickSq;

    for (int i = 0; i < sec->boundaryCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];

        const float d2 = distPointSegSq(wx, wy, a->x, a->y, b->x, b->y);
        if (d2 <= bestD2) {
            bestD2 = d2;
            bestLocal = i;
        }
    }

    return bestLocal;
}




static int splitBoundaryWallAtPoint(int sectorIndex, int localWallIndex, float wx, float wy)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return -1;

    EdSector *sec = &g_edMap.sectors[sectorIndex];
    if (localWallIndex < 0 || localWallIndex >= sec->boundaryCount) return -1;

    if (g_edMap.wallCount >= ED_MAX_WALLS) return -1;
    if (g_edMap.vertCount >= ED_MAX_VERTS) return -1;

    const int wallIndex = sec->wallStart + localWallIndex;

    float sx, sy;
    if (!getWallSplitPreviewPos(wallIndex, wx, wy, &sx, &sy)) {
        return -1;
    }

    EdWall original = g_edMap.walls[wallIndex];

    int newVert = findVertexExact(sx, sy);
    if (newVert < 0) {
        newVert = addVertex(sx, sy);
        if (newVert < 0) return -1;
    }

    if (newVert == original.v0 || newVert == original.v1) {
        return -1;
    }

    /* make room for one new wall directly after this wall */
    for (int i = g_edMap.wallCount; i > wallIndex + 1; i--) {
        g_edMap.walls[i] = g_edMap.walls[i - 1];
    }
    g_edMap.wallCount++;

    /* split original into two */
    g_edMap.walls[wallIndex].v1 = newVert;
    g_edMap.walls[wallIndex + 1] = original;
    g_edMap.walls[wallIndex + 1].v0 = newVert;

    sec->wallCount++;
    sec->boundaryCount++;

    /* shift later sector wallStart values */
    for (int s = sectorIndex + 1; s < g_edMap.sectorCount; s++) {
        g_edMap.sectors[s].wallStart++;
    }

    syncAllPortals();
    return newVert;
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

        for (int i = 0; i < sec->wallCount; i++) {
            const int wi = sec->wallStart + i;
            EdWall *w = &g_edMap.walls[wi];
            const int other = findReversedWall(w->v0, w->v1);

            if (other >= 0 && other != wi) {
                const int otherSector = findSectorOwningWall(other);

                if (otherSector >= 0 && otherSector != s) {
                    setPortalPair(wi, s, other, otherSector);
                }
            }
        }
    }
}

static float innerLoopSignedAreaFromWalls(const int *wallIndices, int count)
{
    float area = 0.0f;

    for (int i = 0; i < count; i++) {
        const EdWall *w = &g_edMap.walls[wallIndices[i]];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];

        area += (a->x * b->y) - (b->x * a->y);
    }

    return area * 0.5f;
}

static int buildInnerSectorsFromSelectedSector(void)
{
    int candidateWalls[ED_MAX_WALLS];
    uint8_t consumed[ED_MAX_WALLS];
    int candidateCount = 0;
    int createdCount = 0;

    if (g_ed.selectedSector < 0 || g_ed.selectedSector >= g_edMap.sectorCount) {
        return 0;
    }

    {
        const EdSector *outer = &g_edMap.sectors[g_ed.selectedSector];
        const int start = outer->wallStart + outer->boundaryCount;
        const int end   = outer->wallStart + outer->wallCount;

        /* collect only inner non-portal non-degenerate walls */
        for (int wi = start; wi < end; wi++) {
            const EdWall *w = &g_edMap.walls[wi];

            if (w->v0 < 0 || w->v1 < 0) continue;
            if (w->v0 == w->v1) continue;
            if (w->flags & RC3D_WALL_PORTAL) continue;

            if ((w->flags & (RC3D_WALL_SOLID | RC3D_WALL_MIDDLE)) == 0) {
                continue;
            }

            if (candidateCount >= ED_MAX_WALLS) {
                break;
            }

            candidateWalls[candidateCount++] = wi;
        }
    }

    if (candidateCount < 3) {
        return 0;
    }

    memset(consumed, 0, sizeof(consumed));

    for (int ci = 0; ci < candidateCount; ci++) {
        int loopWalls[ED_MAX_WALLS];
        int loopCandidateIndices[ED_MAX_WALLS];
        uint8_t tempUsed[ED_MAX_WALLS];
        int loopCount = 0;
        int currentV;
        int startV;

        if (consumed[ci]) {
            continue;
        }

        memset(tempUsed, 0, sizeof(tempUsed));

        {
            const EdWall *w = &g_edMap.walls[candidateWalls[ci]];

            startV = w->v0;
            currentV = w->v1;

            loopWalls[loopCount] = candidateWalls[ci];
            loopCandidateIndices[loopCount] = ci;
            loopCount++;

            tempUsed[ci] = 1;
        }

        while (currentV != startV) {
            int nextCandidate = -1;
            int matchCount = 0;

            for (int cj = 0; cj < candidateCount; cj++) {
                const EdWall *w;

                if (consumed[cj] || tempUsed[cj]) {
                    continue;
                }

                w = &g_edMap.walls[candidateWalls[cj]];

                if (w->v0 == currentV && w->v0 != w->v1) {
                    nextCandidate = cj;
                    matchCount++;
                }
            }

            /* ambiguous or dead-end chain = not a usable loop */
            if (matchCount != 1) {
                loopCount = 0;
                break;
            }

            if (loopCount >= ED_MAX_WALLS) {
                loopCount = 0;
                break;
            }

            {
                const EdWall *w = &g_edMap.walls[candidateWalls[nextCandidate]];

                loopWalls[loopCount] = candidateWalls[nextCandidate];
                loopCandidateIndices[loopCount] = nextCandidate;
                loopCount++;

                tempUsed[nextCandidate] = 1;
                currentV = w->v1;
            }
        }

        if (loopCount < 3) {
            continue;
        }

        {
            const float area = innerLoopSignedAreaFromWalls(loopWalls, loopCount);

            if (absf_local(area) <= 0.0001f) {
                continue;
            }
        }

        if (g_edMap.sectorCount >= ED_MAX_SECTORS) {
            break;
        }

        if ((g_edMap.wallCount + loopCount) > ED_MAX_WALLS) {
            break;
        }

        /* build reversed copy of the loop as the new sector boundary */
        {
            const int newSectorIndex = g_edMap.sectorCount;
            const int newWallStart = g_edMap.wallCount;

            for (int i = loopCount - 1; i >= 0; i--) {
                const EdWall *src = &g_edMap.walls[loopWalls[i]];
                EdWall *dst = &g_edMap.walls[g_edMap.wallCount++];

                dst->v0 = src->v1;
                dst->v1 = src->v0;
                dst->neighbour = -1;
                dst->openBottom = 0.0f;
                dst->openTop = 0.0f;
                dst->upperColor = src->upperColor;
                dst->midColor   = src->midColor;
                dst->lowerColor = src->lowerColor;
                dst->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            }

            g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
            g_edMap.sectors[newSectorIndex].wallCount = loopCount;
            g_edMap.sectors[newSectorIndex].boundaryCount = loopCount;

            /* new sector uses current inspector defaults */
            g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
            g_edMap.sectors[newSectorIndex].ceilHeight  = g_ed.sectorCeil;
            g_edMap.sectors[newSectorIndex].floorColor  = g_ed.sectorFloorColor;
            g_edMap.sectors[newSectorIndex].ceilColor   = g_ed.sectorCeilColor;

            g_edMap.sectorCount++;
            createdCount++;
        }

        for (int i = 0; i < loopCount; i++) {
            consumed[loopCandidateIndices[i]] = 1;
        }
    }

    if (createdCount > 0) {
        syncAllPortals();
        g_ed.hoverWall = -1;
        g_ed.selectedWall = -1;
        g_ed.hoverVert = -1;
        g_ed.selectedVert = -1;
    }

    return createdCount;
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

    const float dx = snapDeltaf(worldX - g_ed.dragSectorStartWorldX);
    const float dy = snapDeltaf(worldY - g_ed.dragSectorStartWorldY);

    for (int i = 0; i < g_ed.dragSectorVertCount; i++) {
        const int vi = g_ed.dragSectorVertIndices[i];
        g_edMap.verts[vi].x = g_ed.dragSectorVertStartX[i] + dx;
        g_edMap.verts[vi].y = g_ed.dragSectorVertStartY[i] + dy;
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

static void deleteSectorByIndex(int sectorIndex)
{
    clearMultiVertexSelection();
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return;
    }

    /* first: any neighbouring walls pointing into this sector
       must be restored to ordinary solid walls */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];
        if (w->neighbour == sectorIndex) {
            resetWallToSolidFromOwnColour(i);
        }
    }

    /* mark all walls owned by this sector as deleted */
    {
        const EdSector *sec = &g_edMap.sectors[sectorIndex];
        const int start = sec->wallStart;
        const int end   = start + sec->wallCount;

        for (int i = start; i < end; i++) {
            g_edMap.walls[i].v0 = -1;
            g_edMap.walls[i].v1 = -1;
            g_edMap.walls[i].neighbour = -1;
        }
    }

    /* remove sector from sector list */
    for (int i = sectorIndex; i < (g_edMap.sectorCount - 1); i++) {
        g_edMap.sectors[i] = g_edMap.sectors[i + 1];
    }
    g_edMap.sectorCount--;

    /* fix wall neighbour indices */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        EdWall *w = &g_edMap.walls[i];

        if (w->neighbour == sectorIndex) {
            w->neighbour = -1;
        } else if (w->neighbour > sectorIndex) {
            w->neighbour--;
        }
    }

    /* fix start sector */
    if (g_edMap.startSector == sectorIndex) {
        g_edMap.startSector = -1;
    } else if (g_edMap.startSector > sectorIndex) {
        g_edMap.startSector--;
    }

    if (g_edMap.startSector < 0) {
        g_edMap.startSector = (g_edMap.sectorCount > 0) ? 0 : -1;
    }

    /* fix editor selection / hover */
    if (g_ed.selectedSector == sectorIndex) {
        g_ed.selectedSector = -1;
        if (g_ed.selectionType == ED_SEL_SECTOR) {
            g_ed.selectionType = ED_SEL_NONE;
        }
    } else if (g_ed.selectedSector > sectorIndex) {
        g_ed.selectedSector--;
    }

    if (g_ed.hoverSector == sectorIndex) {
        g_ed.hoverSector = -1;
    } else if (g_ed.hoverSector > sectorIndex) {
        g_ed.hoverSector--;
    }

    /* clear sector drag state */
    g_ed.draggingSector = 0;

    /* rebuild remaining wall layout and clean orphan verts */
    rebuildSectorWallLayout();
    compactOrphanVertices();
    syncAllPortals();

    /* safety clear */
    g_ed.hoverWall = -1;
    g_ed.selectedWall = -1;
    g_ed.hoverVert = -1;
    g_ed.selectedVert = -1;
}

static int mergeSectorsAcrossWall(int wallIndex)
{
    static EdWall mergedWalls[ED_MAX_WALLS];

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return 0;
    }

    const EdWall *splitA = &g_edMap.walls[wallIndex];
    const int otherWall = findReversedWall(splitA->v0, splitA->v1);

    if (otherWall < 0 || otherWall == wallIndex) {
        return 0;
    }

    const int sectorA = findSectorOwningWall(wallIndex);
    const int sectorB = findSectorOwningWall(otherWall);

    if (sectorA < 0 || sectorB < 0 || sectorA == sectorB) {
        return 0;
    }

    EdSector *secA = &g_edMap.sectors[sectorA];
    EdSector *secB = &g_edMap.sectors[sectorB];

    /* keep this sane: only merge plain boundary sectors */
    if (secA->wallCount != secA->boundaryCount) return 0;
    if (secB->wallCount != secB->boundaryCount) return 0;

    /* sector properties must match or the merged room is nonsense */
    if (absf_local(secA->floorHeight - secB->floorHeight) > ED_EPSILON) return 0;
    if (absf_local(secA->ceilHeight  - secB->ceilHeight)  > ED_EPSILON) return 0;
    if (secA->floorColor != secB->floorColor) return 0;
    if (secA->ceilColor  != secB->ceilColor)  return 0;

    const int localA = wallIndex - secA->wallStart;
    const int localB = otherWall - secB->wallStart;

    if (localA < 0 || localA >= secA->boundaryCount) return 0;
    if (localB < 0 || localB >= secB->boundaryCount) return 0;

    if (secA->boundaryCount < 3 || secB->boundaryCount < 3) return 0;

    int mergedCount = 0;

    /* Walk A after removed wall */
    for (int step = 1; step < secA->boundaryCount; step++) {
        const int idx = (localA + step) % secA->boundaryCount;
        mergedWalls[mergedCount] = g_edMap.walls[secA->wallStart + idx];
        mergedWalls[mergedCount].neighbour = -1;
        mergedCount++;
    }

    /* Walk B after removed reverse wall */
    for (int step = 1; step < secB->boundaryCount; step++) {
        const int idx = (localB + step) % secB->boundaryCount;
        mergedWalls[mergedCount] = g_edMap.walls[secB->wallStart + idx];
        mergedWalls[mergedCount].neighbour = -1;
        mergedCount++;
    }

    if (mergedCount < 3) {
        return 0;
    }

    /* validate the merged chain really forms one clean loop */
    for (int i = 0; i < mergedCount; i++) {
        const int next = (i + 1) % mergedCount;
        if (mergedWalls[i].v1 != mergedWalls[next].v0) {
            return 0;
        }
    }

    const int keepSector = (sectorA < sectorB) ? sectorA : sectorB;
    const int killSector = (sectorA < sectorB) ? sectorB : sectorA;

    const float keepFloor = g_edMap.sectors[keepSector].floorHeight;
    const float keepCeil  = g_edMap.sectors[keepSector].ceilHeight;
    const uint8_t keepFloorCol = g_edMap.sectors[keepSector].floorColor;
    const uint8_t keepCeilCol  = g_edMap.sectors[keepSector].ceilColor;

    /* delete both original sector wall blocks */
    {
        const int startA = secA->wallStart;
        const int endA   = startA + secA->wallCount;
        for (int i = startA; i < endA; i++) {
            g_edMap.walls[i].v0 = -1;
            g_edMap.walls[i].v1 = -1;
            g_edMap.walls[i].neighbour = -1;
        }

        const int startB = secB->wallStart;
        const int endB   = startB + secB->wallCount;
        for (int i = startB; i < endB; i++) {
            g_edMap.walls[i].v0 = -1;
            g_edMap.walls[i].v1 = -1;
            g_edMap.walls[i].neighbour = -1;
        }
    }

    rebuildSectorWallLayout();

    if ((g_edMap.wallCount + mergedCount) > ED_MAX_WALLS) {
        return 0;
    }

    /* remove killed sector entry */
    for (int i = killSector; i < (g_edMap.sectorCount - 1); i++) {
        g_edMap.sectors[i] = g_edMap.sectors[i + 1];
    }
    g_edMap.sectorCount--;

    /* fix editor references */
    if (g_edMap.startSector == killSector || g_edMap.startSector == keepSector) {
        g_edMap.startSector = keepSector;
    } else if (g_edMap.startSector > killSector) {
        g_edMap.startSector--;
    }

    if (g_ed.selectedSector == killSector || g_ed.selectedSector == keepSector) {
        g_ed.selectedSector = keepSector;
        g_ed.selectionType = ED_SEL_SECTOR;
    } else if (g_ed.selectedSector > killSector) {
        g_ed.selectedSector--;
    }

    if (g_ed.hoverSector == killSector || g_ed.hoverSector == keepSector) {
        g_ed.hoverSector = keepSector;
    } else if (g_ed.hoverSector > killSector) {
        g_ed.hoverSector--;
    }

    /* append clean merged boundary */
    {
        const int newStart = g_edMap.wallCount;

        for (int i = 0; i < mergedCount; i++) {
            g_edMap.walls[g_edMap.wallCount++] = mergedWalls[i];
        }

        g_edMap.sectors[keepSector].wallStart = newStart;
        g_edMap.sectors[keepSector].wallCount = mergedCount;
        g_edMap.sectors[keepSector].boundaryCount = mergedCount;
        g_edMap.sectors[keepSector].floorHeight = keepFloor;
        g_edMap.sectors[keepSector].ceilHeight = keepCeil;
        g_edMap.sectors[keepSector].floorColor = keepFloorCol;
        g_edMap.sectors[keepSector].ceilColor = keepCeilCol;
    }

    g_ed.draggingSector = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingVertex = 0;

    g_ed.selectedWall = -1;
    g_ed.hoverWall = -1;
    g_ed.selectedVert = -1;
    g_ed.hoverVert = -1;

    compactOrphanVertices();
    syncAllPortals();

    return 1;
}

static void deleteWallByIndex(int wallIndex)
{
    clearMultiVertexSelection();
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return;
    }

    /* if this wall has a reversed partner in another sector,
       try to merge the two sectors instead of just deleting one side */
    if (mergeSectorsAcrossWall(wallIndex)) {
        return;
    }

    /* otherwise normal single-wall delete */
    {
        EdWall *w = &g_edMap.walls[wallIndex];
        const int other = findReversedWall(w->v0, w->v1);

        if (other >= 0 && other != wallIndex) {
            resetWallToSolidFromOwnColour(other);
        }
    }

    g_edMap.walls[wallIndex].v0 = -1;
    g_edMap.walls[wallIndex].v1 = -1;
    g_edMap.walls[wallIndex].neighbour = -1;

    rebuildSectorWallLayout();
    compactOrphanVertices();
    syncAllPortals();

    g_ed.hoverWall = -1;
    g_ed.selectedWall = -1;
    g_ed.hoverVert = -1;
    g_ed.selectedVert = -1;
}

static void deleteVertexByIndex(int vertIndex)
{
    clearMultiVertexSelection();
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
    const int other = findReversedWall(w->v0, w->v1);

    /* this side */
    w->neighbour   = -1;
    w->openBottom  = 0.0f;
    w->openTop     = 0.0f;
    w->upperColor  = 0;
    w->midColor    = midColor;
    w->lowerColor  = 0;
    w->flags       = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;

    /* if the reversed partner exists, make THAT solid too */
    if (other >= 0 && other != wallIndex) {
        EdWall *ow = &g_edMap.walls[other];

        uint8_t otherCol = midColor;
        if (otherCol == 0) {
            otherCol = ow->midColor;
            if (otherCol == 0) otherCol = ow->upperColor;
            if (otherCol == 0) otherCol = ow->lowerColor;
            if (otherCol == 0) otherCol = 14;
        }

        ow->neighbour   = -1;
        ow->openBottom  = 0.0f;
        ow->openTop     = 0.0f;
        ow->upperColor  = 0;
        ow->midColor    = otherCol;
        ow->lowerColor  = 0;
        ow->flags       = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    }
}

static void resetWallToSolidFromOwnColour(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    EdWall *w = &g_edMap.walls[wallIndex];

    /* pick a sensible solid colour from whatever the wall already has */
    uint8_t solidCol = w->midColor;
    if (solidCol == 0) solidCol = w->upperColor;
    if (solidCol == 0) solidCol = w->lowerColor;
    if (solidCol == 0) solidCol = 14;

    w->neighbour   = -1;
    w->openBottom  = 0.0f;
    w->openTop     = 0.0f;
    w->upperColor  = 0;
    w->midColor    = solidCol;
    w->lowerColor  = 0;
    w->flags       = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
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
    clearMultiVertexSelection();
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

    clearAllSelections();

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

static void clearMultiVertexSelection(void)
{
    memset(g_ed.selectedVerts, 0, sizeof(g_ed.selectedVerts));
    g_ed.selectedVertCount = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiVertCount = 0;
}

static void clearAllSelections(void)
{
    g_ed.selectionType = ED_SEL_NONE;
    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    clearMultiVertexSelection();
}



static int findSelectedVertexNearMouse(int mouseX, int mouseY)
{
    float bestDistSq = (float)(ED_PICK_DIST_PX * ED_PICK_DIST_PX);
    int best = -1;

    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;
        float d2;
        int dx, dy;

        if (!g_ed.selectedVerts[i]) continue;

        worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);

        dx = sx - mouseX;
        dy = sy - mouseY;
        d2 = (float)(dx * dx + dy * dy);

        if (d2 <= bestDistSq) {
            bestDistSq = d2;
            best = i;
        }
    }

    return best;
}

static void beginBoxSelect(int mouseX, int mouseY)
{
    g_ed.boxSelecting = 1;
    g_ed.boxStartMouseX = mouseX;
    g_ed.boxStartMouseY = mouseY;
    g_ed.boxEndMouseX = mouseX;
    g_ed.boxEndMouseY = mouseY;
}

static void updateBoxSelect(int mouseX, int mouseY)
{
    if (!g_ed.boxSelecting) return;

    g_ed.boxEndMouseX = mouseX;
    g_ed.boxEndMouseY = mouseY;
}

static void finalizeBoxSelect(void)
{
    int x0, y0, x1, y1;

    if (!g_ed.boxSelecting) return;

    x0 = g_ed.boxStartMouseX;
    y0 = g_ed.boxStartMouseY;
    x1 = g_ed.boxEndMouseX;
    y1 = g_ed.boxEndMouseY;

    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    clearAllSelections();

    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;

        worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);

        if (sx >= x0 && sx <= x1 && sy >= y0 && sy <= y1) {
            g_ed.selectedVerts[i] = 1;
            g_ed.selectedVertCount++;
        }
    }

    g_ed.boxSelecting = 0;
}

static void beginMultiVertexDrag(float worldX, float worldY)
{
    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiVertCount = 0;

    if (g_ed.selectedVertCount <= 0) return;

    g_ed.dragMultiStartWorldX = worldX;
    g_ed.dragMultiStartWorldY = worldY;

    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (!g_ed.selectedVerts[i]) continue;

        if (g_ed.dragMultiVertCount >= ED_MAX_VERTS) break;

        g_ed.dragMultiVertIndices[g_ed.dragMultiVertCount] = i;
        g_ed.dragMultiVertStartX[g_ed.dragMultiVertCount] = g_edMap.verts[i].x;
        g_ed.dragMultiVertStartY[g_ed.dragMultiVertCount] = g_edMap.verts[i].y;
        g_ed.dragMultiVertCount++;
    }

    if (g_ed.dragMultiVertCount > 0) {
        g_ed.draggingMultiVertex = 1;
    }
}

static void dragMultiVertexSelectionTo(float worldX, float worldY)
{
    float dx, dy;

    if (!g_ed.draggingMultiVertex) return;

    dx = snapDeltaf(worldX - g_ed.dragMultiStartWorldX);
    dy = snapDeltaf(worldY - g_ed.dragMultiStartWorldY);

    for (int i = 0; i < g_ed.dragMultiVertCount; i++) {
        const int vi = g_ed.dragMultiVertIndices[i];

        if (vi < 0 || vi >= g_edMap.vertCount) continue;

        g_edMap.verts[vi].x = g_ed.dragMultiVertStartX[i] + dx;
        g_edMap.verts[vi].y = g_ed.dragMultiVertStartY[i] + dy;
    }

    syncAllPortals();
}

static void drawBoxSelectRect(void)
{
    int x0, y0, x1, y1;

    if (!g_ed.boxSelecting) return;

    x0 = g_ed.boxStartMouseX;
    y0 = g_ed.boxStartMouseY;
    x1 = g_ed.boxEndMouseX;
    y1 = g_ed.boxEndMouseY;

    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    drawRectL(x0, y0, x1 - x0 + 1, y1 - y0 + 1, 9);
}



static void beginNewMap(void)
{
    memset(&g_edMap, 0, sizeof(g_edMap));
    g_edMap.startSector = 0;
    g_edMap.startX = 0.0f;
    g_edMap.startY = 0.0f;
    g_edMap.startAngle = 0.0f;

    g_ed.statusText[0] = '\0';
    g_ed.statusTimer = 0.0f;

    g_ed.confirmVisible = 0;
    g_ed.confirmAction = ED_CONFIRM_NONE;
    g_ed.confirmText[0] = '\0';

    g_ed.currentGridStep = ED_GRID_STEP;
    g_ed.tinyGridEnabled = 0;
    g_ed.ui_menu_visable = 0;

    memset(g_ed.selectedVerts, 0, sizeof(g_ed.selectedVerts));
    g_ed.selectedVertCount = 0;

    g_ed.boxSelecting = 0;
    g_ed.boxStartMouseX = 0;
    g_ed.boxStartMouseY = 0;
    g_ed.boxEndMouseX = 0;
    g_ed.boxEndMouseY = 0;

    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiStartWorldX = 0.0f;
    g_ed.dragMultiStartWorldY = 0.0f;
    g_ed.dragMultiVertCount = 0;

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
    g_ed.sectorFloorColor = 13;
    g_ed.sectorCeilColor = 3;

    g_ed.newWallUpperColor = 10;
    g_ed.newWallMidColor   = 10;
    g_ed.newWallLowerColor = 10;


    g_ed.copiedSectorFloor = 0.0f;
    g_ed.copiedSectorCeil = 0.0f;
    g_ed.hasCopiedSectorFloor = 0;
    g_ed.hasCopiedSectorCeil = 0;

    g_ed.copiedSectorFloorColor = 0;
    g_ed.copiedSectorCeilColor = 0;
    g_ed.hasCopiedSectorFloorColor = 0;
    g_ed.hasCopiedSectorCeilColor = 0;

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



static void openConfirmDialog(EdConfirmAction action, const char *text)
{
    g_ed.confirmVisible = 1;
    g_ed.confirmAction = action;
    rc3dGuiDirty();

    if (text) {
        snprintf(g_ed.confirmText, sizeof(g_ed.confirmText), "%s", text);
    } else {
        g_ed.confirmText[0] = '\0';
    }
}

static void closeConfirmDialog(void)
{
    g_ed.confirmVisible = 0;
    g_ed.confirmAction = ED_CONFIRM_NONE;
    g_ed.confirmText[0] = '\0';
    rc3dGuiDirty();
}

static void acceptConfirmDialog(void)
{
    EdConfirmAction action = g_ed.confirmAction;

    closeConfirmDialog();

    switch (action) {
        case ED_CONFIRM_NEW_MAP:
            beginNewMap();
            setEditorStatus("Started new blank map");
            break;

        default:
            break;
    }
}

static void drawConfirmPopup(void)
{
    int boxW = 400;
    int boxH = 96;
    int x = (EDIT_VIEW_PORT_WIDTH / 2) - (boxW / 2);
    int y = (EDIT_VIEW_PORT_HEIGHT / 2) - (boxH / 2);

    if (!g_ed.confirmVisible) return;

    /* dim background a bit */
    //drawRect(0, 0, EDIT_VIEW_PORT_WIDTH, EDIT_VIEW_PORT_HEIGHT, 4);

    drawRect(x, y, boxW, boxH, ED_UI_BG);
    drawRectL(x, y, boxW, boxH, ED_UI_BORDER);

    drawText(x + 12, y + 12, "Confirm", 31);
    drawText(x + 12, y + 34, g_ed.confirmText, ED_TEXT_COL);
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
    int count = 0;

    EditorMap newMap;

    if (!f) return 0;

    memset(&newMap, 0, sizeof(newMap));
    newMap.startSector = 0;
    newMap.startX = 0.0f;
    newMap.startY = 0.0f;
    newMap.startAngle = 0.0f;

    if (fscanf(f, "%63s", tag) != 1 || strcmp(tag, "MAPEDIT1") != 0) {
        fclose(f);
        return 0;
    }

    if (fscanf(f, "%63s", tag) != 1 || strcmp(tag, "START") != 0) {
        fclose(f);
        return 0;
    }

    if (fscanf(f, "%f %f %f %d",
               &newMap.startX,
               &newMap.startY,
               &newMap.startAngle,
               &newMap.startSector) != 4) {
        fclose(f);
        return 0;
    }

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "VERTS") != 0) {
        fclose(f);
        return 0;
    }
    if (count < 0 || count > ED_MAX_VERTS) {
        fclose(f);
        return 0;
    }

    newMap.vertCount = count;
    for (int i = 0; i < count; i++) {
        if (fscanf(f, "%f %f",
                   &newMap.verts[i].x,
                   &newMap.verts[i].y) != 2) {
            fclose(f);
            return 0;
        }
    }

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "WALLS") != 0) {
        fclose(f);
        return 0;
    }
    if (count < 0 || count > ED_MAX_WALLS) {
        fclose(f);
        return 0;
    }

    newMap.wallCount = count;
    for (int i = 0; i < count; i++) {
        unsigned uc, mc, lc, flags;

        if (fscanf(f, "%d %d %d %f %f %u %u %u %u",
                   &newMap.walls[i].v0,
                   &newMap.walls[i].v1,
                   &newMap.walls[i].neighbour,
                   &newMap.walls[i].openBottom,
                   &newMap.walls[i].openTop,
                   &uc, &mc, &lc, &flags) != 9) {
            fclose(f);
            return 0;
        }

        newMap.walls[i].upperColor = (uint8_t)uc;
        newMap.walls[i].midColor   = (uint8_t)mc;
        newMap.walls[i].lowerColor = (uint8_t)lc;
        newMap.walls[i].flags      = (uint8_t)flags;
    }

    if (fscanf(f, "%63s %d", tag, &count) != 2 || strcmp(tag, "SECTORS") != 0) {
        fclose(f);
        return 0;
    }
    if (count < 0 || count > ED_MAX_SECTORS) {
        fclose(f);
        return 0;
    }

    newMap.sectorCount = count;
    for (int i = 0; i < count; i++) {
        unsigned fc, cc;

        if (fscanf(f, "%d %d %d %f %f %u %u",
                   &newMap.sectors[i].wallStart,
                   &newMap.sectors[i].wallCount,
                   &newMap.sectors[i].boundaryCount,
                   &newMap.sectors[i].floorHeight,
                   &newMap.sectors[i].ceilHeight,
                   &fc, &cc) != 7) {
            fclose(f);
            return 0;
        }

        newMap.sectors[i].floorColor = (uint8_t)fc;
        newMap.sectors[i].ceilColor  = (uint8_t)cc;
    }

    fclose(f);

    /* only commit after full successful parse */
    g_edMap = newMap;

    /* editor state cleanup, but keep camera/zoom */
    clearDraft();
    clearMultiVertexSelection();

    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = -1;

    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    g_ed.selectionType = ED_SEL_NONE;

    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.draggingPan = 0;

    g_ed.splitPreviewValid = 0;
    g_ed.splitPreviewX = 0.0f;
    g_ed.splitPreviewY = 0.0f;

    syncAllPortals();

    return 1;
}

int exportBinaryMap(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    /* file header */
    {
        const char magic[8] = { 'R','C','3','D','M','A','P','1' };
        if (fwrite(magic, 1, sizeof(magic), f) != sizeof(magic)) {
            fclose(f);
            return 0;
        }
    }

    /* counts */
    {
        uint32_t vertCount   = (uint32_t)g_edMap.vertCount;
        uint32_t wallCount   = (uint32_t)g_edMap.wallCount;
        uint32_t sectorCount = (uint32_t)g_edMap.sectorCount;

        if (fwrite(&vertCount,   sizeof(vertCount),   1, f) != 1 ||
            fwrite(&wallCount,   sizeof(wallCount),   1, f) != 1 ||
            fwrite(&sectorCount, sizeof(sectorCount), 1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    /* start info */
    {
        int32_t startSector = (int32_t)g_edMap.startSector;
        float startX = g_edMap.startX;
        float startY = g_edMap.startY;
        float startAngle = g_edMap.startAngle;

        if (fwrite(&startSector, sizeof(startSector), 1, f) != 1 ||
            fwrite(&startX,      sizeof(startX),      1, f) != 1 ||
            fwrite(&startY,      sizeof(startY),      1, f) != 1 ||
            fwrite(&startAngle,  sizeof(startAngle),  1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    /* vertices */
    for (int i = 0; i < g_edMap.vertCount; i++) {
        float x = g_edMap.verts[i].x;
        float y = g_edMap.verts[i].y;

        if (fwrite(&x, sizeof(x), 1, f) != 1 ||
            fwrite(&y, sizeof(y), 1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    /* walls */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];

        int32_t v0        = (int32_t)w->v0;
        int32_t v1        = (int32_t)w->v1;
        int32_t neighbour = (int32_t)w->neighbour;
        float openBottom  = w->openBottom;
        float openTop     = w->openTop;
        uint8_t upper     = w->upperColor;
        uint8_t mid       = w->midColor;
        uint8_t lower     = w->lowerColor;
        uint8_t flags     = w->flags;

        if (fwrite(&v0,         sizeof(v0),         1, f) != 1 ||
            fwrite(&v1,         sizeof(v1),         1, f) != 1 ||
            fwrite(&neighbour,  sizeof(neighbour),  1, f) != 1 ||
            fwrite(&openBottom, sizeof(openBottom), 1, f) != 1 ||
            fwrite(&openTop,    sizeof(openTop),    1, f) != 1 ||
            fwrite(&upper,      sizeof(upper),      1, f) != 1 ||
            fwrite(&mid,        sizeof(mid),        1, f) != 1 ||
            fwrite(&lower,      sizeof(lower),      1, f) != 1 ||
            fwrite(&flags,      sizeof(flags),      1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    /* sectors */
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];

        int32_t wallStart     = (int32_t)s->wallStart;
        int32_t wallCount     = (int32_t)s->wallCount;
        int32_t boundaryCount = (int32_t)s->boundaryCount;
        float floorHeight     = s->floorHeight;
        float ceilHeight      = s->ceilHeight;
        uint8_t floorColor    = s->floorColor;
        uint8_t ceilColor     = s->ceilColor;

        if (fwrite(&wallStart,     sizeof(wallStart),     1, f) != 1 ||
            fwrite(&wallCount,     sizeof(wallCount),     1, f) != 1 ||
            fwrite(&boundaryCount, sizeof(boundaryCount), 1, f) != 1 ||
            fwrite(&floorHeight,   sizeof(floorHeight),   1, f) != 1 ||
            fwrite(&ceilHeight,    sizeof(ceilHeight),    1, f) != 1 ||
            fwrite(&floorColor,    sizeof(floorColor),    1, f) != 1 ||
            fwrite(&ceilColor,     sizeof(ceilColor),     1, f) != 1) {
            fclose(f);
            return 0;
        }
    }

    fclose(f);
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

    /* ---------------------------------------------------- */
    /* selected sector:
       if click hits boundary, use split mode
       otherwise allow normal inner drafting                */
    /* ---------------------------------------------------- */
    if (g_ed.selectedSector >= 0 && g_ed.draftCount < 2) {
        int v = findVertexExact(wx, wy);

        /* exact existing boundary vertex -> split mode point */
        if (v >= 0) {
            if (findBoundaryVertexIndexInSector(g_ed.selectedSector, v) >= 0) {
                if (g_ed.draftCount > 0 &&
                    g_ed.draftVertIndices[g_ed.draftCount - 1] == v) {
                    return;
                }

                g_ed.draftVertIndices[g_ed.draftCount++] = v;
                return;
            }
        }

        /* boundary wall hit -> split mode point by auto-splitting wall */
        {
            const int localWall = findBoundaryWallNearPointInSector(g_ed.selectedSector, wx, wy);
            if (localWall >= 0) {
                v = splitBoundaryWallAtPoint(g_ed.selectedSector, localWall, wx, wy);
                if (v >= 0) {
                    if (g_ed.draftCount > 0 &&
                        g_ed.draftVertIndices[g_ed.draftCount - 1] == v) {
                        return;
                    }

                    g_ed.draftVertIndices[g_ed.draftCount++] = v;
                    return;
                }
            }
        }

        /* no boundary hit -> fall through to NORMAL drafting inside selected sector */
    }

    /* ---------------------------------------------------- */
    /* normal drafting mode                                 */
    /* ---------------------------------------------------- */
    {
        const int v = findOrAddVertex(wx, wy);
        if (v < 0) return;

        if (g_ed.draftCount > 0 &&
            g_ed.draftVertIndices[g_ed.draftCount - 1] == v) {
            return;
        }

        g_ed.draftVertIndices[g_ed.draftCount++] = v;
    }
}


static int findBoundaryVertexIndexInSector(int sectorIndex, int vertIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return -1;

    const EdSector *sec = &g_edMap.sectors[sectorIndex];

    for (int i = 0; i < sec->boundaryCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        if (w->v0 == vertIndex) {
            return i;
        }
    }

    return -1;
}

static int splitSelectedSectorByDraftLine(void)
{
    EdWall newWallsA[ED_MAX_WALLS];
    EdWall newWallsB[ED_MAX_WALLS];

    if (g_ed.selectedSector < 0 || g_ed.selectedSector >= g_edMap.sectorCount) return 0;
    if (g_ed.draftCount != 2) return 0;
    if (g_edMap.sectorCount >= ED_MAX_SECTORS) return 0;

    EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];

    /* keep this simple: only split plain outer boundary sectors */
    if (sec->wallCount != sec->boundaryCount) {
        return 0;
    }

    const int splitV0 = g_ed.draftVertIndices[0];
    const int splitV1 = g_ed.draftVertIndices[1];

    if (splitV0 == splitV1) return 0;

    const int i0 = findBoundaryVertexIndexInSector(g_ed.selectedSector, splitV0);
    const int i1 = findBoundaryVertexIndexInSector(g_ed.selectedSector, splitV1);

    if (i0 < 0 || i1 < 0) return 0;

    /* same or already adjacent = pointless */
    if (i0 == i1) return 0;
    if (((i0 + 1) % sec->boundaryCount) == i1) return 0;
    if (((i1 + 1) % sec->boundaryCount) == i0) return 0;

    int countA = 0;
    int countB = 0;

    /* ---------------------------------------------------- */
    /* path A: from splitV0 to splitV1                      */
    /* ---------------------------------------------------- */
    {
        int idx = i0;
        for (;;) {
            EdWall w = g_edMap.walls[sec->wallStart + idx];

            /* copied walls should be rebuilt clean later */
            w.neighbour = -1;

            newWallsA[countA++] = w;

            if (w.v1 == splitV1) {
                break;
            }

            idx = (idx + 1) % sec->boundaryCount;
            if (idx == i0) return 0;
        }

        /* close A with divider wall */
        newWallsA[countA].v0 = splitV1;
        newWallsA[countA].v1 = splitV0;
        newWallsA[countA].neighbour = -1;
        newWallsA[countA].openBottom = 0.0f;
        newWallsA[countA].openTop = 0.0f;
        newWallsA[countA].upperColor = g_ed.newWallUpperColor;
        newWallsA[countA].midColor   = g_ed.newWallMidColor;
        newWallsA[countA].lowerColor = g_ed.newWallLowerColor;
        newWallsA[countA].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        countA++;
    }

    /* ---------------------------------------------------- */
    /* path B: from splitV1 to splitV0                      */
    /* ---------------------------------------------------- */
    {
        int idx = i1;
        for (;;) {
            EdWall w = g_edMap.walls[sec->wallStart + idx];

            /* copied walls should be rebuilt clean later */
            w.neighbour = -1;

            newWallsB[countB++] = w;

            if (w.v1 == splitV0) {
                break;
            }

            idx = (idx + 1) % sec->boundaryCount;
            if (idx == i1) return 0;
        }

        /* close B with divider wall */
        newWallsB[countB].v0 = splitV0;
        newWallsB[countB].v1 = splitV1;
        newWallsB[countB].neighbour = -1;
        newWallsB[countB].openBottom = 0.0f;
        newWallsB[countB].openTop = 0.0f;
        newWallsB[countB].upperColor = g_ed.newWallUpperColor;
        newWallsB[countB].midColor   = g_ed.newWallMidColor;
        newWallsB[countB].lowerColor = g_ed.newWallLowerColor;
        newWallsB[countB].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        countB++;
    }

    if (countA < 3 || countB < 3) return 0;
    if ((g_edMap.wallCount - sec->wallCount + countA + countB) > ED_MAX_WALLS) return 0;

    /* validate continuity */
    for (int i = 0; i < countA; i++) {
        const int next = (i + 1) % countA;
        if (newWallsA[i].v1 != newWallsA[next].v0) return 0;
    }

    for (int i = 0; i < countB; i++) {
        const int next = (i + 1) % countB;
        if (newWallsB[i].v1 != newWallsB[next].v0) return 0;
    }

    const float floorH = sec->floorHeight;
    const float ceilH  = sec->ceilHeight;
    const uint8_t floorC = sec->floorColor;
    const uint8_t ceilC  = sec->ceilColor;

    const int oldSelectedSector = g_ed.selectedSector;

    /* delete old sector boundary/walls */
    {
        const int start = sec->wallStart;
        const int end   = start + sec->wallCount;
        for (int i = start; i < end; i++) {
            g_edMap.walls[i].v0 = -1;
            g_edMap.walls[i].v1 = -1;
            g_edMap.walls[i].neighbour = -1;
        }
    }

    rebuildSectorWallLayout();

    int startA, startB;
    int newSector;

    /* sector A reuses selected sector */
    startA = g_edMap.wallCount;
    for (int i = 0; i < countA; i++) {
        g_edMap.walls[g_edMap.wallCount++] = newWallsA[i];
    }

    g_edMap.sectors[oldSelectedSector].wallStart = startA;
    g_edMap.sectors[oldSelectedSector].wallCount = countA;
    g_edMap.sectors[oldSelectedSector].boundaryCount = countA;
    g_edMap.sectors[oldSelectedSector].floorHeight = floorH;
    g_edMap.sectors[oldSelectedSector].ceilHeight = ceilH;
    g_edMap.sectors[oldSelectedSector].floorColor = floorC;
    g_edMap.sectors[oldSelectedSector].ceilColor = ceilC;

    /* sector B is new */
    newSector = g_edMap.sectorCount;
    startB = g_edMap.wallCount;
    for (int i = 0; i < countB; i++) {
        g_edMap.walls[g_edMap.wallCount++] = newWallsB[i];
    }

    g_edMap.sectors[newSector].wallStart = startB;
    g_edMap.sectors[newSector].wallCount = countB;
    g_edMap.sectors[newSector].boundaryCount = countB;
    g_edMap.sectors[newSector].floorHeight = floorH;
    g_edMap.sectors[newSector].ceilHeight = ceilH;
    g_edMap.sectors[newSector].floorColor = floorC;
    g_edMap.sectors[newSector].ceilColor = ceilC;
    g_edMap.sectorCount++;

    /* ---------------------------------------------------- */
    /* FORCE the divider pair to become a portal pair       */
    /* ---------------------------------------------------- */
    {
        const int splitWallA = startA + (countA - 1);
        const int splitWallB = startB + (countB - 1);

        setPortalPair(splitWallA, oldSelectedSector, splitWallB, newSector);
    }

    /* rebuild all other shared walls too */
    syncAllPortals();

    clearDraft();

    g_ed.selectionType = ED_SEL_SECTOR;
    g_ed.selectedSector = oldSelectedSector;
    g_ed.hoverWall = -1;
    g_ed.selectedWall = -1;
    g_ed.hoverVert = -1;
    g_ed.selectedVert = -1;

    return 1;
}



static void finalizeDraftSector(void)
{
    if (g_ed.draftCount < 3) return;
    if (g_edMap.sectorCount >= ED_MAX_SECTORS) return;
    if ((g_edMap.wallCount + g_ed.draftCount) > ED_MAX_WALLS) return;

    {
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
            w->upperColor = g_ed.newWallUpperColor;
            w->midColor   = g_ed.newWallMidColor;
            w->lowerColor = g_ed.newWallLowerColor;
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
    }

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

    {
        const EdVec2 *outerA = &g_edMap.verts[outerV0];
        const EdVec2 *outerB = &g_edMap.verts[outerV1];

        const int draftV0 = g_ed.draftVertIndices[draftEdgeIndex];
        const int draftV1 = g_ed.draftVertIndices[(draftEdgeIndex + 1) % g_ed.draftCount];

        const EdVec2 *draftA = &g_edMap.verts[draftV0];
        const EdVec2 *draftB = &g_edMap.verts[draftV1];

        const float odx = outerB->x - outerA->x;
        const float ody = outerB->y - outerA->y;
        const float lenSq = (odx * odx) + (ody * ody);

        int splitV0, splitV1;
        int needLeadPiece, needTrailPiece;
        int pieceCount;
        int sharedPieceLocal;
        int outerSharedWall;
        int newSectorIndex;
        int newWallStart;
        int newSharedWall;
        int draftSameAsOuter;
        EdWall outerPieces[3];
        EdWall templateWall;

        if (lenSq <= ED_EPSILON) return 0;

        {
            const float ta = (((draftA->x - outerA->x) * odx) + ((draftA->y - outerA->y) * ody)) / lenSq;
            const float tb = (((draftB->x - outerA->x) * odx) + ((draftB->y - outerA->y) * ody)) / lenSq;

            if (ta <= tb) {
                splitV0 = findOrAddVertex(draftA->x, draftA->y);
                splitV1 = findOrAddVertex(draftB->x, draftB->y);
            } else {
                splitV0 = findOrAddVertex(draftB->x, draftB->y);
                splitV1 = findOrAddVertex(draftA->x, draftA->y);
            }
        }

        if (splitV0 < 0 || splitV1 < 0) return 0;
        if (splitV0 == splitV1) return 0;

        templateWall = g_edMap.walls[g_edMap.sectors[outerSectorIndex].wallStart + outerLocalWall];

        needLeadPiece =
            !pointsSameEps(g_edMap.verts[splitV0].x, g_edMap.verts[splitV0].y,
                           outerA->x, outerA->y, 0.01f);

        needTrailPiece =
            !pointsSameEps(g_edMap.verts[splitV1].x, g_edMap.verts[splitV1].y,
                           outerB->x, outerB->y, 0.01f);

        pieceCount = 0;

        if (needLeadPiece) {
            outerPieces[pieceCount] = templateWall;
            outerPieces[pieceCount].v0 = outerV0;
            outerPieces[pieceCount].v1 = splitV0;
            outerPieces[pieceCount].neighbour = -1;
            outerPieces[pieceCount].openBottom = 0.0f;
            outerPieces[pieceCount].openTop = 0.0f;
            outerPieces[pieceCount].upperColor = 0;
            outerPieces[pieceCount].midColor = (templateWall.midColor == 0) ? g_ed.newWallMidColor : templateWall.midColor;
            outerPieces[pieceCount].lowerColor = 0;
            outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            pieceCount++;
        }

        sharedPieceLocal = pieceCount;
        outerPieces[pieceCount] = templateWall;
        outerPieces[pieceCount].v0 = splitV0;
        outerPieces[pieceCount].v1 = splitV1;
        outerPieces[pieceCount].neighbour = -1;
        outerPieces[pieceCount].openBottom = 0.0f;
        outerPieces[pieceCount].openTop = 0.0f;
        outerPieces[pieceCount].upperColor = 0;
        outerPieces[pieceCount].midColor = (templateWall.midColor == 0) ? g_ed.newWallMidColor : templateWall.midColor;
        outerPieces[pieceCount].lowerColor = 0;
        outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        pieceCount++;

        if (needTrailPiece) {
            outerPieces[pieceCount] = templateWall;
            outerPieces[pieceCount].v0 = splitV1;
            outerPieces[pieceCount].v1 = outerV1;
            outerPieces[pieceCount].neighbour = -1;
            outerPieces[pieceCount].openBottom = 0.0f;
            outerPieces[pieceCount].openTop = 0.0f;
            outerPieces[pieceCount].upperColor = 0;
            outerPieces[pieceCount].midColor = (templateWall.midColor == 0) ? g_ed.newWallMidColor : templateWall.midColor;
            outerPieces[pieceCount].lowerColor = 0;
            outerPieces[pieceCount].flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            pieceCount++;
        }

        if (pieceCount <= 0) return 0;

        if ((g_edMap.wallCount - 1 + pieceCount + g_ed.draftCount) > ED_MAX_WALLS) {
            return 0;
        }

        removeWallFromSector(outerSectorIndex, outerLocalWall);
        insertWallsIntoSector(outerSectorIndex, outerLocalWall, outerPieces, pieceCount);

        outerSharedWall = g_edMap.sectors[outerSectorIndex].wallStart + outerLocalWall + sharedPieceLocal;

        newSectorIndex = g_edMap.sectorCount;
        newWallStart = g_edMap.wallCount;

        {
            EdWall *w = &g_edMap.walls[g_edMap.wallCount++];

            w->v0 = splitV1;
            w->v1 = splitV0;
            w->neighbour = -1;
            w->openBottom = 0.0f;
            w->openTop = 0.0f;
            w->upperColor = g_ed.newWallUpperColor;
            w->midColor   = g_ed.newWallMidColor;
            w->lowerColor = g_ed.newWallLowerColor;
            w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
        }

        draftSameAsOuter = (draftV0 == splitV0 && draftV1 == splitV1);

        if (draftSameAsOuter) {
            for (int step = 0; step < (g_ed.draftCount - 1); step++) {
                int fromIdx = (draftEdgeIndex - step + g_ed.draftCount) % g_ed.draftCount;
                int toIdx   = (draftEdgeIndex - step - 1 + g_ed.draftCount) % g_ed.draftCount;

                int fromV = g_ed.draftVertIndices[fromIdx];
                int toV   = g_ed.draftVertIndices[toIdx];

                EdWall *w = &g_edMap.walls[g_edMap.wallCount++];

                if (fromIdx == draftEdgeIndex) {
                    fromV = splitV0;
                }
                if (toIdx == ((draftEdgeIndex + 1) % g_ed.draftCount)) {
                    toV = splitV1;
                }

                w->v0 = fromV;
                w->v1 = toV;
                w->neighbour = -1;
                w->openBottom = 0.0f;
                w->openTop = 0.0f;
                w->upperColor = g_ed.newWallUpperColor;
                w->midColor   = g_ed.newWallMidColor;
                w->lowerColor = g_ed.newWallLowerColor;
                w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            }
        } else {
            for (int step = 1; step < g_ed.draftCount; step++) {
                int fromIdx = (draftEdgeIndex + step) % g_ed.draftCount;
                int toIdx   = (draftEdgeIndex + step + 1) % g_ed.draftCount;

                int fromV = g_ed.draftVertIndices[fromIdx];
                int toV   = g_ed.draftVertIndices[toIdx];

                EdWall *w = &g_edMap.walls[g_edMap.wallCount++];

                if (fromIdx == ((draftEdgeIndex + 1) % g_ed.draftCount)) {
                    fromV = splitV0;
                }
                if (toIdx == draftEdgeIndex) {
                    toV = splitV1;
                }

                w->v0 = fromV;
                w->v1 = toV;
                w->neighbour = -1;
                w->openBottom = 0.0f;
                w->openTop = 0.0f;
                w->upperColor = g_ed.newWallUpperColor;
                w->midColor   = g_ed.newWallMidColor;
                w->lowerColor = g_ed.newWallLowerColor;
                w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            }
        }

        g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
        g_edMap.sectors[newSectorIndex].wallCount = g_ed.draftCount;
        g_edMap.sectors[newSectorIndex].boundaryCount = g_ed.draftCount;
        g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
        g_edMap.sectors[newSectorIndex].ceilHeight = g_ed.sectorCeil;
        g_edMap.sectors[newSectorIndex].floorColor = g_ed.sectorFloorColor;
        g_edMap.sectors[newSectorIndex].ceilColor = g_ed.sectorCeilColor;
        g_edMap.sectorCount++;

        newSharedWall = newWallStart;

        setPortalPair(outerSharedWall, outerSectorIndex, newSharedWall, newSectorIndex);

        syncAllPortals();
        clearDraft();
        return 1;
    }
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
        w->upperColor = g_ed.newWallUpperColor;
        w->midColor   = g_ed.newWallMidColor;
        w->lowerColor = g_ed.newWallLowerColor;
        w->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    }

    g_edMap.wallCount += g_ed.draftCount;
    sec->wallCount += g_ed.draftCount;

    syncAllPortals();
    clearDraft();
}


static void cleanMapCompact(void)
{
    int sectorRemap[ED_MAX_SECTORS];
    EdSector newSectors[ED_MAX_SECTORS];
    int newSectorCount = 0;

    clearMultiVertexSelection();

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

static void cleanMapCompactWithReport(int *removedVerts, int *removedWalls, int *removedSectors)
{
    int oldVerts   = g_edMap.vertCount;
    int oldWalls   = g_edMap.wallCount;
    int oldSectors = g_edMap.sectorCount;

    cleanMapCompact();

    if (removedVerts)   *removedVerts   = oldVerts   - g_edMap.vertCount;
    if (removedWalls)   *removedWalls   = oldWalls   - g_edMap.wallCount;
    if (removedSectors) *removedSectors = oldSectors - g_edMap.sectorCount;
}


static void compactOrphanVertices(void)
{
    int used[ED_MAX_VERTS];
    int remap[ED_MAX_VERTS];
    int newCount = 0;

    clearMultiVertexSelection();

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

static int mergeCloseVertices(float epsilon)
{
    int mergedAny = 0;

    if (epsilon <= 0.0f) {
        return 0;
    }

    for (int i = 0; i < g_edMap.vertCount; i++) {
        int j = i + 1;

        while (j < g_edMap.vertCount) {
            const float dx = g_edMap.verts[i].x - g_edMap.verts[j].x;
            const float dy = g_edMap.verts[i].y - g_edMap.verts[j].y;
            const float d2 = (dx * dx) + (dy * dy);

            if (d2 <= (epsilon * epsilon)) {
                mergeVertexInto(j, i);
                mergedAny = 1;

                /* vert list shrank, so keep checking same j */
                continue;
            }

            j++;
        }
    }

    return mergedAny;
}

static void repairMapTopology(void)
{
    float eps = g_ed.currentGridStep * 0.05f;

    if (eps < 0.001f) eps = 0.001f;
    if (eps > 0.05f)  eps = 0.05f;

    clearMultiVertexSelection();

    mergeCloseVertices(eps);

    rebuildSectorWallLayout();
    compactOrphanVertices();
    syncAllPortals();

    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = -1;

    if (g_ed.selectedVert >= g_edMap.vertCount) g_ed.selectedVert = -1;
    if (g_ed.selectedWall >= g_edMap.wallCount) g_ed.selectedWall = -1;
    if (g_ed.selectedSector >= g_edMap.sectorCount) g_ed.selectedSector = -1;

    if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert < 0) g_ed.selectionType = ED_SEL_NONE;
    if (g_ed.selectionType == ED_SEL_WALL   && g_ed.selectedWall < 0) g_ed.selectionType = ED_SEL_NONE;
    if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector < 0) g_ed.selectionType = ED_SEL_NONE;

    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.splitPreviewValid = 0;
}




static void updateHover(float worldX, float worldY, int mouseX, int mouseY)
{
    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = findSectorForPoint(worldX, worldY);

    /* -------------------------------------------------- */
    /* vertex hover                                       */
    /* -------------------------------------------------- */
    {
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
    }

    /* -------------------------------------------------- */
    /* wall hover                                         */
    /* for shared/reversed portal walls, prefer:          */
    /* 1) wall owned by selected sector                   */
    /* 2) wall owned by hovered sector                    */
    /* 3) otherwise nearest wall                          */
    /* -------------------------------------------------- */
    {
        const float worldPick = (float)ED_PICK_DIST_PX / g_ed.zoom;
        const float worldPickSq = worldPick * worldPick;

        int bestWall = -1;
        float bestWallDistSq = 99999999.0f;
        int bestPriority = -999999;

        for (int i = 0; i < g_edMap.wallCount; i++) {
            const EdWall *w = &g_edMap.walls[i];
            const EdVec2 *a = &g_edMap.verts[w->v0];
            const EdVec2 *b = &g_edMap.verts[w->v1];
            const float d2 = distPointSegSq(worldX, worldY, a->x, a->y, b->x, b->y);

            int owner;
            int priority = 0;

            if (d2 > worldPickSq) {
                continue;
            }

            owner = findSectorOwningWall(i);

            if (g_ed.selectedSector >= 0 && owner == g_ed.selectedSector) {
                priority = 3;
            }
            else if (g_ed.hoverSector >= 0 && owner == g_ed.hoverSector) {
                priority = 2;
            }
            else if (w->neighbour >= 0) {
                /* shared boundary / portal line gets slight preference */
                priority = 1;
            }

            if (priority > bestPriority) {
                bestPriority = priority;
                bestWallDistSq = d2;
                bestWall = i;
            }
            else if (priority == bestPriority && d2 < bestWallDistSq) {
                bestWallDistSq = d2;
                bestWall = i;
            }
        }

        g_ed.hoverWall = bestWall;
    }
}





static void drawGrid(void)
{
    const float leftW   = g_ed.camX - (EDIT_VIEW_PORT_WIDTH * 0.5f) / g_ed.zoom;
    const float rightW  = g_ed.camX + (EDIT_VIEW_PORT_WIDTH * 0.5f) / g_ed.zoom;
    const float topW    = g_ed.camY - (EDIT_VIEW_PORT_HEIGHT * 0.5f) / g_ed.zoom;
    const float bottomW = g_ed.camY + (EDIT_VIEW_PORT_HEIGHT * 0.5f) / g_ed.zoom;

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

        for (int gy = yCount0; gy < yCount1; gy++) {
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

        for (int gy = yCount0; gy < yCount1; gy++) {
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

void drawHoverPanel(void)
{
    char buf[256];
    int vertex_id = -1;
    int wall_id   = -1;
    int sector_id = -1;

    const int panelX = 0;
    const int panelY = EDIT_VIEW_PORT_HEIGHT - 1;
    const int panelW = SCREEN_W;
    const int panelH = ED_HOVER_FOCUS_INFO_PANEL + 1;

    drawRect(panelX, panelY, panelW, panelH, ED_UI_BG);
    drawRectL(panelX, panelY, panelW, panelH, ED_UI_BORDER);


//#define ED_MAX_VERTS            4096
//#define ED_MAX_WALLS            4096
//#define ED_MAX_SECTORS          512
//#define ED_MAX_DRAFT_POINTS     128

    snprintf(buf, sizeof(buf), "[Verts=%d / %d], [Walls=%d / %d], Sectors=[%d / %d] ", 
        g_edMap.vertCount,   ED_MAX_VERTS,        
        g_edMap.wallCount,   ED_MAX_WALLS,
        g_edMap.sectorCount, ED_MAX_SECTORS);
    drawText(SCREEN_W - (strlen(buf) * 8), EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);

    if(g_ed.draftCount){
        snprintf(buf, sizeof(buf), "Draft: %d   SignedArea: %.2f   Grid: %.1f", g_ed.draftCount, draftSignedArea(), g_ed.currentGridStep);
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);
        return;
    }

    if (g_ed.selectedVertCount > 0 && g_ed.selectionType == ED_SEL_NONE) {
        snprintf(buf, sizeof(buf), "Selected vertices: %d", g_ed.selectedVertCount);
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);
        return;
    }

    /* selected takes priority over hover */
    if (g_ed.hoverVert    >= 0) vertex_id = g_ed.hoverVert;
    if (g_ed.selectedVert >= 0) vertex_id = g_ed.selectedVert;

    if (g_ed.hoverWall    >= 0) wall_id = g_ed.hoverWall;
    if (g_ed.selectedWall >= 0) wall_id = g_ed.selectedWall;

    if (g_ed.hoverSector    >= 0) sector_id = g_ed.hoverSector;
    if (g_ed.selectedSector >= 0) sector_id = g_ed.selectedSector;

    /* priority: vertex, then wall, then sector */
    if (vertex_id >= 0) {
        const EdVec2 *v = &g_edMap.verts[vertex_id];

        snprintf(buf, sizeof(buf), "Vertex %d  X %.2f  Y %.2f",
                 vertex_id, v->x, v->y);
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);
        return;
    }

    if (wall_id >= 0) {
        const EdWall *w = &g_edMap.walls[wall_id];

        int owner = findSectorOwningWall(wall_id);

        snprintf(buf, sizeof(buf),
                "Wall: %d  Vertices: %d/%d  Sectors: %d/%d  Bottom: %.2f  Top: %.2f",
                wall_id, w->v0, w->v1, owner, w->neighbour,
                w->openBottom, w->openTop);
                
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);

        return;
    }

    if (sector_id >= 0) {
        const EdSector *s = &g_edMap.sectors[sector_id];

        snprintf(buf, sizeof(buf),
                 "Sector %d  Floor %.2f  Ceil %.2f  Walls %d  Boundary %d",
                 sector_id, s->floorHeight, s->ceilHeight,
                 s->wallCount, s->boundaryCount);
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);
        return;
    }

    drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, "No hover / selection", ED_TEXT_COL);
}


static void drawInspectorPanel(void)
{
    char buf[256];
    const int px = EDIT_VIEW_PORT_WIDTH;
    int py = 0;
    
    const int pw = ED_INSPECTOR_PANEL;
    const int ph = EDIT_VIEW_PORT_HEIGHT;


    drawRect(px, py, pw, ph, ED_UI_BG);
    drawLine(px, py, px + pw - 1, py, ED_UI_BORDER);
    drawLine(px, py + ph - 1, px + pw - 1, py + ph - 1, ED_UI_BORDER);
    drawLine(px, py, px, py + ph - 1, ED_UI_BORDER);
    drawLine(px + pw - 1, py, px + pw - 1, py + ph - 1, ED_UI_BORDER);

    drawText(px + 8, py + 8, "INSPECTOR: ", ED_TEXT_COL);

    snprintf(buf, sizeof(buf),
             "Selection mode: %s",
             (g_ed.selectionType == ED_SEL_VERTEX) ? "VERTEX" :
             (g_ed.selectionType == ED_SEL_WALL)   ? "WALL" :
             (g_ed.selectionType == ED_SEL_SECTOR) ? "SECTOR" : "NONE");
    drawText(px + 110, py + 8, buf, ED_TEXT_COL);

    if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert >= 0) {
        const EdVec2 *v = &g_edMap.verts[g_ed.selectedVert];
        snprintf(buf, sizeof(buf), "Vertex %d", g_ed.selectedVert);
        drawText(px + 8, py + 40, buf, 31);

        snprintf(buf, sizeof(buf), "x: %.2f", v->x);
        drawText(px + 8, py + 60, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "y: %.2f", v->y);
        drawText(px + 8, py + 80, buf, ED_TEXT_COL);

        drawText(px + 8, py + 120, "LMB drag vertex", ED_TEXT_COL);
        drawText(px + 8, py + 140, "Drop onto another vertex to merge (hold shift)", ED_TEXT_COL);
    }
    else if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
        const EdWall *w = &g_edMap.walls[g_ed.selectedWall];
        snprintf(buf, sizeof(buf), "Wall %d", g_ed.selectedWall);
        drawText(px + 8, py + 40, buf, 31);

        snprintf(buf, sizeof(buf), "v0: %d   v1: %d   neighbour: %d", w->v0, w->v1, w->neighbour);
        drawText(px + 8, py + 60, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "openTop: %.2f", w->openTop);
        drawText(px + 8, py + 100, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "openBottom: %.2f", w->openBottom);
        drawText(px + 8, py + 130, buf, ED_TEXT_COL);



        snprintf(buf, sizeof(buf), "settings flags: %u", (unsigned)w->flags);
        drawText(px + 8, py + 170, buf, ED_TEXT_COL);
        snprintf(buf, sizeof(buf), "Wall configurations:");
        drawText(px + 8, py + 190, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "upper: %u   mid: %u   lower: %u", (unsigned)w->upperColor, (unsigned)w->midColor, (unsigned)w->lowerColor);
        drawText(px + 8, py + 320, buf, ED_TEXT_COL);
    }
    else if (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0) {
        snprintf(buf, sizeof(buf), "Multi-vertex selection");
        drawText(px + 8, py + 40, buf, 31);

        snprintf(buf, sizeof(buf), "selected verts: %d", g_ed.selectedVertCount);
        drawText(px + 8, py + 70, buf, ED_TEXT_COL);

        drawText(px + 8, py + 110, "LMB drag selected vertices", ED_TEXT_COL);
        drawText(px + 8, py + 130, "Click empty area and drag to make new selection box", ED_TEXT_COL);
    }


    /// SECTOR EDITOR /// INSPECTOR MODE
    else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        snprintf(buf, sizeof(buf), "Sector %d", g_ed.selectedSector);
        drawText(px + 8, py + 40, buf, 31);

        snprintf(buf, sizeof(buf), "wallStart: %d", sec->wallStart);
        drawText(px + 8, py + 60, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "wallCount: %d   boundaryCount: %d",
                 sec->wallCount, sec->boundaryCount);
        drawText(px + 8, py + 80, buf, ED_TEXT_COL);


        snprintf(buf, sizeof(buf), "floor-level: %.2f", sec->floorHeight);
        drawText(px + 8, py + 120, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), " ceil-level: %.2f", sec->ceilHeight);
        drawText(px + 8, py + 150, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "floorCol: %u   ceilCol: %u",
                 (unsigned)sec->floorColor, (unsigned)sec->ceilColor);
        drawText(px + 8, py + 190, buf, ED_TEXT_COL);

        drawText(px + 8, py + 210, "SHIFT+F/G floor", ED_TEXT_COL);
        drawText(px + 8, py + 230, "SHIFT+C/V ceil", ED_TEXT_COL);
        drawText(px + 8, py + 250, "SHIFT+J/K floor color", ED_TEXT_COL);
        drawText(px + 8, py + 270, "SHIFT+N/M ceil color", ED_TEXT_COL);


    }
    else {
        drawText(px + 8, py + 42, "Nothing selected", ED_TEXT_COL);

        drawText(px + 8, py + 80, "NEW DRAFT DEFAULTS", 31);

        snprintf(buf, sizeof(buf), "sector floor: %.2f", g_ed.sectorFloor);
        drawText(px + 8, py + 104, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "sector ceil : %.2f", g_ed.sectorCeil);
        drawText(px + 8, py + 124, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "floor col   : %u", (unsigned)g_ed.sectorFloorColor);
        drawText(px + 8, py + 144, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "ceil col    : %u", (unsigned)g_ed.sectorCeilColor);
        drawText(px + 8, py + 164, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "wall upper  : %u", (unsigned)g_ed.newWallUpperColor);
        drawText(px + 8, py + 194, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "wall mid    : %u", (unsigned)g_ed.newWallMidColor);
        drawText(px + 8, py + 214, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "wall lower  : %u", (unsigned)g_ed.newWallLowerColor);
        drawText(px + 8, py + 234, buf, ED_TEXT_COL);

        drawText(px + 8, py + 264, "F/G sector floor   C/V sector ceil", ED_TEXT_COL);
        drawText(px + 8, py + 284, "J/K floor col      N/M ceil col", ED_TEXT_COL);
        drawText(px + 8, py + 304, "A/S wall upper  D/F wall mid  H/J wall lower", ED_TEXT_COL);
    }
}


static void drawTopMenuBar(void)
{
    drawRect(ED_TOPBAR_X, ED_TOPBAR_Y, ED_TOPBAR_W, ED_TOPBAR_H, ED_UI_BG);
    drawLine(ED_TOPBAR_X, ED_TOPBAR_Y, ED_TOPBAR_X + ED_TOPBAR_W - 1, ED_TOPBAR_Y, ED_UI_BORDER);
    drawLine(ED_TOPBAR_X, ED_TOPBAR_Y + ED_TOPBAR_H - 1,
             ED_TOPBAR_X + ED_TOPBAR_W - 1, ED_TOPBAR_Y + ED_TOPBAR_H - 1, ED_UI_BORDER);
    drawLine(ED_TOPBAR_X, ED_TOPBAR_Y, ED_TOPBAR_X, ED_TOPBAR_Y + ED_TOPBAR_H - 1, ED_UI_BORDER);
    drawLine(ED_TOPBAR_X + ED_TOPBAR_W - 1, ED_TOPBAR_Y,
             ED_TOPBAR_X + ED_TOPBAR_W - 1, ED_TOPBAR_Y + ED_TOPBAR_H - 1, ED_UI_BORDER);

    //drawText(ED_TOPBAR_X + 8, ED_TOPBAR_Y + ED_UI_PAD, "F1-More | F10-quit |", ED_UI_TEXT_INFOTYPE);
}

static void drawExpandedEditorPanel(void)
{
    int y = ED_PANEL_Y + ED_UI_PAD;
    const int x = ED_PANEL_X + ED_UI_PAD;

    drawRect(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_W, ED_PANEL_H, ED_UI_BG);
    drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y, ED_UI_BORDER);
    drawLine(ED_PANEL_X, ED_PANEL_Y + ED_PANEL_H - 1,
             ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + ED_PANEL_H - 1, ED_UI_BORDER);
    drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X, ED_PANEL_Y + ED_PANEL_H - 1, ED_UI_BORDER);
    drawLine(ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y,
             ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + ED_PANEL_H - 1, ED_UI_BORDER);

    drawText(x, y, "RC3D EDITOR ::: F12 to launch the test map", 31);
    y += ED_ROW_STEP;

    drawText(x, y, "LMB select/drag, RMB add draft point, MMB pan, Wheel zoom", ED_TEXT_COL);
    y += ED_ROW_STEP;

    drawText(x, y, "[ENTER] finish draft, [ESC] clear draft, [DEL] delete hovered", ED_TEXT_COL);
    y += ED_ROW_STEP;
    drawText(x, y, "[F8] auto-build sectors from closed inner wall loops", ED_TEXT_COL);
    y += ED_ROW_STEP;
    drawText(x, y, "CTRL+Z undo, CTRL+Y redo, [TAB] grid, [F6] clean map. [F7] Repair map, [SHIFT+DRAG] drop = merge", ED_TEXT_COL);
    y += 6;//ED_ROW_STEP;
    drawText(x, y, "________________________________________________________________________________________________", ED_TEXT_COL);
    y += ED_ROW_STEP;
    if(g_ed.selectionType == ED_SEL_WALL){
        drawText(x, y, "--- WALL HELP", 2);
        y += ED_ROW_STEP;
        drawText(x, y, "[R]/[T] bottom level,  [Y]/[U] top level", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[A]/[S] upper texture, [D]/[F] middle texture, [H]/[J] lower texture", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[SPACE] split wall (insert vertex)", ED_TEXT_COL);
    }
    if(g_ed.selectionType == ED_SEL_SECTOR){
        drawText(x, y, "--- SECTOR HELP:", 2);
        y += ED_ROW_STEP;
        drawText(x, y, "[NUM_1] COPY  floor level, [NUM_2] COPY  ceiling level", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[NUM_4] PASTE floor level, [NUM_5] PASTE ceiling level", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[SHIFT+NUM_1] COPY  floor texture_id, [SHIFT+NUM_2] COPY  ceiling texture_id", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[SHIFT+NUM_4] PASTE floor texture_id, [SHIFT+NUM_5] PASTE ceiling texture_id", ED_TEXT_COL);
        y += ED_ROW_STEP;
    }

    //snprintf(buf, sizeof(buf), "Start: %.2f %.2f   Angle: %.2f", g_edMap.startX, g_edMap.startY, g_edMap.startAngle);
    //drawText(x, y, buf, ED_TEXT_COL);
    //y += ED_ROW_STEP;

}

static void drawWallNormal(int wallIndex, uint8_t colour)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    const EdWall *w = &g_edMap.walls[wallIndex];
    const EdVec2 *a = &g_edMap.verts[w->v0];
    const EdVec2 *b = &g_edMap.verts[w->v1];

    const float dx = b->x - a->x;
    const float dy = b->y - a->y;
    const float len = sqrtf((dx * dx) + (dy * dy));
    if (len <= 0.0001f) return;

    /* midpoint of wall */
    const float mx = (a->x + b->x) * 0.5f;
    const float my = (a->y + b->y) * 0.5f;

    /* right-hand normal from v0->v1 */
    const float nx =  dy / len;
    const float ny = -dx / len;

    /* draw length in world units */
    const float normalLen = 0.15f;

    const float ex = mx + (nx * normalLen);
    const float ey = my + (ny * normalLen);

    int sx0, sy0, sx1, sy1;
    worldToScreen(mx, my, &sx0, &sy0);
    worldToScreen(ex, ey, &sx1, &sy1);

    drawLine(sx0, sy0, sx1, sy1, colour);

    /* tiny tip marker so it reads better */
    //drawRect(sx1 - 1, sy1 - 1, 3, 3, colour);
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
        if(!(w->flags & RC3D_WALL_PORTAL))
            drawWallNormal(i, c);
    }

    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;
        uint8_t c = ED_VERT_COL;

        if (g_ed.selectedVerts[i]) c = 27;
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

    drawBoxSelectRect();

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

#include "rcgui.h"

RCGUI_Context g_ui;

void rc3dEditInit(void)
{
    const int btnx_stdwidth = 78;
    int btnx_off = 12;
    beginNewMap();

    rcguiInit(&g_ui);
    initRememberedDialogDirs();

    /* top bar */
    rcguiCreateButton(&g_ui, GUI_BTN_HELP,     btnx_off, 10, 72, ED_BTN_H, "F1: Help");      btnx_off += 72+4;
    rcguiCreateButton(&g_ui, GUI_BTN_QUIT,     btnx_off, 10, 72, ED_BTN_H, "F10:Quit");      btnx_off += 72+4;
    rcguiCreateButton(&g_ui, GUI_BTN_UNDO,     btnx_off, 10, 50, ED_BTN_H, "Undo");          btnx_off += 50+4;
    rcguiCreateButton(&g_ui, GUI_BTN_REDO,     btnx_off, 10, 50, ED_BTN_H, "Redo");          btnx_off += 50+4;
    rcguiCreateButton(&g_ui, GUI_BTN_NEWMAP,   btnx_off, 10, 70, ED_BTN_H, "New map");       btnx_off += 70+4;
    rcguiCreateButton(&g_ui, GUI_BTN_LOAD,     btnx_off, 10, 72, ED_BTN_H, "F2: Load");      btnx_off += 72+4;
    rcguiCreateButton(&g_ui, GUI_BTN_SAVE,     btnx_off, 10, 72, ED_BTN_H, "F3: Save");      btnx_off += 72+4;
    rcguiCreateButton(&g_ui, GUI_BTN_EXPORT,   btnx_off, 10, 90, ED_BTN_H, "F5: Export");    btnx_off += 90+4;
    rcguiCreateButton(&g_ui, GUI_BTN_GRID,     btnx_off, 10, 80, ED_BTN_H, "TAB: Grid 1.0"); btnx_off += 80+4;
    rcguiCreateButton(&g_ui, GUI_BTN_FINISH,   btnx_off, 10, 62, ED_BTN_H, "Finish");        btnx_off += 62+4;
    rcguiCreateButton(&g_ui, GUI_BTN_CLRDRAFT, btnx_off, 10, 72, ED_BTN_H, "ClrDraft");      btnx_off += 72+4;
    rcguiCreateButton(&g_ui, GUI_BTN_CLEANMAP, btnx_off, 10, 72, ED_BTN_H, "CleanMap");      btnx_off += 72+4;

    #define controloff 33
    #define controloffw 6 + (SCREEN_W - ED_INSPECTOR_PANEL)
    /* expanded panel button rows aligned to 16px font + 6px spacing */
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_SOLID,         controloffw + (84 * 0), 178 + controloff, 80, ED_BTN_H, "1:Solid");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_PORTAL,        controloffw + (84 * 1), 178 + controloff, 80, ED_BTN_H, "2:Portal");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_WINDOW,        controloffw + (84 * 2), 178 + controloff, 80, ED_BTN_H, "3:Window");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_DOOR,          controloffw + (84 * 3), 178 + controloff, 80, ED_BTN_H, "4:Door");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_SPLIT,         controloffw + (84 * 4), 178 + controloff, 80, ED_BTN_H, "Split");

    // sector inspector UI - sector
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 166 + controloffw,  82 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS,  194 + controloffw,  82 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS,  166 + controloffw, 110 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS,   194 + controloffw, 110 + controloff, 24, 24, "+");

    // sector inspector UI - wall
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 152 + controloffw, 62 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS,  184 + controloffw, 62 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 152 + controloffw, 90 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS,  184 + controloffw, 90 + controloff, 24, 24, "+");


    // confirmation dialog box buttons
    rcguiCreateButton(&g_ui, GUI_BTN_CONFIRM_YES,
                      (EDIT_VIEW_PORT_WIDTH / 2) - 70,
                      (EDIT_VIEW_PORT_HEIGHT / 2) + 10,
                      56, ED_BTN_H, "Yes");

    rcguiCreateButton(&g_ui, GUI_BTN_CONFIRM_NO,
                      (EDIT_VIEW_PORT_WIDTH / 2) + 14,
                      (EDIT_VIEW_PORT_HEIGHT / 2) + 10,
                      56, ED_BTN_H, "No");
}




static void pathDirnameFromFile(char *outDir, size_t outDirSize, const char *path)
{
    const char *slash;
    size_t len;

    if (!outDir || outDirSize == 0) return;

    outDir[0] = '\0';

    if (!path || !path[0]) {
        snprintf(outDir, outDirSize, ".");
        return;
    }

    slash = strrchr(path, '/');
    if (!slash) {
        snprintf(outDir, outDirSize, ".");
        return;
    }

    len = (size_t)(slash - path);

    if (len == 0) {
        snprintf(outDir, outDirSize, "/");
        return;
    }

    if (len >= outDirSize) {
        len = outDirSize - 1;
    }

    memcpy(outDir, path, len);
    outDir[len] = '\0';
}

static void initRememberedDialogDirs(void)
{
    if (g_dialogDirsInit) return;

    snprintf(g_mapDialogDir, sizeof(g_mapDialogDir), ".");
    snprintf(g_exportDialogDir, sizeof(g_exportDialogDir), ".");

    g_dialogDirsInit = 1;
}





static int runCommandGetLine(const char *cmd, char *out, size_t outSize)
{
    FILE *fp;
    size_t len;

    if (!cmd || !out || outSize == 0) return 0;

    out[0] = '\0';

    fp = popen(cmd, "r");
    if (!fp) return 0;

    if (!fgets(out, (int)outSize, fp)) {
        pclose(fp);
        out[0] = '\0';
        return 0;
    }

    pclose(fp);

    len = strlen(out);
    while (len > 0 && (out[len - 1] == '\n' || out[len - 1] == '\r')) {
        out[len - 1] = '\0';
        len--;
    }

    return (out[0] != '\0');
}

static int editorOpenMapDialog(char *outPath, size_t outPathSize)
{
    char cmd[1400];

    initRememberedDialogDirs();

    snprintf(cmd, sizeof(cmd),
        "kdialog --getopenfilename \"%s\" \"*.txt *.map|Map files\"",
        g_mapDialogDir);

    if (!runCommandGetLine(cmd, outPath, outPathSize)) {
        return 0;
    }

    pathDirnameFromFile(g_mapDialogDir, sizeof(g_mapDialogDir), outPath);
    return 1;
}

static int editorSaveMapDialog(char *outPath, size_t outPathSize)
{
    char startPath[1400];
    char cmd[1600];

    initRememberedDialogDirs();

    snprintf(startPath, sizeof(startPath), "%s/rc3d_map.txt", g_mapDialogDir);

    snprintf(cmd, sizeof(cmd),
        "kdialog --getsavefilename \"%s\" \"*.txt *.map|Map files\"",
        startPath);

    if (!runCommandGetLine(cmd, outPath, outPathSize)) {
        return 0;
    }

    pathDirnameFromFile(g_mapDialogDir, sizeof(g_mapDialogDir), outPath);
    return 1;
}






#define RC3D_EXPORT_MODE_C      0
#define RC3D_EXPORT_MODE_BINARY 1

static int hasFileExtInsensitive(const char *path, const char *ext)
{
    size_t pathLen, extLen;
    const char *p;

    if (!path || !ext) return 0;

    pathLen = strlen(path);
    extLen  = strlen(ext);

    if (pathLen < extLen) return 0;

    p = path + (pathLen - extLen);

    while (*p && *ext) {
        char a = *p++;
        char b = *ext++;

        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');

        if (a != b) return 0;
    }

    return (*p == '\0' && *ext == '\0');
}

static void appendFileExtIfMissing(char *path, size_t pathSize, const char *ext)
{
    size_t len;
    size_t extLen;

    if (!path || !ext) return;

    if (hasFileExtInsensitive(path, ext)) {
        return;
    }

    len = strlen(path);
    extLen = strlen(ext);

    if ((len + extLen + 1) >= pathSize) {
        return;
    }

    strcat(path, ext);
}

static int editorExportMapDialog(char *outPath, size_t outPathSize, int *outType)
{
    char startPath[1400];
    char cmd[1600];

    if (!outPath || outPathSize == 0 || !outType) {
        return 0;
    }

    initRememberedDialogDirs();

    snprintf(startPath, sizeof(startPath), "%s/rc3d_map_export.bin", g_exportDialogDir);

    snprintf(cmd, sizeof(cmd),
        "kdialog --getsavefilename \"%s\" \"*.bin *.c|(C source files)\"",
        startPath);

    if (!runCommandGetLine(cmd, outPath, outPathSize)) {
        return 0;
    }

    if (hasFileExtInsensitive(outPath, ".c")) {
        *outType = RC3D_EXPORT_MODE_C;
    } else if (hasFileExtInsensitive(outPath, ".bin")) {
        *outType = RC3D_EXPORT_MODE_BINARY;
    } else {
        /* default to binary if user typed no extension */
        *outType = RC3D_EXPORT_MODE_BINARY;
        appendFileExtIfMissing(outPath, outPathSize, ".bin");
    }

    pathDirnameFromFile(g_exportDialogDir, sizeof(g_exportDialogDir), outPath);
    return 1;
}

/*
static int exportMapByMode(const char *path, int savemode)
{
    if(savemode == RC3D_EXPORT_MODE_BINARY)
        return exportBinaryMap(path);
    else
        return exportCStringMap(path);
}

*/

static void handleEditorUI(int mouseX, int mouseY,
                           int leftDown, int leftPressed, int leftReleased,
                           float worldX, float worldY)
{
    int hit;
    EdWall *w = 0;
    EdSector *sec = 0;

    rcguiUpdate(&g_ui, mouseX, mouseY, leftDown, leftPressed, leftReleased);

    const uint8_t *modKeys = SDL_GetKeyboardState(NULL);
    float uiStep = 0.1f;

    if (modKeys[SDL_SCANCODE_LCTRL] || modKeys[SDL_SCANCODE_RCTRL]) {
        uiStep = 10.0f;
    } else if (modKeys[SDL_SCANCODE_LSHIFT] || modKeys[SDL_SCANCODE_RSHIFT]) {
        uiStep = 1.0f;
    }

    g_ed.uiHotId = rcguiGetHotButton(&g_ui);
    g_ed.uiActiveId = rcguiGetActiveButton(&g_ui);
    g_ed.uiMouseCaptured = (g_ed.uiHotId != 0) || (g_ed.uiActiveId != 0);

    rcguiSetButtonText(&g_ui, GUI_BTN_GRID, g_ed.tinyGridEnabled ? "Grid 0.1" : "Grid 1.0");

    rcguiSetButtonDisabled(&g_ui, GUI_BTN_UNDO, (g_undoCount <= 0));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_REDO, (g_redoCount <= 0));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_FINISH, (g_ed.draftCount < 2));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_CLRDRAFT, (g_ed.draftCount <= 0));

    /* -------------------------------------------------- */
    /* default: hide all expandable-panel buttons first   */
    /* -------------------------------------------------- */
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SOLID, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PORTAL, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_WINDOW, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_DOOR, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SPLIT, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 0);

    // dialog box (yes no)
    rcguiSetButtonVisible(&g_ui, GUI_BTN_CONFIRM_YES, g_ed.confirmVisible);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_CONFIRM_NO,  g_ed.confirmVisible);

    /* -------------------------------------------------- */
    /* only enable lower panel controls if menu is open   */
    /* -------------------------------------------------- */
    //if (g_ed.ui_menu_visable) 
    {
        if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
            w = &g_edMap.walls[g_ed.selectedWall];

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SOLID, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PORTAL, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_WINDOW, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_DOOR, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SPLIT, 1);

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 1);

            rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_SPLIT, !g_ed.splitPreviewValid);
        }

        if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
            sec = &g_edMap.sectors[g_ed.selectedSector];

            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 1);
        }
    }

    hit = rcguiGetButtonHit(&g_ui);

    if (g_ed.confirmVisible) {
        switch (hit) {
            case GUI_BTN_CONFIRM_YES:
                acceptConfirmDialog();
                break;

            case GUI_BTN_CONFIRM_NO:
                closeConfirmDialog();
                break;

            default:
                break;
        }

        g_ed.uiMouseCaptured = 1;
        return;
    }

    switch (hit) {
        case GUI_BTN_HELP:
            g_ed.ui_menu_visable = 1 - g_ed.ui_menu_visable;
        break; 

        case GUI_BTN_NEWMAP:
            openConfirmDialog(ED_CONFIRM_NEW_MAP, "Discard current map and start a new blank map?");
        break;
        
        case GUI_BTN_UNDO:
            performUndo();
            break;

        case GUI_BTN_REDO:
            performRedo();
            break;

        //case GUI_BTN_SAVE:
            //saveTextMap("rc3d_map.txt");
            //break;

        //case GUI_BTN_LOAD:
            //loadTextMap("rc3d_map.txt");
            //break;

        //case GUI_BTN_EXPORT:
            //exportCStringMap("rc3d_map_export.c");
            //break;


        case GUI_BTN_SAVE:
        {
            char path[1024];
            if (editorSaveMapDialog(path, sizeof(path))) {
                saveTextMap(path);
            }
        }
        break;

        case GUI_BTN_LOAD:
        {
            char path[1024];
            if (editorOpenMapDialog(path, sizeof(path))) {
                loadTextMap(path);
            }
        }
        break;


        case GUI_BTN_EXPORT:
        {
            char path[1024];
            int exportType;

            if (editorExportMapDialog(path, sizeof(path), &exportType)) {
                if (exportType == RC3D_EXPORT_MODE_BINARY) {
                    exportBinaryMap(path);
                } else {
                    exportCStringMap(path);
                }
            }
        }
        break;



        case GUI_BTN_GRID:
            g_ed.tinyGridEnabled = !g_ed.tinyGridEnabled;
            g_ed.currentGridStep = g_ed.tinyGridEnabled ? ED_GRID_STEP_TINY : ED_GRID_STEP;
            break;

        case GUI_BTN_FINISH:
        {
            if (g_ed.draftCount == 2) {
                pushUndoState();

                if (!splitSelectedSectorByDraftLine()) {
                    /* failed: leave draft as-is */
                }
            }
            else if (g_ed.draftCount >= 3) {
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
        } break;

        case GUI_BTN_CLRDRAFT:
            clearDraft();
            break;

        case GUI_BTN_CLEANMAP:
        {
            int rv, rw, rs;
            char msg[256];

            pushUndoState();
            cleanMapCompactWithReport(&rv, &rw, &rs);

            snprintf(msg, sizeof(msg),
                "Cleaned: removed %d %s, %d %s, %d %s",
                rv, (rv == 1) ? "vertex" : "vertices",
                rw, (rw == 1) ? "wall"   : "walls",
                rs, (rs == 1) ? "sector" : "sectors");
            setEditorStatus(msg);
        }
        break;

        case GUI_BTN_WALL_SOLID:
            if (w) {
                pushUndoState();
                makeWallSolid(g_ed.selectedWall, (w->midColor == 0) ? 14 : w->midColor);
            }
            break;

        case GUI_BTN_WALL_PORTAL:
            if (w) {
                pushUndoState();
                tryMakeWallPortal(g_ed.selectedWall);
            }
            break;

        case GUI_BTN_WALL_WINDOW:
            if (w) {
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
            break;

        case GUI_BTN_WALL_DOOR:
            if (w) {
                float ob = w->openBottom;
                float ot = w->openTop;
                if (ot <= ob) {
                    ob = 0.0f;
                    ot = 1.6f;
                }
                pushUndoState();
                makeWallDoor(g_ed.selectedWall, ob, ot,
                             (w->midColor == 0) ? 14 : w->midColor);
            }
            break;

        case GUI_BTN_WALL_SPLIT:
            if (w && g_ed.splitPreviewValid) {
                pushUndoState();
                splitWallAtSelected(g_ed.selectedWall, worldX, worldY);
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorHeight -= uiStep;
                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }
                syncAllPortals();
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorHeight += uiStep;
                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }
                syncAllPortals();
            }
            break;

        case GUI_BTN_SECTOR_CEIL_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilHeight -= uiStep;
                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }
                syncAllPortals();
            }
            break;

        case GUI_BTN_SECTOR_CEIL_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilHeight += uiStep;
                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }
                syncAllPortals();
            }
            break;

        case GUI_BTN_WALL_OPENBOT_MINUS:
            if (w) {
                pushUndoState();
                w->openBottom -= uiStep;
                if (w->openTop < w->openBottom) {
                    float t = w->openTop;
                    w->openTop = w->openBottom;
                    w->openBottom = t;
                }
            }
            break;

        case GUI_BTN_WALL_OPENBOT_PLUS:
            if (w) {
                pushUndoState();
                w->openBottom += uiStep;
                if (w->openTop < w->openBottom) {
                    float t = w->openTop;
                    w->openTop = w->openBottom;
                    w->openBottom = t;
                }
            }
            break;

        case GUI_BTN_WALL_OPENTOP_MINUS:
            if (w) {
                pushUndoState();
                w->openTop -= uiStep;
                if (w->openTop < w->openBottom) {
                    float t = w->openTop;
                    w->openTop = w->openBottom;
                    w->openBottom = t;
                }
            }
            break;

        case GUI_BTN_WALL_OPENTOP_PLUS:
            if (w) {
                pushUndoState();
                w->openTop += uiStep;
                if (w->openTop < w->openBottom) {
                    float t = w->openTop;
                    w->openTop = w->openBottom;
                    w->openBottom = t;
                }
            }
            break;

        default:
            break;
    }
}

static void drawStatusPopup(void)
{
    int w, x, y;

    if (g_ed.statusTimer <= 0.0f || g_ed.statusText[0] == '\0') {
        return;
    }

    rc3dGuiDirty();

    w = (int)strlen(g_ed.statusText) * ED_FONT_W + 16;
    if (w < 120) w = 120;

    x = (EDIT_VIEW_PORT_WIDTH / 2) - (w / 2);
    y = (EDIT_VIEW_PORT_HEIGHT - 30);

    drawRect(x, y, w, 24, 13);
    drawRectL(x, y, w, 24, 27);
    drawText(x + 8, y + 4, g_ed.statusText, 2);
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

    if (g_ed.statusTimer > 0.0f) {
        g_ed.statusTimer -= dt;
        if (g_ed.statusTimer < 0.0f) {
            g_ed.statusTimer = 0.0f;
            g_ed.statusText[0] = '\0';
        }
    }

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

    
    if (leftDown && !g_ed.prevLeftDown && !g_ed.uiMouseCaptured) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
        g_ed.draggingMultiVertex = 0;
        g_ed.boxSelecting = 0;

        /* -------------------------------------------------- */
        /* existing multi-vertex selection: drag it if clicked */
        /* on one of its verts, otherwise start a new box      */
        /* -------------------------------------------------- */
        if (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0) {
            if (findSelectedVertexNearMouse(mouseX, mouseY) >= 0) {
                pushUndoState();
                beginMultiVertexDrag(worldX, worldY);
            } else {
                clearAllSelections();
                beginBoxSelect(mouseX, mouseY);
            }
        }
        else {
            /* try normal single-object selection first */
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
            else {
                /* clicked empty space -> box select */
                beginBoxSelect(mouseX, mouseY);
            }
        }
    }


    /* drag selected vertex */
    if (leftDown && g_ed.draggingVertex && g_ed.selectedVert >= 0) {
        const float dx = snapDeltaf(worldX - g_ed.dragStartWorldX);
        const float dy = snapDeltaf(worldY - g_ed.dragStartWorldY);

        g_edMap.verts[g_ed.selectedVert].x = g_ed.dragVertexStartX + dx;
        g_edMap.verts[g_ed.selectedVert].y = g_ed.dragVertexStartY + dy;

        syncAllPortals();
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    /* drag selected wall */
    if (leftDown && g_ed.draggingWall && g_ed.selectedWall >= 0) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        const float dx = snapDeltaf(worldX - g_ed.dragWallStartWorldX);
        const float dy = snapDeltaf(worldY - g_ed.dragWallStartWorldY);

        g_edMap.verts[w->v0].x = g_ed.dragWallV0StartX + dx;
        g_edMap.verts[w->v0].y = g_ed.dragWallV0StartY + dy;

        if (w->v1 != w->v0) {
            g_edMap.verts[w->v1].x = g_ed.dragWallV1StartX + dx;
            g_edMap.verts[w->v1].y = g_ed.dragWallV1StartY + dy;
        }

        syncAllPortals();
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    

    if (leftDown && g_ed.draggingSector && g_ed.selectedSector >= 0) {
        dragSelectedSectorTo(worldX, worldY);
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    // box selecting
    if (leftDown && g_ed.boxSelecting) {
        updateBoxSelect(mouseX, mouseY);
    }

    if (leftDown && g_ed.draggingMultiVertex) {
        dragMultiVertexSelectionTo(worldX, worldY);
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

    if (!leftDown && g_ed.prevLeftDown) {
        if (g_ed.boxSelecting) {
            finalizeBoxSelect();
        }
    }

    if (!leftDown) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
        g_ed.draggingMultiVertex = 0;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_DELETE)) {
        if (g_ed.selectedVert >= 0) {
            pushUndoState();
            deleteVertexByIndex(g_ed.selectedVert);
        } else if (g_ed.selectedWall >= 0) {
            pushUndoState();
            deleteWallByIndex(g_ed.selectedWall);
        } else if (g_ed.selectedSector >= 0) {
            pushUndoState();
            deleteSectorByIndex(g_ed.selectedSector);
        } else if (g_ed.hoverVert >= 0) {
            pushUndoState();
            deleteVertexByIndex(g_ed.hoverVert);
        } else if (g_ed.hoverWall >= 0) {
            pushUndoState();
            deleteWallByIndex(g_ed.hoverWall);
        } else if (g_ed.hoverSector >= 0) {
            pushUndoState();
            deleteSectorByIndex(g_ed.hoverSector);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F7)) {
        pushUndoState();
        repairMapTopology();
        setEditorStatus("Repaired map topology / portals");
    }


    if (keyPressedOnce(keys, SDL_SCANCODE_F8)) {
        if (g_ed.selectedSector >= 0) {
            int made;
            char msg[128];

            pushUndoState();
            made = buildInnerSectorsFromSelectedSector();

            if (made > 0) {
                snprintf(msg, sizeof(msg),
                    "Built %d inner sector%s from closed wall loop%s",
                    made, (made == 1) ? "" : "s",
                    (made == 1) ? "" : "s");
                setEditorStatus(msg);
            } else {
                setEditorStatus("No valid closed inner wall loops found in selected sector");
            }
        } else {
            setEditorStatus("Select the outer sector first");
        }
    }











    if (keyPressedOnce(keys, SDL_SCANCODE_BACKSPACE)) {
        if (g_ed.draftCount > 0) {
            g_ed.draftCount--;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F6)) {
        int rv, rw, rs;
        char msg[256];

        pushUndoState();
        cleanMapCompactWithReport(&rv, &rw, &rs);

        snprintf(msg, sizeof(msg),
         "Cleaned: removed %d %s, %d %s, %d %s",
         rv, (rv == 1) ? "vertex" : "vertices",
         rw, (rw == 1) ? "wall"   : "walls",
         rs, (rs == 1) ? "sector" : "sectors");

        setEditorStatus(msg);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F1)) {
        g_ed.ui_menu_visable = 1 - g_ed.ui_menu_visable;
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
        if (g_ed.draftCount == 2) {
            pushUndoState();

            if (!splitSelectedSectorByDraftLine()) {
                /* failed: leave draft as-is */
            }
        }
        else if (g_ed.draftCount >= 3) {
            const float area = draftSignedArea();

            pushUndoState();

            if (finalizeDraftSectorAttached()) {
                /* done */
            }
            else {
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

        /* SHIFT + keypad = colour copy/paste */
        if ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_1)) {
            g_ed.copiedSectorFloorColor = sec->floorColor;
            g_ed.hasCopiedSectorFloorColor = 1;

            {
                char msg[128];
                snprintf(msg, sizeof(msg), "Copied sector floor colour: %u", (unsigned)sec->floorColor);
                setEditorStatus(msg);
            }
        }

        if ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_2)) {
            g_ed.copiedSectorCeilColor = sec->ceilColor;
            g_ed.hasCopiedSectorCeilColor = 1;

            {
                char msg[128];
                snprintf(msg, sizeof(msg), "Copied sector ceiling colour: %u", (unsigned)sec->ceilColor);
                setEditorStatus(msg);
            }
        }

        if ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_4)) {
            if (g_ed.hasCopiedSectorFloorColor) {
                pushUndoState();
                sec->floorColor = g_ed.copiedSectorFloorColor;
                syncAllPortals();

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Pasted sector floor colour: %u", (unsigned)sec->floorColor);
                    setEditorStatus(msg);
                }
            } else {
                setEditorStatus("No copied sector floor colour");
            }
        }

        if ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_5)) {
            if (g_ed.hasCopiedSectorCeilColor) {
                pushUndoState();
                sec->ceilColor = g_ed.copiedSectorCeilColor;
                syncAllPortals();

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Pasted sector ceiling colour: %u", (unsigned)sec->ceilColor);
                    setEditorStatus(msg);
                }
            } else {
                setEditorStatus("No copied sector ceiling colour");
            }
        }

        /* plain keypad = height copy/paste */
        if (!(keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_1)) {
            g_ed.copiedSectorFloor = sec->floorHeight;
            g_ed.hasCopiedSectorFloor = 1;

            {
                char msg[128];
                snprintf(msg, sizeof(msg), "Copied sector floor height: %.2f", sec->floorHeight);
                setEditorStatus(msg);
            }
        }

        if (!(keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_2)) {
            g_ed.copiedSectorCeil = sec->ceilHeight;
            g_ed.hasCopiedSectorCeil = 1;

            {
                char msg[128];
                snprintf(msg, sizeof(msg), "Copied sector ceiling height: %.2f", sec->ceilHeight);
                setEditorStatus(msg);
            }
        }

        if (!(keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_4)) {
            if (g_ed.hasCopiedSectorFloor) {
                pushUndoState();
                sec->floorHeight = g_ed.copiedSectorFloor;

                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }

                syncAllPortals();

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Pasted sector floor height: %.2f", sec->floorHeight);
                    setEditorStatus(msg);
                }
            } else {
                setEditorStatus("No copied sector floor height");
            }
        }

        if (!(keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_5)) {
            if (g_ed.hasCopiedSectorCeil) {
                pushUndoState();
                sec->ceilHeight = g_ed.copiedSectorCeil;

                if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                    sec->ceilHeight = sec->floorHeight + 0.1f;
                }

                syncAllPortals();

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Pasted sector ceiling height: %.2f", sec->ceilHeight);
                    setEditorStatus(msg);
                }
            } else {
                setEditorStatus("No copied sector ceiling height");
            }
        }

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


        /* new wall defaults for drafted geometry */
        
        // edit the wall
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
    if (g_ed.selectedWall < 0) {
        if (keyPressedOnce(keys, SDL_SCANCODE_A)) g_ed.newWallUpperColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_S)) g_ed.newWallUpperColor++;

        if (keyPressedOnce(keys, SDL_SCANCODE_D)) g_ed.newWallMidColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_F)) g_ed.newWallMidColor++;

        if (keyPressedOnce(keys, SDL_SCANCODE_H)) g_ed.newWallLowerColor--;
        if (keyPressedOnce(keys, SDL_SCANCODE_J)) g_ed.newWallLowerColor++;
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
        char path[1024];
        if (editorOpenMapDialog(path, sizeof(path))) {
            loadTextMap(path);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F3)) {
        char path[1024];
        if (editorSaveMapDialog(path, sizeof(path))) {
            saveTextMap(path);
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F5)) {
        char path[1024];
        int exportType;

        if (editorExportMapDialog(path, sizeof(path), &exportType)) {
            pushUndoState();
            cleanMapCompact();

            if (exportType == RC3D_EXPORT_MODE_BINARY) {
                exportBinaryMap(path);
            } else {
                exportCStringMap(path);
            }
        }
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

    drawTopMenuBar();
    drawStatusPopup();
    drawConfirmPopup();

    if (g_ed.ui_menu_visable) {
        drawExpandedEditorPanel();
    }

    drawInspectorPanel();
    drawHoverPanel();
    rcguiDraw(&g_ui);
}


