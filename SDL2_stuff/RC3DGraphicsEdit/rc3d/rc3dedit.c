// file: rc3edit.c

#include "rc3dedit.h"

#include <SDL2/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "../gfx.h"
#include "rc3d_map.h"

#define RC3D_WALL_PORTAL        0x01
#define RC3D_WALL_UPPER         0x02
#define RC3D_WALL_MIDDLE        0x04
#define RC3D_WALL_LOWER         0x08
#define RC3D_WALL_SOLID         0x10
#define RC3D_WALL_MANUAL_TARGET 0x20
#define RC3D_WALL_TRANSPARENCY  0x40    

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define DEG2RAD(d) ((d) * ((float)M_PI / 180.0f))
#define RAD2DEG(r) ((r) * (180.0f / (float)M_PI))

#define ED_MAX_VERTS            4096
#define ED_MAX_WALLS            4096
#define ED_MAX_SECTORS          512
#define ED_MAX_DRAFT_POINTS     128

#define ED_ORPHANED_SECTOR_TEXTURE  254      // when deleting a sector and it leaves walls behind, set it to this texture

// tab toggles hi-res to low res
#define ED_GRID_STEP_TINY       0.1f
#define ED_GRID_STEP            1.0f

#define ED_EPSILON              0.001f
#define ED_PICK_DIST_PX         25



#define RC3D_EXPORT_MODE_C      0
#define RC3D_EXPORT_MODE_BINARY 1


//// GUI system settings
#define ED_UI_PAD               6
#define ED_FONT_W               8
#define ED_FONT_H               16
#define ED_ROW_STEP             (ED_FONT_H + ED_UI_PAD)

#define ED_TOPBAR_X             6
#define ED_TOPBAR_Y             6
#define ED_TOPBAR_W             814
#define ED_TOPBAR_H             (ED_FONT_H + (ED_UI_PAD * 2))

#define ED_BOTTOMBAR_X             6
#define ED_BOTTOMBAR_Y             SCREEN_H - 60
#define ED_BOTTOMBAR_W             890
#define ED_BOTTOMBAR_H             (ED_FONT_H + (ED_UI_PAD * 2))

#define ED_PANEL_X              6
#define ED_PANEL_Y              (ED_TOPBAR_Y + ED_TOPBAR_H + 4)
#define ED_PANEL_W              782
#define ED_PANEL_H              280



// GUI parts
#define ED_UI_TEXT_INFOTYPE         29

#define ED_UI_BTN_BG                9
#define ED_UI_BTN_BG_ACTIVE         8
#define ED_UI_BTN_HOVER             8



#define ED_UI_BTN_BORDER_ACTIVE     9
#define ED_UI_BTN_BORDER            8
#define ED_UI_BTN_TEXT              2


#define ED_UI_BTN_ACTIVE            9  // background colour
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
    GUI_BTN_VALIDATOR,

    GUI_BTN_WALL_SOLID,
    GUI_BTN_WALL_PORTAL,
    GUI_BTN_WALL_WINDOW,
    GUI_BTN_WALL_DOOR,
    GUI_BTN_WALL_TRANSPARENCY,
    GUI_BTN_WALL_SPLIT,


    GUI_BTN_WALL_CLAMP_XL,
    GUI_BTN_WALL_CLAMP_XR,
    GUI_BTN_WALL_CLAMP_YT,
    GUI_BTN_WALL_CLAMP_YB,


    GUI_BTN_SECTOR_FLOOR_MINUS,
    GUI_BTN_SECTOR_FLOOR_PLUS,
    GUI_BTN_SECTOR_CEIL_MINUS,
    GUI_BTN_SECTOR_CEIL_PLUS,

    GUI_BTN_WALL_OPENBOT_MINUS,
    GUI_BTN_WALL_OPENBOT_PLUS,
    GUI_BTN_WALL_OPENTOP_MINUS,
    GUI_BTN_WALL_OPENTOP_PLUS,
    GUI_BTN_WALL_TEX_ROT_MINUS,
    GUI_BTN_WALL_TEX_ROT_PLUS,
    GUI_BTN_WALL_TEX_ROT_RESET,

    GUI_BTN_CONFIRM_YES,
    GUI_BTN_CONFIRM_NO,

    GUI_BTN_SECTOR_CUTTER,      // F6
    GUI_BTN_REPAIR_TOPOLOGY,    // F7
    GUI_BTN_CLEANMAP,           // F8
    GUI_BTN_MAPVALIDATOR,       // F9
    GUI_BTN_LAUNCH_TEST_MAP,    // F12


    GUI_BTN_SECTOR_FTEX_SX_MINUS,
    GUI_BTN_SECTOR_FTEX_SX_PLUS,
    GUI_BTN_SECTOR_FTEX_SY_MINUS,
    GUI_BTN_SECTOR_FTEX_SY_PLUS,
    GUI_BTN_SECTOR_FTEX_ROT_MINUS,
    GUI_BTN_SECTOR_FTEX_ROT_PLUS,
    GUI_BTN_SECTOR_FTEX_ROT_RESET,

    GUI_BTN_SECTOR_CTEX_SX_MINUS,
    GUI_BTN_SECTOR_CTEX_SX_PLUS,
    GUI_BTN_SECTOR_CTEX_SY_MINUS,
    GUI_BTN_SECTOR_CTEX_SY_PLUS,
    GUI_BTN_SECTOR_CTEX_ROT_MINUS,
    GUI_BTN_SECTOR_CTEX_ROT_PLUS,
    GUI_BTN_SECTOR_CTEX_ROT_RESET,
};


typedef enum {
    ED_ACT_NONE = 0,
    ED_ACT_HELP,
    ED_ACT_NEW_MAP,
    ED_ACT_QUIT,

    ED_ACT_UNDO,
    ED_ACT_REDO,
    ED_ACT_LOAD,
    ED_ACT_SAVE,
    ED_ACT_EXPORT,
    ED_ACT_TOGGLE_GRID,
    ED_ACT_FINISH_DRAFT,
    ED_ACT_CLEAR_DRAFT,
    ED_ACT_SECTOR_CUTTER,
    ED_ACT_REPAIR_TOPOLOGY,
    ED_ACT_CLEAN_MAP,
    ED_ACT_VALIDATE_MAP,
    ED_ACT_RUN_TEST,
    ED_ACT_WALL_SOLID,
    ED_ACT_WALL_PORTAL,
    ED_ACT_WALL_WINDOW,
    ED_ACT_WALL_DOOR,
    ED_ACT_WALL_SPLIT,
    ED_ACT_WALL_TRANSPARENCY,

    ED_ACT_WALL_EXTRUDE
} EdAction;



typedef enum {
    ED_CONFIRM_NONE = 0,
    ED_CONFIRM_NEW_MAP,
    ED_CONFIRM_QUIT
} EdConfirmAction;


typedef enum {
    ED_SEL_NONE = 0,
    ED_SEL_VERTEX,
    ED_SEL_WALL,
    ED_SEL_SECTOR
} EdSelectionType;

typedef enum {
    ED_VAL_TARGET_NONE = 0,
    ED_VAL_TARGET_VERTEX,
    ED_VAL_TARGET_WALL,
    ED_VAL_TARGET_SECTOR
} EdValidatorTargetType;


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

    uint8_t flags;       // wall behaviour/type flags
    uint32_t tex_flags;  // texture clamp/uv behaviour flags
} EdWall;


typedef struct {
    int wallStart;
    int wallCount;
    int boundaryCount;

    float floorHeight;
    float ceilHeight;

    uint8_t floorColor;
    uint8_t ceilColor;

    float floorTexScaleX;
    float floorTexScaleY;
    float floorTexAngle;

    float ceilTexScaleX;
    float ceilTexScaleY;
    float ceilTexAngle;
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
    // validator window
    uint8_t ui_validator_visable;
    int validatorRan;
    int validatorIssueCount;
    int validatorSelectedIssue;

    char validatorLines[64][128];
    uint8_t validatorTargetType[64];
    int validatorTargetIndex[64];

    // status window
    char statusText[256];
    float statusTimer;

    int undoHistoryVisible;
    int undoHistorySelectedPos;
    int undoHistoryScrollPos;

    int confirmVisible;
    EdConfirmAction confirmAction;
    char confirmText[256];
    int requestQuit;

    int bUseVectorFill;
    int textureBrowserOffset;
    int textureBrowserTarget;

    
    int textureScrollbarDragging;
    int textureScrollbarDragOffsetY;

} EditorState;



static EditorMap g_edMap;
static EditorState g_ed;

static char g_mapDialogDir[1024];
static char g_exportDialogDir[1024];
static int g_dialogDirsInit = 0;

static int mergeCloseVertices(float epsilon);
static void repairMapTopology(void);


#define ED_HISTORY_MAX 1000
#define ED_UNDO_HISTORY_VISIBLE_ROWS 10
#define ED_UNDO_HISTORY_ROW_H       24
#define ED_UNDO_HISTORY_POPUP_W     660
#define ED_UNDO_HISTORY_POPUP_H     336

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

// rotation of selections - prototypes
static int collectSelectedVertexPivot(float *outCx, float *outCy);
static void rotateSelectedVertices(float angleRad);

// sector fill stuffs, prototypes
static uint8_t getSectorFillColour(int sectorIndex);
static int getSectorFillStepY(int sectorIndex);
static void drawFilledSector2D(int sectorIndex);

/////// GUI PARTS
static void handleEditorUI(int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased, float worldX, float worldY);
static void drawValidatorPanel(void);
static void validateMap(void);
static void validatorAddLineEx(uint8_t targetType, int targetIndex, const char *fmt, ...);
static void validatorAddLine(const char *fmt, ...);
static void validatorSelectIssue(int issueIndex);
static void validatorNextIssue(void);
static void validatorPrevIssue(void);

// map editing
// extruding
static int extrudeWallToNewSector(int wallIndex, float depth);
static void doExtrudeWall(void);

static void pushUndoState(void);
static void resetUndoRedoHistory(void);
static void drawUndoHistoryPopup(void);
static void openUndoHistoryPopup(void);
static void closeUndoHistoryPopup(void);
static int handleUndoHistoryPopupInput(const uint8_t *keys,
                                       int mouseX,
                                       int mouseY,
                                       int leftPressed,
                                       int mouseWheelY);
static void finishEditorInputFrame(const uint8_t *keys,
                                   int leftDown,
                                   int rightDown,
                                   int middleDown,
                                   int mouseX,
                                   int mouseY);


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

static float absf_local(float v){
    return (v < 0.0f) ? -v : v;
}

static int clampi_local(int v, int lo, int hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float clampf_local(float v, float lo, float hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float snapDeltaf(float v){
    float step = g_ed.currentGridStep;

    if (step < ED_GRID_STEP_TINY) {
        step = ED_GRID_STEP_TINY;
    }

    return roundf(v / step) * step;
}

static float snapf(float v){
    float step = g_ed.currentGridStep;

    if (step < ED_GRID_STEP_TINY) {
        step = ED_GRID_STEP_TINY;
    }

    return roundf(v / step) * step;
}

/////////////////////// image previewer ////////////////////////


#define TEXTURE_WIDTH           64
#define TEXTURE_HEIGHT          64
#define TEXTURE_PARTS           3
#define TEXTURE_LIBRARY_COUNT   256

#define TEX_THUMB_W             32
#define TEX_THUMB_H             32

#define TEX_BROWSER_COLS        5
#define TEX_BROWSER_ROWS        6
#define TEX_BROWSER_CELL_W      80
#define TEX_BROWSER_CELL_H      52
#define ED_INSPECTOR_TEXTURE_WINDOW_HEIGHT      47 + (TEX_BROWSER_ROWS * (TEX_THUMB_H + 20) )

#define TEX_BROWSER_HEADER_H      46
#define TEX_BROWSER_GRID_Y        52
#define TEX_BROWSER_SCROLL_W      12
#define TEX_BROWSER_SCROLL_PAD    6

enum {
    TEXVIEW_UPPER = 0,
    TEXVIEW_MIDDLE,
    TEXVIEW_LOWER
};

typedef enum {
    TEX_TARGET_NONE = 0,

    TEX_TARGET_WALL_UPPER,
    TEX_TARGET_WALL_MIDDLE,
    TEX_TARGET_WALL_LOWER,

    TEX_TARGET_SECTOR_FLOOR,
    TEX_TARGET_SECTOR_CEIL,

    TEX_TARGET_DEFAULT_WALL_UPPER,
    TEX_TARGET_DEFAULT_WALL_MIDDLE,
    TEX_TARGET_DEFAULT_WALL_LOWER,

    TEX_TARGET_DEFAULT_SECTOR_FLOOR,
    TEX_TARGET_DEFAULT_SECTOR_CEIL
} TextureTarget;

/*
    // DO NOT DELETE THIS REFERENCE!
    00 - no texture
    01 - brick
    02 - dirt
    03 - grass
    04 - lava
    05 - water
    06 - wood
    255 - skybox
*/

static uint8_t *textureview[TEXTURE_PARTS] = { NULL, NULL, NULL };
static int textureviewLoadedIndex[TEXTURE_PARTS] = { -1, -1, -1 };

/* full cache for browser/thumbs */
static uint8_t *g_textureCache[TEXTURE_LIBRARY_COUNT] = { NULL };
static uint8_t g_textureCacheLoaded[TEXTURE_LIBRARY_COUNT] = { 0 };

static int pointInRectLocal(int px, int py, int x, int y, int w, int h){
    return (px >= x && px < (x + w) && py >= y && py < (y + h));
}

static uint8_t *getTexturePtr(uint8_t index)
{
    char filename[256];

    if ((int)index < 0 || index >= TEXTURE_LIBRARY_COUNT) {
        return NULL;
    }

    if (!g_textureCache[index]) {
        g_textureCache[index] = malloc(TEXTURE_WIDTH * TEXTURE_HEIGHT);
        if (!g_textureCache[index]) {
            return NULL;
        }
        memset(g_textureCache[index], 0, TEXTURE_WIDTH * TEXTURE_HEIGHT);
        g_textureCacheLoaded[index] = 0;
    }

    if (!g_textureCacheLoaded[index]) {
        snprintf(filename, sizeof(filename), "./textures/%02u.ppb", (unsigned)index);
        LoadPPB(filename, g_textureCache[index]);
        g_textureCacheLoaded[index] = 1;
    }

    return g_textureCache[index];
}

static void getTexture(uint8_t index, uint8_t part)
{
    uint8_t *src;

    if (part >= TEXTURE_PARTS) {
        return;
    }

    if (!textureview[part]) {
        textureview[part] = malloc(TEXTURE_WIDTH * TEXTURE_HEIGHT);
        if (!textureview[part]) {
            return;
        }
        textureviewLoadedIndex[part] = -1;
    }

    if (textureviewLoadedIndex[part] == (int)index) {
        return;
    }

    src = getTexturePtr(index);
    if (!src) {
        memset(textureview[part], 0, TEXTURE_WIDTH * TEXTURE_HEIGHT);
        textureviewLoadedIndex[part] = -1;
        return;
    }

    memcpy(textureview[part], src, TEXTURE_WIDTH * TEXTURE_HEIGHT);
    textureviewLoadedIndex[part] = (int)index;
}


static void drawTextureThumb(int x, int y, uint8_t *tex)
{
    int ty, tx;

    drawRect(x, y, TEX_THUMB_W, TEX_THUMB_H, 0);

    if (!tex) {
        drawRectL(x, y, TEX_THUMB_W, TEX_THUMB_H, 31);
        return;
    }

    for (ty = 0; ty < TEX_THUMB_H; ty++) {
        for (tx = 0; tx < TEX_THUMB_W; tx++) {
            uint8_t c = tex[((ty * 2) * TEXTURE_WIDTH) + (tx * 2)];
            drawRect(x + tx, y + ty, 1, 1, c);
        }
    }

    //drawRectL(x, y, TEX_THUMB_W, TEX_THUMB_H, 31);
}

static const char *textureTargetName(TextureTarget t)
{
    switch (t) {
        case TEX_TARGET_WALL_UPPER:           return "Wall Upper";
        case TEX_TARGET_WALL_MIDDLE:          return "Wall Middle";
        case TEX_TARGET_WALL_LOWER:           return "Wall Lower";
        case TEX_TARGET_SECTOR_FLOOR:         return "Sector Floor";
        case TEX_TARGET_SECTOR_CEIL:          return "Sector Ceiling";
        case TEX_TARGET_DEFAULT_WALL_UPPER:   return "Default Wall Upper";
        case TEX_TARGET_DEFAULT_WALL_MIDDLE:  return "Default Wall Middle";
        case TEX_TARGET_DEFAULT_WALL_LOWER:   return "Default Wall Lower";
        case TEX_TARGET_DEFAULT_SECTOR_FLOOR: return "Default Sector Floor";
        case TEX_TARGET_DEFAULT_SECTOR_CEIL:  return "Default Sector Ceiling";
        default:                              return "None";
    }
}

static int textureTargetCurrentIndex(TextureTarget t)
{
    if (g_ed.selectionType == ED_SEL_WALL &&
        g_ed.selectedWall >= 0 &&
        g_ed.selectedWall < g_edMap.wallCount) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        switch (t) {
            case TEX_TARGET_WALL_UPPER:  return w->upperColor;
            case TEX_TARGET_WALL_MIDDLE: return w->midColor;
            case TEX_TARGET_WALL_LOWER:  return w->lowerColor;
            default: break;
        }
    }

    if (g_ed.selectionType == ED_SEL_SECTOR &&
        g_ed.selectedSector >= 0 &&
        g_ed.selectedSector < g_edMap.sectorCount) {
        EdSector *s = &g_edMap.sectors[g_ed.selectedSector];

        switch (t) {
            case TEX_TARGET_SECTOR_FLOOR: return s->floorColor;
            case TEX_TARGET_SECTOR_CEIL:  return s->ceilColor;
            default: break;
        }
    }

    switch (t) {
        case TEX_TARGET_DEFAULT_WALL_UPPER:   return g_ed.newWallUpperColor;
        case TEX_TARGET_DEFAULT_WALL_MIDDLE:  return g_ed.newWallMidColor;
        case TEX_TARGET_DEFAULT_WALL_LOWER:   return g_ed.newWallLowerColor;
        case TEX_TARGET_DEFAULT_SECTOR_FLOOR: return g_ed.sectorFloorColor;
        case TEX_TARGET_DEFAULT_SECTOR_CEIL:  return g_ed.sectorCeilColor;
        default:                              return -1;
    }
}

static void textureTargetApply(TextureTarget t, uint8_t texIndex)
{
    if (texIndex >= TEXTURE_LIBRARY_COUNT) {
        return;
    }

    if (g_ed.selectionType == ED_SEL_WALL &&
        g_ed.selectedWall >= 0 &&
        g_ed.selectedWall < g_edMap.wallCount) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        switch (t) {
            case TEX_TARGET_WALL_UPPER:  w->upperColor = texIndex; return;
            case TEX_TARGET_WALL_MIDDLE: w->midColor   = texIndex; return;
            case TEX_TARGET_WALL_LOWER:  w->lowerColor = texIndex; return;
            default: break;
        }
    }

    if (g_ed.selectionType == ED_SEL_SECTOR &&
        g_ed.selectedSector >= 0 &&
        g_ed.selectedSector < g_edMap.sectorCount) {
        EdSector *s = &g_edMap.sectors[g_ed.selectedSector];

        switch (t) {
            case TEX_TARGET_SECTOR_FLOOR: s->floorColor = texIndex; syncAllPortals(); return;
            case TEX_TARGET_SECTOR_CEIL:  s->ceilColor  = texIndex; syncAllPortals(); return;
            default: break;
        }
    }

    switch (t) {
        case TEX_TARGET_DEFAULT_WALL_UPPER:   g_ed.newWallUpperColor = texIndex; return;
        case TEX_TARGET_DEFAULT_WALL_MIDDLE:  g_ed.newWallMidColor   = texIndex; return;
        case TEX_TARGET_DEFAULT_WALL_LOWER:   g_ed.newWallLowerColor = texIndex; return;
        case TEX_TARGET_DEFAULT_SECTOR_FLOOR: g_ed.sectorFloorColor  = texIndex; return;
        case TEX_TARGET_DEFAULT_SECTOR_CEIL:  g_ed.sectorCeilColor   = texIndex; return;
        default: return;
    }
}

static void getTextureBrowserRect(int *x, int *y, int *w, int *h)
{
    *x = EDIT_VIEW_PORT_WIDTH + 8;
    *y = 608;
    *w = ED_INSPECTOR_PANEL - 16;
    *h = ED_INSPECTOR_TEXTURE_WINDOW_HEIGHT;    // long def!! urgg
}

static void drawTextureTargetButton(int x, int y, int w, int h, const char *text, int active)
{
    uint8_t bg = active ? ED_UI_BTN_BG_ACTIVE : ED_UI_BTN_BG;  // active button
    uint8_t border = active ? ED_UI_BTN_BORDER_ACTIVE : ED_UI_BTN_BORDER;
    uint8_t textCol = active ? 16: ED_UI_BTN_TEXT;

    drawRect(x, y, w, h, bg);
    drawRectL(x, y, w, h, border);
    drawText(x + 4, y + 2, text, textCol);
}

static int getTextureBrowserVisibleRows(void)
{
    return TEX_BROWSER_ROWS;
}

static int getTextureBrowserTotalRows(void)
{
    return (TEXTURE_LIBRARY_COUNT + TEX_BROWSER_COLS - 1) / TEX_BROWSER_COLS;
}

static int getTextureBrowserMaxRowOffset(void)
{
    int maxOff = getTextureBrowserTotalRows() - getTextureBrowserVisibleRows();
    if (maxOff < 0) maxOff = 0;
    return maxOff;
}

static void clampTextureBrowserOffset(void)
{
    int maxOff = getTextureBrowserMaxRowOffset();

    if (g_ed.textureBrowserOffset < 0) {
        g_ed.textureBrowserOffset = 0;
    }
    if (g_ed.textureBrowserOffset > maxOff) {
        g_ed.textureBrowserOffset = maxOff;
    }
}

static void getTextureBrowserGridRect(int *x, int *y, int *w, int *h)
{
    int bx, by, bw, bh;

    getTextureBrowserRect(&bx, &by, &bw, &bh);

    *x = bx + 8;
    *y = by + TEX_BROWSER_GRID_Y;
    *w = (TEX_BROWSER_COLS * TEX_BROWSER_CELL_W);
    *h = (TEX_BROWSER_ROWS * TEX_BROWSER_CELL_H);
}

static void getTextureBrowserScrollbarRect(int *x, int *y, int *w, int *h)
{
    int bx, by, bw, bh;

    getTextureBrowserRect(&bx, &by, &bw, &bh);

    *w = TEX_BROWSER_SCROLL_W;

    /* keep scrollbar aligned to the texture grid height,
       but inset it slightly so the thumb does not look too tall */
    *h = (TEX_BROWSER_ROWS * TEX_BROWSER_CELL_H) - 12;
    if (*h < 16) *h = 16;

    *x = bx + bw - TEX_BROWSER_SCROLL_W - TEX_BROWSER_SCROLL_PAD;
    *y = by + TEX_BROWSER_GRID_Y;
}

static void getTextureBrowserThumbRect(int *x, int *y, int *w, int *h)
{
    int sx, sy, sw, sh;
    int totalRows = getTextureBrowserTotalRows();
    int visibleRows = getTextureBrowserVisibleRows();
    int maxOffset = getTextureBrowserMaxRowOffset();

    getTextureBrowserScrollbarRect(&sx, &sy, &sw, &sh);

    *x = sx;
    *w = sw;

    if (totalRows <= 0 || visibleRows >= totalRows) {
        *y = sy;
        *h = sh;
        return;
    }

    *h = (sh * visibleRows) / totalRows;

    /* minimum thumb size */
    if (*h < 14) *h = 14;
    if (*h > sh) *h = sh;

    if (maxOffset <= 0) {
        *y = sy;
        return;
    }

    *y = sy + ((sh - *h) * g_ed.textureBrowserOffset) / maxOffset;
}


static int textureBrowserOffsetFromThumbTopY(int thumbTopY)
{
    int sx, sy, sw, sh;
    int tx, ty, tw, th;
    int maxOffset;
    int travel;
    int rel;

    getTextureBrowserScrollbarRect(&sx, &sy, &sw, &sh);
    getTextureBrowserThumbRect(&tx, &ty, &tw, &th);

    maxOffset = getTextureBrowserMaxRowOffset();
    travel = sh - th;

    if (travel <= 0 || maxOffset <= 0) {
        return 0;
    }

    rel = thumbTopY - sy;
    if (rel < 0) rel = 0;
    if (rel > travel) rel = travel;

    return (rel * maxOffset + (travel / 2)) / travel;
}



static void drawTextureBrowser(void)
{
    int bx, by, bw, bh;
    int cellStartIndex;
    int tx, ty;
    int selectedIndex;

    clampTextureBrowserOffset();
    getTextureBrowserRect(&bx, &by, &bw, &bh);

    drawRect(bx, by, bw, bh, 16);
    drawRectL(bx, by, bw, bh, ED_UI_BORDER);

    drawText(bx + 6, by + 4, "TEXTURE EXPLORER", 29);
    drawText(bx + 170, by + 4, textureTargetName((TextureTarget)g_ed.textureBrowserTarget), ED_TEXT_COL);

    /* target select row */
    {
        int byBtn = by + 22;

        if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
            drawTextureTargetButton(bx + 6,   byBtn, 72, 20, "Upper",  g_ed.textureBrowserTarget == TEX_TARGET_WALL_UPPER);
            drawTextureTargetButton(bx + 84,  byBtn, 72, 20, "Middle", g_ed.textureBrowserTarget == TEX_TARGET_WALL_MIDDLE);
            drawTextureTargetButton(bx + 162, byBtn, 72, 20, "Lower",  g_ed.textureBrowserTarget == TEX_TARGET_WALL_LOWER);
        }
        else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
            drawTextureTargetButton(bx + 6,  byBtn, 72, 20, "Floor",   g_ed.textureBrowserTarget == TEX_TARGET_SECTOR_FLOOR);
            drawTextureTargetButton(bx + 84, byBtn, 72, 20, "Ceiling", g_ed.textureBrowserTarget == TEX_TARGET_SECTOR_CEIL);
        }
        else {
            drawTextureTargetButton(bx + 6,   byBtn, 72, 20, "F.Floor", g_ed.textureBrowserTarget == TEX_TARGET_DEFAULT_SECTOR_FLOOR);
            drawTextureTargetButton(bx + 84,  byBtn, 72, 20, "F.Ceil",  g_ed.textureBrowserTarget == TEX_TARGET_DEFAULT_SECTOR_CEIL);
            drawTextureTargetButton(bx + 162, byBtn, 72, 20, "W.Up",    g_ed.textureBrowserTarget == TEX_TARGET_DEFAULT_WALL_UPPER);
            drawTextureTargetButton(bx + 240, byBtn, 72, 20, "W.Mid",   g_ed.textureBrowserTarget == TEX_TARGET_DEFAULT_WALL_MIDDLE);
            drawTextureTargetButton(bx + 318, byBtn, 72, 20, "W.Low",   g_ed.textureBrowserTarget == TEX_TARGET_DEFAULT_WALL_LOWER);
        }
    }


    cellStartIndex = g_ed.textureBrowserOffset * TEX_BROWSER_COLS;
    selectedIndex = textureTargetCurrentIndex((TextureTarget)g_ed.textureBrowserTarget);

    for (ty = 0; ty < TEX_BROWSER_ROWS; ty++) {
        for (tx = 0; tx < TEX_BROWSER_COLS; tx++) {
            int slot = (ty * TEX_BROWSER_COLS) + tx;
            int texIndex = cellStartIndex + slot;
            int cx = bx + 8 + (tx * TEX_BROWSER_CELL_W);
            int cy = by + TEX_BROWSER_GRID_Y + (ty * TEX_BROWSER_CELL_H);

            if (texIndex >= TEXTURE_LIBRARY_COUNT) {
                continue;
            }

            drawRect(cx - 2, cy - 2, 44, 44, 0);

            if (texIndex == selectedIndex) {
                drawRectL(cx - 2, cy - 2, 44, 44, 27);
                drawRectL(cx - 1, cy - 1, 42, 42, 27);
                drawRectL(cx, cy, 40, 40, 27);
            } else {
                drawRectL(cx - 2, cy - 2, 44, 44, 6);
            }

            drawTextureThumb(cx + 4, cy + 4, getTexturePtr((uint8_t)texIndex));

            {
                char tbuf[16];
                snprintf(tbuf, sizeof(tbuf), "$%02X", texIndex);
                drawText(cx + 45, cy + 10, tbuf, ED_TEXT_COL);
            }
        }
    }

    /* scrollbar */
    {
        int sx, sy, sw, sh;
        int tx, ty, tw, th;

        getTextureBrowserScrollbarRect(&sx, &sy, &sw, &sh);
        getTextureBrowserThumbRect(&tx, &ty, &tw, &th);

        drawRect(sx, sy, sw, sh, ED_COLOUR_SCROLL_BAR_BG);
        drawRectL(sx, sy, sw, sh, ED_COLOUR_SCROLL_BAR_FRAME);


        drawRect(tx + 1, ty + 1, tw - 2, th - 2, ED_COLOUR_SCROLL_BAR);
        drawRectL(tx, ty, tw, th, ED_COLOUR_SCROLL_BAR_FRAME);
    }
}

static int handleTextureBrowserMouse(int mouseX, int mouseY, int leftDown, int leftPressed, int mouseWheelY)
{
    int bx, by, bw, bh;

    getTextureBrowserRect(&bx, &by, &bw, &bh);

    /* keep dragging even if mouse leaves the browser rect */
    if (g_ed.textureScrollbarDragging) {
        int sx, sy, sw, sh;
        int tx, ty, tw, th;
        int newThumbTop;

        if (!leftDown) {
            g_ed.textureScrollbarDragging = 0;
            return 1;
        }

        getTextureBrowserScrollbarRect(&sx, &sy, &sw, &sh);
        getTextureBrowserThumbRect(&tx, &ty, &tw, &th);

        newThumbTop = mouseY - g_ed.textureScrollbarDragOffsetY;
        g_ed.textureBrowserOffset = textureBrowserOffsetFromThumbTopY(newThumbTop);
        clampTextureBrowserOffset();
        return 1;
    }

    if (pointInRectLocal(mouseX, mouseY, bx, by, bw, bh)) {
        if (mouseWheelY != 0) {
            g_ed.textureBrowserOffset -= mouseWheelY;
            clampTextureBrowserOffset();
        }

        if (leftPressed) {
            int byBtn = by + 22;

            if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
                if (pointInRectLocal(mouseX, mouseY, bx + 6,   byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_WALL_UPPER;  return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 84,  byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_WALL_MIDDLE; return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 162, byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_WALL_LOWER;  return 1; }
            }
            else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
                if (pointInRectLocal(mouseX, mouseY, bx + 6,  byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_SECTOR_FLOOR; return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 84, byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_SECTOR_CEIL;  return 1; }
            }
            else {
                if (pointInRectLocal(mouseX, mouseY, bx + 6,   byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_SECTOR_FLOOR; return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 84,  byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_SECTOR_CEIL;  return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 162, byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_UPPER;   return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 240, byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_MIDDLE;  return 1; }
                if (pointInRectLocal(mouseX, mouseY, bx + 318, byBtn, 72, 20)) { g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_LOWER;   return 1; }
            }

            /* scrollbar click / drag */
            {
                int sx, sy, sw, sh;
                int tx, ty, tw, th;

                getTextureBrowserScrollbarRect(&sx, &sy, &sw, &sh);
                getTextureBrowserThumbRect(&tx, &ty, &tw, &th);

                /* clicked directly on thumb -> begin drag */
                if (pointInRectLocal(mouseX, mouseY, tx, ty, tw, th)) {
                    g_ed.textureScrollbarDragging = 1;
                    g_ed.textureScrollbarDragOffsetY = mouseY - ty;
                    return 1;
                }

                /* clicked on scrollbar track -> page thumb to that position and start drag */
                if (pointInRectLocal(mouseX, mouseY, sx, sy, sw, sh)) {
                    int newThumbTop = mouseY - (th / 2);

                    g_ed.textureBrowserOffset = textureBrowserOffsetFromThumbTopY(newThumbTop);
                    clampTextureBrowserOffset();

                    getTextureBrowserThumbRect(&tx, &ty, &tw, &th);
                    g_ed.textureScrollbarDragging = 1;
                    g_ed.textureScrollbarDragOffsetY = mouseY - ty;
                    return 1;
                }
            }

            /* texture cell click */
            {
                int tx, ty;
                int cellStartIndex = g_ed.textureBrowserOffset * TEX_BROWSER_COLS;

                for (ty = 0; ty < TEX_BROWSER_ROWS; ty++) {
                    for (tx = 0; tx < TEX_BROWSER_COLS; tx++) {
                        int slot = (ty * TEX_BROWSER_COLS) + tx;
                        int texIndex = cellStartIndex + slot;
                        int cx = bx + 8 + (tx * TEX_BROWSER_CELL_W);
                        int cy = by + TEX_BROWSER_GRID_Y + (ty * TEX_BROWSER_CELL_H);

                        (void)slot;

                        if (texIndex >= TEXTURE_LIBRARY_COUNT) {
                            continue;
                        }

                        if (pointInRectLocal(mouseX, mouseY, cx - 2, cy - 2, 44, 44)) {
                            pushUndoState();
                            textureTargetApply((TextureTarget)g_ed.textureBrowserTarget, (uint8_t)texIndex);
                            return 1;
                        }
                    }
                }
            }
        }

        return 1;
    }

    return 0;
}



//////////////////////////////////////////////////////////////
static void toggleWallTexFlag(int wallIndex, uint32_t flag)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;
    if (flag == 0) return;

    w = &g_edMap.walls[wallIndex];
    w->tex_flags ^= flag;
}



static void validatorAddLineEx(uint8_t targetType, int targetIndex, const char *fmt, ...)
{
    va_list ap;
    int idx;

    if (g_ed.validatorIssueCount < 0) {
        g_ed.validatorIssueCount = 0;
    }

    if (g_ed.validatorIssueCount >= 64) {
        return;
    }

    idx = g_ed.validatorIssueCount;

    g_ed.validatorTargetType[idx] = targetType;
    g_ed.validatorTargetIndex[idx] = targetIndex;

    va_start(ap, fmt);
    vsnprintf(g_ed.validatorLines[idx],
              sizeof(g_ed.validatorLines[idx]),
              fmt, ap);
    va_end(ap);

    g_ed.validatorIssueCount++;
}



static void validatorSelectIssue(int issueIndex)
{
    uint8_t type;
    int idx;

    if (issueIndex < 0 || issueIndex >= g_ed.validatorIssueCount) {
        return;
    }

    g_ed.validatorSelectedIssue = issueIndex;

    type = g_ed.validatorTargetType[issueIndex];
    idx  = g_ed.validatorTargetIndex[issueIndex];

    clearAllSelections();

    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = -1;

    switch (type) {
        case ED_VAL_TARGET_VERTEX:
            if (idx >= 0 && idx < g_edMap.vertCount) {
                g_ed.selectionType = ED_SEL_VERTEX;
                g_ed.selectedVert = idx;
                g_ed.hoverVert = idx;

                g_ed.camX = g_edMap.verts[idx].x;
                g_ed.camY = g_edMap.verts[idx].y;
            }
            break;

        case ED_VAL_TARGET_WALL:
            if (idx >= 0 && idx < g_edMap.wallCount) {
                const EdWall *w = &g_edMap.walls[idx];
                const EdVec2 *a = &g_edMap.verts[w->v0];
                const EdVec2 *b = &g_edMap.verts[w->v1];

                g_ed.selectionType = ED_SEL_WALL;
                g_ed.selectedWall = idx;
                g_ed.hoverWall = idx;

                g_ed.camX = (a->x + b->x) * 0.5f;
                g_ed.camY = (a->y + b->y) * 0.5f;
            }
            break;

        case ED_VAL_TARGET_SECTOR:
            if (idx >= 0 && idx < g_edMap.sectorCount) {
                const EdSector *sec = &g_edMap.sectors[idx];
                float minX, minY, maxX, maxY;
                int found = 0;

                g_ed.selectionType = ED_SEL_SECTOR;
                g_ed.selectedSector = idx;
                g_ed.hoverSector = idx;

                for (int i = 0; i < sec->boundaryCount; i++) {
                    const EdWall *w = &g_edMap.walls[sec->wallStart + i];
                    const EdVec2 *v = &g_edMap.verts[w->v0];

                    if (!found) {
                        minX = maxX = v->x;
                        minY = maxY = v->y;
                        found = 1;
                    } else {
                        if (v->x < minX) minX = v->x;
                        if (v->x > maxX) maxX = v->x;
                        if (v->y < minY) minY = v->y;
                        if (v->y > maxY) maxY = v->y;
                    }
                }

                if (found) {
                    g_ed.camX = (minX + maxX) * 0.5f;
                    g_ed.camY = (minY + maxY) * 0.5f;
                }
            }
            break;

        default:
            break;
    }
}



static void validatorNextIssue(void)
{
    if (g_ed.validatorIssueCount <= 0) return;

    if (g_ed.validatorSelectedIssue < 0 || g_ed.validatorSelectedIssue >= g_ed.validatorIssueCount) {
        g_ed.validatorSelectedIssue = 0;
    } else {
        g_ed.validatorSelectedIssue++;
        if (g_ed.validatorSelectedIssue >= g_ed.validatorIssueCount) {
            g_ed.validatorSelectedIssue = 0;
        }
    }

    validatorSelectIssue(g_ed.validatorSelectedIssue);
}

static void validatorPrevIssue(void)
{
    if (g_ed.validatorIssueCount <= 0) return;

    if (g_ed.validatorSelectedIssue < 0 || g_ed.validatorSelectedIssue >= g_ed.validatorIssueCount) {
        g_ed.validatorSelectedIssue = g_ed.validatorIssueCount - 1;
    } else {
        g_ed.validatorSelectedIssue--;
        if (g_ed.validatorSelectedIssue < 0) {
            g_ed.validatorSelectedIssue = g_ed.validatorIssueCount - 1;
        }
    }

    validatorSelectIssue(g_ed.validatorSelectedIssue);
}

static void validatorAddLine(const char *fmt, ...)
{
    va_list ap;
    int idx;

    if (g_ed.validatorIssueCount < 0) {
        g_ed.validatorIssueCount = 0;
    }

    if (g_ed.validatorIssueCount >= 64) {
        return;
    }

    idx = g_ed.validatorIssueCount;

    g_ed.validatorTargetType[idx] = ED_VAL_TARGET_NONE;
    g_ed.validatorTargetIndex[idx] = -1;

    va_start(ap, fmt);
    vsnprintf(g_ed.validatorLines[idx],
              sizeof(g_ed.validatorLines[idx]),
              fmt, ap);
    va_end(ap);

    g_ed.validatorIssueCount++;
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

    g_ed.textureScrollbarDragging = 0;
    g_ed.textureScrollbarDragOffsetY = 0;

    g_ed.splitPreviewValid = 0;
}

static void resetUndoRedoHistory(void)
{
    g_undoCount = 0;
    g_redoCount = 0;
    g_ed.undoHistoryVisible = 0;
    g_ed.undoHistorySelectedPos = 0;
    g_ed.undoHistoryScrollPos = 0;
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

static int getUndoHistoryMaxScrollPos(void)
{
    int maxScroll = g_undoCount - ED_UNDO_HISTORY_VISIBLE_ROWS;
    if (maxScroll < 0) maxScroll = 0;
    return maxScroll;
}

static void clampUndoHistoryPopupState(void)
{
    const int maxScroll = getUndoHistoryMaxScrollPos();

    if (g_undoCount <= 0) {
        g_ed.undoHistoryVisible = 0;
        g_ed.undoHistorySelectedPos = 0;
        g_ed.undoHistoryScrollPos = 0;
        return;
    }

    if (g_ed.undoHistorySelectedPos < 0) {
        g_ed.undoHistorySelectedPos = 0;
    }
    if (g_ed.undoHistorySelectedPos >= g_undoCount) {
        g_ed.undoHistorySelectedPos = g_undoCount - 1;
    }

    if (g_ed.undoHistoryScrollPos < 0) {
        g_ed.undoHistoryScrollPos = 0;
    }
    if (g_ed.undoHistoryScrollPos > maxScroll) {
        g_ed.undoHistoryScrollPos = maxScroll;
    }

    if (g_ed.undoHistorySelectedPos < g_ed.undoHistoryScrollPos) {
        g_ed.undoHistoryScrollPos = g_ed.undoHistorySelectedPos;
    }
    if (g_ed.undoHistorySelectedPos >= (g_ed.undoHistoryScrollPos + ED_UNDO_HISTORY_VISIBLE_ROWS)) {
        g_ed.undoHistoryScrollPos = g_ed.undoHistorySelectedPos - ED_UNDO_HISTORY_VISIBLE_ROWS + 1;
    }

    if (g_ed.undoHistoryScrollPos > maxScroll) {
        g_ed.undoHistoryScrollPos = maxScroll;
    }
}

static void appendRedoSnapshot(const EditorSnapshot *snapshot)
{
    if (!snapshot) return;

    if (g_redoCount == ED_HISTORY_MAX) {
        memmove(&g_redoStack[0], &g_redoStack[1], sizeof(EditorSnapshot) * (ED_HISTORY_MAX - 1));
        g_redoCount = ED_HISTORY_MAX - 1;
    }

    g_redoStack[g_redoCount] = *snapshot;
    g_redoCount++;
}

static int undoHistoryStackIndexFromPos(int pos)
{
    if (pos < 0 || pos >= g_undoCount) return -1;
    return g_undoCount - 1 - pos;
}

static void describeSnapshotSummary(const EditorSnapshot *snapshot, char *out, size_t outSize)
{
    const char *focus = "Map";

    if (!out || outSize == 0) return;

    out[0] = '\0';

    if (!snapshot) {
        snprintf(out, outSize, "Unknown snapshot");
        return;
    }

    if (snapshot->selectionType == ED_SEL_WALL &&
        snapshot->selectedWall >= 0 &&
        snapshot->selectedWall < snapshot->map.wallCount) {
        static char wallBuf[32];
        snprintf(wallBuf, sizeof(wallBuf), "Wall %d", snapshot->selectedWall);
        focus = wallBuf;
    } else if (snapshot->selectionType == ED_SEL_SECTOR &&
               snapshot->selectedSector >= 0 &&
               snapshot->selectedSector < snapshot->map.sectorCount) {
        static char sectorBuf[32];
        snprintf(sectorBuf, sizeof(sectorBuf), "Sector %d", snapshot->selectedSector);
        focus = sectorBuf;
    } else if (snapshot->selectionType == ED_SEL_VERTEX &&
               snapshot->selectedVert >= 0 &&
               snapshot->selectedVert < snapshot->map.vertCount) {
        static char vertBuf[32];
        snprintf(vertBuf, sizeof(vertBuf), "Vertex %d", snapshot->selectedVert);
        focus = vertBuf;
    } else if (snapshot->selectedVertCount > 0) {
        static char multiBuf[48];
        snprintf(multiBuf, sizeof(multiBuf), "Multi-select (%d verts)", snapshot->selectedVertCount);
        focus = multiBuf;
    } else if (snapshot->draftCount > 0) {
        static char draftBuf[48];
        snprintf(draftBuf, sizeof(draftBuf), "Draft (%d pts)", snapshot->draftCount);
        focus = draftBuf;
    } else if (snapshot->map.vertCount == 0 &&
               snapshot->map.wallCount == 0 &&
               snapshot->map.sectorCount == 0) {
        focus = "Blank map";
    }

    snprintf(out, outSize,
             "%s  |  V:%d W:%d S:%d",
             focus,
             snapshot->map.vertCount,
             snapshot->map.wallCount,
             snapshot->map.sectorCount);
}

static void closeUndoHistoryPopup(void)
{
    g_ed.undoHistoryVisible = 0;
    g_ed.undoHistorySelectedPos = 0;
    g_ed.undoHistoryScrollPos = 0;
    rc3dGuiDirty();
}

static void openUndoHistoryPopup(void)
{
    if (g_undoCount <= 0) {
        setEditorStatus("Undo history is empty");
        return;
    }

    g_ed.undoHistoryVisible = 1;
    g_ed.undoHistorySelectedPos = 0;
    g_ed.undoHistoryScrollPos = 0;
    g_ed.draggingPan = 0;
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.textureScrollbarDragging = 0;
    clampUndoHistoryPopupState();
    rc3dGuiDirty();
}

static void performUndoToHistoryPosition(int pos)
{
    EditorSnapshot current;
    char summary[160];
    const int targetIndex = undoHistoryStackIndexFromPos(pos);

    if (targetIndex < 0) return;

    captureSnapshot(&current);
    appendRedoSnapshot(&current);

    for (int i = g_undoCount - 1; i > targetIndex; i--) {
        appendRedoSnapshot(&g_undoStack[i]);
    }

    describeSnapshotSummary(&g_undoStack[targetIndex], summary, sizeof(summary));

    g_undoCount = targetIndex;
    restoreSnapshot(&g_undoStack[targetIndex]);
    closeUndoHistoryPopup();
    setEditorStatus(summary);
}

static void getUndoHistoryPopupRect(int *x, int *y, int *w, int *h)
{
    if (x) *x = (EDIT_VIEW_PORT_WIDTH - ED_UNDO_HISTORY_POPUP_W) / 2;
    if (y) *y = (EDIT_VIEW_PORT_HEIGHT - ED_UNDO_HISTORY_POPUP_H) / 2;
    if (w) *w = ED_UNDO_HISTORY_POPUP_W;
    if (h) *h = ED_UNDO_HISTORY_POPUP_H;
}

static int handleUndoHistoryPopupInput(const uint8_t *keys,
                                       int mouseX,
                                       int mouseY,
                                       int leftPressed,
                                       int mouseWheelY)
{
    int px, py, pw, ph;
    const int ctrlDown = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];

    if (!g_ed.undoHistoryVisible) return 0;

    if (g_undoCount <= 0) {
        closeUndoHistoryPopup();
        return 1;
    }

    getUndoHistoryPopupRect(&px, &py, &pw, &ph);

    if (keyPressedOnce(keys, SDL_SCANCODE_ESCAPE) ||
        keyPressedOnce(keys, SDL_SCANCODE_F11) ||
        (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_H))) {
        closeUndoHistoryPopup();
        return 1;
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_UP)) {
        g_ed.undoHistorySelectedPos--;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_DOWN)) {
        g_ed.undoHistorySelectedPos++;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_PAGEUP)) {
        g_ed.undoHistorySelectedPos -= ED_UNDO_HISTORY_VISIBLE_ROWS;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_PAGEDOWN)) {
        g_ed.undoHistorySelectedPos += ED_UNDO_HISTORY_VISIBLE_ROWS;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_HOME)) {
        g_ed.undoHistorySelectedPos = 0;
    }
    if (keyPressedOnce(keys, SDL_SCANCODE_END)) {
        g_ed.undoHistorySelectedPos = g_undoCount - 1;
    }

    if (mouseWheelY != 0) {
        g_ed.undoHistoryScrollPos -= mouseWheelY;
        g_ed.undoHistorySelectedPos -= mouseWheelY;
    }

    clampUndoHistoryPopupState();

    if (keyPressedOnce(keys, SDL_SCANCODE_RETURN) ||
        keyPressedOnce(keys, SDL_SCANCODE_KP_ENTER)) {
        performUndoToHistoryPosition(g_ed.undoHistorySelectedPos);
        return 1;
    }

    if (leftPressed) {
        const int rowsY = py + 56;
        const int rowsH = ED_UNDO_HISTORY_VISIBLE_ROWS * ED_UNDO_HISTORY_ROW_H;

        if (!pointInRectLocal(mouseX, mouseY, px, py, pw, ph)) {
            closeUndoHistoryPopup();
            return 1;
        }

        if (pointInRectLocal(mouseX, mouseY, px + 12, rowsY, pw - 24, rowsH)) {
            const int row = (mouseY - rowsY) / ED_UNDO_HISTORY_ROW_H;
            const int pos = g_ed.undoHistoryScrollPos + row;

            if (pos >= 0 && pos < g_undoCount) {
                g_ed.undoHistorySelectedPos = pos;
                clampUndoHistoryPopupState();
                performUndoToHistoryPosition(pos);
                return 1;
            }
        }
    }

    return 1;
}

static void finishEditorInputFrame(const uint8_t *keys,
                                   int leftDown,
                                   int rightDown,
                                   int middleDown,
                                   int mouseX,
                                   int mouseY)
{
    memcpy(g_ed.prevKeys, keys, SDL_NUM_SCANCODES);
    g_ed.prevLeftDown = leftDown;
    g_ed.prevRightDown = rightDown;
    g_ed.prevMiddleDown = middleDown;

    g_ed.lastMouseX = mouseX;
    g_ed.lastMouseY = mouseY;
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

static void clearWallTexFlags(EdWall *w)
{
    if (!w) return;
    w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
}

static float wrapAnglePositive(float angleRad)
{
    const float tau = (float)(M_PI * 2.0);

    angleRad = fmodf(angleRad, tau);
    if (angleRad < 0.0f) {
        angleRad += tau;
    }

    return angleRad;
}

static float wallTexAngleFromFlags(uint32_t texFlags)
{
    const uint32_t packed =
        (texFlags & RC3D_TEX_WALL_ANGLE_MASK) >> RC3D_TEX_WALL_ANGLE_SHIFT;

    return (float)packed * ((float)(M_PI * 2.0) / 65536.0f);
}

static uint32_t wallTexFlagsWithAngle(uint32_t texFlags, float angleRad)
{
    const float wrapped = wrapAnglePositive(angleRad);
    const uint32_t packed =
        ((uint32_t)lroundf((wrapped / (float)(M_PI * 2.0)) * 65536.0f)) & 0xFFFFu;

    texFlags &= ~RC3D_TEX_WALL_ANGLE_MASK;
    texFlags |= (packed << RC3D_TEX_WALL_ANGLE_SHIFT) & RC3D_TEX_WALL_ANGLE_MASK;
    return texFlags;
}

static float getWallTexAngle(const EdWall *w)
{
    if (!w) return 0.0f;
    return wallTexAngleFromFlags(w->tex_flags);
}

static void setWallTexAngle(EdWall *w, float angleRad)
{
    if (!w) return;
    w->tex_flags = wallTexFlagsWithAngle(w->tex_flags, angleRad);
}

static void setWallTexAngleEx(int wallIndex, float angleRad)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    w = &g_edMap.walls[wallIndex];
    setWallTexAngle(w, angleRad);
}

static void adjustWallTexAngle(int wallIndex, float deltaRad)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    w = &g_edMap.walls[wallIndex];
    setWallTexAngle(w, getWallTexAngle(w) + deltaRad);
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
                dst->tex_flags = RC3D_TEX_FLAG_DEFAULT;
            }

            g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
            g_edMap.sectors[newSectorIndex].wallCount = loopCount;
            g_edMap.sectors[newSectorIndex].boundaryCount = loopCount;
            g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
            g_edMap.sectors[newSectorIndex].ceilHeight  = g_ed.sectorCeil;
            g_edMap.sectors[newSectorIndex].floorColor  = g_ed.sectorFloorColor;
            g_edMap.sectors[newSectorIndex].ceilColor   = g_ed.sectorCeilColor;

            g_edMap.sectors[newSectorIndex].floorTexScaleX = 1.0f;
            g_edMap.sectors[newSectorIndex].floorTexScaleY = 1.0f;
            g_edMap.sectors[newSectorIndex].floorTexAngle  = 0.0f;

            g_edMap.sectors[newSectorIndex].ceilTexScaleX = 1.0f;
            g_edMap.sectors[newSectorIndex].ceilTexScaleY = 1.0f;
            g_edMap.sectors[newSectorIndex].ceilTexAngle  = 0.0f;

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



static void validateMap(void)
{
    int usedVerts[ED_MAX_VERTS];

    g_ed.validatorRan = 1;
    g_ed.validatorIssueCount = 0;
    g_ed.validatorSelectedIssue = -1;

    memset(g_ed.validatorLines, 0, sizeof(g_ed.validatorLines));
    memset(g_ed.validatorTargetType, 0, sizeof(g_ed.validatorTargetType));

    for (int i = 0; i < 64; i++) {
        g_ed.validatorTargetIndex[i] = -1;
    }

    memset(usedVerts, 0, sizeof(usedVerts));

    /* basic counts */
    if (g_edMap.vertCount < 0 || g_edMap.vertCount > ED_MAX_VERTS) {
        validatorAddLine("Invalid vertex count: %d", g_edMap.vertCount);
    }

    if (g_edMap.wallCount < 0 || g_edMap.wallCount > ED_MAX_WALLS) {
        validatorAddLine("Invalid wall count: %d", g_edMap.wallCount);
    }

    if (g_edMap.sectorCount < 0 || g_edMap.sectorCount > ED_MAX_SECTORS) {
        validatorAddLine("Invalid sector count: %d", g_edMap.sectorCount);
    }

    /* walls */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];

        if (w->v0 < 0 || w->v0 >= g_edMap.vertCount) {
            validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                               "Wall %d has invalid v0: %d", i, w->v0);
            continue;
        }

        if (w->v1 < 0 || w->v1 >= g_edMap.vertCount) {
            validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                               "Wall %d has invalid v1: %d", i, w->v1);
            continue;
        }

        usedVerts[w->v0] = 1;
        usedVerts[w->v1] = 1;

        if (w->v0 == w->v1) {
            validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                               "Wall %d is zero-length (v0 == v1 == %d)", i, w->v0);
        }

        if (w->neighbour < -1 || w->neighbour >= g_edMap.sectorCount) {
            validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                               "Wall %d has invalid neighbour sector: %d", i, w->neighbour);
        }

        if (w->openTop < w->openBottom) {
            validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                               "Wall %d has openTop < openBottom", i);
        }

        if (w->neighbour >= 0) {
            const int other = findReversedWall(w->v0, w->v1);

            if (other < 0 || other == i) {
                validatorAddLineEx(ED_VAL_TARGET_WALL, i,
                                   "Portal wall %d has no reversed partner", i);
            }
        }
    }

    /* orphan verts */
    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (!usedVerts[i]) {
            validatorAddLineEx(ED_VAL_TARGET_VERTEX, i,
                               "Vertex %d is orphaned", i);
        }
    }

    /* sectors */
    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];

        if (sec->wallStart < 0 || sec->wallStart > g_edMap.wallCount) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has invalid wallStart: %d", s, sec->wallStart);
            continue;
        }

        if (sec->wallCount < 0 || (sec->wallStart + sec->wallCount) > g_edMap.wallCount) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has invalid wall range", s);
            continue;
        }

        if (sec->boundaryCount < 0 || sec->boundaryCount > sec->wallCount) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has invalid boundaryCount", s);
        }

        if (sec->boundaryCount < 3) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has less than 3 boundary walls", s);
        }

        if (sec->ceilHeight < sec->floorHeight + 0.1f) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has ceil too low / floor too high", s);
        }

        for (int i = 0; i < sec->boundaryCount; i++) {
            const int wi = sec->wallStart + i;
            const int nextWi = sec->wallStart + ((i + 1) % sec->boundaryCount);

            if (wi < 0 || wi >= g_edMap.wallCount || nextWi < 0 || nextWi >= g_edMap.wallCount) {
                validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                                   "Sector %d boundary index out of range", s);
                break;
            }

            if (g_edMap.walls[wi].v1 != g_edMap.walls[nextWi].v0) {
                validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                                   "Sector %d boundary broken between walls %d and %d",
                                   s, wi, nextWi);
                break;
            }
        }
    }

    /* start sector */
    if (g_edMap.startSector < -1 || g_edMap.startSector >= g_edMap.sectorCount) {
        validatorAddLine("Start sector invalid: %d", g_edMap.startSector);
    }

    if (g_ed.validatorIssueCount == 0) {
        validatorAddLine("No validator issues found");
    }

    if (g_ed.validatorIssueCount > 0) {
        validatorSelectIssue(0);
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
    w->tex_flags = RC3D_TEX_FLAG_DEFAULT;

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
        ow->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
    if (solidCol == 0) solidCol = g_ed.newWallMidColor;

    w->neighbour   = -1;
    w->openBottom  = 0.0f;
    w->openTop     = 0.0f;
    w->upperColor  = g_ed.newWallUpperColor;
    w->midColor    = solidCol;
    w->lowerColor  = g_ed.newWallLowerColor;
    w->flags       = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
    w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
    w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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

static int collectSelectedVertexPivot(float *outCx, float *outCy)
{
    float minX, minY, maxX, maxY;
    int found = 0;

    if (!outCx || !outCy) return 0;
    if (g_ed.selectedVertCount <= 0) return 0;

    minX = 0.0f;
    minY = 0.0f;
    maxX = 0.0f;
    maxY = 0.0f;

    for (int i = 0; i < g_edMap.vertCount; i++) {
        if (!g_ed.selectedVerts[i]) continue;

        if (!found) {
            minX = maxX = g_edMap.verts[i].x;
            minY = maxY = g_edMap.verts[i].y;
            found = 1;
        } else {
            if (g_edMap.verts[i].x < minX) minX = g_edMap.verts[i].x;
            if (g_edMap.verts[i].x > maxX) maxX = g_edMap.verts[i].x;
            if (g_edMap.verts[i].y < minY) minY = g_edMap.verts[i].y;
            if (g_edMap.verts[i].y > maxY) maxY = g_edMap.verts[i].y;
        }
    }

    if (!found) return 0;

    *outCx = (minX + maxX) * 0.5f;
    *outCy = (minY + maxY) * 0.5f;
    return 1;
}

static void rotateSelectedVertices(float angleRad)
{
    float cx, cy;
    float s, c;

    if (g_ed.selectionType != ED_SEL_NONE) return;
    if (g_ed.selectedVertCount <= 1) return;

    if (!collectSelectedVertexPivot(&cx, &cy)) return;

    s = sinf(angleRad);
    c = cosf(angleRad);

    for (int i = 0; i < g_edMap.vertCount; i++) {
        float dx, dy;
        float rx, ry;

        if (!g_ed.selectedVerts[i]) continue;

        dx = g_edMap.verts[i].x - cx;
        dy = g_edMap.verts[i].y - cy;

        rx = (dx * c) - (dy * s);
        ry = (dx * s) + (dy * c);

        g_edMap.verts[i].x = snapf(cx + rx);
        g_edMap.verts[i].y = snapf(cy + ry);
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
    resetUndoRedoHistory();

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
    g_ed.ui_validator_visable = 0;
    g_ed.validatorRan = 0;
    g_ed.validatorIssueCount = 0;
    memset(g_ed.validatorLines, 0, sizeof(g_ed.validatorLines));
    g_ed.validatorSelectedIssue = -1;
    memset(g_ed.validatorTargetType, 0, sizeof(g_ed.validatorTargetType));
    for (int i = 0; i < 64; i++) {
        g_ed.validatorTargetIndex[i] = -1;
    }

    for (int i = 0; i < TEXTURE_PARTS; i++) {
        textureviewLoadedIndex[i] = -1;
    }
    g_ed.textureBrowserOffset = 0;
    g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_MIDDLE;
    for (int i = 0; i < TEXTURE_LIBRARY_COUNT; i++) {
        g_textureCacheLoaded[i] = 0;
    }

    memset(g_ed.selectedVerts, 0, sizeof(g_ed.selectedVerts));
    g_ed.selectedVertCount = 0;

    g_ed.bUseVectorFill = 0;

    g_ed.boxSelecting = 0;
    g_ed.boxStartMouseX = 0;
    g_ed.boxStartMouseY = 0;
    g_ed.boxEndMouseX = 0;
    g_ed.boxEndMouseY = 0;

    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiStartWorldX = 0.0f;
    g_ed.dragMultiStartWorldY = 0.0f;
    g_ed.dragMultiVertCount = 0;

    g_ed.textureBrowserOffset = 0;
    g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_MIDDLE;
    g_ed.textureScrollbarDragging = 0;
    g_ed.textureScrollbarDragOffsetY = 0;

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
    g_ed.sectorFloorColor = ED_DEFAULT_SECTOR_FLOOR_TEX_ID;
    g_ed.sectorCeilColor = ED_DEFAULT_SECTOR_CEILING_TEX_ID;


    g_ed.newWallUpperColor = ED_DEFAULT_WALL_UPPER_TEX_ID;
    g_ed.newWallMidColor   = ED_DEFAULT_WALL_MED_TEX_ID;
    g_ed.newWallLowerColor = ED_DEFAULT_WALL_LOWER_TEX_ID;


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

    g_ed.requestQuit = 0;
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

int rc3dEditConsumeQuitRequest(void)
{
    int r = g_ed.requestQuit;
    g_ed.requestQuit = 0;
    return r;
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

        case ED_CONFIRM_QUIT:
            g_ed.requestQuit = 1;
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
        fprintf(f, "%d %d %d %.6f %.6f %u %u %u %u %u\n",
            w->v0, w->v1, w->neighbour,
            w->openBottom, w->openTop,
            (unsigned)w->upperColor,
            (unsigned)w->midColor,
            (unsigned)w->lowerColor,
            (unsigned)w->flags,
            (unsigned)w->tex_flags);
    }

    fprintf(f, "SECTORS %d\n", g_edMap.sectorCount);
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f, "%d %d %d %.6f %.6f %u %u %.6f %.6f %.6f %.6f %.6f %.6f\n",
                s->wallStart, s->wallCount, s->boundaryCount,
                s->floorHeight, s->ceilHeight,
                (unsigned)s->floorColor,
                (unsigned)s->ceilColor,
                s->floorTexScaleX,
                s->floorTexScaleY,
                s->floorTexAngle,
                s->ceilTexScaleX,
                s->ceilTexScaleY,
                s->ceilTexAngle);
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
        unsigned tex_flags;

        if (fscanf(f, "%d %d %d %f %f %u %u %u %u %u",
                &newMap.walls[i].v0,
                &newMap.walls[i].v1,
                &newMap.walls[i].neighbour,
                &newMap.walls[i].openBottom,
                &newMap.walls[i].openTop,
                &uc, &mc, &lc, &flags, &tex_flags) != 10) {
            fclose(f);
            return 0;
        }

        newMap.walls[i].upperColor = (uint8_t)uc;
        newMap.walls[i].midColor   = (uint8_t)mc;
        newMap.walls[i].lowerColor = (uint8_t)lc;
        newMap.walls[i].flags      = (uint8_t)flags;
        newMap.walls[i].tex_flags  = (uint32_t)tex_flags;
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

        if (fscanf(f, "%d %d %d %f %f %u %u %f %f %f %f %f %f",
                &newMap.sectors[i].wallStart,
                &newMap.sectors[i].wallCount,
                &newMap.sectors[i].boundaryCount,
                &newMap.sectors[i].floorHeight,
                &newMap.sectors[i].ceilHeight,
                &fc, &cc,
                &newMap.sectors[i].floorTexScaleX,
                &newMap.sectors[i].floorTexScaleY,
                &newMap.sectors[i].floorTexAngle,
                &newMap.sectors[i].ceilTexScaleX,
                &newMap.sectors[i].ceilTexScaleY,
                &newMap.sectors[i].ceilTexAngle) != 13) {
            fclose(f);
            return 0;
        }

        newMap.sectors[i].floorColor = (uint8_t)fc;
        newMap.sectors[i].ceilColor  = (uint8_t)cc;
    }

    fclose(f);

    /* only commit after full successful parse */
    g_edMap = newMap;
    resetUndoRedoHistory();

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
        uint8_t upper      = w->upperColor;
        uint8_t mid        = w->midColor;
        uint8_t lower      = w->lowerColor;
        uint8_t flags      = w->flags;
        uint32_t tex_flags = w->tex_flags;

        if (fwrite(&v0,         sizeof(v0),         1, f) != 1 ||
            fwrite(&v1,         sizeof(v1),         1, f) != 1 ||
            fwrite(&neighbour,  sizeof(neighbour),  1, f) != 1 ||
            fwrite(&openBottom, sizeof(openBottom), 1, f) != 1 ||
            fwrite(&openTop,    sizeof(openTop),    1, f) != 1 ||
            fwrite(&upper,      sizeof(upper),      1, f) != 1 ||
            fwrite(&mid,        sizeof(mid),        1, f) != 1 ||
            fwrite(&lower,      sizeof(lower),      1, f) != 1 ||
            fwrite(&flags,      sizeof(flags),      1, f) != 1 ||
            fwrite(&tex_flags,  sizeof(tex_flags),  1, f) != 1) {
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

        float floorTexScaleX  = s->floorTexScaleX;
        float floorTexScaleY  = s->floorTexScaleY;
        float floorTexAngle   = s->floorTexAngle;
        float ceilTexScaleX   = s->ceilTexScaleX;
        float ceilTexScaleY   = s->ceilTexScaleY;
        float ceilTexAngle    = s->ceilTexAngle;

        if (fwrite(&wallStart,      sizeof(wallStart),      1, f) != 1 ||
            fwrite(&wallCount,      sizeof(wallCount),      1, f) != 1 ||
            fwrite(&boundaryCount,  sizeof(boundaryCount),  1, f) != 1 ||
            fwrite(&floorHeight,    sizeof(floorHeight),    1, f) != 1 ||
            fwrite(&ceilHeight,     sizeof(ceilHeight),     1, f) != 1 ||
            fwrite(&floorColor,     sizeof(floorColor),     1, f) != 1 ||
            fwrite(&ceilColor,      sizeof(ceilColor),      1, f) != 1 ||
            fwrite(&floorTexScaleX, sizeof(floorTexScaleX), 1, f) != 1 ||
            fwrite(&floorTexScaleY, sizeof(floorTexScaleY), 1, f) != 1 ||
            fwrite(&floorTexAngle,  sizeof(floorTexAngle),  1, f) != 1 ||
            fwrite(&ceilTexScaleX,  sizeof(ceilTexScaleX),  1, f) != 1 ||
            fwrite(&ceilTexScaleY,  sizeof(ceilTexScaleY),  1, f) != 1 ||
            fwrite(&ceilTexAngle,   sizeof(ceilTexAngle),   1, f) != 1) {
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
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u, %u, %u, %uu },\n",
            w->v0, w->v1, w->neighbour, w->openBottom, w->openTop,
            (unsigned)w->upperColor,
            (unsigned)w->midColor,
            (unsigned)w->lowerColor,
            (unsigned)w->flags,
            (unsigned)w->tex_flags);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const RC3D_Sector g_sectors[] = {\n");
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f,
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff },\n",
            s->wallStart, s->wallCount, s->boundaryCount,
            s->floorHeight, s->ceilHeight,
            (unsigned)s->floorColor, (unsigned)s->ceilColor,
            s->floorTexScaleX, s->floorTexScaleY, s->floorTexAngle,
            s->ceilTexScaleX,  s->ceilTexScaleY,  s->ceilTexAngle);
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
        newWallsA[countA].tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
        newWallsB[countB].tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
            w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
        }

        g_edMap.sectors[g_edMap.sectorCount].wallStart = wallStart;
        g_edMap.sectors[g_edMap.sectorCount].wallCount = g_ed.draftCount;
        g_edMap.sectors[g_edMap.sectorCount].boundaryCount = g_ed.draftCount;
        g_edMap.sectors[g_edMap.sectorCount].floorHeight = g_ed.sectorFloor;
        g_edMap.sectors[g_edMap.sectorCount].ceilHeight = g_ed.sectorCeil;
        g_edMap.sectors[g_edMap.sectorCount].floorColor = g_ed.sectorFloorColor;
        g_edMap.sectors[g_edMap.sectorCount].ceilColor = g_ed.sectorCeilColor;

        g_edMap.sectors[g_edMap.sectorCount].floorTexScaleX = 1.0f;
        g_edMap.sectors[g_edMap.sectorCount].floorTexScaleY = 1.0f;
        g_edMap.sectors[g_edMap.sectorCount].floorTexAngle  = 0.0f;

        g_edMap.sectors[g_edMap.sectorCount].ceilTexScaleX = 1.0f;
        g_edMap.sectors[g_edMap.sectorCount].ceilTexScaleY = 1.0f;
        g_edMap.sectors[g_edMap.sectorCount].ceilTexAngle  = 0.0f;

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
            outerPieces[pieceCount].tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
        outerPieces[pieceCount].tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
            outerPieces[pieceCount].tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
            w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
                w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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
                w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
            }
        }

        g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
        g_edMap.sectors[newSectorIndex].wallCount = g_ed.draftCount;
        g_edMap.sectors[newSectorIndex].boundaryCount = g_ed.draftCount;
        g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
        g_edMap.sectors[newSectorIndex].ceilHeight = g_ed.sectorCeil;
        g_edMap.sectors[newSectorIndex].floorColor = g_ed.sectorFloorColor;
        g_edMap.sectors[newSectorIndex].ceilColor = g_ed.sectorCeilColor;

        g_edMap.sectors[newSectorIndex].floorTexScaleX = 1.0f;
        g_edMap.sectors[newSectorIndex].floorTexScaleY = 1.0f;
        g_edMap.sectors[newSectorIndex].floorTexAngle  = 0.0f;

        g_edMap.sectors[newSectorIndex].ceilTexScaleX = 1.0f;
        g_edMap.sectors[newSectorIndex].ceilTexScaleY = 1.0f;
        g_edMap.sectors[newSectorIndex].ceilTexAngle  = 0.0f;

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
        w->tex_flags = RC3D_TEX_FLAG_DEFAULT;
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

restart_scan:
    for (int i = 0; i < g_edMap.vertCount; i++) {
        for (int j = i + 1; j < g_edMap.vertCount; j++) {
            const float dx = g_edMap.verts[i].x - g_edMap.verts[j].x;
            const float dy = g_edMap.verts[i].y - g_edMap.verts[j].y;
            const float d2 = (dx * dx) + (dy * dy);

            if (d2 <= (epsilon * epsilon)) {
                mergeVertexInto(j, i);
                mergedAny = 1;
                goto restart_scan;
            }
        }
    }

    return mergedAny;
}


static void repairMapTopology(void)
{
    float exactEps = 0.0005f;
    float nearEps  = g_ed.currentGridStep * 0.05f;
    int mergedAny = 0;

    if (nearEps < 0.001f) nearEps = 0.001f;
    if (nearEps > 0.05f)  nearEps = 0.05f;

    clearMultiVertexSelection();

    /* first make wall layout sane before touching verts */
    rebuildSectorWallLayout();

    /* pass 1: crush exact/stacked duplicates */
    mergedAny |= mergeCloseVertices(exactEps);

    /* pass 2: small near-merge for almost-snapped junk */
    mergedAny |= mergeCloseVertices(nearEps);

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

    if (mergedAny) {
        setEditorStatus("Topolgy Repair: Repair merged overlapping / near vertices and rebuilt portals");
    } else {
        setEditorStatus("Topolgy Repair: Repair found nothing to merge");
    }
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


static uint8_t getSectorFillColour(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return ED_UI_BG;
    }

    if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector == sectorIndex) {
        return ED_COLOUR_SELECTED_SECTOR;
    }

    if (g_ed.hoverSector == sectorIndex) {
        return ED_GRID_MAJOR_COL;
    }

    /* use the sector floor colour as the base fill tint */
    return g_edMap.sectors[sectorIndex].floorColor;
}

static int getSectorFillStepY(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return 4;
    }

    /* denser fill for hovered/selected sectors */
    if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector == sectorIndex) {
        return 2;
    }

    if (g_ed.hoverSector == sectorIndex) {
        return 3;
    }

    return 4;
}

static void drawFilledSector2D(int sectorIndex)
{
    static int polyX[ED_MAX_WALLS];
    static int polyY[ED_MAX_WALLS];
    static int nodes[ED_MAX_WALLS];

    const EdSector *sec;
    int count;
    int minY, maxY;
    uint8_t fillCol;
    int stepY;

    if(g_ed.bUseVectorFill == 0) return;

    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    sec = &g_edMap.sectors[sectorIndex];
    if (sec->boundaryCount < 3) return;
    if (sec->boundaryCount > ED_MAX_WALLS) return;

    count = sec->boundaryCount;

    /* build polygon from ordered boundary walls using v0 vertices */
    for (int i = 0; i < count; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];

        if (w->v0 < 0 || w->v0 >= g_edMap.vertCount) return;

        worldToScreen(g_edMap.verts[w->v0].x, g_edMap.verts[w->v0].y, &polyX[i], &polyY[i]);
    }

    minY = polyY[0];
    maxY = polyY[0];

    for (int i = 1; i < count; i++) {
        if (polyY[i] < minY) minY = polyY[i];
        if (polyY[i] > maxY) maxY = polyY[i];
    }

    if (maxY < 0 || minY >= EDIT_VIEW_PORT_HEIGHT) return;

    minY = clampi_local(minY, 0, EDIT_VIEW_PORT_HEIGHT - 1);
    maxY = clampi_local(maxY, 0, EDIT_VIEW_PORT_HEIGHT - 1);

    fillCol = getSectorFillColour(sectorIndex);
    stepY   = getSectorFillStepY(sectorIndex);
    if (stepY < 1) stepY = 1;

    for (int y = minY; y <= maxY; y += stepY) {
        int nodeCount = 0;

        for (int i = 0, j = count - 1; i < count; j = i++) {
            const int yi = polyY[i];
            const int yj = polyY[j];
            const int xi = polyX[i];
            const int xj = polyX[j];

            if (((yi < y) && (yj >= y)) || ((yj < y) && (yi >= y))) {
                if (nodeCount < ED_MAX_WALLS) {
                    nodes[nodeCount++] = xi + (int)(((float)(y - yi) / (float)(yj - yi)) * (float)(xj - xi));
                }
            }
        }

        /* simple insertion sort */
        for (int i = 1; i < nodeCount; i++) {
            int v = nodes[i];
            int k = i - 1;
            while (k >= 0 && nodes[k] > v) {
                nodes[k + 1] = nodes[k];
                k--;
            }
            nodes[k + 1] = v;
        }

        for (int i = 0; i + 1 < nodeCount; i += 2) {
            int x0 = nodes[i];
            int x1 = nodes[i + 1];

            if (x0 > x1) {
                int t = x0;
                x0 = x1;
                x1 = t;
            }

            if (x1 < 0 || x0 >= EDIT_VIEW_PORT_WIDTH) {
                continue;
            }

            x0 = clampi_local(x0, 0, EDIT_VIEW_PORT_WIDTH - 1);
            x1 = clampi_local(x1, 0, EDIT_VIEW_PORT_WIDTH - 1);

            if (x1 >= x0) {
                drawLineDots(x0, y, x1, y, fillCol);
            }
        }
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

            drawLineGridDots(sx0, sy0, sx1, sy1, ED_GRID_MICRO_COL);
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

            drawLineGridDots(sx0, sy0, sx1, sy1, ED_GRID_MICRO_COL);
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
                "Wall: %d  Vertices: %d/%d  Sectors: %d/%d  Bottom: %.3f  Top: %.3f",
                wall_id, w->v0, w->v1, owner, w->neighbour,
                w->openBottom, w->openTop);
                
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);

        return;
    }

    if (sector_id >= 0) {
        const EdSector *s = &g_edMap.sectors[sector_id];

        snprintf(buf, sizeof(buf),
                 "Sector %d  Floor %.3f  Ceil %.3f  Walls %d  Boundary %d",
                 sector_id, s->floorHeight, s->ceilHeight,
                 s->wallCount, s->boundaryCount);
        drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, buf, ED_TEXT_COL);
        return;
    }

    drawText(8, EDIT_VIEW_PORT_HEIGHT + 4, "No hover / selection", ED_TEXT_COL);
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




static void drawBottomMenuBar(void)
{
    drawRect(ED_BOTTOMBAR_X,  ED_BOTTOMBAR_Y, ED_BOTTOMBAR_W, ED_BOTTOMBAR_H, ED_UI_BG);
    drawLine(ED_BOTTOMBAR_X,  ED_BOTTOMBAR_Y, ED_BOTTOMBAR_X + ED_BOTTOMBAR_W - 1, ED_BOTTOMBAR_Y, ED_UI_BORDER);
    drawLine(ED_BOTTOMBAR_X,  ED_BOTTOMBAR_Y + ED_BOTTOMBAR_H - 1,
             ED_BOTTOMBAR_X + ED_BOTTOMBAR_W - 1, ED_BOTTOMBAR_Y + ED_BOTTOMBAR_H - 1, ED_UI_BORDER);
    drawLine(ED_BOTTOMBAR_X,  ED_BOTTOMBAR_Y, ED_BOTTOMBAR_X, ED_BOTTOMBAR_Y + ED_BOTTOMBAR_H - 1, ED_UI_BORDER);
    drawLine(ED_BOTTOMBAR_X + ED_BOTTOMBAR_W - 1, ED_BOTTOMBAR_Y,
             ED_BOTTOMBAR_X + ED_BOTTOMBAR_W - 1, ED_BOTTOMBAR_Y + ED_BOTTOMBAR_H - 1, ED_UI_BORDER);

    //drawText(ED_TOPBAR_X + 8, ED_TOPBAR_Y + ED_UI_PAD, "F1-More | F10-quit |", ED_UI_TEXT_INFOTYPE);
}



static void drawValidatorPanel(void)
{
    int y = ED_PANEL_Y + ED_UI_PAD;
    const int x = ED_PANEL_X + ED_UI_PAD;
    int panelH = 0;

    for (int pass = 0; pass < 2; pass++) {
        panelH = y - ED_PANEL_Y + ED_UI_PAD;
        y = ED_PANEL_Y + ED_UI_PAD;

        drawRect(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_W, panelH, ED_UI_BG);
        drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y, ED_UI_BORDER);
        drawLine(ED_PANEL_X, ED_PANEL_Y + panelH - 1,
                 ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + panelH - 1, ED_UI_BORDER);
        drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X, ED_PANEL_Y + panelH - 1, ED_UI_BORDER);
        drawLine(ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y,
                 ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + panelH - 1, ED_UI_BORDER);

        drawText(x, y, "MAP VALIDATOR ::: F9 close ::: [ / ] cycle issues", 29);
        y += ED_ROW_STEP;

        if (!g_ed.validatorRan) {
            drawText(x, y, "Validator has not been run yet.", ED_TEXT_COL);
            y += ED_ROW_STEP;
        } else {
            char buf[96];

            snprintf(buf, sizeof(buf), "Issues: %d   Current: %d",
                     g_ed.validatorIssueCount,
                     (g_ed.validatorIssueCount > 0) ? (g_ed.validatorSelectedIssue + 1) : 0);
            drawText(x, y, buf, ED_TEXT_COL);
            y += ED_ROW_STEP;

            for (int i = 0; i < g_ed.validatorIssueCount && i < 64; i++) {
                uint8_t col = ED_TEXT_COL;

                if (i == g_ed.validatorSelectedIssue) {
                    drawRect(x - 4, y - 2, ED_PANEL_W - 20, ED_FONT_H + 4, 7);
                    col = 29;
                }

                drawText(x, y, g_ed.validatorLines[i], col);
                y += ED_ROW_STEP;

                if (y > (EDIT_VIEW_PORT_HEIGHT - ED_ROW_STEP)) {
                    drawText(x, y, "...more issues not shown", 30);
                    y += ED_ROW_STEP;
                    break;
                }
            }
        }
    }
}




static void drawExpandedEditorPanel(void)
{
    int y = ED_PANEL_Y + ED_UI_PAD;
    const int x = ED_PANEL_X + ED_UI_PAD;

    //int gHeight = ED_PANEL_H;
    int gHeight = 0;

    for(int crapLoop = 0; crapLoop< 2; crapLoop++){
        gHeight = y - ED_PANEL_Y + ED_UI_PAD;
        y = ED_PANEL_Y + ED_UI_PAD;
        drawRect(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_W, gHeight, ED_UI_BG);
        drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y, ED_UI_BORDER);
        drawLine(ED_PANEL_X, ED_PANEL_Y + gHeight - 1,
                ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + gHeight - 1, ED_UI_BORDER);
        drawLine(ED_PANEL_X, ED_PANEL_Y, ED_PANEL_X, ED_PANEL_Y + gHeight - 1, ED_UI_BORDER);
        drawLine(ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y,
                ED_PANEL_X + ED_PANEL_W - 1, ED_PANEL_Y + gHeight - 1, ED_UI_BORDER);

        drawText(x, y, "RC3D EDITOR ::: F12 to launch the test map", 31);
        y += ED_ROW_STEP;

        drawText(x, y, "LMB select/drag, RMB add draft point, MMB pan, Wheel zoom", ED_TEXT_COL);
        y += ED_ROW_STEP;

        drawText(x, y, "[ENTER] finish draft, [ESC] clear draft, [DEL] delete hovered", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[F6] auto-build sectors from closed inner wall loops", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[']  enable/disable filled view", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "CTRL+Z undo, CTRL+Y redo, F11 or CTRL+H history, [TAB] grid", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[F7] Repair Topology, [F8] clean map, [SHIFT+DRAG] drop = merge", ED_TEXT_COL);
        y += 6;//ED_ROW_STEP;

        if((g_ed.selectionType == ED_SEL_VERTEX) ||
             (g_ed.selectionType == ED_SEL_WALL) ||
             (g_ed.selectionType == ED_SEL_SECTOR) ||
            (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0))
        {
            drawText(x, y, "________________________________________________________________________________________________", ED_TEXT_COL);
            y += ED_ROW_STEP;
        } else 
            y+= 10;
        
        if(g_ed.selectionType == ED_SEL_VERTEX){
            drawText(x, y, "--- VERTEX HELP", 2);
            y += ED_ROW_STEP;
            drawText(x, y, "[ARROWS] 0.01   [SHIFT+ARROWS] 0.001   [ALT+ARROWS] 0.0001", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[SHIFT+DROP] onto another vertex = merge", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(g_ed.selectionType == ED_SEL_WALL){
            drawText(x, y, "--- WALL HELP", 2);
            y += ED_ROW_STEP;
            drawText(x, y, "[R]/[T] bottom level,  [Y]/[U] top level", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[Q]/[W] wall texture angle", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[A]/[S] upper texture, [D]/[F] middle texture, [H]/[J] lower texture", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[SPACE] split wall (insert vertex)   [5] extrude wall", ED_TEXT_COL);
            y += ED_ROW_STEP;
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
        if(g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0){
            drawText(x, y, "--- MULTI-SELECT HELP:", 2);
            y += ED_ROW_STEP;
            drawText(x, y, "Multi-select: [ARROW-LEFT]/[ARROW-RIGHT] rotate, SHIFT big step, ALT fine step", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }

        //snprintf(buf, sizeof(buf), "Start: %.2f %.2f   Angle: %.2f", g_edMap.startX, g_edMap.startY, g_edMap.startAngle);
        //drawText(x, y, buf, ED_TEXT_COL);
        //y += ED_ROW_STEP;

    }
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
    /* -------------------------------------------------- */
    /* draw sector fills first                            */
    /* -------------------------------------------------- */
    for (int s = 0; s < g_edMap.sectorCount; s++) {
        drawFilledSector2D(s);
    }

    /* -------------------------------------------------- */
    /* walls                                              */
    /* -------------------------------------------------- */
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];
        int x0, y0, x1, y1;

        worldToScreen(a->x, a->y, &x0, &y0);
        worldToScreen(b->x, b->y, &x1, &y1);

        {
            uint8_t c = ED_COLOUR_WALL;

            if (w->flags & RC3D_WALL_PORTAL) c = ED_PORTAL_COL;
            if (w->flags & RC3D_WALL_MIDDLE) c = ED_COLOUR_WALL;
            if ((w->flags & (RC3D_WALL_UPPER | RC3D_WALL_LOWER)) && !(w->flags & RC3D_WALL_MIDDLE) && !(w->flags & RC3D_WALL_PORTAL)) {
                c = 32;
            }

            /* base line */
            drawLine(x0, y0, x1, y1, c);

            /* perpendicular screen-space normal for highlight thickness */
            {
                int dx = x1 - x0;
                int dy = y1 - y0;
                int ox = 0;
                int oy = 0;

                if (dx != 0 || dy != 0) {
                    if (abs(dx) >= abs(dy)) {
                        /* mostly horizontal -> thicken vertically */
                        ox = 0;
                        oy = 1;
                    } else {
                        /* mostly vertical -> thicken horizontally */
                        ox = 1;
                        oy = 0;
                    }
                }

                /* hover wall */
                if (i == g_ed.hoverWall) {
                    drawLine(x0 - ox, y0 - oy, x1 - ox, y1 - oy, ED_COLOUR_HOVER_WALL);
                    drawLine(x0 + ox, y0 + oy, x1 + ox, y1 + oy, ED_COLOUR_HOVER_WALL);

                    drawRect(x0 - 2, y0 - 2, 5, 5, ED_COLOUR_HOVER_WALL);
                    drawRect(x1 - 2, y1 - 2, 5, 5, ED_COLOUR_HOVER_WALL);
                }

                /* selected wall */
                if (g_ed.selectionType == ED_SEL_WALL && i == g_ed.selectedWall) {
                    drawLine(x0 - (ox * 2), y0 - (oy * 2), x1 - (ox * 2), y1 - (oy * 2), 31);
                    drawLine(x0 + (ox * 2), y0 + (oy * 2), x1 + (ox * 2), y1 + (oy * 2), 31);

                    drawLine(x0 - ox, y0 - oy, x1 - ox, y1 - oy, ED_COLOUR_SELECTED_WALL);
                    drawLine(x0 + ox, y0 + oy, x1 + ox, y1 + oy, ED_COLOUR_SELECTED_WALL);

                    drawRectL(x0 - 4, y0 - 4, 9, 9, 31);
                    drawRectL(x1 - 4, y1 - 4, 9, 9, 31);
                    drawRect(x0 - 2, y0 - 2, 5, 5, ED_COLOUR_SELECTED_WALL);
                    drawRect(x1 - 2, y1 - 2, 5, 5, ED_COLOUR_SELECTED_WALL);
                }
            }

            if (!(w->flags & RC3D_WALL_PORTAL)) {
                drawWallNormal(i, c);
            }
        }
    }

    /* -------------------------------------------------- */
    /* vertices                                           */
    /* -------------------------------------------------- */
    for (int i = 0; i < g_edMap.vertCount; i++) {
        int sx, sy;
        uint8_t c = ED_COLOUR_VERTEX;

        worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);

        /* default */
        drawRect(sx - 2, sy - 2, 5, 5, c);

        /* multi-select */
        if (g_ed.selectedVerts[i]) {
            drawRect(sx - 3, sy - 3, 7, 7, 27);
            drawRect(sx - 1, sy - 1, 3, 3, 31);
        }

        /* hover */
        if (i == g_ed.hoverVert) {
            drawRectL(sx - 5, sy - 5, 11, 11, ED_COLOUR_VERTEX_HOVER);
            drawRect(sx - 3, sy - 3, 7, 7, ED_COLOUR_VERTEX_HOVER);
            //drawRect(sx - 1, sy - 1, 3, 3, 31);
        }

        /* selected vertex gets strongest highlight */
        if (g_ed.selectionType == ED_SEL_VERTEX && i == g_ed.selectedVert) {
            drawRectL(sx - 7, sy - 7, 15, 15, ED_COLOUR_VERTEX_SELECTED);
            //drawRectL(sx - 5, sy - 5, 11, 11, 27);
            drawRect(sx - 3, sy - 3, 7, 7, ED_COLOUR_VERTEX_SELECTED);
            //drawRect(sx - 1, sy - 1, 3, 3, 31);
        }
    }

    /* -------------------------------------------------- */
    /* draft                                              */
    /* -------------------------------------------------- */
    for (int i = 0; i < g_ed.draftCount; i++) {
        const EdVec2 *a = &g_edMap.verts[g_ed.draftVertIndices[i]];
        int sx, sy;
        worldToScreen(a->x, a->y, &sx, &sy);
        drawRect(sx - 2, sy - 2, 5, 5, ED_COLOUR_DRAFTWALL);

        if (i > 0) {
            const EdVec2 *b = &g_edMap.verts[g_ed.draftVertIndices[i - 1]];
            int dx0, dy0, dx1, dy1;
            worldToScreen(b->x, b->y, &dx0, &dy0);
            worldToScreen(a->x, a->y, &dx1, &dy1);
            drawLine(dx0, dy0, dx1, dy1, ED_COLOUR_DRAFTWALL);
        }
    }

    /* -------------------------------------------------- */
    /* selected sector boundary highlight                 */
    /* -------------------------------------------------- */
    if (g_ed.selectionType == ED_SEL_SECTOR &&
        g_ed.selectedSector >= 0 &&
        g_ed.selectedSector < g_edMap.sectorCount) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
        int found = 0;

        for (int i = 0; i < sec->boundaryCount; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            const EdVec2 *a = &g_edMap.verts[w->v0];
            const EdVec2 *b = &g_edMap.verts[w->v1];
            int x0, y0, x1, y1;

            worldToScreen(a->x, a->y, &x0, &y0);
            worldToScreen(b->x, b->y, &x1, &y1);

            {
                int dx = x1 - x0;
                int dy = y1 - y0;
                int ox = 0;
                int oy = 0;

                if (dx != 0 || dy != 0) {
                    if (abs(dx) >= abs(dy)) {
                        ox = 0;
                        oy = 1;
                    } else {
                        ox = 1;
                        oy = 0;
                    }
                }

                drawLine(x0 - ox, y0 - oy, x1 - ox, y1 - oy, 1);
                drawLine(x0 + ox, y0 + oy, x1 + ox, y1 + oy, 1);
                drawLine(x0, y0, x1, y1, ED_COLOUR_SELECTED_SECTOR);
            }

            if (!found) {
                minX = maxX = a->x;
                minY = maxY = a->y;
                found = 1;
            } else {
                if (a->x < minX) minX = a->x;
                if (a->x > maxX) maxX = a->x;
                if (a->y < minY) minY = a->y;
                if (a->y > maxY) maxY = a->y;
            }
        }

        if (found) {
            int cx, cy;
            worldToScreen((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, &cx, &cy);
            drawRectL(cx - 8, cy - 8, 17, 17, 31);
            drawRectL(cx - 5, cy - 5, 11, 11, ED_COLOUR_SELECTED_SECTOR);
        }
    }

    /* -------------------------------------------------- */
    /* split preview                                      */
    /* -------------------------------------------------- */
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


    btnx_off = 12;
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CUTTER,   btnx_off, ED_BOTTOMBAR_Y + 4, 142, ED_BTN_H, "F6: Sector Build"); btnx_off += 142+4;
    rcguiCreateButton(&g_ui, GUI_BTN_REPAIR_TOPOLOGY, btnx_off, ED_BOTTOMBAR_Y + 4, 162, ED_BTN_H, "F7: Topology Repair"); btnx_off += 162+4;
    rcguiCreateButton(&g_ui, GUI_BTN_CLEANMAP,        btnx_off, ED_BOTTOMBAR_Y + 4, 122, ED_BTN_H, "F8: Clean Map"); btnx_off += 122+4;
    rcguiCreateButton(&g_ui, GUI_BTN_MAPVALIDATOR,    btnx_off, ED_BOTTOMBAR_Y + 4, 152, ED_BTN_H, "F9: Map validator"); btnx_off += 152+4;
    rcguiCreateButton(&g_ui, GUI_BTN_LAUNCH_TEST_MAP, btnx_off, ED_BOTTOMBAR_Y + 4, 152, ED_BTN_H, "F12: Test Map"); btnx_off += 152+4;

    

    #define controloff 33
    #define controloffw 16 + (SCREEN_W - ED_INSPECTOR_PANEL)
    /* expanded panel button rows aligned to 16px font + 6px spacing */
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_SOLID,    controloffw + (84 * 0), 194 + controloff, 80, ED_BTN_H, "1:Solid");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_PORTAL,   controloffw + (84 * 1), 194 + controloff, 80, ED_BTN_H, "2:Portal");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_WINDOW,   controloffw + (84 * 2), 194 + controloff, 80, ED_BTN_H, "3:Window");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_DOOR,     controloffw + (84 * 3), 194 + controloff, 80, ED_BTN_H, "4:Door");
    //rcguiCreateButton(&g_ui, GUI_BTN_WALL_SPLIT,    controloffw + (84 * 4), 194 + controloff, 80, ED_BTN_H, "Split");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TRANSPARENCY, controloffw + (84 * 4), 194 + controloff, 80, ED_BTN_H, "Transp.");
    

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_XL, controloffw + (84 * 2), 256 + controloff, 80, ED_BTN_H, "Clamp XL");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_XR, controloffw + (84 * 3), 256 + controloff, 80, ED_BTN_H, "Clamp XR");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_YT, controloffw + (84 * 2), 288 + controloff, 80, ED_BTN_H, "Clamp YT");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_YB, controloffw + (84 * 3), 288 + controloff, 80, ED_BTN_H, "Clamp YB");

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 162 + controloffw, 320 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS,  194 + controloffw, 320 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 226 + controloffw, 320 + controloff, 60, 24, "Reset");
    

    // sector inspector UI - wall
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 182 + controloffw, 88 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS,  214 + controloffw, 88 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 182 + controloffw, 116 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS,  214 + controloffw, 116 + controloff, 24, 24, "+");


    // sector inspector UI - sector / heights
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 136 + controloffw, 114 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS,  164 + controloffw, 114 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS,  336 + controloffw, 114 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS,   364 + controloffw, 114 + controloff, 24, 24, "+");


    /* sector inspector UI - texture transform floor */
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SX_MINUS, 136 + controloffw, 228 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SX_PLUS,  164 + controloffw, 228 + controloff, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SY_MINUS, 336 + controloffw, 228 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SY_PLUS,  364 + controloffw, 228 + controloff, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_MINUS, 166 + controloffw, 258 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_PLUS,  194 + controloffw, 258 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_RESET, 226 + controloffw, 258 + controloff, 60, 24, "Reset");

    // sector inspector UI - texture transform ceiling
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SX_MINUS, 136 + controloffw, 316 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SX_PLUS,  164 + controloffw, 316 + controloff, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SY_MINUS, 336 + controloffw, 316 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SY_PLUS,  364 + controloffw, 316 + controloff, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_MINUS, 166 + controloffw, 348 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_PLUS,  194 + controloffw, 348 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_RESET, 226 + controloffw, 348 + controloff, 60, 24, "Reset");


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

static void doSectorCutter(){
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

extern void launchRaycastGame(const char *mapPath);
void doRunDemoGame(void){
    const char *mapPath = "./testmap.bin";
    if (!exportBinaryMap(mapPath)) {
        setEditorStatus("Failed to export test map");
        return;
    }

    launchRaycastGame(mapPath);
}

static void doRepairTopology(){
    pushUndoState();
    repairMapTopology();
}

static void doCleanMap(){
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

static void doValidateMap(){
    if (!g_ed.ui_validator_visable) {
        validateMap();
        g_ed.ui_validator_visable = 1;
        g_ed.ui_menu_visable = 0;
        
    } else {
        g_ed.ui_validator_visable = 0;
    }
}

static void doToggleGrid(void)
{
    g_ed.tinyGridEnabled = !g_ed.tinyGridEnabled;
    g_ed.currentGridStep = g_ed.tinyGridEnabled ? ED_GRID_STEP_TINY : ED_GRID_STEP;
}

static void doSaveMap(void)
{
    char path[1024];

    if (!editorSaveMapDialog(path, sizeof(path))) {
        setEditorStatus("Save cancelled");
        return;
    }

    if (!saveTextMap(path)) {
        setEditorStatus("Save failed");
        return;
    }

    setEditorStatus("Map saved");
}

static void doExportMap(void)
{
    char path[1024];
    int exportType;
    EditorMap savedMap;

    if (!editorExportMapDialog(path, sizeof(path), &exportType)) {
        setEditorStatus("Export cancelled");
        return;
    }

    savedMap = g_edMap;

    cleanMapCompact();

    if (exportType == RC3D_EXPORT_MODE_BINARY) {
        if (!exportBinaryMap(path)) {
            g_edMap = savedMap;
            setEditorStatus("Binary export failed");
            return;
        }
        g_edMap = savedMap;
        setEditorStatus("Binary map exported");
    } else {
        if (!exportCStringMap(path)) {
            g_edMap = savedMap;
            setEditorStatus("C export failed");
            return;
        }
        g_edMap = savedMap;
        setEditorStatus("C map exported");
    }
}

static void doLoadMap(void)
{
    char path[1024];

    if (!editorOpenMapDialog(path, sizeof(path))) {
        setEditorStatus("Load cancelled");
        return;
    }

    if (!loadTextMap(path)) {
        setEditorStatus("Load failed");
        return;
    }

    setEditorStatus("Map loaded");
}

static void doFinishDraft(void)
{
    if (g_ed.draftCount == 2) {
        pushUndoState();

        if (!splitSelectedSectorByDraftLine()) {
            performUndo();
        }
    }
    else if (g_ed.draftCount >= 3) {
        const float area = draftSignedArea();
        int changed = 0;

        pushUndoState();

        if (finalizeDraftSectorAttached()) {
            changed = 1;
        } else {
            if (area < 0.0f) {
                if (g_edMap.sectorCount < ED_MAX_SECTORS &&
                    (g_edMap.wallCount + g_ed.draftCount) <= ED_MAX_WALLS) {
                    finalizeDraftSector();
                    changed = 1;
                }
            } else if (area > 0.0f) {
                if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
                    if ((g_edMap.wallCount + g_ed.draftCount) <= ED_MAX_WALLS) {
                        finalizeDraftInnerSolid();
                        changed = 1;
                    }
                } else {
                    if (g_edMap.sectorCount < ED_MAX_SECTORS &&
                        (g_edMap.wallCount + g_ed.draftCount) <= ED_MAX_WALLS) {
                        finalizeDraftSector();
                        changed = 1;
                    }
                }
            }
        }

        if (!changed) {
            performUndo();
        }
    }
}



static void doWallMakeSolid(void)
{
    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        return;
    }

    {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];
        pushUndoState();
        makeWallSolid(g_ed.selectedWall, (w->midColor == 0) ? ED_COLOUR_WALL : w->midColor);
    }
}

static void doWallMakePortal(void)
{
    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        return;
    }

    pushUndoState();
    tryMakeWallPortal(g_ed.selectedWall);
}

static void doWallMakeWindow(void)
{
    float ob, ot;
    EdWall *w;

    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        return;
    }

    w = &g_edMap.walls[g_ed.selectedWall];
    ob = w->openBottom;
    ot = w->openTop;

    if (ot <= ob) {
        ob = 0.5f;
        ot = 1.4f;
    }

    pushUndoState();
    makeWallWindow(g_ed.selectedWall, ob, ot,
                   (w->upperColor == 0) ? ED_COLOUR_WALL : w->upperColor,
                   (w->lowerColor == 0) ? ED_COLOUR_WALL : w->lowerColor);
}

static void doWallMakeDoor(void)
{
    float ob, ot;
    EdWall *w;

    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        return;
    }

    w = &g_edMap.walls[g_ed.selectedWall];
    ob = w->openBottom;
    ot = w->openTop;

    if (ot <= ob) {
        ob = 0.0f;
        ot = 1.6f;
    }

    pushUndoState();
    makeWallDoor(g_ed.selectedWall, ob, ot,
                 (w->midColor == 0) ? ED_COLOUR_WALL : w->midColor);
}

static void doWallMakeTransparent(void)
{
    EdWall *w;

    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        return;
    }

    pushUndoState();

    w = &g_edMap.walls[g_ed.selectedWall];

    if (w->flags & RC3D_WALL_TRANSPARENCY) {
        w->flags &= ~RC3D_WALL_TRANSPARENCY;
    } else {
        w->flags |= RC3D_WALL_TRANSPARENCY;
    }
}

static void doWallSplitAtCursor(float worldX, float worldY)
{
    int wallToSplit = -1;

    if (!g_ed.splitPreviewValid) {
        return;
    }

    if (g_ed.hoverWall >= 0) {
        wallToSplit = g_ed.hoverWall;
    } else if (g_ed.selectedWall >= 0) {
        wallToSplit = g_ed.selectedWall;
    }

    if (wallToSplit < 0) {
        return;
    }

    pushUndoState();

    if (!splitWallAtSelected(wallToSplit, worldX, worldY)) {
        performUndo();
    }
}


static void executeEditorAction(EdAction action, float worldX, float worldY)
{
    switch (action) {
        case ED_ACT_HELP:
            g_ed.ui_menu_visable = 1 - g_ed.ui_menu_visable;
            if (g_ed.ui_menu_visable) {
                g_ed.ui_validator_visable = 0;
            }
            break;

        case ED_ACT_NEW_MAP:
            openConfirmDialog(ED_CONFIRM_NEW_MAP, "Discard current map and start a new blank map?");
            break;

        case ED_ACT_QUIT: openConfirmDialog(ED_CONFIRM_QUIT, "Quit editor?"); break;

        case ED_ACT_UNDO: performUndo(); break;
        case ED_ACT_REDO: performRedo(); break;
        case ED_ACT_LOAD: doLoadMap(); break;
        case ED_ACT_SAVE: doSaveMap(); break;
        case ED_ACT_EXPORT: doExportMap(); break;
        case ED_ACT_TOGGLE_GRID: doToggleGrid(); break;
        case ED_ACT_FINISH_DRAFT: doFinishDraft(); break;
        case ED_ACT_CLEAR_DRAFT: clearDraft(); break;
        case ED_ACT_SECTOR_CUTTER: doSectorCutter(); break;
        case ED_ACT_REPAIR_TOPOLOGY: doRepairTopology(); break;
        case ED_ACT_CLEAN_MAP: doCleanMap(); break;
        case ED_ACT_VALIDATE_MAP: doValidateMap(); break;
        case ED_ACT_RUN_TEST: doRunDemoGame(); break;
        case ED_ACT_WALL_SOLID: doWallMakeSolid(); break;
        case ED_ACT_WALL_PORTAL: doWallMakePortal(); break;
        case ED_ACT_WALL_WINDOW: doWallMakeWindow(); break;
        case ED_ACT_WALL_DOOR: doWallMakeDoor(); break;
        case ED_ACT_WALL_SPLIT: doWallSplitAtCursor(worldX, worldY); break;
        case ED_ACT_WALL_TRANSPARENCY: doWallMakeTransparent(); break;
        case ED_ACT_WALL_EXTRUDE: doExtrudeWall(); break;
        

        case ED_ACT_NONE: break;

        default:
            break;
    }
}



static int extrudeWallToNewSector(int wallIndex, float depth)
{
    int ownerSector;
    EdWall *srcWall;

    int v0, v1;
    float ax, ay, bx, by;
    float dx, dy, len;
    float nx, ny;

    float mx, my;
    float testX, testY;

    int newV0, newV1;
    int newSectorIndex;
    int newWallStart;

    EdWall *wShared;
    EdWall *wSide0;
    EdWall *wFar;
    EdWall *wSide1;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return 0;
    if (depth <= 0.0f) return 0;

    srcWall = &g_edMap.walls[wallIndex];

    if (srcWall->v0 < 0 || srcWall->v1 < 0) return 0;
    if (srcWall->v0 == srcWall->v1) return 0;

    if (srcWall->neighbour >= 0) return 0;

    ownerSector = findSectorOwningWall(wallIndex);
    if (ownerSector < 0 || ownerSector >= g_edMap.sectorCount) return 0;

    if (g_edMap.sectorCount >= ED_MAX_SECTORS) return 0;
    if ((g_edMap.wallCount + 4) > ED_MAX_WALLS) return 0;
    if ((g_edMap.vertCount + 2) > ED_MAX_VERTS) return 0;

    v0 = srcWall->v0;
    v1 = srcWall->v1;

    ax = g_edMap.verts[v0].x;
    ay = g_edMap.verts[v0].y;
    bx = g_edMap.verts[v1].x;
    by = g_edMap.verts[v1].y;

    dx = bx - ax;
    dy = by - ay;
    len = sqrtf((dx * dx) + (dy * dy));
    if (len <= ED_EPSILON) return 0;

    nx =  dy / len;
    ny = -dx / len;

    mx = (ax + bx) * 0.5f;
    my = (ay + by) * 0.5f;

    testX = mx + (nx * 0.25f);
    testY = my + (ny * 0.25f);

    if (pointInSector(testX, testY, ownerSector)) {
        nx = -nx;
        ny = -ny;
    }

    newV0 = addVertex(
        snapf(ax + (nx * depth)),
        snapf(ay + (ny * depth))
    );
    if (newV0 < 0) return 0;

    newV1 = addVertex(
        snapf(bx + (nx * depth)),
        snapf(by + (ny * depth))
    );
    if (newV1 < 0) {
        g_edMap.vertCount--;
        return 0;
    }

    if ((newV0 == v0) || (newV0 == v1) || (newV1 == v0) || (newV1 == v1)) {
        g_edMap.vertCount -= 2;
        return 0;
    }

    if (pointsSameEps(g_edMap.verts[newV0].x, g_edMap.verts[newV0].y,
                      g_edMap.verts[newV1].x, g_edMap.verts[newV1].y, 0.0001f)) {
        g_edMap.vertCount -= 2;
        return 0;
    }

    newSectorIndex = g_edMap.sectorCount;
    newWallStart   = g_edMap.wallCount;

    wShared = &g_edMap.walls[g_edMap.wallCount++];
    wSide0  = &g_edMap.walls[g_edMap.wallCount++];
    wFar    = &g_edMap.walls[g_edMap.wallCount++];
    wSide1  = &g_edMap.walls[g_edMap.wallCount++];

    wShared->v0 = v1;
    wShared->v1 = v0;
    wShared->neighbour = -1;
    wShared->openBottom = 0.0f;
    wShared->openTop = 0.0f;
    wShared->upperColor = g_ed.newWallUpperColor;
    wShared->midColor   = g_ed.newWallMidColor;
    wShared->lowerColor = g_ed.newWallLowerColor;
    wShared->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    wShared->tex_flags = RC3D_TEX_FLAG_DEFAULT;

    wSide0->v0 = v0;
    wSide0->v1 = newV0;
    wSide0->neighbour = -1;
    wSide0->openBottom = 0.0f;
    wSide0->openTop = 0.0f;
    wSide0->upperColor = g_ed.newWallUpperColor;
    wSide0->midColor   = g_ed.newWallMidColor;
    wSide0->lowerColor = g_ed.newWallLowerColor;
    wSide0->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    wSide0->tex_flags  = RC3D_TEX_FLAG_DEFAULT;

    wFar->v0 = newV0;
    wFar->v1 = newV1;
    wFar->neighbour = -1;
    wFar->openBottom = 0.0f;
    wFar->openTop = 0.0f;
    wFar->upperColor = g_ed.newWallUpperColor;
    wFar->midColor   = g_ed.newWallMidColor;
    wFar->lowerColor = g_ed.newWallLowerColor;
    wFar->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    wFar->tex_flags    = RC3D_TEX_FLAG_DEFAULT;

    wSide1->v0 = newV1;
    wSide1->v1 = v1;
    wSide1->neighbour = -1;
    wSide1->openBottom = 0.0f;
    wSide1->openTop = 0.0f;
    wSide1->upperColor = g_ed.newWallUpperColor;
    wSide1->midColor   = g_ed.newWallMidColor;
    wSide1->lowerColor = g_ed.newWallLowerColor;
    wSide1->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    wSide1->tex_flags  = RC3D_TEX_FLAG_DEFAULT;

    g_edMap.sectors[newSectorIndex].wallStart = newWallStart;
    g_edMap.sectors[newSectorIndex].wallCount = 4;
    g_edMap.sectors[newSectorIndex].boundaryCount = 4;
    g_edMap.sectors[newSectorIndex].floorHeight = g_ed.sectorFloor;
    g_edMap.sectors[newSectorIndex].ceilHeight  = g_ed.sectorCeil;
    g_edMap.sectors[newSectorIndex].floorColor  = g_ed.sectorFloorColor;
    g_edMap.sectors[newSectorIndex].ceilColor   = g_ed.sectorCeilColor;

    g_edMap.sectors[newSectorIndex].floorTexScaleX = 1.0f;
    g_edMap.sectors[newSectorIndex].floorTexScaleY = 1.0f;
    g_edMap.sectors[newSectorIndex].floorTexAngle  = 0.0f;

    g_edMap.sectors[newSectorIndex].ceilTexScaleX = 1.0f;
    g_edMap.sectors[newSectorIndex].ceilTexScaleY = 1.0f;
    g_edMap.sectors[newSectorIndex].ceilTexAngle  = 0.0f;

    g_edMap.sectorCount++;

    setPortalPair(wallIndex, ownerSector, newWallStart + 0, newSectorIndex);

    syncAllPortals();

    g_ed.selectionType = ED_SEL_SECTOR;
    g_ed.selectedSector = newSectorIndex;
    g_ed.selectedWall = -1;
    g_ed.selectedVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverVert = -1;
    g_ed.hoverSector = newSectorIndex;

    return 1;
}



static void doExtrudeWall(void)
{
    if (g_ed.selectedWall < 0 || g_ed.selectedWall >= g_edMap.wallCount) {
        setEditorStatus("Select a wall first");
        return;
    }

    pushUndoState();

    if (!extrudeWallToNewSector(g_ed.selectedWall, 2.0f)) {
        performUndo();
        setEditorStatus("Wall extrude failed");
        return;
    }

    setEditorStatus("Extruded wall into new sector");
}







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
    } else if (modKeys[SDL_SCANCODE_LALT] || modKeys[SDL_SCANCODE_RALT]) {
        uiStep = 0.001f;
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
    //rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SPLIT, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TRANSPARENCY, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 0);


    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 0);

    // sector texture settings
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SY_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SY_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_RESET, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SY_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SY_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_RESET, 0);


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
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TRANSPARENCY, 1);
            //rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SPLIT, 1);

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 1);

            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XL, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) ? "Clamp XL\x2" : "Clamp XL"); //  \x2 is font for a tick glyph
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XR, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) ? "Clamp XR\x2" : "Clamp XR");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YT, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYT) ? "Clamp YT\x2" : "Clamp YT");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YB, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYB) ? "Clamp YB\x2" : "Clamp YB");

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 1);

            //rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_SPLIT, !g_ed.splitPreviewValid);
        }

        if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
            sec = &g_edMap.sectors[g_ed.selectedSector];

            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 1);


            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SX_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SX_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SY_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_SY_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_RESET, 1);

            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SX_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SX_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SY_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_SY_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_RESET, 1);
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
        case GUI_BTN_HELP:     executeEditorAction(ED_ACT_HELP, worldX, worldY); break;
        case GUI_BTN_NEWMAP:   executeEditorAction(ED_ACT_NEW_MAP, worldX, worldY); break;
        case GUI_BTN_QUIT:     executeEditorAction(ED_ACT_QUIT, worldX, worldY); break;
        case GUI_BTN_UNDO:     executeEditorAction(ED_ACT_UNDO, worldX, worldY); break;
        case GUI_BTN_REDO:     executeEditorAction(ED_ACT_REDO, worldX, worldY); break;
        case GUI_BTN_LOAD:     executeEditorAction(ED_ACT_LOAD, worldX, worldY); break;
        case GUI_BTN_SAVE:     executeEditorAction(ED_ACT_SAVE, worldX, worldY); break;
        case GUI_BTN_EXPORT:   executeEditorAction(ED_ACT_EXPORT, worldX, worldY); break;
        case GUI_BTN_GRID:     executeEditorAction(ED_ACT_TOGGLE_GRID, worldX, worldY); break;
        case GUI_BTN_FINISH:   executeEditorAction(ED_ACT_FINISH_DRAFT, worldX, worldY); break;
        case GUI_BTN_CLRDRAFT: executeEditorAction(ED_ACT_CLEAR_DRAFT, worldX, worldY); break;
        case GUI_BTN_SECTOR_CUTTER: executeEditorAction(ED_ACT_SECTOR_CUTTER, worldX, worldY); break;
        case GUI_BTN_REPAIR_TOPOLOGY: executeEditorAction(ED_ACT_REPAIR_TOPOLOGY, worldX, worldY); break;
        case GUI_BTN_CLEANMAP: executeEditorAction(ED_ACT_CLEAN_MAP, worldX, worldY); break;
        case GUI_BTN_MAPVALIDATOR: executeEditorAction(ED_ACT_VALIDATE_MAP, worldX, worldY); break;
        case GUI_BTN_LAUNCH_TEST_MAP: executeEditorAction(ED_ACT_RUN_TEST, worldX, worldY); break;
        case GUI_BTN_WALL_SOLID: executeEditorAction(ED_ACT_WALL_SOLID, worldX, worldY); break;
        case GUI_BTN_WALL_PORTAL: executeEditorAction(ED_ACT_WALL_PORTAL, worldX, worldY); break;
        case GUI_BTN_WALL_WINDOW: executeEditorAction(ED_ACT_WALL_WINDOW, worldX, worldY); break;
        case GUI_BTN_WALL_DOOR: executeEditorAction(ED_ACT_WALL_DOOR, worldX, worldY); break;
        case GUI_BTN_WALL_SPLIT: executeEditorAction(ED_ACT_WALL_SPLIT, worldX, worldY); break;
        case GUI_BTN_WALL_TRANSPARENCY: executeEditorAction(ED_ACT_WALL_TRANSPARENCY, worldX, worldY); break;


        // sector texture settings //////////////////////////////////////////////////
        case GUI_BTN_SECTOR_FTEX_SX_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorTexScaleX -= 0.1f;
                if (sec->floorTexScaleX < 0.1f) sec->floorTexScaleX = 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_FTEX_SX_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorTexScaleX += 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_FTEX_SY_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorTexScaleY -= 0.1f;
                if (sec->floorTexScaleY < 0.1f) sec->floorTexScaleY = 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_FTEX_SY_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorTexScaleY += 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_FTEX_ROT_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorTexAngle -= DEG2RAD(15.0f);
            }
            break;

        case GUI_BTN_SECTOR_FTEX_ROT_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorTexAngle += DEG2RAD(15.0f);
            }
            break;

        case GUI_BTN_SECTOR_CTEX_SX_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexScaleX -= 0.1f;
                if (sec->ceilTexScaleX < 0.1f) sec->ceilTexScaleX = 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_CTEX_SX_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexScaleX += 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_CTEX_SY_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexScaleY -= 0.1f;
                if (sec->ceilTexScaleY < 0.1f) sec->ceilTexScaleY = 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_CTEX_SY_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexScaleY += 0.1f;
            }
            break;

        case GUI_BTN_SECTOR_CTEX_ROT_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexAngle -= DEG2RAD(15.0f);
            }
            break;

        case GUI_BTN_SECTOR_CTEX_ROT_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilTexAngle += DEG2RAD(15.0f);
            }
            break;

        case GUI_BTN_SECTOR_CTEX_ROT_RESET:
            if(sec){
                pushUndoState();
                sec->ceilTexAngle = 0.0f;
            }
            break;

        case GUI_BTN_SECTOR_FTEX_ROT_RESET:
            if(sec){
                pushUndoState();
                sec->floorTexAngle = 0.0f;
            }
            break;

        case GUI_BTN_WALL_CLAMP_XL:
            if (w) {
                pushUndoState();
                toggleWallTexFlag(g_ed.selectedWall, RC3D_TEX_FLAG_CLAMPXL);
            }
            break;

        case GUI_BTN_WALL_CLAMP_XR:
            if (w) {
                pushUndoState();
                toggleWallTexFlag(g_ed.selectedWall, RC3D_TEX_FLAG_CLAMPXR);
            }
            break;

        case GUI_BTN_WALL_CLAMP_YT:
            if (w) {
                pushUndoState();
                toggleWallTexFlag(g_ed.selectedWall, RC3D_TEX_FLAG_CLAMPYT);
            }
            break;

        case GUI_BTN_WALL_CLAMP_YB:
            if (w) {
                pushUndoState();
                toggleWallTexFlag(g_ed.selectedWall, RC3D_TEX_FLAG_CLAMPYB);
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

        case GUI_BTN_WALL_TEX_ROT_MINUS:
            if (w) {
                pushUndoState();
                adjustWallTexAngle(g_ed.selectedWall, -DEG2RAD(15.0f));
            }
            break;

        case GUI_BTN_WALL_TEX_ROT_PLUS:
            if (w) {
                pushUndoState();
                adjustWallTexAngle(g_ed.selectedWall, DEG2RAD(15.0f));
            }
            break;

        case GUI_BTN_WALL_TEX_ROT_RESET:
            if (w) {
                pushUndoState();
                setWallTexAngleEx(g_ed.selectedWall, DEG2RAD(0));
            }
            break;

        default:
            break;
    }
}


static void drawInspectorPanel(void)
{
    char buf[256];
    const int px = EDIT_VIEW_PORT_WIDTH;
    volatile int py = 0;
    const int pw = ED_INSPECTOR_PANEL;
    const int ph = EDIT_VIEW_PORT_HEIGHT;

    py = 0;
    drawRect(px, py, pw, ph, ED_UI_BG);
    drawLine(px, py, px + pw - 1, py, ED_UI_BORDER);
    drawLine(px, py + ph - 1, px + pw - 1, py + ph - 1, ED_UI_BORDER);
    drawLine(px, py, px, py + ph - 1, ED_UI_BORDER);
    drawLine(px + pw - 1, py, px + pw - 1, py + ph - 1, ED_UI_BORDER);



    drawRect(px + 6, py + 6, pw - 12, 24, ED_COLOUR_TEXT_BAR_BG);
    drawRectL(px + 6, py + 6, pw - 12, 24, ED_COLOUR_TEXT_BAR_BORDER);
    drawText(px + 12, py + 10, "INSPECTOR", ED_INSPECTOR_TITLE_COL);

    snprintf(buf, sizeof(buf),
        "Mode: %s",
        (g_ed.selectionType == ED_SEL_VERTEX) ? "VERTEX" :
        (g_ed.selectionType == ED_SEL_WALL)   ? "WALL" :
        (g_ed.selectionType == ED_SEL_SECTOR) ? "SECTOR" : "NONE");
    drawText(px + 120, py + 10, buf, ED_INSPECTOR_TEXT_COL);

    if (g_ed.selectionType == ED_SEL_VERTEX && g_ed.selectedVert >= 0) {
        const EdVec2 *v = &g_edMap.verts[g_ed.selectedVert];

        drawRect(px + 8, py + 40, pw - 16, 150, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py + 40, pw - 16, 150, ED_INSPECTOR_PARENT_PANELS_FRAME);

        snprintf(buf, sizeof(buf), "VERTEX %d", g_ed.selectedVert);
        drawText(px + 16, py + 48, buf, ED_INSPECTOR_TEXT_COL);

        snprintf(buf, sizeof(buf), "X: %.4f", v->x);
        drawText(px + 16, py + 74, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Y: %.4f", v->y);
        drawText(px + 16, py + 96, buf, ED_TEXT_COL);

        drawText(px + 16, py + 124, "LMB drag vertex", ED_TEXT_COL);
        drawText(px + 16, py + 144, "SHIFT-drop onto another = merge", ED_TEXT_COL);
        drawText(px + 16, py + 164, "Arrows 0.01  Shift 0.001  Alt 0.0001", ED_TEXT_COL);
    }
    ///// Wall texture / edit mode /////////////////////////////////////////
    else if (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall >= 0) {
        const EdWall *w = &g_edMap.walls[g_ed.selectedWall];
        const int clampXL = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) != 0;
        const int clampXR = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) != 0;
        const int clampYT = (w->tex_flags & RC3D_TEX_FLAG_CLAMPYT) != 0;
        const int clampYB = (w->tex_flags & RC3D_TEX_FLAG_CLAMPYB) != 0;
        const float wallTexAngleDeg = RAD2DEG(getWallTexAngle(w));

        py = 40;

        drawRect(px + 8, py, pw - 16, 346, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 346, ED_INSPECTOR_PARENT_PANELS_FRAME);
        py += 6;

        snprintf(buf, sizeof(buf), "WALL %d", g_ed.selectedWall);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Verts: %d -> %d", w->v0, w->v1);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        snprintf(buf, sizeof(buf), "Neighbour: %d", w->neighbour);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;
        py += 8;

        snprintf(buf, sizeof(buf), "Open Top: %.3f", w->openTop);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Open Bottom: %.3f", w->openBottom);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Flags: %u", (unsigned)w->flags);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        snprintf(buf, sizeof(buf), "Upper: %u   Mid: %u   Lower: %u", (unsigned)w->upperColor, (unsigned)w->midColor, (unsigned)w->lowerColor);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        py += 30;   // additional 

        snprintf(buf, sizeof(buf), "Tex flags: 0x%08X", (unsigned)w->tex_flags);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Clamp X  L[%s] R[%s]", clampXL ? "\x2" : " ", clampXR ? "\x2" : " ");
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Clamp Y  T[%s] B[%s]", clampYT ? "\x2" : " ", clampYB ? "\x2" : " ");
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Tex angle: %.1f\xb0", wallTexAngleDeg);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

    }
    else if (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0) {
        drawRect(px + 8, py + 40, pw - 16, 130, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py + 40, pw - 16, 130, ED_INSPECTOR_PARENT_PANELS_FRAME);

        drawText(px + 16, py + 48, "MULTI VERTEX SELECTION", ED_INSPECTOR_TEXT_COL);

        snprintf(buf, sizeof(buf), "Selected verts: %d", g_ed.selectedVertCount);
        drawText(px + 16, py + 76, buf, ED_TEXT_COL);

        drawText(px + 16, py + 104, "LMB drag selected vertices", ED_TEXT_COL);
        drawText(px + 16, py + 126, "Arrow Left / Right rotates selection", ED_TEXT_COL);
        drawText(px + 16, py + 148, "SHIFT = 15\xb0, ALT = 1\xb0", ED_TEXT_COL);
    }

    /// SELECTED SECTORS ///////////////////////////////////////////////////////
    
    else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        py = 40;

        drawRect(px + 8, py, pw - 16, 374, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 374, ED_INSPECTOR_PARENT_PANELS_FRAME);

        py += 8;

        snprintf(buf, sizeof(buf), "SECTOR %d", g_ed.selectedSector);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL);  py += 20;
        py += 4;

        snprintf(buf, sizeof(buf), "wallStart: %d", sec->wallStart);
        drawText(px + 16, py, buf, ED_TEXT_COL);  py += 20;

        snprintf(buf, sizeof(buf), "wallCount: %d   boundaryCount: %d",
                 sec->wallCount, sec->boundaryCount);
        drawText(px + 16, py, buf, ED_TEXT_COL);  py += 20;

        py += 8;

        // heights ////////////////////////////////
        drawRect(px + 12,  py, pw - 24, 58, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 58, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "HEIGHTS", ED_INSPECTOR_PANELS_HEADER_TEXT);      py += 20;

        py += 6;
        snprintf(buf, sizeof(buf), "Floor: %.3f", sec->floorHeight);
        drawText(px + 18,  py, buf, ED_TEXT_COL);   

        snprintf(buf, sizeof(buf), "Ceil : %.3f", sec->ceilHeight);
        drawText(px + 218,  py, buf, ED_TEXT_COL);   py += 30;

        //py += 6;
        // TEXTURES ///////////////////////////////
        
        drawRect(px + 12,  py, pw - 24, 48, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 48, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "TEXTURES", ED_INSPECTOR_PANELS_HEADER_TEXT);     py += 20;

        snprintf(buf, sizeof(buf), "Floor tex: %u   Ceil tex: %u",
                 (unsigned)sec->floorColor, (unsigned)sec->ceilColor);
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 20;


        // FLOOR Texture UVs //////////////////////
        py += 6;
        drawRect(px + 12,  py, pw - 24, 86, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 86, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "FLOOR UV", ED_INSPECTOR_PANELS_HEADER_TEXT);  py += 20;

        py += 6;
        snprintf(buf, sizeof(buf), "Scale X:%.3f", sec->floorTexScaleX);
        drawText(px + 18,  py, buf, ED_TEXT_COL);  

        snprintf(buf, sizeof(buf), "Scale Y:%.3f", sec->floorTexScaleY);
        drawText(px + 218,  py, buf, ED_TEXT_COL); py += 30; 

        snprintf(buf, sizeof(buf), "Rotate : %.1f\xb0", RAD2DEG(sec->floorTexAngle));
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 20;

        // CEILING Texture UVs ////////////////////////////
        py += 8;
        drawRect(px + 12,  py, pw - 24, 86, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 86, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "CEILING UV", ED_INSPECTOR_PANELS_HEADER_TEXT);  py += 20;
        py += 6;
        snprintf(buf, sizeof(buf), "Scale X:%.3f", sec->ceilTexScaleX);
        drawText(px + 18,  py, buf, ED_TEXT_COL); 

        snprintf(buf, sizeof(buf), "Scale Y:%.3f", sec->ceilTexScaleY);
        drawText(px + 218,  py, buf, ED_TEXT_COL);  py += 30;

        snprintf(buf, sizeof(buf), "Rotate : %.1f deg", RAD2DEG(sec->ceilTexAngle));
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 30;
    }
    else {
        drawRect(px + 8, py + 40, pw - 16, 318, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py + 40, pw - 16, 318, ED_INSPECTOR_PARENT_PANELS_FRAME);

        drawText(px + 16, py + 48, "NEW DRAFT DEFAULTS", ED_INSPECTOR_TEXT_COL);

        snprintf(buf, sizeof(buf), "Sector floor: %.2f", g_ed.sectorFloor);
        drawText(px + 16, py + 80, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Sector ceil : %.2f", g_ed.sectorCeil);
        drawText(px + 16, py + 102, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Floor tex   : %u", (unsigned)g_ed.sectorFloorColor);
        drawText(px + 16, py + 132, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Ceil tex    : %u", (unsigned)g_ed.sectorCeilColor);
        drawText(px + 16, py + 154, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Wall upper  : %u", (unsigned)g_ed.newWallUpperColor);
        drawText(px + 16, py + 184, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Wall mid    : %u", (unsigned)g_ed.newWallMidColor);
        drawText(px + 16, py + 206, buf, ED_TEXT_COL);

        snprintf(buf, sizeof(buf), "Wall lower  : %u", (unsigned)g_ed.newWallLowerColor);
        drawText(px + 16, py + 228, buf, ED_TEXT_COL);

        drawText(px + 16, py + 262, "[F]/[G] floor   [C]/[V] ceil", ED_TEXT_COL);
        drawText(px + 16, py + 284, "[J]/[K] floor tex  [N]/[M] ceil tex", ED_TEXT_COL);
        drawText(px + 16, py + 306, "[A]/[S] upper  [D]/[F] mid  [H]/[J] lower", ED_TEXT_COL);
    }

    drawTextureBrowser();
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
    y = (EDIT_VIEW_PORT_HEIGHT - 80);

    drawRect(x, y, w, 24, ED_COLOUR_TEXT_BAR_BG);
    drawRectL(x, y, w, 24, ED_COLOUR_TEXT_BAR_BORDER);
    drawText(x + 8, y + 4, g_ed.statusText, ED_TEXT_COL);
}

static void drawUndoHistoryPopup(void)
{
    char buf[256];
    int px, py, pw, ph;
    int rowY;
    int visibleRows;

    if (!g_ed.undoHistoryVisible || g_undoCount <= 0) {
        return;
    }

    getUndoHistoryPopupRect(&px, &py, &pw, &ph);

    drawRect(0, 0, EDIT_VIEW_PORT_WIDTH, EDIT_VIEW_PORT_HEIGHT-1, ED_COLOUR_TEXT_BAR_BG);
    drawRect(px, py, pw, ph, ED_UI_BG);
    drawRectL(px, py, pw, ph, ED_UI_BORDER);

    drawText(px + 12, py + 12, "Undo History", ED_INSPECTOR_TEXT_COL);
    drawText(px + 12, py + 32, "Newest undo is at the top. Enter or click restores that snapshot.", ED_TEXT_COL);

    rowY = py + 56;
    visibleRows = g_undoCount - g_ed.undoHistoryScrollPos;
    if (visibleRows > ED_UNDO_HISTORY_VISIBLE_ROWS) {
        visibleRows = ED_UNDO_HISTORY_VISIBLE_ROWS;
    }

    for (int row = 0; row < visibleRows; row++) {
        char summary[180];
        int rowPos = g_ed.undoHistoryScrollPos + row;
        int stackIndex = undoHistoryStackIndexFromPos(rowPos);
        const int selected = (rowPos == g_ed.undoHistorySelectedPos);
        const int itemY = rowY + (row * ED_UNDO_HISTORY_ROW_H);
        const uint8_t bg = selected ? ED_COLOUR_BTN_BG_ACTIVE : ED_COLOUR_BTN_BG;
        const uint8_t border = selected ? ED_COLOUR_BTN_FRAME : ED_COLOUR_BTN_FRAME_DISABLED;
        const uint8_t textCol = selected ? ED_COLOUR_BTN_TXT_DISABLED : ED_COLOUR_BTN_TEXT;

        if (stackIndex < 0) {
            continue;
        }

        describeSnapshotSummary(&g_undoStack[stackIndex], summary, sizeof(summary));
        snprintf(buf, sizeof(buf), "%2d. %s", rowPos + 1, summary);

        drawRect(px + 12, itemY, pw - 24, ED_UNDO_HISTORY_ROW_H - 2, bg);
        drawRectL(px + 12, itemY, pw - 24, ED_UNDO_HISTORY_ROW_H - 2, border);
        drawText(px + 20, itemY + 4, buf, textCol);
    }

    snprintf(buf, sizeof(buf),
             "Showing %d-%d of %d   |[Up]/[Down] move [PageUp]/[PageDown] scroll [Esc] close",
             g_ed.undoHistoryScrollPos + 1,
             g_ed.undoHistoryScrollPos + visibleRows,
             g_undoCount);
    drawText(px + 12, py + ph - 24, buf, ED_TEXT_COL);
}


void rc3dEditUpdate(float dt,
                    const uint8_t *keys,
                    int mouseX,
                    int mouseY,
                    uint32_t mouseButtons,
                    int mouseWheelY)
{
    (void)dt;

    const int ctrlDown = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
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

    if (!g_ed.undoHistoryVisible &&
        !g_ed.confirmVisible &&
        (keyPressedOnce(keys, SDL_SCANCODE_F11) ||
         (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_H)))) {
        openUndoHistoryPopup();
        finishEditorInputFrame(keys, leftDown, rightDown, middleDown, mouseX, mouseY);
        return;
    }

    if (handleUndoHistoryPopupInput(keys, mouseX, mouseY, leftPressed, mouseWheelY)) {
        g_ed.uiMouseCaptured = 1;
        finishEditorInputFrame(keys, leftDown, rightDown, middleDown, mouseX, mouseY);
        return;
    }

    {
        int wheelUsedByUI = 0;

        /* let inspector/browser consume wheel first */
        if (mouseWheelY != 0) {
            wheelUsedByUI = handleTextureBrowserMouse(mouseX, mouseY, leftDown, 0, mouseWheelY);
            if (wheelUsedByUI) {
                g_ed.uiMouseCaptured = 1;
            }
        }

        /* only zoom if UI did not consume wheel */
        if (mouseWheelY != 0 && !wheelUsedByUI) {
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
    if (handleTextureBrowserMouse(mouseX, mouseY, leftDown, leftPressed, 0)) {
        g_ed.uiMouseCaptured = 1;
    }

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
        g_ed.textureScrollbarDragging = 0;
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


    if (keyPressedOnce(keys, SDL_SCANCODE_APOSTROPHE)){
        g_ed.bUseVectorFill = 1 - g_ed.bUseVectorFill;
    }

    if (g_ed.ui_validator_visable) {
        if (keyPressedOnce(keys, SDL_SCANCODE_RIGHTBRACKET)) {
            validatorNextIssue();
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_LEFTBRACKET)) {
            validatorPrevIssue();
        }
    }



    if (keyPressedOnce(keys, SDL_SCANCODE_BACKSPACE)) {
        if (g_ed.draftCount > 0) {
            g_ed.draftCount--;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F1)) {
        executeEditorAction(ED_ACT_HELP, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F6)) {
        executeEditorAction(ED_ACT_SECTOR_CUTTER, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F7)) {
        executeEditorAction(ED_ACT_REPAIR_TOPOLOGY, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F8)) {
        executeEditorAction(ED_ACT_CLEAN_MAP, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F9)) {
        executeEditorAction(ED_ACT_VALIDATE_MAP, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F10)) {
        executeEditorAction(ED_ACT_QUIT, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_ESCAPE)) {
        executeEditorAction(ED_ACT_CLEAR_DRAFT, worldX, worldY);
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) && keyPressedOnce(keys, SDL_SCANCODE_Z)) {
        executeEditorAction(ED_ACT_UNDO, worldX, worldY);
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) && keyPressedOnce(keys, SDL_SCANCODE_Y)) {
        executeEditorAction(ED_ACT_REDO, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_TAB)) {
        executeEditorAction(ED_ACT_TOGGLE_GRID, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_RETURN)) {
        executeEditorAction(ED_ACT_FINISH_DRAFT, worldX, worldY);
    }

    /* sector defaults for NEW drafted sectors only when nothing is selected */
    if (g_ed.selectedWall < 0 && g_ed.selectedSector < 0 && g_ed.selectionType != ED_SEL_VERTEX) {
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
    }

    /* selected sector editing */
    if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
        EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        int changed = 0;

        /* SHIFT + keypad = colour copy/paste */
        if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
            if (keyPressedOnce(keys, SDL_SCANCODE_KP_1)) {
                g_ed.copiedSectorFloorColor = sec->floorColor;
                g_ed.hasCopiedSectorFloorColor = 1;
                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Copied sector floor colour: %u", (unsigned)sec->floorColor);
                    setEditorStatus(msg);
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_2)) {
                g_ed.copiedSectorCeilColor = sec->ceilColor;
                g_ed.hasCopiedSectorCeilColor = 1;
                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Copied sector ceiling colour: %u", (unsigned)sec->ceilColor);
                    setEditorStatus(msg);
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_4)) {
                if (g_ed.hasCopiedSectorFloorColor) {
                    pushUndoState();
                    sec->floorColor = g_ed.copiedSectorFloorColor;
                    changed = 1;

                    {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "Pasted sector floor colour: %u", (unsigned)sec->floorColor);
                        setEditorStatus(msg);
                    }
                } else {
                    setEditorStatus("No copied sector floor colour");
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_5)) {
                if (g_ed.hasCopiedSectorCeilColor) {
                    pushUndoState();
                    sec->ceilColor = g_ed.copiedSectorCeilColor;
                    changed = 1;

                    {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "Pasted sector ceiling colour: %u", (unsigned)sec->ceilColor);
                        setEditorStatus(msg);
                    }
                } else {
                    setEditorStatus("No copied sector ceiling colour");
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_F)) { pushUndoState(); sec->floorHeight -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_G)) { pushUndoState(); sec->floorHeight += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_C)) { pushUndoState(); sec->ceilHeight  -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_V)) { pushUndoState(); sec->ceilHeight  += 0.1f; changed = 1; }

            if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                sec->ceilHeight = sec->floorHeight + 0.1f;
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_J)) { pushUndoState(); sec->floorColor--; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_K)) { pushUndoState(); sec->floorColor++; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_N)) { pushUndoState(); sec->ceilColor--;  changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_M)) { pushUndoState(); sec->ceilColor++;  changed = 1; }
        }
        else
        {
            /* plain keypad = height copy/paste */
            if (keyPressedOnce(keys, SDL_SCANCODE_KP_1)) {
                g_ed.copiedSectorFloor = sec->floorHeight;
                g_ed.hasCopiedSectorFloor = 1;

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Copied sector floor height: %.2f", sec->floorHeight);
                    setEditorStatus(msg);
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_2)) {
                g_ed.copiedSectorCeil = sec->ceilHeight;
                g_ed.hasCopiedSectorCeil = 1;

                {
                    char msg[128];
                    snprintf(msg, sizeof(msg), "Copied sector ceiling height: %.2f", sec->ceilHeight);
                    setEditorStatus(msg);
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_4)) {
                if (g_ed.hasCopiedSectorFloor) {
                    pushUndoState();
                    sec->floorHeight = g_ed.copiedSectorFloor;
                    if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                        sec->ceilHeight = sec->floorHeight + 0.1f;
                    }
                    changed = 1;

                    {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "Pasted sector floor height: %.2f", sec->floorHeight);
                        setEditorStatus(msg);
                    }
                } else {
                    setEditorStatus("No copied sector floor height");
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_KP_5)) {
                if (g_ed.hasCopiedSectorCeil) {
                    pushUndoState();
                    sec->ceilHeight = g_ed.copiedSectorCeil;
                    if (sec->ceilHeight < sec->floorHeight + 0.1f) {
                        sec->ceilHeight = sec->floorHeight + 0.1f;
                    }
                    changed = 1;

                    {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "Pasted sector ceiling height: %.3f", sec->ceilHeight);
                        setEditorStatus(msg);
                    }
                } else {
                    setEditorStatus("No copied sector ceiling height");
                }
            }

            if (keyPressedOnce(keys, SDL_SCANCODE_1)) { pushUndoState(); sec->floorTexScaleX -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_2)) { pushUndoState(); sec->floorTexScaleX += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_3)) { pushUndoState(); sec->floorTexScaleY -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_4)) { pushUndoState(); sec->floorTexScaleY += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_5)) { pushUndoState(); sec->floorTexAngle  -= DEG2RAD(15.0f); changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_6)) { pushUndoState(); sec->floorTexAngle  += DEG2RAD(15.0f); changed = 1; }

            if (keyPressedOnce(keys, SDL_SCANCODE_Q)) { pushUndoState(); sec->ceilTexScaleX -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_W)) { pushUndoState(); sec->ceilTexScaleX += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_E)) { pushUndoState(); sec->ceilTexScaleY -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_R)) { pushUndoState(); sec->ceilTexScaleY += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_T)) { pushUndoState(); sec->ceilTexAngle  -= DEG2RAD(15.0f); changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_Y)) { pushUndoState(); sec->ceilTexAngle  += DEG2RAD(15.0f); changed = 1; }

            if (sec->floorTexScaleX < 0.1f) sec->floorTexScaleX = 0.1f;
            if (sec->floorTexScaleY < 0.1f) sec->floorTexScaleY = 0.1f;
            if (sec->ceilTexScaleX  < 0.1f) sec->ceilTexScaleX  = 0.1f;
            if (sec->ceilTexScaleY  < 0.1f) sec->ceilTexScaleY  = 0.1f;
        }

        if (changed) {
            syncAllPortals();
        }
    }

    if (g_ed.selectionType == ED_SEL_VERTEX &&
        g_ed.selectedVert >= 0 &&
        g_ed.selectedVert < g_edMap.vertCount) {
        EdVec2 *v = &g_edMap.verts[g_ed.selectedVert];
        float dx = 0.0f;
        float dy = 0.0f;
        float nudgeStep = 0.01f;

        if (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) {
            nudgeStep = 0.0001f;
        } else if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
            nudgeStep = 0.001f;
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_LEFT))  dx -= nudgeStep;
        if (keyPressedOnce(keys, SDL_SCANCODE_RIGHT)) dx += nudgeStep;
        if (keyPressedOnce(keys, SDL_SCANCODE_UP))    dy -= nudgeStep;
        if (keyPressedOnce(keys, SDL_SCANCODE_DOWN))  dy += nudgeStep;

        if (dx != 0.0f || dy != 0.0f) {
            pushUndoState();
            v->x += dx;
            v->y += dy;
            syncAllPortals();
            updateHover(worldX, worldY, mouseX, mouseY);
        }
    }

    /* selected wall editing */
    if (g_ed.selectedWall >= 0 && g_ed.selectedWall < g_edMap.wallCount) {
        EdWall *w = &g_edMap.walls[g_ed.selectedWall];

        if (keyPressedOnce(keys, SDL_SCANCODE_1)) executeEditorAction(ED_ACT_WALL_SOLID, worldX, worldY);
        if (keyPressedOnce(keys, SDL_SCANCODE_2)) executeEditorAction(ED_ACT_WALL_PORTAL, worldX, worldY);
        if (keyPressedOnce(keys, SDL_SCANCODE_3)) executeEditorAction(ED_ACT_WALL_WINDOW, worldX, worldY);
        if (keyPressedOnce(keys, SDL_SCANCODE_4)) executeEditorAction(ED_ACT_WALL_DOOR, worldX, worldY);
        if (keyPressedOnce(keys, SDL_SCANCODE_5)) executeEditorAction(ED_ACT_WALL_EXTRUDE, worldX, worldY);

        if (keyPressedOnce(keys, SDL_SCANCODE_R)) w->openBottom -= 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_T)) w->openBottom += 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_Y)) w->openTop    -= 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_U)) w->openTop    += 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_Q)) {
            pushUndoState();
            adjustWallTexAngle(g_ed.selectedWall, -DEG2RAD(15.0f));
        }
        if (keyPressedOnce(keys, SDL_SCANCODE_W)) {
            pushUndoState();
            adjustWallTexAngle(g_ed.selectedWall, DEG2RAD(15.0f));
        }

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
            executeEditorAction(ED_ACT_WALL_SPLIT, worldX, worldY);
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

    if (!(g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) &&
        !(g_ed.selectedWall   >= 0 && g_ed.selectedWall   < g_edMap.wallCount) &&
        g_ed.selectionType != ED_SEL_VERTEX &&
        g_ed.selectedVertCount <= 0)
    {
        if (keyPressedOnce(keys, SDL_SCANCODE_Q)) {
            g_edMap.startAngle -= 0.1f;
        }
        if (keyPressedOnce(keys, SDL_SCANCODE_E)) {
            g_edMap.startAngle += 0.1f;
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F2)) {
        executeEditorAction(ED_ACT_LOAD, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F3)) {
        executeEditorAction(ED_ACT_SAVE, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_F5)) {
        executeEditorAction(ED_ACT_EXPORT, worldX, worldY);
    }



    // rotate selected vertices
    if (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 1) {
        float rotateStep = 90.0f * (float)(M_PI / 180.0f);  // default rotate (90 is perfect)

        // starts to get funny here, BUT should keep to snapping to current grid!
        if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) {
            rotateStep = 15.0f * (float)(M_PI / 180.0f);
        } else if (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) {
            rotateStep = 1.0f * (float)(M_PI / 180.0f);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_LEFT)) {
            pushUndoState();
            rotateSelectedVertices(-rotateStep);
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_RIGHT)) {
            pushUndoState();
            rotateSelectedVertices(+rotateStep);
        }
    }






    if (g_ed.selectionType == ED_SEL_WALL) {
        if (g_ed.textureBrowserTarget != TEX_TARGET_WALL_UPPER &&
            g_ed.textureBrowserTarget != TEX_TARGET_WALL_MIDDLE &&
            g_ed.textureBrowserTarget != TEX_TARGET_WALL_LOWER) {
            g_ed.textureBrowserTarget = TEX_TARGET_WALL_MIDDLE;
        }
    }
    else if (g_ed.selectionType == ED_SEL_SECTOR) {
        if (g_ed.textureBrowserTarget != TEX_TARGET_SECTOR_FLOOR &&
            g_ed.textureBrowserTarget != TEX_TARGET_SECTOR_CEIL) {
            g_ed.textureBrowserTarget = TEX_TARGET_SECTOR_FLOOR;
        }
    }
    else {
        if (g_ed.textureBrowserTarget == TEX_TARGET_NONE) {
            g_ed.textureBrowserTarget = TEX_TARGET_DEFAULT_WALL_MIDDLE;
        }
    }

    finishEditorInputFrame(keys, leftDown, rightDown, middleDown, mouseX, mouseY);
}

void rc3dEditRender(void)
{
    drawGrid();
    drawMapGeometry();
    drawStartMarker();

    drawTopMenuBar();
    drawBottomMenuBar();
    drawStatusPopup();
    drawConfirmPopup();

    if (g_ed.ui_menu_visable) {
        drawExpandedEditorPanel();
    }

    if (g_ed.ui_validator_visable) {
        drawValidatorPanel();
    }

    drawInspectorPanel();
    drawHoverPanel();
    rcguiDraw(&g_ui);
    drawUndoHistoryPopup();
}
