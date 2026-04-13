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
#define ED_CLICK_DRAG_TOLERANCE_PX 5
#define ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT 64.0f



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
#define ED_BOTTOMBAR_W             758
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
    GUI_BTN_SECTOR_GLOW_MINUS,
    GUI_BTN_SECTOR_GLOW_PLUS,
    GUI_BTN_SECTOR_TAG_MINUS,
    GUI_BTN_SECTOR_TAG_PLUS,
    GUI_BTN_SECTOR_STATE_MINUS,
    GUI_BTN_SECTOR_STATE_PLUS,
    GUI_BTN_SECTOR_FLOOR_MIN_MINUS,
    GUI_BTN_SECTOR_FLOOR_MIN_PLUS,
    GUI_BTN_SECTOR_FLOOR_MAX_MINUS,
    GUI_BTN_SECTOR_FLOOR_MAX_PLUS,
    GUI_BTN_SECTOR_CEIL_MIN_MINUS,
    GUI_BTN_SECTOR_CEIL_MIN_PLUS,
    GUI_BTN_SECTOR_CEIL_MAX_MINUS,
    GUI_BTN_SECTOR_CEIL_MAX_PLUS,
    GUI_BTN_SECTOR_FLOOR_FLOW_MINUS,
    GUI_BTN_SECTOR_FLOOR_FLOW_PLUS,
    GUI_BTN_SECTOR_CEIL_FLOW_MINUS,
    GUI_BTN_SECTOR_CEIL_FLOW_PLUS,
    GUI_BTN_SECTOR_COPY_PROPS,
    GUI_BTN_SECTOR_PASTE_PROPS,

    GUI_BTN_WALL_COPY_PROPS,
    GUI_BTN_WALL_PASTE_PROPS,
    GUI_BTN_WALL_OPENBOT_MINUS,
    GUI_BTN_WALL_OPENBOT_PLUS,
    GUI_BTN_WALL_OPENTOP_MINUS,
    GUI_BTN_WALL_OPENTOP_PLUS,
    GUI_BTN_WALL_TEX_SX_MINUS,
    GUI_BTN_WALL_TEX_SX_PLUS,
    GUI_BTN_WALL_TEX_SY_MINUS,
    GUI_BTN_WALL_TEX_SY_PLUS,
    GUI_BTN_WALL_TEX_ROT_MINUS,
    GUI_BTN_WALL_TEX_ROT_PLUS,
    GUI_BTN_WALL_TEX_ROT_RESET,
    GUI_BTN_WALL_TEX_BRIGHT_MINUS,
    GUI_BTN_WALL_TEX_BRIGHT_PLUS,

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
    ED_PENDING_LEFT_NONE = 0,
    ED_PENDING_LEFT_MULTI_DRAG,
    ED_PENDING_LEFT_VERTEX,
    ED_PENDING_LEFT_WALL_CLICK_OR_BOX,
    ED_PENDING_LEFT_WALL_DRAG,
    ED_PENDING_LEFT_SECTOR_CLICK_OR_BOX,
    ED_PENDING_LEFT_SECTOR_DRAG,
    ED_PENDING_LEFT_EMPTY_CLICK,
    ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX
} EdPendingLeftAction;

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
    int index;
    float depth;
} IsoSortEntry;


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
    uint32_t tex_flags;  // texture clamp/uv behaviour flags, packed wall brightness
    float texScaleX;
    float texScaleY;
} EdWall;


typedef struct {
    int wallStart;
    int wallCount;
    int boundaryCount;

    float floorHeight;
    float ceilHeight;

    uint8_t floorColor;
    uint8_t ceilColor;
    uint8_t glowlevel;

    int tagId;
    uint32_t stateFlags;
    float floorMinHeight;
    float floorMaxHeight;
    float ceilMinHeight;
    float ceilMaxHeight;
    float floorFlowHeight;
    float ceilFlowHeight;

    float floorTexScaleX;
    float floorTexScaleY;
    float floorTexAngle;

    float ceilTexScaleX;
    float ceilTexScaleY;
    float ceilTexAngle;
} EdSector;

typedef struct {
    float floorHeight;
    float ceilHeight;
    uint8_t floorColor;
    uint8_t ceilColor;
    uint8_t glowlevel;

    int tagId;
    uint32_t stateFlags;
    float floorMinHeight;
    float floorMaxHeight;
    float ceilMinHeight;
    float ceilMaxHeight;
    float floorFlowHeight;
    float ceilFlowHeight;

    float floorTexScaleX;
    float floorTexScaleY;
    float floorTexAngle;

    float ceilTexScaleX;
    float ceilTexScaleY;
    float ceilTexAngle;
} EdSectorClipboard;

typedef struct {
    float openBottom;
    float openTop;
    uint8_t upperColor;
    uint8_t midColor;
    uint8_t lowerColor;
    uint8_t flags;
    uint32_t tex_flags;
    float texScaleX;
    float texScaleY;
} EdWallClipboard;

typedef struct {
    EdVec2 verts[ED_MAX_VERTS];
    int vertCount;

    EdWall walls[ED_MAX_WALLS];
    int wallCount;
    EdSector sectors[ED_MAX_SECTORS];
    int sectorCount;

    float anchorX;
    float anchorY;
} EdSectorGeometryClipboard;


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
    EdSectorClipboard copiedSectorProps;
    int hasCopiedSectorProps;
    int copiedSectorPropsSourceSector;
    uint8_t copiedWallUpperColor;
    uint8_t copiedWallMidColor;
    uint8_t copiedWallLowerColor;
    int hasCopiedWallTexture;
    float copiedWallTexScaleX;
    float copiedWallTexScaleY;
    int hasCopiedWallScale;
    float copiedWallTexAngle;
    int hasCopiedWallRotation;
    EdWallClipboard copiedWallProps;
    int hasCopiedWallProps;
    int copiedWallPropsSourceWall;
    EdSectorGeometryClipboard copiedSectorGeometry;
    int hasCopiedSectorGeometry;
    int copiedSectorGeometrySourceSector;


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

    /* multi-wall selection */
    uint8_t selectedWalls[ED_MAX_WALLS];
    int selectedWallCount;

    /* multi-sector selection */
    uint8_t selectedSectors[ED_MAX_SECTORS];
    int selectedSectorCount;

    /* box select */
    int boxSelecting;
    int boxSelectWalls;
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
    int   dragWallVertCount;
    int   dragWallVertIndices[ED_MAX_VERTS];
    float dragWallVertStartX[ED_MAX_VERTS];
    float dragWallVertStartY[ED_MAX_VERTS];

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
    int bUseTextureFill;
    int isometricView;
    int textureBrowserOffset;
    int textureBrowserTarget;

    
    int textureScrollbarDragging;
    int textureScrollbarDragOffsetY;

    int pendingLeftMouseDown;
    EdPendingLeftAction pendingLeftAction;
    int pendingLeftMouseX;
    int pendingLeftMouseY;
    float pendingLeftWorldX;
    float pendingLeftWorldY;
    int pendingLeftTargetIndex;
    int pendingLeftCtrlDown;
    int pendingLeftAltDown;
    int pendingLeftBoxSelectWalls;

} EditorState;



static EditorMap g_edMap;
static EditorState g_ed;
static void clearAllSelections(void);

static void clearMultiWallSelection(void)
{
    memset(g_ed.selectedWalls, 0, sizeof(g_ed.selectedWalls));
    g_ed.selectedWallCount = 0;
}

static int hasSingleWallSelection(void)
{
    return (g_ed.selectionType == ED_SEL_WALL) &&
           (g_ed.selectedWall >= 0) &&
           (g_ed.selectedWall < g_edMap.wallCount);
}

static int hasMultiWallSelection(void)
{
    return (g_ed.selectionType == ED_SEL_NONE) &&
           (g_ed.selectedWallCount > 0);
}

static int hasAnyWallEditSelection(void)
{
    return hasSingleWallSelection() || (g_ed.selectedWallCount > 0);
}

static void clearMultiSectorSelection(void)
{
    memset(g_ed.selectedSectors, 0, sizeof(g_ed.selectedSectors));
    g_ed.selectedSectorCount = 0;
}

static int hasSingleSectorSelection(void)
{
    return (g_ed.selectionType == ED_SEL_SECTOR) &&
           (g_ed.selectedSector >= 0) &&
           (g_ed.selectedSector < g_edMap.sectorCount);
}

static int hasMultiSectorSelection(void)
{
    return (g_ed.selectionType == ED_SEL_NONE) &&
           (g_ed.selectedSectorCount > 0);
}

static int hasAnySectorEditSelection(void)
{
    return hasSingleSectorSelection() || (g_ed.selectedSectorCount > 0);
}

static int getFirstMultiSelectedSectorIndex(void)
{
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        if (g_ed.selectedSectors[i]) {
            return i;
        }
    }

    return -1;
}

static int getPrimarySectorEditIndex(void)
{
    if (hasSingleSectorSelection()) {
        return g_ed.selectedSector;
    }

    return getFirstMultiSelectedSectorIndex();
}

static int isSectorInEditSelection(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return 0;
    }

    if (hasSingleSectorSelection()) {
        return g_ed.selectedSector == sectorIndex;
    }

    return g_ed.selectedSectors[sectorIndex] != 0;
}

static void addMultiSectorSelection(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;
    if (g_ed.selectedSectors[sectorIndex]) return;

    g_ed.selectedSectors[sectorIndex] = 1;
    g_ed.selectedSectorCount++;
}

static void removeMultiSectorSelection(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;
    if (!g_ed.selectedSectors[sectorIndex]) return;

    g_ed.selectedSectors[sectorIndex] = 0;
    if (g_ed.selectedSectorCount > 0) {
        g_ed.selectedSectorCount--;
    }
}

static void normalizeSectorSelectionState(void)
{
    if (g_ed.selectedSectorCount == 1 && g_ed.selectionType == ED_SEL_NONE) {
        const int sectorIndex = getFirstMultiSelectedSectorIndex();

        clearMultiSectorSelection();
        if (sectorIndex >= 0 && sectorIndex < g_edMap.sectorCount) {
            g_ed.selectionType = ED_SEL_SECTOR;
            g_ed.selectedSector = sectorIndex;
        }
    }

    if (g_ed.selectedSectorCount <= 0) {
        clearMultiSectorSelection();
    }

    if (!hasSingleSectorSelection() && g_ed.selectionType == ED_SEL_SECTOR) {
        g_ed.selectionType = ED_SEL_NONE;
        g_ed.selectedSector = -1;
    }
}

static void toggleSectorMultiSelection(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return;
    }

    if (g_ed.selectionType == ED_SEL_VERTEX ||
        g_ed.selectionType == ED_SEL_WALL ||
        g_ed.selectedVertCount > 0 ||
        g_ed.selectedWallCount > 0) {
        clearAllSelections();
    }

    if (hasSingleSectorSelection()) {
        addMultiSectorSelection(g_ed.selectedSector);
        g_ed.selectionType = ED_SEL_NONE;
        g_ed.selectedSector = -1;
    }

    if (g_ed.selectedSectors[sectorIndex]) {
        removeMultiSectorSelection(sectorIndex);
    } else {
        addMultiSectorSelection(sectorIndex);
    }

    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    normalizeSectorSelectionState();
}

static int collectSectorEditSelectionIndices(int *outIndices, int maxOut)
{
    int count = 0;

    if (!outIndices || maxOut <= 0) {
        return 0;
    }

    if (hasSingleSectorSelection()) {
        outIndices[0] = g_ed.selectedSector;
        return 1;
    }

    for (int i = 0; i < g_edMap.sectorCount && count < maxOut; i++) {
        if (!g_ed.selectedSectors[i]) continue;
        outIndices[count++] = i;
    }

    return count;
}

static void remapMultiSectorSelectionFromOldToNew(const int *sectorRemap, int oldSectorCount)
{
    uint8_t oldSelected[ED_MAX_SECTORS];

    if (!sectorRemap || oldSectorCount <= 0) {
        clearMultiSectorSelection();
        return;
    }

    memcpy(oldSelected, g_ed.selectedSectors, sizeof(oldSelected));
    clearMultiSectorSelection();

    for (int i = 0; i < oldSectorCount && i < ED_MAX_SECTORS; i++) {
        const int remapped = sectorRemap[i];

        if (!oldSelected[i]) continue;
        if (remapped < 0 || remapped >= g_edMap.sectorCount) continue;
        addMultiSectorSelection(remapped);
    }

    normalizeSectorSelectionState();
}

static int getWallEditSelectionCount(void)
{
    if (hasSingleWallSelection()) {
        return 1;
    }

    return g_ed.selectedWallCount;
}

static int getFirstMultiSelectedWallIndex(void)
{
    for (int i = 0; i < g_edMap.wallCount; i++) {
        if (g_ed.selectedWalls[i]) {
            return i;
        }
    }

    return -1;
}

static int getPrimaryWallEditIndex(void)
{
    if (hasSingleWallSelection()) {
        return g_ed.selectedWall;
    }

    return getFirstMultiSelectedWallIndex();
}

static int isWallInEditSelection(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return 0;
    }

    if (hasSingleWallSelection()) {
        return g_ed.selectedWall == wallIndex;
    }

    return g_ed.selectedWalls[wallIndex] != 0;
}

static void addMultiWallSelection(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;
    if (g_ed.selectedWalls[wallIndex]) return;

    g_ed.selectedWalls[wallIndex] = 1;
    g_ed.selectedWallCount++;
}

static void removeMultiWallSelection(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;
    if (!g_ed.selectedWalls[wallIndex]) return;

    g_ed.selectedWalls[wallIndex] = 0;
    if (g_ed.selectedWallCount > 0) {
        g_ed.selectedWallCount--;
    }
}

static void normalizeWallSelectionState(void)
{
    if (g_ed.selectedWallCount == 1 && g_ed.selectionType == ED_SEL_NONE) {
        const int wallIndex = getFirstMultiSelectedWallIndex();

        clearMultiWallSelection();
        if (wallIndex >= 0 && wallIndex < g_edMap.wallCount) {
            g_ed.selectionType = ED_SEL_WALL;
            g_ed.selectedWall = wallIndex;
        }
    }

    if (g_ed.selectedWallCount <= 0) {
        clearMultiWallSelection();
    }

    if (!hasSingleWallSelection() && g_ed.selectionType == ED_SEL_WALL) {
        g_ed.selectionType = ED_SEL_NONE;
        g_ed.selectedWall = -1;
    }
}

static void toggleWallMultiSelection(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return;
    }

    if (g_ed.selectionType == ED_SEL_VERTEX ||
        g_ed.selectionType == ED_SEL_SECTOR ||
        g_ed.selectedVertCount > 0 ||
        g_ed.selectedSectorCount > 0) {
        clearAllSelections();
    }

    if (hasSingleWallSelection()) {
        addMultiWallSelection(g_ed.selectedWall);
        g_ed.selectionType = ED_SEL_NONE;
        g_ed.selectedWall = -1;
    }

    if (g_ed.selectedWalls[wallIndex]) {
        removeMultiWallSelection(wallIndex);
    } else {
        addMultiWallSelection(wallIndex);
    }

    g_ed.selectedVert = -1;
    g_ed.selectedSector = -1;
    normalizeWallSelectionState();
}

static int collectWallEditSelectionIndices(int *outIndices, int maxOut)
{
    int count = 0;

    if (!outIndices || maxOut <= 0) {
        return 0;
    }

    if (hasSingleWallSelection()) {
        outIndices[0] = g_ed.selectedWall;
        return 1;
    }

    for (int i = 0; i < g_edMap.wallCount && count < maxOut; i++) {
        if (!g_ed.selectedWalls[i]) continue;
        outIndices[count++] = i;
    }

    return count;
}

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
    EdSectorClipboard copiedSectorProps;
    int hasCopiedSectorProps;
    int copiedSectorPropsSourceSector;
    uint8_t copiedWallUpperColor;
    uint8_t copiedWallMidColor;
    uint8_t copiedWallLowerColor;
    int hasCopiedWallTexture;
    float copiedWallTexScaleX;
    float copiedWallTexScaleY;
    int hasCopiedWallScale;
    float copiedWallTexAngle;
    int hasCopiedWallRotation;
    EdWallClipboard copiedWallProps;
    int hasCopiedWallProps;
    int copiedWallPropsSourceWall;
    EdSectorGeometryClipboard copiedSectorGeometry;
    int hasCopiedSectorGeometry;
    int copiedSectorGeometrySourceSector;

    int selectedVert;
    int selectedWall;
    int selectedSector;

    uint8_t selectedVerts[ED_MAX_VERTS];
    int selectedVertCount;
    uint8_t selectedWalls[ED_MAX_WALLS];
    int selectedWallCount;
    uint8_t selectedSectors[ED_MAX_SECTORS];
    int selectedSectorCount;

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
static int pointInSector(float px, float py, int sectorIndex);
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
static void clearMultiWallSelection(void);
static void setWallTexScaleX(EdWall *w, float scaleX);
static void setWallTexScaleY(EdWall *w, float scaleY);
static float getWallTexAngle(const EdWall *w);
static void clearPendingLeftMouseAction(void);
static void beginBoxSelect(int mouseX, int mouseY, int selectWalls);
static void updateBoxSelect(int mouseX, int mouseY);
static void beginMultiVertexDrag(float worldX, float worldY);
static void beginSectorDrag(int sectorIndex, float worldX, float worldY);
static int findBoundaryWallNearPointInSector(int sectorIndex, float wx, float wy);
static int splitBoundaryWallAtPoint(int sectorIndex, int localWallIndex, float wx, float wy);
static float distPointSegSq(float px, float py, float ax, float ay, float bx, float by);
static void cleanMapCompactWithReport(int *removedVerts, int *removedWalls, int *removedSectors);
static void sanitizeSectorProperties(EdSector *sec);
static void drawStatusPopup(void);
static void acceptConfirmDialog(void);
static void closeConfirmDialog(void);
static void openConfirmDialog(EdConfirmAction action, const char *text);
static int buildInnerSectorsFromSelectedSector(void);
static float innerLoopSignedAreaFromWalls(const int *wallIndices, int count);
static void remapMultiSectorSelectionFromOldToNew(const int *sectorRemap, int oldSectorCount);
static void resetSectorGeometryClipboard(void);
static void copySelectedSectorGeometryToClipboard(void);
static int pasteSectorGeometryFromClipboard(float targetWorldX, float targetWorldY);

static void pathDirnameFromFile(char *outDir, size_t outDirSize, const char *path);
static void initRememberedDialogDirs(void);

// rotation of selections - prototypes
static int collectSelectedVertexPivot(float *outCx, float *outCy);
static void rotateSelectedVertices(float angleRad);
static void drawSectorSelectionHighlight(int sectorIndex);

// sector fill stuffs, prototypes
static uint8_t getSectorFillColour(int sectorIndex);
static int getSectorFillStepY(int sectorIndex);
static int wrapTextureCoordLocal(int v, int size);
static void drawTexturedSectorSpan(int x0, int x1, int y, const EdSector *sec);
static void drawTexturedSector2DBruteForceScanline(int sectorIndex,
                                                   int minX,
                                                   int maxX,
                                                   int y,
                                                   const EdSector *sec);
static void drawVectorSector2DBruteForceScanline(int sectorIndex,
                                                 int minX,
                                                 int maxX,
                                                 int y,
                                                 uint8_t fillCol);
static void drawFilledSector2D(int sectorIndex);
static void drawFilledSectorIsoBruteForceScanline(int sectorIndex,
                                                  int minX,
                                                  int maxX,
                                                  int y,
                                                  const EdSector *sec);
static void screenToIsoWorldOnPlaneF(float sx, float sy, float wz, float *wx, float *wy);
static void screenToIsoWorldOnPlane(int sx, int sy, float wz, float *wx, float *wy);
static void worldToIsoScreen(float wx, float wy, float wz, int *sx, int *sy);
static void drawIsometricPreview(void);

/////// GUI PARTS
static void handleEditorUI(int mouseX, int mouseY, int leftDown, int leftPressed, int leftReleased, float worldX, float worldY);
static void refreshEditorUIButtonState(void);
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
static int hasSingleWallSelection(void);
static int hasMultiWallSelection(void);
static int hasAnyWallEditSelection(void);
static int getWallEditSelectionCount(void);
static int getPrimaryWallEditIndex(void);
static int isWallInEditSelection(int wallIndex);
static void normalizeWallSelectionState(void);
static void toggleWallMultiSelection(int wallIndex);
static int collectWallEditSelectionIndices(int *outIndices, int maxOut);


#define ED_GUI_DIRTY_FRAME_COUNT 2

int g_dirtyGui = 0;

void rc3dGuiDirty(void){
    if (g_dirtyGui < ED_GUI_DIRTY_FRAME_COUNT) {
        g_dirtyGui = ED_GUI_DIRTY_FRAME_COUNT;
    }
}

int rc3dGuiCheckDirty(void){
    if (g_dirtyGui > 0) {
        g_dirtyGui--;
        return 1;
    }

    return 0;
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

// images may need shrinkin, or scaling at least to fit the THUMB-W/Hs
#define TEX_THUMB_W             64
#define TEX_THUMB_H             64

#define TEX_BROWSER_COLS            3
#define TEX_BROWSER_ROWS            8
#define TEX_BROWSER_CELL_W          66
#define TEX_BROWSER_CELL_H          66
#define TEX_BROWSER_CELL_GAP_X      8
#define TEX_BROWSER_CELL_GAP_Y      6
#define TEX_BROWSER_HEADER_H      32
#define TEX_BROWSER_GRID_Y        56
#define TEX_BROWSER_SCROLL_W      24
#define TEX_BROWSER_SCROLL_PAD    6
#define TEX_BROWSER_BOTTOM_PAD    10
#define TEX_BROWSER_OUTER_PAD     8
#define TEX_BROWSER_BUTTON_H      20
#define TEX_BROWSER_BUTTON_GAP    6
#define TEX_BROWSER_PREVIEW_PAD   4
#define TEX_BROWSER_FRAME_PAD     2
#define TEX_BROWSER_LABEL_GAP     2
#define TEX_BROWSER_LABEL_TEXT_W  (ED_FONT_W * 3)

#define TEX_BROWSER_PANEL_WIDTH     320

#define TEX_BROWSER_OUTER_W         (TEX_THUMB_W + (TEX_BROWSER_PREVIEW_PAD * 2) + (TEX_BROWSER_FRAME_PAD * 4))
#define TEX_BROWSER_OUTER_H         (TEX_THUMB_H + (TEX_BROWSER_PREVIEW_PAD * 2) + (TEX_BROWSER_FRAME_PAD * 4))
#define TEX_BROWSER_CELL_USED_W     ((TEX_BROWSER_CELL_W > TEX_BROWSER_OUTER_W) ? TEX_BROWSER_CELL_W : TEX_BROWSER_OUTER_W)
#define TEX_BROWSER_CELL_USED_H     ((TEX_BROWSER_CELL_H > (TEX_BROWSER_OUTER_H + TEX_BROWSER_LABEL_GAP + ED_FONT_H)) ? TEX_BROWSER_CELL_H : (TEX_BROWSER_OUTER_H + TEX_BROWSER_LABEL_GAP + ED_FONT_H))
#define ED_INSPECTOR_TEXTURE_WINDOW_HEIGHT      (TEX_BROWSER_GRID_Y + (TEX_BROWSER_ROWS * TEX_BROWSER_CELL_USED_H) + ((TEX_BROWSER_ROWS - 1) * TEX_BROWSER_CELL_GAP_Y) + TEX_BROWSER_SCROLL_PAD)

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

typedef struct {
    TextureTarget target;
    const char *label;
} TextureBrowserTargetButtonDef;

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

static uint8_t *getTexturePtr(int index)
{
    char filename[256];

    if (index < 0 || index >= TEXTURE_LIBRARY_COUNT) {
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
        LoadPPB(filename, g_textureCache[index], TEXTURE_WIDTH, TEXTURE_HEIGHT);
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
            const int srcY = (ty * TEXTURE_HEIGHT) / TEX_THUMB_H;
            const int srcX = (tx * TEXTURE_WIDTH) / TEX_THUMB_W;
            uint8_t c = tex[(srcY * TEXTURE_WIDTH) + srcX];
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

static int isTextureBrowserVisible(void)
{
    if (g_ed.confirmVisible ||
        g_ed.undoHistoryVisible ||
        g_ed.ui_validator_visable ||
        g_ed.ui_menu_visable) {
        return 0;
    }

    return hasAnyWallEditSelection() || hasAnySectorEditSelection();
}

static int wallTextureTargetIndexForWall(const EdWall *w, TextureTarget t)
{
    if (!w) {
        return -1;
    }

    switch (t) {
        case TEX_TARGET_WALL_UPPER:  return w->upperColor;
        case TEX_TARGET_WALL_MIDDLE: return w->midColor;
        case TEX_TARGET_WALL_LOWER:  return w->lowerColor;
        default:                     return -1;
    }
}

static int textureTargetCurrentIndex(TextureTarget t)
{
    if (hasAnyWallEditSelection()) {
        const int firstWall = getPrimaryWallEditIndex();

        if (firstWall >= 0 && firstWall < g_edMap.wallCount) {
            const int current = wallTextureTargetIndexForWall(&g_edMap.walls[firstWall], t);

            if (hasSingleWallSelection()) {
                return current;
            }

            for (int i = 0; i < g_edMap.wallCount; i++) {
                if (!g_ed.selectedWalls[i]) continue;
                if (wallTextureTargetIndexForWall(&g_edMap.walls[i], t) != current) {
                    return -1;
                }
            }

            return current;
        }
    }

    if (hasAnySectorEditSelection()) {
        int sectorIndices[ED_MAX_SECTORS];
        const int sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);

        if (sectorCount > 0) {
            int current = -1;

            switch (t) {
                case TEX_TARGET_SECTOR_FLOOR:
                    current = g_edMap.sectors[sectorIndices[0]].floorColor;
                    break;

                case TEX_TARGET_SECTOR_CEIL:
                    current = g_edMap.sectors[sectorIndices[0]].ceilColor;
                    break;

                default:
                    break;
            }

            if (current >= 0) {
                for (int i = 1; i < sectorCount; i++) {
                    const EdSector *s = &g_edMap.sectors[sectorIndices[i]];
                    const int other = (t == TEX_TARGET_SECTOR_FLOOR)
                        ? s->floorColor
                        : s->ceilColor;

                    if (other != current) {
                        return -1;
                    }
                }

                return current;
            }
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

static void textureTargetApply(TextureTarget t, int texIndex)
{
    if (texIndex < 0 || texIndex >= TEXTURE_LIBRARY_COUNT) {
        return;
    }

    if (hasAnyWallEditSelection()) {
        int wallIndices[ED_MAX_WALLS];
        const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

        for (int i = 0; i < wallCount; i++) {
            EdWall *w = &g_edMap.walls[wallIndices[i]];

            switch (t) {
                case TEX_TARGET_WALL_UPPER:  w->upperColor = (uint8_t)texIndex; break;
                case TEX_TARGET_WALL_MIDDLE: w->midColor   = (uint8_t)texIndex; break;
                case TEX_TARGET_WALL_LOWER:  w->lowerColor = (uint8_t)texIndex; break;
                default: break;
            }
        }

        if (wallCount > 0) {
            return;
        }
    }

    if (hasAnySectorEditSelection()) {
        int sectorIndices[ED_MAX_SECTORS];
        const int sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);

        for (int i = 0; i < sectorCount; i++) {
            EdSector *s = &g_edMap.sectors[sectorIndices[i]];

            switch (t) {
                case TEX_TARGET_SECTOR_FLOOR:
                    s->floorColor = (uint8_t)texIndex;
                    break;

                case TEX_TARGET_SECTOR_CEIL:
                    s->ceilColor = (uint8_t)texIndex;
                    break;

                default:
                    break;
            }
        }

        if (sectorCount > 0 &&
            (t == TEX_TARGET_SECTOR_FLOOR || t == TEX_TARGET_SECTOR_CEIL)) {
            syncAllPortals();
            return;
        }
    }

    switch (t) {
        case TEX_TARGET_DEFAULT_WALL_UPPER:   g_ed.newWallUpperColor = (uint8_t)texIndex; return;
        case TEX_TARGET_DEFAULT_WALL_MIDDLE:  g_ed.newWallMidColor   = (uint8_t)texIndex; return;
        case TEX_TARGET_DEFAULT_WALL_LOWER:   g_ed.newWallLowerColor = (uint8_t)texIndex; return;
        case TEX_TARGET_DEFAULT_SECTOR_FLOOR: g_ed.sectorFloorColor  = (uint8_t)texIndex; return;
        case TEX_TARGET_DEFAULT_SECTOR_CEIL:  g_ed.sectorCeilColor   = (uint8_t)texIndex; return;
        default: return;
    }
}

static void getTextureBrowserRect(int *x, int *y, int *w, int *h)
{
    *x = ED_PANEL_X + 2;
    *w = TEX_BROWSER_PANEL_WIDTH - 16;
    *h = ED_INSPECTOR_TEXTURE_WINDOW_HEIGHT;    // long def!! urgg
    *y = ED_PANEL_Y;
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

static int getTextureBrowserTargetButtons(TextureBrowserTargetButtonDef *outButtons, int maxButtons)
{
    int count = 0;

    if (!outButtons || maxButtons <= 0) {
        return 0;
    }

    if (hasAnyWallEditSelection()) {
        if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_WALL_UPPER, "Upper" };
        if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_WALL_MIDDLE, "Middle" };
        if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_WALL_LOWER, "Lower" };
        return count;
    }

    if (hasAnySectorEditSelection()) {
        if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_SECTOR_FLOOR, "Floor" };
        if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_SECTOR_CEIL, "Ceiling" };
        return count;
    }

    if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_DEFAULT_SECTOR_FLOOR, "F.Floor" };
    if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_DEFAULT_SECTOR_CEIL, "F.Ceil" };
    if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_DEFAULT_WALL_UPPER, "W.Up" };
    if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_DEFAULT_WALL_MIDDLE, "W.Mid" };
    if (count < maxButtons) outButtons[count++] = (TextureBrowserTargetButtonDef){ TEX_TARGET_DEFAULT_WALL_LOWER, "W.Low" };
    return count;
}

static void getTextureBrowserTargetButtonRect(int index, int buttonCount, int *x, int *y, int *w, int *h)
{
    int bx, by, bw, bh;
    int availableW;
    int buttonW;

    getTextureBrowserRect(&bx, &by, &bw, &bh);
    (void)bh;

    availableW = bw - (TEX_BROWSER_OUTER_PAD * 2);
    if (buttonCount <= 0) {
        buttonCount = 1;
    }

    buttonW = (availableW - (TEX_BROWSER_BUTTON_GAP * (buttonCount - 1))) / buttonCount;
    if (buttonW < 32) {
        buttonW = 32;
    }

    *x = bx + TEX_BROWSER_OUTER_PAD + (index * (buttonW + TEX_BROWSER_BUTTON_GAP));
    *y = by + 22;
    *w = buttonW;
    *h = TEX_BROWSER_BUTTON_H;
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
    (void)bh;

    *x = bx + TEX_BROWSER_OUTER_PAD;
    *y = by + TEX_BROWSER_GRID_Y;
    *w = (TEX_BROWSER_COLS * TEX_BROWSER_CELL_USED_W) +
         ((TEX_BROWSER_COLS - 1) * TEX_BROWSER_CELL_GAP_X);
    *h = (TEX_BROWSER_ROWS * TEX_BROWSER_CELL_USED_H) +
         ((TEX_BROWSER_ROWS - 1) * TEX_BROWSER_CELL_GAP_Y);
}

static void getTextureBrowserScrollbarRect(int *x, int *y, int *w, int *h)
{
    int bx, by, bw, bh;

    getTextureBrowserRect(&bx, &by, &bw, &bh);

    *w = TEX_BROWSER_SCROLL_W;
    *h = bh - TEX_BROWSER_GRID_Y - TEX_BROWSER_SCROLL_PAD;
    if (*h < 16) *h = 16;

    *x = bx + bw - TEX_BROWSER_SCROLL_W - TEX_BROWSER_SCROLL_PAD;
    *y = by + TEX_BROWSER_GRID_Y;
}

static void getTextureBrowserCellRect(int col, int row, int *x, int *y, int *w, int *h)
{
    int gx, gy, gw, gh;

    getTextureBrowserGridRect(&gx, &gy, &gw, &gh);
    (void)gw;
    (void)gh;

    *x = gx + (col * (TEX_BROWSER_CELL_USED_W + TEX_BROWSER_CELL_GAP_X));
    *y = gy + (row * (TEX_BROWSER_CELL_USED_H + TEX_BROWSER_CELL_GAP_Y));
    *w = TEX_BROWSER_CELL_USED_W;
    *h = TEX_BROWSER_CELL_USED_H;
}

static void getTextureBrowserCellLayout(int cellX, int cellY, int cellW, int cellH,
                                        int *frameX, int *frameY, int *frameW, int *frameH,
                                        int *thumbX, int *thumbY, int *labelX, int *labelY)
{
    const int previewW = TEX_THUMB_W + (TEX_BROWSER_PREVIEW_PAD * 2);
    const int previewH = TEX_THUMB_H + (TEX_BROWSER_PREVIEW_PAD * 2);
    const int outerW = previewW + (TEX_BROWSER_FRAME_PAD * 2);
    const int outerH = previewH + (TEX_BROWSER_FRAME_PAD * 2);
    const int contentH = TEX_BROWSER_OUTER_H + TEX_BROWSER_LABEL_GAP + ED_FONT_H;
    const int contentY = cellY + ((cellH - contentH) / 2);
    const int outerX = cellX + ((cellW - TEX_BROWSER_OUTER_W) / 2);

    *frameW = outerW;
    *frameH = outerH;

    *frameX = outerX + TEX_BROWSER_FRAME_PAD;
    *frameY = contentY + TEX_BROWSER_FRAME_PAD;
    if (*frameY < cellY + TEX_BROWSER_FRAME_PAD) {
        *frameY = cellY + TEX_BROWSER_FRAME_PAD;
    }

    *labelX = cellX + ((cellW - TEX_BROWSER_LABEL_TEXT_W) / 2);
    if (*labelX < cellX) {
        *labelX = cellX;
    }
    *labelY = contentY + TEX_BROWSER_OUTER_H + TEX_BROWSER_LABEL_GAP;

    *thumbX = *frameX + TEX_BROWSER_FRAME_PAD + TEX_BROWSER_PREVIEW_PAD;
    *thumbY = *frameY + TEX_BROWSER_FRAME_PAD + TEX_BROWSER_PREVIEW_PAD;
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
    TextureBrowserTargetButtonDef buttons[5];
    int buttonCount;
    const char *targetName;
    int targetNameX;

    if (!isTextureBrowserVisible()) {
        return;
    }

    clampTextureBrowserOffset();
    getTextureBrowserRect(&bx, &by, &bw, &bh);

    drawRect(bx, by, bw, bh, 16);
    drawRectL(bx, by, bw, bh, ED_UI_BORDER);

    buttonCount = getTextureBrowserTargetButtons(buttons, 5);
    targetName = textureTargetName((TextureTarget)g_ed.textureBrowserTarget);
    targetNameX = bx + bw - TEX_BROWSER_SCROLL_PAD - ((int)strlen(targetName) * ED_FONT_W);
    if (targetNameX < bx + 170) {
        targetNameX = bx + 170;
    }

    drawText(bx + 6, by + 4, "TEXTURE EXPLORER", ED_TEXTURE_EXPLORER_HEADER_TEXT);
    drawText(targetNameX, by + 4, targetName, ED_TEXT_COL);

    /* target select row */
    for (int i = 0; i < buttonCount; i++) {
        int btnX, btnY, btnW, btnH;

        getTextureBrowserTargetButtonRect(i, buttonCount, &btnX, &btnY, &btnW, &btnH);
        drawTextureTargetButton(btnX, btnY, btnW, btnH, buttons[i].label,
                                g_ed.textureBrowserTarget == (int)buttons[i].target);
    }


    cellStartIndex = g_ed.textureBrowserOffset * TEX_BROWSER_COLS;
    selectedIndex = textureTargetCurrentIndex((TextureTarget)g_ed.textureBrowserTarget);

    for (ty = 0; ty < TEX_BROWSER_ROWS; ty++) {
        for (tx = 0; tx < TEX_BROWSER_COLS; tx++) {
            int slot = (ty * TEX_BROWSER_COLS) + tx;
            int texIndex = cellStartIndex + slot;
            int cx, cy, cw, ch;
            int frameX, frameY, frameW, frameH;
            int thumbX, thumbY;
            int labelX, labelY;

            if (texIndex >= TEXTURE_LIBRARY_COUNT) {
                continue;
            }

            getTextureBrowserCellRect(tx, ty, &cx, &cy, &cw, &ch);
            getTextureBrowserCellLayout(cx, cy, cw, ch,
                                        &frameX, &frameY, &frameW, &frameH,
                                        &thumbX, &thumbY, &labelX, &labelY);

            drawRect(frameX - TEX_BROWSER_FRAME_PAD, frameY - TEX_BROWSER_FRAME_PAD,
                     frameW + (TEX_BROWSER_FRAME_PAD * 2), frameH + (TEX_BROWSER_FRAME_PAD * 2), 0);

            if (texIndex == selectedIndex) {
                drawRectL(frameX - TEX_BROWSER_FRAME_PAD, frameY - TEX_BROWSER_FRAME_PAD,
                          frameW + (TEX_BROWSER_FRAME_PAD * 2), frameH + (TEX_BROWSER_FRAME_PAD * 2),
                          ED_TEXTURE_EXPLORER_SELECT_FRAME);
                drawRectL(frameX - 1, frameY - 1, frameW + 2, frameH + 2, ED_TEXTURE_EXPLORER_SELECT_FRAME);
                drawRectL(frameX, frameY, frameW, frameH, ED_TEXTURE_EXPLORER_SELECT_FRAME);
            } else {
                drawRectL(frameX - TEX_BROWSER_FRAME_PAD, frameY - TEX_BROWSER_FRAME_PAD,
                          frameW + (TEX_BROWSER_FRAME_PAD * 2), frameH + (TEX_BROWSER_FRAME_PAD * 2), 6);
            }

            drawTextureThumb(thumbX, thumbY, getTexturePtr(texIndex));

            {
                char tbuf[16];
                snprintf(tbuf, sizeof(tbuf), "$%02X", texIndex);
                drawText(labelX, labelY, tbuf, ED_TEXT_COL);
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

    if (!isTextureBrowserVisible()) {
        g_ed.textureScrollbarDragging = 0;
        return 0;
    }

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
            TextureBrowserTargetButtonDef buttons[5];
            const int buttonCount = getTextureBrowserTargetButtons(buttons, 5);

            for (int i = 0; i < buttonCount; i++) {
                int btnX, btnY, btnW, btnH;

                getTextureBrowserTargetButtonRect(i, buttonCount, &btnX, &btnY, &btnW, &btnH);
                if (pointInRectLocal(mouseX, mouseY, btnX, btnY, btnW, btnH)) {
                    g_ed.textureBrowserTarget = buttons[i].target;
                    return 1;
                }
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
                        int cx, cy, cw, ch;

                        (void)slot;

                        if (texIndex >= TEXTURE_LIBRARY_COUNT) {
                            continue;
                        }

                        getTextureBrowserCellRect(tx, ty, &cx, &cy, &cw, &ch);

                        if (pointInRectLocal(mouseX, mouseY, cx, cy, cw, ch)) {
                            pushUndoState();
                            textureTargetApply((TextureTarget)g_ed.textureBrowserTarget, texIndex);
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
    s->copiedSectorProps = g_ed.copiedSectorProps;
    s->hasCopiedSectorProps = g_ed.hasCopiedSectorProps;
    s->copiedSectorPropsSourceSector = g_ed.copiedSectorPropsSourceSector;
    s->copiedWallUpperColor = g_ed.copiedWallUpperColor;
    s->copiedWallMidColor = g_ed.copiedWallMidColor;
    s->copiedWallLowerColor = g_ed.copiedWallLowerColor;
    s->hasCopiedWallTexture = g_ed.hasCopiedWallTexture;
    s->copiedWallTexScaleX = g_ed.copiedWallTexScaleX;
    s->copiedWallTexScaleY = g_ed.copiedWallTexScaleY;
    s->hasCopiedWallScale = g_ed.hasCopiedWallScale;
    s->copiedWallTexAngle = g_ed.copiedWallTexAngle;
    s->hasCopiedWallRotation = g_ed.hasCopiedWallRotation;
    s->copiedWallProps = g_ed.copiedWallProps;
    s->hasCopiedWallProps = g_ed.hasCopiedWallProps;
    s->copiedWallPropsSourceWall = g_ed.copiedWallPropsSourceWall;
    s->copiedSectorGeometry = g_ed.copiedSectorGeometry;
    s->hasCopiedSectorGeometry = g_ed.hasCopiedSectorGeometry;
    s->copiedSectorGeometrySourceSector = g_ed.copiedSectorGeometrySourceSector;

    s->selectedVert = g_ed.selectedVert;
    s->selectedWall = g_ed.selectedWall;
    s->selectedSector = g_ed.selectedSector;

    memcpy(s->selectedVerts, g_ed.selectedVerts, sizeof(g_ed.selectedVerts));
    s->selectedVertCount = g_ed.selectedVertCount;
    memcpy(s->selectedWalls, g_ed.selectedWalls, sizeof(g_ed.selectedWalls));
    s->selectedWallCount = g_ed.selectedWallCount;
    memcpy(s->selectedSectors, g_ed.selectedSectors, sizeof(g_ed.selectedSectors));
    s->selectedSectorCount = g_ed.selectedSectorCount;

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
    g_ed.copiedSectorProps = s->copiedSectorProps;
    g_ed.hasCopiedSectorProps = s->hasCopiedSectorProps;
    g_ed.copiedSectorPropsSourceSector = s->copiedSectorPropsSourceSector;
    g_ed.copiedWallUpperColor = s->copiedWallUpperColor;
    g_ed.copiedWallMidColor = s->copiedWallMidColor;
    g_ed.copiedWallLowerColor = s->copiedWallLowerColor;
    g_ed.hasCopiedWallTexture = s->hasCopiedWallTexture;
    g_ed.copiedWallTexScaleX = s->copiedWallTexScaleX;
    g_ed.copiedWallTexScaleY = s->copiedWallTexScaleY;
    g_ed.hasCopiedWallScale = s->hasCopiedWallScale;
    g_ed.copiedWallTexAngle = s->copiedWallTexAngle;
    g_ed.hasCopiedWallRotation = s->hasCopiedWallRotation;
    g_ed.copiedWallProps = s->copiedWallProps;
    g_ed.hasCopiedWallProps = s->hasCopiedWallProps;
    g_ed.copiedWallPropsSourceWall = s->copiedWallPropsSourceWall;
    g_ed.copiedSectorGeometry = s->copiedSectorGeometry;
    g_ed.hasCopiedSectorGeometry = s->hasCopiedSectorGeometry;
    g_ed.copiedSectorGeometrySourceSector = s->copiedSectorGeometrySourceSector;

    g_ed.selectedVert = s->selectedVert;
    g_ed.selectedWall = s->selectedWall;
    g_ed.selectedSector = s->selectedSector;

    memcpy(g_ed.selectedVerts, s->selectedVerts, sizeof(g_ed.selectedVerts));
    g_ed.selectedVertCount = s->selectedVertCount;
    memcpy(g_ed.selectedWalls, s->selectedWalls, sizeof(g_ed.selectedWalls));
    g_ed.selectedWallCount = s->selectedWallCount;
    memcpy(g_ed.selectedSectors, s->selectedSectors, sizeof(g_ed.selectedSectors));
    g_ed.selectedSectorCount = s->selectedSectorCount;

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
    g_ed.boxSelectWalls = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.dragMultiVertCount = 0;
    clearPendingLeftMouseAction();

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
    } else if (snapshot->selectedWallCount > 0) {
        static char multiWallBuf[48];
        snprintf(multiWallBuf, sizeof(multiWallBuf), "Multi-select (%d walls)", snapshot->selectedWallCount);
        focus = multiWallBuf;
    } else if (snapshot->selectedSectorCount > 0) {
        static char multiSectorBuf[52];
        snprintf(multiSectorBuf, sizeof(multiSectorBuf), "Multi-select (%d sectors)", snapshot->selectedSectorCount);
        focus = multiSectorBuf;
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

static float getIsoScaleX(void)
{
    return g_ed.zoom;
}

static float getIsoScaleY(void)
{
    return g_ed.zoom * 0.5f;
}

static float getIsoScaleZ(void)
{
    return g_ed.zoom * 0.5f;
}

static float getIsoCenterX(void)
{
    return EDIT_VIEW_PORT_WIDTH * 0.5f;
}

static float getIsoCenterY(void)
{
    return EDIT_VIEW_PORT_HEIGHT * 0.58f;
}

static void worldToIsoScreenF(float wx, float wy, float wz, float *sx, float *sy)
{
    const float dx = wx - g_ed.camX;
    const float dy = wy - g_ed.camY;

    *sx = getIsoCenterX() + ((dx - dy) * getIsoScaleX());
    *sy = getIsoCenterY() + ((dx + dy) * getIsoScaleY()) - (wz * getIsoScaleZ());
}

static void screenToIsoWorldOnPlaneF(float sx, float sy, float wz, float *wx, float *wy)
{
    const float isoScaleX = getIsoScaleX();
    const float isoScaleY = getIsoScaleY();
    const float a = (sx - getIsoCenterX()) / isoScaleX;
    const float b = ((sy - getIsoCenterY()) + (wz * getIsoScaleZ())) / isoScaleY;
    const float dx = (a + b) * 0.5f;
    const float dy = (b - a) * 0.5f;

    *wx = g_ed.camX + dx;
    *wy = g_ed.camY + dy;
}

static void screenToIsoWorldOnPlane(int sx, int sy, float wz, float *wx, float *wy)
{
    screenToIsoWorldOnPlaneF((float)sx, (float)sy, wz, wx, wy);
}

static void worldToIsoScreen(float wx, float wy, float wz, int *sx, int *sy)
{
    float fx, fy;

    worldToIsoScreenF(wx, wy, wz, &fx, &fy);
    *sx = (int)lroundf(fx);
    *sy = (int)lroundf(fy);
}

static void panIsoCameraByScreenDelta(int dx, int dy)
{
    const float isoScaleX = getIsoScaleX();
    const float isoScaleY = getIsoScaleY();
    const float a = (float)dx / isoScaleX;
    const float b = (float)dy / isoScaleY;

    g_ed.camX -= (a + b) * 0.5f;
    g_ed.camY += (a - b) * 0.5f;
}

static int compareIsoSortEntry(const void *a, const void *b)
{
    const IsoSortEntry *ea = (const IsoSortEntry *)a;
    const IsoSortEntry *eb = (const IsoSortEntry *)b;

    if (ea->depth < eb->depth) return -1;
    if (ea->depth > eb->depth) return 1;
    return ea->index - eb->index;
}

static float isoPolygonSignedArea(const float *xs, const float *ys, int count)
{
    float twiceArea = 0.0f;

    for (int i = 0, j = count - 1; i < count; j = i++) {
        twiceArea += (xs[j] * ys[i]) - (ys[j] * xs[i]);
    }

    return twiceArea * 0.5f;
}

static void drawIsoTexturedSectorSpan(int x0, int x1, int y, const EdSector *sec)
{
    uint8_t *tex;
    uint8_t *dst;
    float scaleX, scaleY;
    float cosA, sinA;
    float worldX, worldY;
    float worldStepX, worldStepY;
    float texU, texV;
    float texUStep, texVStep;

    if (!sec) {
        return;
    }

    if ((unsigned)y >= EDIT_VIEW_PORT_HEIGHT || x1 < x0) {
        return;
    }

    tex = getTexturePtr(sec->floorColor);
    if (!tex) {
        drawLineDots(x0, y, x1, y, getSectorFillColour((int)(sec - g_edMap.sectors)));
        return;
    }

    scaleX = sec->floorTexScaleX;
    scaleY = sec->floorTexScaleY;

    if (fabsf(scaleX) < 0.001f) scaleX = 0.1f;
    if (fabsf(scaleY) < 0.001f) scaleY = 0.1f;

    cosA = cosf(sec->floorTexAngle);
    sinA = sinf(sec->floorTexAngle);

    screenToIsoWorldOnPlane(x0, y, sec->floorHeight, &worldX, &worldY);

    worldStepX = 0.5f / getIsoScaleX();
    worldStepY = -0.5f / getIsoScaleX();

    texU = (((worldX * cosA) - (worldY * sinA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleX;
    texV = (((worldX * sinA) + (worldY * cosA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleY;

    texUStep =
        (((worldStepX * cosA) - (worldStepY * sinA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleX;
    texVStep =
        (((worldStepX * sinA) + (worldStepY * cosA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleY;

    dst = &fb[(y * SCREEN_W) + x0];

    for (int x = x0; x <= x1; x++) {
        const int tx = wrapTextureCoordLocal((int)floorf(texU), TEXTURE_WIDTH);
        const int ty = wrapTextureCoordLocal((int)floorf(texV), TEXTURE_HEIGHT);

        *dst++ = tex[(ty * TEXTURE_WIDTH) + tx];

        texU += texUStep;
        texV += texVStep;
    }
}

static void drawFilledSectorIsoBruteForceScanline(int sectorIndex,
                                                  int minX,
                                                  int maxX,
                                                  int y,
                                                  const EdSector *sec)
{
    int runStart = -1;
    float worldX;
    float worldY;
    const float worldStepX = 0.5f / getIsoScaleX();
    const float worldStepY = -0.5f / getIsoScaleX();

    if (!sec || minX > maxX) {
        return;
    }

    screenToIsoWorldOnPlaneF((float)minX + 0.5f,
                             (float)y + 0.5f,
                             sec->floorHeight,
                             &worldX,
                             &worldY);

    for (int x = minX; x <= maxX; x++) {
        const int inside = pointInSector(worldX, worldY, sectorIndex);

        if (inside) {
            if (runStart < 0) {
                runStart = x;
            }
        } else if (runStart >= 0) {
            drawIsoTexturedSectorSpan(runStart, x - 1, y, sec);
            runStart = -1;
        }

        worldX += worldStepX;
        worldY += worldStepY;
    }

    if (runStart >= 0) {
        drawIsoTexturedSectorSpan(runStart, maxX, y, sec);
    }
}

static void drawFilledSectorIso(int sectorIndex)
{
    static float intersections[ED_MAX_WALLS];

    const EdSector *sec;
    float minFx, maxFx;
    float minFy, maxFy;
    int minY, maxY;
    int minX, maxX;

    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    sec = &g_edMap.sectors[sectorIndex];
    if (sec->boundaryCount < 3) return;
    if (sec->boundaryCount > ED_MAX_WALLS) return;

    {
        const EdWall *w = &g_edMap.walls[sec->wallStart];
        if (w->v0 < 0 || w->v0 >= g_edMap.vertCount) {
            return;
        }

        worldToIsoScreenF(g_edMap.verts[w->v0].x,
                          g_edMap.verts[w->v0].y,
                          sec->floorHeight,
                          &minFx,
                          &minFy);
        maxFx = minFx;
        maxFy = minFy;
    }

    for (int i = 1; i < sec->boundaryCount; i++) {
        const EdWall *w = &g_edMap.walls[sec->wallStart + i];
        float fx, fy;

        if (w->v0 < 0 || w->v0 >= g_edMap.vertCount) {
            return;
        }

        worldToIsoScreenF(g_edMap.verts[w->v0].x,
                          g_edMap.verts[w->v0].y,
                          sec->floorHeight,
                          &fx,
                          &fy);

        if (fx < minFx) minFx = fx;
        if (fx > maxFx) maxFx = fx;
        if (fy < minFy) minFy = fy;
        if (fy > maxFy) maxFy = fy;
    }

    minX = (int)floorf(minFx);
    maxX = (int)ceilf(maxFx);
    minY = (int)floorf(minFy);
    maxY = (int)ceilf(maxFy);

    if (maxX < 0 || minX >= EDIT_VIEW_PORT_WIDTH) {
        return;
    }

    if (maxY < 0 || minY >= EDIT_VIEW_PORT_HEIGHT) {
        return;
    }

    minX = clampi_local(minX, 0, EDIT_VIEW_PORT_WIDTH - 1);
    maxX = clampi_local(maxX, 0, EDIT_VIEW_PORT_WIDTH - 1);
    minY = clampi_local(minY, 0, EDIT_VIEW_PORT_HEIGHT - 1);
    maxY = clampi_local(maxY, 0, EDIT_VIEW_PORT_HEIGHT - 1);

    for (int y = minY; y <= maxY; y++) {
        int nodeCount = 0;
        const float scanY = (float)y + 0.5f;
        const float scanWorldSum =
            g_ed.camX + g_ed.camY +
            ((scanY - getIsoCenterY()) + (sec->floorHeight * getIsoScaleZ())) / getIsoScaleY();

        for (int i = 0; i < sec->boundaryCount; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            const EdVec2 *a = &g_edMap.verts[w->v0];
            const EdVec2 *b = &g_edMap.verts[w->v1];
            const float sumA = a->x + a->y;
            const float sumB = b->x + b->y;
            const float da = sumA - scanWorldSum;
            const float db = sumB - scanWorldSum;

            if (fabsf(sumB - sumA) <= 0.0001f) {
                continue;
            }

            if (((da < 0.0f) && (db >= 0.0f)) || ((db < 0.0f) && (da >= 0.0f))) {
                const float t = (scanWorldSum - sumA) / (sumB - sumA);
                const float hitX = a->x + ((b->x - a->x) * t);
                const float hitY = a->y + ((b->y - a->y) * t);

                if (nodeCount < ED_MAX_WALLS) {
                    intersections[nodeCount++] =
                        getIsoCenterX() + (((hitX - g_ed.camX) - (hitY - g_ed.camY)) * getIsoScaleX());
                }
            }
        }

        if (nodeCount <= 0) {
            continue;
        }

        for (int i = 1; i < nodeCount; i++) {
            const float v = intersections[i];
            int k = i - 1;

            while (k >= 0 && intersections[k] > v) {
                intersections[k + 1] = intersections[k];
                k--;
            }
            intersections[k + 1] = v;
        }

        if ((nodeCount & 1) != 0) {
            drawFilledSectorIsoBruteForceScanline(sectorIndex, minX, maxX, y, sec);
            continue;
        }

        for (int i = 0; i + 1 < nodeCount; i += 2) {
            int x0 = (int)ceilf(intersections[i]);
            int x1 = (int)floorf(intersections[i + 1]);

            if (x1 < minX || x0 > maxX) {
                continue;
            }

            x0 = clampi_local(x0, minX, maxX);
            x1 = clampi_local(x1, minX, maxX);

            if (x1 >= x0) {
                drawIsoTexturedSectorSpan(x0, x1, y, sec);
            }
        }
    }
}

static void drawIsoWallFace(int wallIndex,
                            float zBottom,
                            float zTop,
                            uint8_t textureIndex,
                            int transparent,
                            int flipFacing)
{
    float polyX[4];
    float polyY[4];
    float minX, maxX;
    int minY, maxY;
    uint8_t *tex;
    const EdWall *w;
    const EdVec2 *a;
    const EdVec2 *b;
    const EdVec2 *v0;
    const EdVec2 *v1;
    float ax0, ay0, bx0, by0;
    float faceDx, faceDy, faceLen;
    float texScaleX, texScaleY;
    float cosA, sinA;
    float signedArea;
    int backFace;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return;
    }

    if (zTop <= zBottom + 0.001f) {
        return;
    }

    tex = getTexturePtr(textureIndex);
    if (!tex) {
        return;
    }

    w = &g_edMap.walls[wallIndex];
    v0 = &g_edMap.verts[w->v0];
    v1 = &g_edMap.verts[w->v1];
    a = flipFacing ? v1 : v0;
    b = flipFacing ? v0 : v1;

    worldToIsoScreenF(a->x, a->y, zBottom, &polyX[0], &polyY[0]);
    worldToIsoScreenF(b->x, b->y, zBottom, &polyX[1], &polyY[1]);
    worldToIsoScreenF(b->x, b->y, zTop,    &polyX[2], &polyY[2]);
    worldToIsoScreenF(a->x, a->y, zTop,    &polyX[3], &polyY[3]);

    signedArea = isoPolygonSignedArea(polyX, polyY, 4);
    if (fabsf(signedArea) <= 0.5f) {
        return;
    }
    backFace = (signedArea < 0.0f);

    minX = polyX[0];
    maxX = polyX[0];
    minY = (int)floorf(polyY[0]);
    maxY = (int)ceilf(polyY[0]);

    for (int i = 1; i < 4; i++) {
        if (polyX[i] < minX) minX = polyX[i];
        if (polyX[i] > maxX) maxX = polyX[i];
        if ((int)floorf(polyY[i]) < minY) minY = (int)floorf(polyY[i]);
        if ((int)ceilf(polyY[i]) > maxY) maxY = (int)ceilf(polyY[i]);
    }

    if (maxX < 0.0f || minX >= EDIT_VIEW_PORT_WIDTH) {
        return;
    }

    if (maxY < 0 || minY >= EDIT_VIEW_PORT_HEIGHT) {
        return;
    }

    minY = clampi_local(minY, 0, EDIT_VIEW_PORT_HEIGHT - 1);
    maxY = clampi_local(maxY, 0, EDIT_VIEW_PORT_HEIGHT - 1);

    worldToIsoScreenF(a->x, a->y, zBottom, &ax0, &ay0);
    worldToIsoScreenF(b->x, b->y, zBottom, &bx0, &by0);

    faceDx = b->x - a->x;
    faceDy = b->y - a->y;
    faceLen = sqrtf((faceDx * faceDx) + (faceDy * faceDy));
    if (faceLen <= 0.0001f) {
        return;
    }

    texScaleX = w->texScaleX;
    texScaleY = w->texScaleY;
    if (fabsf(texScaleX) < 0.001f) texScaleX = 0.1f;
    if (fabsf(texScaleY) < 0.001f) texScaleY = 0.1f;

    cosA = cosf(getWallTexAngle(w));
    sinA = sinf(getWallTexAngle(w));

    for (int y = minY; y <= maxY; y++) {
        float intersections[4];
        int nodeCount = 0;
        const float scanY = (float)y + 0.5f;

        for (int i = 0, j = 3; i < 4; j = i++) {
            const float yi = polyY[i];
            const float yj = polyY[j];
            const float xi = polyX[i];
            const float xj = polyX[j];

            if (((yi < scanY) && (yj >= scanY)) || ((yj < scanY) && (yi >= scanY))) {
                if (nodeCount < 4) {
                    const float t = (scanY - yi) / (yj - yi);
                    intersections[nodeCount++] = xi + (t * (xj - xi));
                }
            }
        }

        if (nodeCount < 2) {
            continue;
        }

        if (intersections[0] > intersections[1]) {
            const float t = intersections[0];
            intersections[0] = intersections[1];
            intersections[1] = t;
        }

        {
            int x0 = (int)ceilf(intersections[0]);
            int x1 = (int)floorf(intersections[1]);
            const float spanX = bx0 - ax0;

            if (x1 < 0 || x0 >= EDIT_VIEW_PORT_WIDTH) {
                continue;
            }

            x0 = clampi_local(x0, 0, EDIT_VIEW_PORT_WIDTH - 1);
            x1 = clampi_local(x1, 0, EDIT_VIEW_PORT_WIDTH - 1);

            for (int x = x0; x <= x1; x++) {
                const float sampleX = (float)x + 0.5f;
                const float sampleY = (float)y + 0.5f;
                float along;
                float baseY;
                float relHeight;
                float texU;
                float texV;
                int tx, ty;

                if (fabsf(spanX) <= 0.001f) {
                    continue;
                }

                along = (sampleX - ax0) / spanX;
                if (along < 0.0f || along > 1.0f) {
                    continue;
                }

                baseY = ay0 + ((by0 - ay0) * along);
                relHeight = (baseY - sampleY) / getIsoScaleZ();
                if (relHeight < 0.0f || relHeight > (zTop - zBottom)) {
                    continue;
                }

                texU = ((along * faceLen) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / texScaleX;
                texV = (relHeight * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / texScaleY;

                {
                    const float rotatedU = (texU * cosA) - (texV * sinA);
                    const float rotatedV = (texU * sinA) + (texV * cosA);
                    tx = wrapTextureCoordLocal((int)floorf(rotatedU), TEXTURE_WIDTH);
                    ty = wrapTextureCoordLocal((int)floorf(rotatedV), TEXTURE_HEIGHT);
                }

                if (transparent && (((x + y) & 1) != 0)) {
                    continue;
                }

                /* Ghost hidden faces so they read as see-through, not full solids. */
                if (backFace && (((x + y) & 1) != 0)) {
                    continue;
                }

                fb[(y * SCREEN_W) + x] = tex[(ty * TEXTURE_WIDTH) + tx];
            }
        }
    }
}

static void drawIsoWallSections(int wallIndex, int ownerSector)
{
    const EdWall *w;
    const EdSector *owner;
    int transparent;
    const EdVec2 *a;
    const EdVec2 *b;
    float dx, dy, len;
    float midX, midY;
    float rightNx, rightNy;
    float sampleDist;
    int interiorOnRight = 1;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) {
        return;
    }

    if (ownerSector < 0 || ownerSector >= g_edMap.sectorCount) {
        return;
    }

    w = &g_edMap.walls[wallIndex];
    owner = &g_edMap.sectors[ownerSector];
    transparent = (w->flags & RC3D_WALL_TRANSPARENCY) ? 1 : 0;
    a = &g_edMap.verts[w->v0];
    b = &g_edMap.verts[w->v1];

    dx = b->x - a->x;
    dy = b->y - a->y;
    len = sqrtf((dx * dx) + (dy * dy));

    if (len > 0.0001f) {
        midX = (a->x + b->x) * 0.5f;
        midY = (a->y + b->y) * 0.5f;
        rightNx = dy / len;
        rightNy = -dx / len;
        sampleDist = clampf_local(g_ed.currentGridStep * 0.2f, 0.05f, 0.25f);
        if (sampleDist > (len * 0.25f)) {
            sampleDist = len * 0.25f;
        }

        if (sampleDist > 0.0001f) {
            const int rightInside =
                pointInSector(midX + (rightNx * sampleDist),
                              midY + (rightNy * sampleDist),
                              ownerSector);
            const int leftInside =
                pointInSector(midX - (rightNx * sampleDist),
                              midY - (rightNy * sampleDist),
                              ownerSector);

            if (rightInside != leftInside) {
                interiorOnRight = rightInside;
            }
        }
    }

    if ((w->flags & RC3D_WALL_LOWER) && (w->openBottom > owner->floorHeight)) {
        drawIsoWallFace(wallIndex,
                        owner->floorHeight,
                        w->openBottom,
                        w->lowerColor,
                        transparent,
                        !interiorOnRight);
    }

    if (w->flags & RC3D_WALL_MIDDLE) {
        if (w->flags & RC3D_WALL_SOLID) {
            drawIsoWallFace(wallIndex,
                            owner->floorHeight,
                            owner->ceilHeight,
                            w->midColor,
                            transparent,
                            !interiorOnRight);
        } else if (w->openTop > w->openBottom) {
            drawIsoWallFace(wallIndex,
                            w->openBottom,
                            w->openTop,
                            w->midColor,
                            transparent,
                            !interiorOnRight);
        }
    }

    if ((w->flags & RC3D_WALL_UPPER) && (owner->ceilHeight > w->openTop)) {
        drawIsoWallFace(wallIndex,
                        w->openTop,
                        owner->ceilHeight,
                        w->upperColor,
                        transparent,
                        !interiorOnRight);
    }
}

static void drawIsoStartMarker(void)
{
    float startZ = 0.0f;
    int sx, sy, fx, fy;

    if (g_edMap.startSector >= 0 && g_edMap.startSector < g_edMap.sectorCount) {
        startZ = g_edMap.sectors[g_edMap.startSector].floorHeight;
    }

    worldToIsoScreen(g_edMap.startX, g_edMap.startY, startZ, &sx, &sy);
    worldToIsoScreen(g_edMap.startX + (cosf(g_edMap.startAngle) * 0.5f),
                     g_edMap.startY + (sinf(g_edMap.startAngle) * 0.5f),
                     startZ,
                     &fx,
                     &fy);

    drawRect(sx - 2, sy - 2, 5, 5, ED_START_COL);
    drawLine(sx, sy, fx, fy, ED_START_COL);
}

static void drawIsometricPreview(void)
{
    IsoSortEntry sectorOrder[ED_MAX_SECTORS];
    IsoSortEntry wallOrder[ED_MAX_WALLS];
    int wallOwners[ED_MAX_WALLS];
    int sectorOrderCount = 0;
    int wallOrderCount = 0;

    clearScreen(16);

    for (int i = 0; i < ED_MAX_WALLS; i++) {
        wallOwners[i] = -1;
    }

    for (int s = 0; s < g_edMap.sectorCount; s++) {
        const EdSector *sec = &g_edMap.sectors[s];
        float sumX = 0.0f;
        float sumY = 0.0f;

        if (sec->boundaryCount < 3) {
            continue;
        }

        for (int i = 0; i < sec->boundaryCount; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            const EdVec2 *v = &g_edMap.verts[w->v0];

            sumX += v->x;
            sumY += v->y;
            if ((sec->wallStart + i) >= 0 && (sec->wallStart + i) < ED_MAX_WALLS) {
                wallOwners[sec->wallStart + i] = s;
            }
        }

        sectorOrder[sectorOrderCount].index = s;
        sectorOrder[sectorOrderCount].depth =
            (sumX + sumY) / (float)sec->boundaryCount + (sec->floorHeight * 0.25f);
        sectorOrderCount++;
    }

    qsort(sectorOrder, sectorOrderCount, sizeof(sectorOrder[0]), compareIsoSortEntry);

    for (int i = 0; i < sectorOrderCount; i++) {
        drawFilledSectorIso(sectorOrder[i].index);
    }

    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        const EdVec2 *a = &g_edMap.verts[w->v0];
        const EdVec2 *b = &g_edMap.verts[w->v1];
        const int ownerSector = wallOwners[i];
        float zTop = 0.0f;

        if (ownerSector >= 0 && ownerSector < g_edMap.sectorCount) {
            zTop = g_edMap.sectors[ownerSector].ceilHeight;
        }

        wallOrder[wallOrderCount].index = i;
        wallOrder[wallOrderCount].depth =
            ((a->x + a->y + b->x + b->y) * 0.5f) + (zTop * 0.25f);
        wallOrderCount++;
    }

    qsort(wallOrder, wallOrderCount, sizeof(wallOrder[0]), compareIsoSortEntry);

    for (int i = 0; i < wallOrderCount; i++) {
        drawIsoWallSections(wallOrder[i].index, wallOwners[wallOrder[i].index]);
    }

    drawIsoStartMarker();
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
    w->texScaleX = 1.0f;
    w->texScaleY = 1.0f;
}

static uint8_t wallClipboardFlagsFromWall(const EdWall *w)
{
    if (!w) return 0;

    return (uint8_t)(w->flags & (RC3D_WALL_UPPER |
                                 RC3D_WALL_MIDDLE |
                                 RC3D_WALL_LOWER |
                                 RC3D_WALL_SOLID |
                                 RC3D_WALL_TRANSPARENCY |
                                 RC3D_WALL_MANUAL_TARGET));
}

static uint8_t clampLightLevel(int level)
{
    if (level < 0) return 0;
    if (level > 7) return 7;
    return (uint8_t)level;
}

static float clampSectorTexScale(float scale)
{
    if (scale < 0.1f) return 0.1f;
    return scale;
}

static void normalizeSectorMoveBounds(EdSector *sec)
{
    float temp;

    if (!sec) return;

    if (sec->floorMinHeight > sec->floorMaxHeight) {
        temp = sec->floorMinHeight;
        sec->floorMinHeight = sec->floorMaxHeight;
        sec->floorMaxHeight = temp;
    }

    if (sec->ceilMinHeight > sec->ceilMaxHeight) {
        temp = sec->ceilMinHeight;
        sec->ceilMinHeight = sec->ceilMaxHeight;
        sec->ceilMaxHeight = temp;
    }

    if (sec->floorHeight < sec->floorMinHeight) sec->floorMinHeight = sec->floorHeight;
    if (sec->floorHeight > sec->floorMaxHeight) sec->floorMaxHeight = sec->floorHeight;
    if (sec->ceilHeight < sec->ceilMinHeight) sec->ceilMinHeight = sec->ceilHeight;
    if (sec->ceilHeight > sec->ceilMaxHeight) sec->ceilMaxHeight = sec->ceilHeight;
}

static void resetSectorMovePropertiesToCurrentHeights(EdSector *sec)
{
    if (!sec) return;

    sec->tagId = 0;
    sec->stateFlags = 0u;
    sec->floorMinHeight = sec->floorHeight;
    sec->floorMaxHeight = sec->floorHeight;
    sec->ceilMinHeight = sec->ceilHeight;
    sec->ceilMaxHeight = sec->ceilHeight;
    sec->floorFlowHeight = 0.0f;
    sec->ceilFlowHeight = 0.0f;
}

static void initializeNewSectorDefaults(EdSector *sec, int wallStart, int wallCount, int boundaryCount)
{
    if (!sec) return;

    memset(sec, 0, sizeof(*sec));

    sec->wallStart = wallStart;
    sec->wallCount = wallCount;
    sec->boundaryCount = boundaryCount;
    sec->floorHeight = g_ed.sectorFloor;
    sec->ceilHeight = g_ed.sectorCeil;
    sec->floorColor = g_ed.sectorFloorColor;
    sec->ceilColor = g_ed.sectorCeilColor;
    sec->glowlevel = 0;

    sec->floorTexScaleX = 1.0f;
    sec->floorTexScaleY = 1.0f;
    sec->floorTexAngle = 0.0f;
    sec->ceilTexScaleX = 1.0f;
    sec->ceilTexScaleY = 1.0f;
    sec->ceilTexAngle = 0.0f;

    resetSectorMovePropertiesToCurrentHeights(sec);
    sanitizeSectorProperties(sec);
}

static void sanitizeSectorProperties(EdSector *sec)
{
    if (!sec) return;

    if (sec->ceilHeight < sec->floorHeight + 0.1f) {
        sec->ceilHeight = sec->floorHeight + 0.1f;
    }

    sec->glowlevel = clampLightLevel((int)sec->glowlevel);

    normalizeSectorMoveBounds(sec);

    sec->floorTexScaleX = clampSectorTexScale(sec->floorTexScaleX);
    sec->floorTexScaleY = clampSectorTexScale(sec->floorTexScaleY);
    sec->ceilTexScaleX = clampSectorTexScale(sec->ceilTexScaleX);
    sec->ceilTexScaleY = clampSectorTexScale(sec->ceilTexScaleY);
}

static void resetSectorGeometryClipboard(void)
{
    memset(&g_ed.copiedSectorGeometry, 0, sizeof(g_ed.copiedSectorGeometry));
    g_ed.hasCopiedSectorGeometry = 0;
    g_ed.copiedSectorGeometrySourceSector = -1;
}

static void copySelectedSectorGeometryToClipboard(void)
{
    EdSectorGeometryClipboard *clip = &g_ed.copiedSectorGeometry;
    int sectorIndices[ED_MAX_SECTORS];
    int vertRemap[ED_MAX_VERTS];
    const int sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);
    int localVertCount = 0;
    float minX = 0.0f;
    float minY = 0.0f;
    int haveBounds = 0;
    char msg[160];

    if (sectorCount <= 0) {
        setEditorStatus("Select at least one sector to copy");
        return;
    }

    memset(vertRemap, 0xFF, sizeof(vertRemap));
    memset(clip, 0, sizeof(*clip));

    for (int si = 0; si < sectorCount; si++) {
        const int sectorIndex = sectorIndices[si];
        const EdSector *srcSec;
        EdSector dstSec;

        if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
            resetSectorGeometryClipboard();
            setEditorStatus("Sector geometry copy failed");
            return;
        }

        srcSec = &g_edMap.sectors[sectorIndex];
        if (srcSec->wallCount <= 0 ||
            srcSec->wallCount > ED_MAX_WALLS ||
            (clip->wallCount + srcSec->wallCount) > ED_MAX_WALLS) {
            resetSectorGeometryClipboard();
            setEditorStatus("Sector geometry copy failed");
            return;
        }

        dstSec = *srcSec;
        dstSec.wallStart = clip->wallCount;

        for (int wi = 0; wi < srcSec->wallCount; wi++) {
            const EdWall *srcWall = &g_edMap.walls[srcSec->wallStart + wi];
            EdWall dstWall = *srcWall;

            if (srcWall->v0 < 0 || srcWall->v0 >= g_edMap.vertCount ||
                srcWall->v1 < 0 || srcWall->v1 >= g_edMap.vertCount) {
                resetSectorGeometryClipboard();
                setEditorStatus("Sector geometry copy failed");
                return;
            }

            if (vertRemap[srcWall->v0] < 0) {
                const EdVec2 *v = &g_edMap.verts[srcWall->v0];

                if (localVertCount >= ED_MAX_VERTS) {
                    resetSectorGeometryClipboard();
                    setEditorStatus("Sector geometry copy failed");
                    return;
                }

                vertRemap[srcWall->v0] = localVertCount;
                clip->verts[localVertCount++] = *v;

                if (!haveBounds) {
                    minX = v->x;
                    minY = v->y;
                    haveBounds = 1;
                } else {
                    if (v->x < minX) minX = v->x;
                    if (v->y < minY) minY = v->y;
                }
            }

            if (vertRemap[srcWall->v1] < 0) {
                const EdVec2 *v = &g_edMap.verts[srcWall->v1];

                if (localVertCount >= ED_MAX_VERTS) {
                    resetSectorGeometryClipboard();
                    setEditorStatus("Sector geometry copy failed");
                    return;
                }

                vertRemap[srcWall->v1] = localVertCount;
                clip->verts[localVertCount++] = *v;

                if (!haveBounds) {
                    minX = v->x;
                    minY = v->y;
                    haveBounds = 1;
                } else {
                    if (v->x < minX) minX = v->x;
                    if (v->y < minY) minY = v->y;
                }
            }

            dstWall.v0 = vertRemap[srcWall->v0];
            dstWall.v1 = vertRemap[srcWall->v1];
            dstWall.neighbour = -1;

            if (dstWall.flags & RC3D_WALL_PORTAL) {
                uint8_t solidCol = dstWall.midColor;

                if (solidCol == 0) solidCol = dstWall.upperColor;
                if (solidCol == 0) solidCol = dstWall.lowerColor;
                if (solidCol == 0) solidCol = g_ed.newWallMidColor;

                dstWall.openBottom = 0.0f;
                dstWall.openTop = 0.0f;
                dstWall.upperColor = 0;
                dstWall.midColor = solidCol;
                dstWall.lowerColor = 0;
                dstWall.flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
            }

            clip->walls[clip->wallCount++] = dstWall;
        }

        sanitizeSectorProperties(&dstSec);
        clip->sectors[clip->sectorCount++] = dstSec;
    }

    if (localVertCount <= 0 || !haveBounds || clip->sectorCount <= 0) {
        resetSectorGeometryClipboard();
        setEditorStatus("Sector geometry copy failed");
        return;
    }

    clip->vertCount = localVertCount;
    clip->anchorX = snapf(minX);
    clip->anchorY = snapf(minY);

    g_ed.hasCopiedSectorGeometry = 1;
    g_ed.copiedSectorGeometrySourceSector = sectorIndices[0];

    snprintf(msg, sizeof(msg), "Copied %d sector%s", sectorCount, (sectorCount == 1) ? "" : "s");
    setEditorStatus(msg);
}

static int pasteSectorGeometryFromClipboard(float targetWorldX, float targetWorldY)
{
    const EdSectorGeometryClipboard *clip = &g_ed.copiedSectorGeometry;
    const float targetX = snapf(targetWorldX);
    const float targetY = snapf(targetWorldY);
    const float dx = targetX - clip->anchorX;
    const float dy = targetY - clip->anchorY;
    const int vertBase = g_edMap.vertCount;
    const int wallBase = g_edMap.wallCount;
    const int sectorBase = g_edMap.sectorCount;
    char msg[160];

    if (!g_ed.hasCopiedSectorGeometry) {
        setEditorStatus("No copied sector geometry");
        return 0;
    }

    if (clip->vertCount <= 0 || clip->wallCount <= 0 || clip->sectorCount <= 0) {
        setEditorStatus("Copied sector geometry is invalid");
        return 0;
    }

    if ((g_edMap.vertCount + clip->vertCount) > ED_MAX_VERTS) {
        setEditorStatus("Paste failed: too many vertices");
        return 0;
    }

    if ((g_edMap.wallCount + clip->wallCount) > ED_MAX_WALLS) {
        setEditorStatus("Paste failed: too many walls");
        return 0;
    }

    if ((g_edMap.sectorCount + clip->sectorCount) > ED_MAX_SECTORS) {
        setEditorStatus("Paste failed: too many sectors");
        return 0;
    }

    for (int i = 0; i < clip->vertCount; i++) {
        g_edMap.verts[g_edMap.vertCount].x = clip->verts[i].x + dx;
        g_edMap.verts[g_edMap.vertCount].y = clip->verts[i].y + dy;
        g_edMap.vertCount++;
    }

    for (int i = 0; i < clip->wallCount; i++) {
        EdWall wall = clip->walls[i];

        wall.v0 = vertBase + wall.v0;
        wall.v1 = vertBase + wall.v1;
        wall.neighbour = -1;
        g_edMap.walls[g_edMap.wallCount++] = wall;
    }

    for (int si = 0; si < clip->sectorCount; si++) {
        g_edMap.sectors[g_edMap.sectorCount] = clip->sectors[si];
        g_edMap.sectors[g_edMap.sectorCount].wallStart = wallBase + clip->sectors[si].wallStart;
        sanitizeSectorProperties(&g_edMap.sectors[g_edMap.sectorCount]);
        g_edMap.sectorCount++;
    }

    syncAllPortals();

    clearAllSelections();
    if (clip->sectorCount == 1) {
        g_ed.selectionType = ED_SEL_SECTOR;
        g_ed.selectedSector = sectorBase;
    } else {
        for (int si = 0; si < clip->sectorCount; si++) {
            addMultiSectorSelection(sectorBase + si);
        }
        normalizeSectorSelectionState();
    }
    g_ed.hoverVert = -1;
    g_ed.hoverWall = -1;
    g_ed.hoverSector = sectorBase;
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.boxSelecting = 0;
    clearPendingLeftMouseAction();

    snprintf(msg, sizeof(msg), "Pasted %d sector%s at %.2f, %.2f",
             clip->sectorCount, (clip->sectorCount == 1) ? "" : "s", targetX, targetY);
    setEditorStatus(msg);
    return 1;
}

static void copySectorPropertiesToClipboard(int sectorIndex)
{
    EdSectorClipboard *clip;
    const EdSector *sec;
    char msg[128];

    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    clip = &g_ed.copiedSectorProps;
    sec = &g_edMap.sectors[sectorIndex];

    clip->floorHeight = sec->floorHeight;
    clip->ceilHeight = sec->ceilHeight;
    clip->floorColor = sec->floorColor;
    clip->ceilColor = sec->ceilColor;
    clip->glowlevel = sec->glowlevel;
    clip->tagId = sec->tagId;
    clip->stateFlags = sec->stateFlags;
    clip->floorMinHeight = sec->floorMinHeight;
    clip->floorMaxHeight = sec->floorMaxHeight;
    clip->ceilMinHeight = sec->ceilMinHeight;
    clip->ceilMaxHeight = sec->ceilMaxHeight;
    clip->floorFlowHeight = sec->floorFlowHeight;
    clip->ceilFlowHeight = sec->ceilFlowHeight;

    clip->floorTexScaleX = sec->floorTexScaleX;
    clip->floorTexScaleY = sec->floorTexScaleY;
    clip->floorTexAngle = sec->floorTexAngle;

    clip->ceilTexScaleX = sec->ceilTexScaleX;
    clip->ceilTexScaleY = sec->ceilTexScaleY;
    clip->ceilTexAngle = sec->ceilTexAngle;

    g_ed.hasCopiedSectorProps = 1;
    g_ed.copiedSectorPropsSourceSector = sectorIndex;

    snprintf(msg, sizeof(msg), "Copied sector properties from sector %d", sectorIndex);
    setEditorStatus(msg);
}

static int applySectorPropertiesFromClipboardToSector(int sectorIndex)
{
    EdSector *sec;
    const EdSectorClipboard *clip;

    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return 0;
    if (!g_ed.hasCopiedSectorProps) return 0;

    sec = &g_edMap.sectors[sectorIndex];
    clip = &g_ed.copiedSectorProps;

    sec->floorHeight = clip->floorHeight;
    sec->ceilHeight = clip->ceilHeight;
    sec->floorColor = clip->floorColor;
    sec->ceilColor = clip->ceilColor;
    sec->glowlevel = clip->glowlevel;
    sec->tagId = clip->tagId;
    sec->stateFlags = clip->stateFlags;
    sec->floorMinHeight = clip->floorMinHeight;
    sec->floorMaxHeight = clip->floorMaxHeight;
    sec->ceilMinHeight = clip->ceilMinHeight;
    sec->ceilMaxHeight = clip->ceilMaxHeight;
    sec->floorFlowHeight = clip->floorFlowHeight;
    sec->ceilFlowHeight = clip->ceilFlowHeight;

    sec->floorTexScaleX = clip->floorTexScaleX;
    sec->floorTexScaleY = clip->floorTexScaleY;
    sec->floorTexAngle = clip->floorTexAngle;

    sec->ceilTexScaleX = clip->ceilTexScaleX;
    sec->ceilTexScaleY = clip->ceilTexScaleY;
    sec->ceilTexAngle = clip->ceilTexAngle;

    sanitizeSectorProperties(sec);
    return 1;
}

static int pasteSectorPropertiesFromClipboard(int sectorIndex)
{
    char msg[128];

    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return 0;

    if (!g_ed.hasCopiedSectorProps) {
        setEditorStatus("No copied sector properties");
        return 0;
    }

    if (!applySectorPropertiesFromClipboardToSector(sectorIndex)) {
        return 0;
    }

    snprintf(msg, sizeof(msg), "Pasted sector properties to sector %d", sectorIndex);
    setEditorStatus(msg);
    return 1;
}

static int pasteSectorPropertiesFromClipboardToSelection(void)
{
    int sectorIndices[ED_MAX_SECTORS];
    const int sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);
    char msg[128];

    if (!g_ed.hasCopiedSectorProps) {
        setEditorStatus("No copied sector properties");
        return 0;
    }

    if (sectorCount <= 0) {
        setEditorStatus("Select at least one sector");
        return 0;
    }

    if (sectorCount == 1) {
        if (pasteSectorPropertiesFromClipboard(sectorIndices[0])) {
            syncAllPortals();
            return 1;
        }

        return 0;
    }

    for (int i = 0; i < sectorCount; i++) {
        applySectorPropertiesFromClipboardToSector(sectorIndices[i]);
    }

    syncAllPortals();

    snprintf(msg, sizeof(msg), "Pasted sector properties to %d selected sector%s",
             sectorCount, (sectorCount == 1) ? "" : "s");
    setEditorStatus(msg);
    return 1;
}

static void copyWallPropsToClipboard(int wallIndex)
{
    EdWallClipboard *clip;
    const EdWall *wall;
    char msg[128];

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    clip = &g_ed.copiedWallProps;
    wall = &g_edMap.walls[wallIndex];

    clip->openBottom = wall->openBottom;
    clip->openTop = wall->openTop;
    clip->upperColor = wall->upperColor;
    clip->midColor = wall->midColor;
    clip->lowerColor = wall->lowerColor;
    clip->flags = wallClipboardFlagsFromWall(wall);
    clip->tex_flags = wall->tex_flags;
    clip->texScaleX = wall->texScaleX;
    clip->texScaleY = wall->texScaleY;

    g_ed.hasCopiedWallProps = 1;
    g_ed.copiedWallPropsSourceWall = wallIndex;

    snprintf(msg, sizeof(msg), "Copied wall properties from wall %d", wallIndex);
    setEditorStatus(msg);
}

static void pasteWallPropsFromClipboardToWall(int wallIndex)
{
    EdWall *wall;
    const EdWallClipboard *clip;
    uint8_t preservedPortal;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    wall = &g_edMap.walls[wallIndex];
    clip = &g_ed.copiedWallProps;
    preservedPortal = (uint8_t)(wall->flags & RC3D_WALL_PORTAL);

    wall->openBottom = clip->openBottom;
    wall->openTop = clip->openTop;
    if (wall->openTop < wall->openBottom) {
        const float t = wall->openTop;
        wall->openTop = wall->openBottom;
        wall->openBottom = t;
    }

    wall->upperColor = clip->upperColor;
    wall->midColor = clip->midColor;
    wall->lowerColor = clip->lowerColor;
    wall->flags = preservedPortal | clip->flags;
    wall->tex_flags = clip->tex_flags;
    setWallTexScaleX(wall, clip->texScaleX);
    setWallTexScaleY(wall, clip->texScaleY);
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

static uint8_t wallTexBrightnessFromFlags(uint32_t texFlags)
{
    const uint32_t packed =
        (texFlags & RC3D_TEX_WALL_BRIGHT_MASK) >> RC3D_TEX_WALL_BRIGHT_SHIFT;

    return clampLightLevel((int)packed);
}

static uint32_t wallTexFlagsWithBrightness(uint32_t texFlags, uint8_t brightness)
{
    texFlags &= ~RC3D_TEX_WALL_BRIGHT_MASK;
    texFlags |=
        ((uint32_t)clampLightLevel((int)brightness) << RC3D_TEX_WALL_BRIGHT_SHIFT) &
        RC3D_TEX_WALL_BRIGHT_MASK;
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

static uint8_t getWallTexBrightness(const EdWall *w)
{
    if (!w) return 0;
    return wallTexBrightnessFromFlags(w->tex_flags);
}

static void setWallTexBrightness(EdWall *w, int brightness)
{
    if (!w) return;
    w->tex_flags = wallTexFlagsWithBrightness(w->tex_flags, clampLightLevel(brightness));
}

static float clampWallTexScale(float scale)
{
    if (scale < 0.1f) return 0.1f;
    return scale;
}

static void setWallTexScaleX(EdWall *w, float scaleX)
{
    if (!w) return;
    w->texScaleX = clampWallTexScale(scaleX);
}

static void setWallTexScaleY(EdWall *w, float scaleY)
{
    if (!w) return;
    w->texScaleY = clampWallTexScale(scaleY);
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

static void adjustWallTexBrightness(int wallIndex, int delta)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    w = &g_edMap.walls[wallIndex];
    setWallTexBrightness(w, (int)getWallTexBrightness(w) + delta);
}

static void adjustWallTexScaleX(int wallIndex, float delta)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    w = &g_edMap.walls[wallIndex];
    setWallTexScaleX(w, w->texScaleX + delta);
}

static void adjustWallTexScaleY(int wallIndex, float delta)
{
    EdWall *w;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    w = &g_edMap.walls[wallIndex];
    setWallTexScaleY(w, w->texScaleY + delta);
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
                clearWallTexFlags(dst);
            }

            initializeNewSectorDefaults(&g_edMap.sectors[newSectorIndex],
                                        newWallStart,
                                        loopCount,
                                        loopCount);

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

        if (sec->floorMinHeight > sec->floorMaxHeight) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has floor min above floor max", s);
        }

        if (sec->ceilMinHeight > sec->ceilMaxHeight) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d has ceiling min above ceiling max", s);
        }

        if (sec->floorHeight < sec->floorMinHeight || sec->floorHeight > sec->floorMaxHeight) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d floor height is outside mover range", s);
        }

        if (sec->ceilHeight < sec->ceilMinHeight || sec->ceilHeight > sec->ceilMaxHeight) {
            validatorAddLineEx(ED_VAL_TARGET_SECTOR, s,
                               "Sector %d ceiling height is outside mover range", s);
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
    int sectorIndices[ED_MAX_SECTORS];
    int sectorCount = 0;

    g_ed.draggingSector = 0;
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    if (isSectorInEditSelection(sectorIndex)) {
        sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);
    }

    if (sectorCount <= 0) {
        sectorIndices[0] = sectorIndex;
        sectorCount = 1;
    }

    g_ed.dragSectorStartWorldX = worldX;
    g_ed.dragSectorStartWorldY = worldY;

    if (sectorCount == 1) {
        g_ed.dragSectorVertCount = sectorCollectUniqueVerts(
            sectorIndices[0],
            g_ed.dragSectorVertIndices,
            g_ed.dragSectorVertStartX,
            g_ed.dragSectorVertStartY,
            ED_MAX_VERTS
        );
    } else {
        g_ed.dragSectorVertCount = 0;

        for (int si = 0; si < sectorCount; si++) {
            const int curSector = sectorIndices[si];
            const EdSector *sec;

            if (curSector < 0 || curSector >= g_edMap.sectorCount) continue;

            sec = &g_edMap.sectors[curSector];

            for (int wi = 0; wi < sec->wallCount; wi++) {
                const EdWall *w = &g_edMap.walls[sec->wallStart + wi];
                const int verts[2] = { w->v0, w->v1 };

                for (int vk = 0; vk < 2; vk++) {
                    const int vi = verts[vk];
                    int exists = 0;

                    for (int j = 0; j < g_ed.dragSectorVertCount; j++) {
                        if (g_ed.dragSectorVertIndices[j] == vi) {
                            exists = 1;
                            break;
                        }
                    }

                    if (exists) continue;
                    if (g_ed.dragSectorVertCount >= ED_MAX_VERTS) break;

                    g_ed.dragSectorVertIndices[g_ed.dragSectorVertCount] = vi;
                    g_ed.dragSectorVertStartX[g_ed.dragSectorVertCount] = g_edMap.verts[vi].x;
                    g_ed.dragSectorVertStartY[g_ed.dragSectorVertCount] = g_edMap.verts[vi].y;
                    g_ed.dragSectorVertCount++;
                }
            }
        }
    }

    if (g_ed.dragSectorVertCount > 0) {
        g_ed.draggingSector = 1;
    }
}

static void dragSelectedSectorTo(float worldX, float worldY)
{
    if (!g_ed.draggingSector || g_ed.dragSectorVertCount <= 0) return;

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

    clearMultiWallSelection();

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
    int sectorRemap[ED_MAX_SECTORS];
    const int oldSectorCount = g_edMap.sectorCount;

    clearMultiVertexSelection();
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) {
        return;
    }

    for (int i = 0; i < ED_MAX_SECTORS; i++) {
        sectorRemap[i] = -1;
    }
    for (int i = 0; i < oldSectorCount; i++) {
        if (i < sectorIndex) sectorRemap[i] = i;
        else if (i > sectorIndex) sectorRemap[i] = i - 1;
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

    remapMultiSectorSelectionFromOldToNew(sectorRemap, oldSectorCount);

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
    int sectorRemap[ED_MAX_SECTORS];
    int oldSectorCount;

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
    if (secA->glowlevel  != secB->glowlevel)  return 0;

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
    oldSectorCount = g_edMap.sectorCount;

    const EdSector keepProps = g_edMap.sectors[keepSector];

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

    for (int i = 0; i < ED_MAX_SECTORS; i++) {
        sectorRemap[i] = -1;
    }
    for (int i = 0; i < oldSectorCount; i++) {
        if (i < killSector) sectorRemap[i] = i;
        else if (i > killSector) sectorRemap[i] = i - 1;
    }

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

    remapMultiSectorSelectionFromOldToNew(sectorRemap, oldSectorCount);

    /* append clean merged boundary */
    {
        const int newStart = g_edMap.wallCount;

        for (int i = 0; i < mergedCount; i++) {
            g_edMap.walls[g_edMap.wallCount++] = mergedWalls[i];
        }

        g_edMap.sectors[keepSector] = keepProps;
        g_edMap.sectors[keepSector].wallStart = newStart;
        g_edMap.sectors[keepSector].wallCount = mergedCount;
        g_edMap.sectors[keepSector].boundaryCount = mergedCount;
        sanitizeSectorProperties(&g_edMap.sectors[keepSector]);
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
    clearWallTexFlags(w);

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
        clearWallTexFlags(ow);
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
    clearWallTexFlags(w);
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
    clearWallTexFlags(w);
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
    clearWallTexFlags(w);
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

static void clearPendingLeftMouseAction(void)
{
    g_ed.pendingLeftMouseDown = 0;
    g_ed.pendingLeftAction = ED_PENDING_LEFT_NONE;
    g_ed.pendingLeftMouseX = 0;
    g_ed.pendingLeftMouseY = 0;
    g_ed.pendingLeftWorldX = 0.0f;
    g_ed.pendingLeftWorldY = 0.0f;
    g_ed.pendingLeftTargetIndex = -1;
    g_ed.pendingLeftCtrlDown = 0;
    g_ed.pendingLeftAltDown = 0;
    g_ed.pendingLeftBoxSelectWalls = 0;
}

static int pendingLeftExceededDragTolerance(int mouseX, int mouseY)
{
    const int dx = mouseX - g_ed.pendingLeftMouseX;
    const int dy = mouseY - g_ed.pendingLeftMouseY;
    const int distSq = (dx * dx) + (dy * dy);
    const int tolSq = ED_CLICK_DRAG_TOLERANCE_PX * ED_CLICK_DRAG_TOLERANCE_PX;

    return distSq > tolSq;
}

static void selectExplicitTarget(EdSelectionType type, int index)
{
    clearAllSelections();

    switch (type) {
        case ED_SEL_VERTEX:
            if (index >= 0 && index < g_edMap.vertCount) {
                g_ed.selectionType = ED_SEL_VERTEX;
                g_ed.selectedVert = index;
            }
            break;

        case ED_SEL_WALL:
            if (index >= 0 && index < g_edMap.wallCount) {
                g_ed.selectionType = ED_SEL_WALL;
                g_ed.selectedWall = index;
            }
            break;

        case ED_SEL_SECTOR:
            if (index >= 0 && index < g_edMap.sectorCount) {
                g_ed.selectionType = ED_SEL_SECTOR;
                g_ed.selectedSector = index;
            }
            break;

        case ED_SEL_NONE:
        default:
            break;
    }
}

static void beginVertexDragFromPress(int vertIndex)
{
    if (vertIndex < 0 || vertIndex >= g_edMap.vertCount) return;

    selectExplicitTarget(ED_SEL_VERTEX, vertIndex);
    pushUndoState();

    g_ed.draggingVertex = 1;
    g_ed.dragStartWorldX = g_ed.pendingLeftWorldX;
    g_ed.dragStartWorldY = g_ed.pendingLeftWorldY;
    g_ed.dragVertexStartX = g_edMap.verts[vertIndex].x;
    g_ed.dragVertexStartY = g_edMap.verts[vertIndex].y;
}

static void beginWallDragFromPress(int wallIndex)
{
    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    selectExplicitTarget(ED_SEL_WALL, wallIndex);
    pushUndoState();
    g_ed.draggingWall = 0;
    g_ed.dragWallVertCount = 0;
    g_ed.dragWallStartWorldX = g_ed.pendingLeftWorldX;
    g_ed.dragWallStartWorldY = g_ed.pendingLeftWorldY;

    {
        const EdWall *w = &g_edMap.walls[wallIndex];
        const int verts[2] = { w->v0, w->v1 };

        for (int i = 0; i < 2; i++) {
            const int vi = verts[i];

            if (vi < 0 || vi >= g_edMap.vertCount) continue;

            g_ed.dragWallVertIndices[g_ed.dragWallVertCount] = vi;
            g_ed.dragWallVertStartX[g_ed.dragWallVertCount] = g_edMap.verts[vi].x;
            g_ed.dragWallVertStartY[g_ed.dragWallVertCount] = g_edMap.verts[vi].y;
            g_ed.dragWallVertCount++;

            if (g_ed.dragWallVertCount >= ED_MAX_VERTS) break;
        }
    }

    if (g_ed.dragWallVertCount > 0) {
        g_ed.draggingWall = 1;
    }
}

static void beginWallSelectionDrag(int wallIndex, float worldX, float worldY)
{
    int wallIndices[ED_MAX_WALLS];
    int wallCount = 0;

    g_ed.draggingWall = 0;
    g_ed.dragWallVertCount = 0;

    if (wallIndex < 0 || wallIndex >= g_edMap.wallCount) return;

    if (isWallInEditSelection(wallIndex)) {
        wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
    }

    if (wallCount <= 0) {
        wallIndices[0] = wallIndex;
        wallCount = 1;
    }

    g_ed.dragWallStartWorldX = worldX;
    g_ed.dragWallStartWorldY = worldY;

    for (int wi = 0; wi < wallCount; wi++) {
        const EdWall *w;
        int verts[2];

        if (wallIndices[wi] < 0 || wallIndices[wi] >= g_edMap.wallCount) continue;

        w = &g_edMap.walls[wallIndices[wi]];
        verts[0] = w->v0;
        verts[1] = w->v1;

        for (int vk = 0; vk < 2; vk++) {
            const int vi = verts[vk];
            int exists = 0;

            if (vi < 0 || vi >= g_edMap.vertCount) continue;

            for (int j = 0; j < g_ed.dragWallVertCount; j++) {
                if (g_ed.dragWallVertIndices[j] == vi) {
                    exists = 1;
                    break;
                }
            }

            if (exists) continue;
            if (g_ed.dragWallVertCount >= ED_MAX_VERTS) break;

            g_ed.dragWallVertIndices[g_ed.dragWallVertCount] = vi;
            g_ed.dragWallVertStartX[g_ed.dragWallVertCount] = g_edMap.verts[vi].x;
            g_ed.dragWallVertStartY[g_ed.dragWallVertCount] = g_edMap.verts[vi].y;
            g_ed.dragWallVertCount++;
        }
    }

    if (g_ed.dragWallVertCount > 0) {
        g_ed.draggingWall = 1;
    }
}

static void dragSelectedWallTo(float worldX, float worldY)
{
    if (!g_ed.draggingWall || g_ed.dragWallVertCount <= 0) return;

    {
        const float dx = snapDeltaf(worldX - g_ed.dragWallStartWorldX);
        const float dy = snapDeltaf(worldY - g_ed.dragWallStartWorldY);

        for (int i = 0; i < g_ed.dragWallVertCount; i++) {
            const int vi = g_ed.dragWallVertIndices[i];

            if (vi < 0 || vi >= g_edMap.vertCount) continue;

            g_edMap.verts[vi].x = g_ed.dragWallVertStartX[i] + dx;
            g_edMap.verts[vi].y = g_ed.dragWallVertStartY[i] + dy;
        }
    }

    syncAllPortals();
}

static void beginSectorDragFromPress(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    selectExplicitTarget(ED_SEL_SECTOR, sectorIndex);
    pushUndoState();
    beginSectorDrag(sectorIndex, g_ed.pendingLeftWorldX, g_ed.pendingLeftWorldY);
}

static void promotePendingLeftMouseAction(int mouseX, int mouseY)
{
    switch (g_ed.pendingLeftAction) {
        case ED_PENDING_LEFT_MULTI_DRAG:
            pushUndoState();
            beginMultiVertexDrag(g_ed.pendingLeftWorldX, g_ed.pendingLeftWorldY);
            break;

        case ED_PENDING_LEFT_VERTEX:
            beginVertexDragFromPress(g_ed.pendingLeftTargetIndex);
            break;

        case ED_PENDING_LEFT_WALL_DRAG:
            beginWallDragFromPress(g_ed.pendingLeftTargetIndex);
            break;

        case ED_PENDING_LEFT_SECTOR_DRAG:
            beginSectorDragFromPress(g_ed.pendingLeftTargetIndex);
            break;

        case ED_PENDING_LEFT_WALL_CLICK_OR_BOX:
            if (g_ed.pendingLeftAltDown &&
                isWallInEditSelection(g_ed.pendingLeftTargetIndex)) {
                pushUndoState();
                beginWallSelectionDrag(g_ed.pendingLeftTargetIndex,
                                       g_ed.pendingLeftWorldX,
                                       g_ed.pendingLeftWorldY);
            } else {
                clearAllSelections();
                beginBoxSelect(g_ed.pendingLeftMouseX,
                               g_ed.pendingLeftMouseY,
                               1);
                updateBoxSelect(mouseX, mouseY);
            }
            break;

        case ED_PENDING_LEFT_SECTOR_CLICK_OR_BOX:
            if (g_ed.pendingLeftCtrlDown &&
                isSectorInEditSelection(g_ed.pendingLeftTargetIndex)) {
                pushUndoState();
                beginSectorDrag(g_ed.pendingLeftTargetIndex,
                                g_ed.pendingLeftWorldX,
                                g_ed.pendingLeftWorldY);
            } else if (g_ed.pendingLeftCtrlDown) {
                toggleSectorMultiSelection(g_ed.pendingLeftTargetIndex);
            } else {
                selectExplicitTarget(ED_SEL_SECTOR, g_ed.pendingLeftTargetIndex);
            }
            break;

        case ED_PENDING_LEFT_EMPTY_CLICK:
            clearAllSelections();
            break;

        case ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX:
            clearAllSelections();
            beginBoxSelect(g_ed.pendingLeftMouseX,
                           g_ed.pendingLeftMouseY,
                           g_ed.pendingLeftBoxSelectWalls);
            updateBoxSelect(mouseX, mouseY);
            break;

        case ED_PENDING_LEFT_NONE:
        default:
            break;
    }

    clearPendingLeftMouseAction();
}

static void commitPendingLeftMouseClick(void)
{
    switch (g_ed.pendingLeftAction) {
        case ED_PENDING_LEFT_VERTEX:
            selectExplicitTarget(ED_SEL_VERTEX, g_ed.pendingLeftTargetIndex);
            break;

        case ED_PENDING_LEFT_WALL_CLICK_OR_BOX:
            if (g_ed.pendingLeftAltDown) {
                toggleWallMultiSelection(g_ed.pendingLeftTargetIndex);
            } else {
                selectExplicitTarget(ED_SEL_WALL, g_ed.pendingLeftTargetIndex);
            }
            break;

        case ED_PENDING_LEFT_SECTOR_CLICK_OR_BOX:
            if (g_ed.pendingLeftCtrlDown) {
                toggleSectorMultiSelection(g_ed.pendingLeftTargetIndex);
            } else {
                selectExplicitTarget(ED_SEL_SECTOR, g_ed.pendingLeftTargetIndex);
            }
            break;

        case ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX:
        case ED_PENDING_LEFT_EMPTY_CLICK:
            clearAllSelections();
            break;

        case ED_PENDING_LEFT_MULTI_DRAG:
        case ED_PENDING_LEFT_WALL_DRAG:
        case ED_PENDING_LEFT_SECTOR_DRAG:
        case ED_PENDING_LEFT_NONE:
        default:
            break;
    }

    clearPendingLeftMouseAction();
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
    clearMultiWallSelection();
    clearMultiSectorSelection();
}

static int hasAnyActiveSelection(void)
{
    return (g_ed.selectionType != ED_SEL_NONE) ||
           (g_ed.selectedVertCount > 0) ||
           (g_ed.selectedWallCount > 0) ||
           (g_ed.selectedSectorCount > 0) ||
           g_ed.boxSelecting ||
           g_ed.draggingVertex ||
           g_ed.draggingWall ||
           g_ed.draggingSector ||
           g_ed.draggingMultiVertex ||
           g_ed.pendingLeftMouseDown;
}

static void cancelActiveSelection(void)
{
    clearAllSelections();
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    g_ed.draggingMultiVertex = 0;
    g_ed.boxSelecting = 0;
    g_ed.boxSelectWalls = 0;
    g_ed.splitPreviewValid = 0;
    clearPendingLeftMouseAction();
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

static void beginBoxSelect(int mouseX, int mouseY, int selectWalls)
{
    g_ed.boxSelecting = 1;
    g_ed.boxSelectWalls = selectWalls ? 1 : 0;
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

static int computeOutCodeForRect(float x, float y, float xmin, float ymin, float xmax, float ymax)
{
    int code = 0;

    if (x < xmin) code |= 1;
    else if (x > xmax) code |= 2;

    if (y < ymin) code |= 4;
    else if (y > ymax) code |= 8;

    return code;
}

static int lineIntersectsRectLocal(float x0, float y0,
                                   float x1, float y1,
                                   float xmin, float ymin,
                                   float xmax, float ymax)
{
    int out0 = computeOutCodeForRect(x0, y0, xmin, ymin, xmax, ymax);
    int out1 = computeOutCodeForRect(x1, y1, xmin, ymin, xmax, ymax);

    for (;;) {
        if ((out0 | out1) == 0) {
            return 1;
        }

        if (out0 & out1) {
            return 0;
        }

        {
            const int out = out0 ? out0 : out1;
            float x = 0.0f;
            float y = 0.0f;

            if (out & 8) {
                x = x0 + (x1 - x0) * ((ymax - y0) / (y1 - y0));
                y = ymax;
            } else if (out & 4) {
                x = x0 + (x1 - x0) * ((ymin - y0) / (y1 - y0));
                y = ymin;
            } else if (out & 2) {
                y = y0 + (y1 - y0) * ((xmax - x0) / (x1 - x0));
                x = xmax;
            } else {
                y = y0 + (y1 - y0) * ((xmin - x0) / (x1 - x0));
                x = xmin;
            }

            if (out == out0) {
                x0 = x;
                y0 = y;
                out0 = computeOutCodeForRect(x0, y0, xmin, ymin, xmax, ymax);
            } else {
                x1 = x;
                y1 = y;
                out1 = computeOutCodeForRect(x1, y1, xmin, ymin, xmax, ymax);
            }
        }
    }
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

    if (g_ed.boxSelectWalls) {
        for (int i = 0; i < g_edMap.wallCount; i++) {
            const EdWall *w = &g_edMap.walls[i];
            int sx0, sy0, sx1, sy1;

            if (w->v0 < 0 || w->v0 >= g_edMap.vertCount ||
                w->v1 < 0 || w->v1 >= g_edMap.vertCount) {
                continue;
            }

            worldToScreen(g_edMap.verts[w->v0].x, g_edMap.verts[w->v0].y, &sx0, &sy0);
            worldToScreen(g_edMap.verts[w->v1].x, g_edMap.verts[w->v1].y, &sx1, &sy1);

            if (lineIntersectsRectLocal((float)sx0, (float)sy0,
                                        (float)sx1, (float)sy1,
                                        (float)x0, (float)y0,
                                        (float)x1, (float)y1)) {
                addMultiWallSelection(i);
            }
        }

        normalizeWallSelectionState();
    } else {
        for (int i = 0; i < g_edMap.vertCount; i++) {
            int sx, sy;

            worldToScreen(g_edMap.verts[i].x, g_edMap.verts[i].y, &sx, &sy);

            if (sx >= x0 && sx <= x1 && sy >= y0 && sy <= y1) {
                g_ed.selectedVerts[i] = 1;
                g_ed.selectedVertCount++;
            }
        }
    }

    g_ed.boxSelecting = 0;
    g_ed.boxSelectWalls = 0;
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

    drawRectL(x0, y0, x1 - x0 + 1, y1 - y0 + 1, ED_COLOUR_MULTI_SELECT_RANGE_BOX);
    drawRectL(x0-1, y0-1, x1 - x0 + 3, y1 - y0 + 3, ED_COLOUR_MULTI_SELECT_RANGE_BOX);
}

static void drawSectorSelectionHighlight(int sectorIndex)
{
    if (sectorIndex < 0 || sectorIndex >= g_edMap.sectorCount) return;

    const EdSector *sec = &g_edMap.sectors[sectorIndex];
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
                    oy = 1;
                } else {
                    ox = 1;
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
        drawRectL(cx - 8, cy - 8, 17, 17, ED_COLOUR_SELECTED_SECTOR);
        drawRectL(cx - 5, cy - 5, 11, 11, ED_COLOUR_SELECTED_SECTOR);
    }
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
    memset(g_ed.selectedWalls, 0, sizeof(g_ed.selectedWalls));
    g_ed.selectedWallCount = 0;
    memset(g_ed.selectedSectors, 0, sizeof(g_ed.selectedSectors));
    g_ed.selectedSectorCount = 0;

    g_ed.bUseVectorFill = 0;
    g_ed.bUseTextureFill = 0;
    g_ed.isometricView = 0;

    g_ed.boxSelecting = 0;
    g_ed.boxSelectWalls = 0;
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
    memset(&g_ed.copiedSectorProps, 0, sizeof(g_ed.copiedSectorProps));
    g_ed.hasCopiedSectorProps = 0;
    g_ed.copiedSectorPropsSourceSector = -1;
    g_ed.copiedWallUpperColor = 0;
    g_ed.copiedWallMidColor = 0;
    g_ed.copiedWallLowerColor = 0;
    g_ed.hasCopiedWallTexture = 0;
    g_ed.copiedWallTexScaleX = 1.0f;
    g_ed.copiedWallTexScaleY = 1.0f;
    g_ed.hasCopiedWallScale = 0;
    g_ed.copiedWallTexAngle = 0.0f;
    g_ed.hasCopiedWallRotation = 0;
    memset(&g_ed.copiedWallProps, 0, sizeof(g_ed.copiedWallProps));
    g_ed.hasCopiedWallProps = 0;
    g_ed.copiedWallPropsSourceWall = -1;
    resetSectorGeometryClipboard();

    g_ed.selectionType = ED_SEL_NONE;
    g_ed.selectedVert = -1;
    g_ed.selectedWall = -1;
    g_ed.selectedSector = -1;
    g_ed.draggingVertex = 0;
    g_ed.draggingWall = 0;
    g_ed.draggingSector = 0;
    clearPendingLeftMouseAction();
    g_ed.pendingLeftCtrlDown = 0;
    g_ed.pendingLeftAltDown = 0;

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

    drawText(x + 12, y + 12, "Confirm", ED_INSPECTOR_PANELS_HEADER_TEXT);
    drawText(x + 12, y + 34, g_ed.confirmText, ED_TEXT_COL);
}



static int saveTextMap(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "MAPEDIT4\n");
    fprintf(f, "START %.6f %.6f %.6f %d\n",
            g_edMap.startX, g_edMap.startY, g_edMap.startAngle, g_edMap.startSector);

    fprintf(f, "VERTS %d\n", g_edMap.vertCount);
    for (int i = 0; i < g_edMap.vertCount; i++) {
        fprintf(f, "%.6f %.6f\n", g_edMap.verts[i].x, g_edMap.verts[i].y);
    }

    fprintf(f, "WALLS %d\n", g_edMap.wallCount);
    for (int i = 0; i < g_edMap.wallCount; i++) {
        const EdWall *w = &g_edMap.walls[i];
        fprintf(f, "%d %d %d %.6f %.6f %u %u %u %u %u %.6f %.6f\n",
            w->v0, w->v1, w->neighbour,
            w->openBottom, w->openTop,
            (unsigned)w->upperColor,
            (unsigned)w->midColor,
            (unsigned)w->lowerColor,
            (unsigned)w->flags,
            (unsigned)w->tex_flags,
            w->texScaleX,
            w->texScaleY);
    }

    fprintf(f, "SECTORS %d\n", g_edMap.sectorCount);
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f, "%d %d %d %.6f %.6f %u %u %u %d %u %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f\n",
                s->wallStart, s->wallCount, s->boundaryCount,
                s->floorHeight, s->ceilHeight,
                (unsigned)s->floorColor,
                (unsigned)s->ceilColor,
                (unsigned)clampLightLevel((int)s->glowlevel),
                s->tagId,
                (unsigned)s->stateFlags,
                s->floorMinHeight,
                s->floorMaxHeight,
                s->ceilMinHeight,
                s->ceilMaxHeight,
                s->floorFlowHeight,
                s->ceilFlowHeight,
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
    int mapVersion = 0;

    EditorMap newMap;

    if (!f) return 0;

    memset(&newMap, 0, sizeof(newMap));
    newMap.startSector = 0;
    newMap.startX = 0.0f;
    newMap.startY = 0.0f;
    newMap.startAngle = 0.0f;

    if (fscanf(f, "%63s", tag) != 1) {
        fclose(f);
        return 0;
    }

    if (strcmp(tag, "MAPEDIT1") == 0) {
        mapVersion = 1;
    } else if (strcmp(tag, "MAPEDIT2") == 0) {
        mapVersion = 2;
    } else if (strcmp(tag, "MAPEDIT3") == 0) {
        mapVersion = 3;
    } else if (strcmp(tag, "MAPEDIT4") == 0) {
        mapVersion = 4;
    } else {
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
        clearWallTexFlags(&newMap.walls[i]);

        if (mapVersion >= 3) {
            if (fscanf(f, "%d %d %d %f %f %u %u %u %u %u %f %f",
                    &newMap.walls[i].v0,
                    &newMap.walls[i].v1,
                    &newMap.walls[i].neighbour,
                    &newMap.walls[i].openBottom,
                    &newMap.walls[i].openTop,
                    &uc, &mc, &lc, &flags, &tex_flags,
                    &newMap.walls[i].texScaleX,
                    &newMap.walls[i].texScaleY) != 12) {
                fclose(f);
                return 0;
            }
        } else {
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
        }

        newMap.walls[i].upperColor = (uint8_t)uc;
        newMap.walls[i].midColor   = (uint8_t)mc;
        newMap.walls[i].lowerColor = (uint8_t)lc;
        newMap.walls[i].flags      = (uint8_t)flags;
        newMap.walls[i].tex_flags  = (uint32_t)tex_flags;
        setWallTexScaleX(&newMap.walls[i], newMap.walls[i].texScaleX);
        setWallTexScaleY(&newMap.walls[i], newMap.walls[i].texScaleY);
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
        unsigned fc, cc, glow = 0;
        unsigned stateFlags = 0;

        if (mapVersion >= 4) {
            if (fscanf(f, "%d %d %d %f %f %u %u %u %d %u %f %f %f %f %f %f %f %f %f %f %f %f",
                    &newMap.sectors[i].wallStart,
                    &newMap.sectors[i].wallCount,
                    &newMap.sectors[i].boundaryCount,
                    &newMap.sectors[i].floorHeight,
                    &newMap.sectors[i].ceilHeight,
                    &fc, &cc, &glow,
                    &newMap.sectors[i].tagId,
                    &stateFlags,
                    &newMap.sectors[i].floorMinHeight,
                    &newMap.sectors[i].floorMaxHeight,
                    &newMap.sectors[i].ceilMinHeight,
                    &newMap.sectors[i].ceilMaxHeight,
                    &newMap.sectors[i].floorFlowHeight,
                    &newMap.sectors[i].ceilFlowHeight,
                    &newMap.sectors[i].floorTexScaleX,
                    &newMap.sectors[i].floorTexScaleY,
                    &newMap.sectors[i].floorTexAngle,
                    &newMap.sectors[i].ceilTexScaleX,
                    &newMap.sectors[i].ceilTexScaleY,
                    &newMap.sectors[i].ceilTexAngle) != 22) {
                fclose(f);
                return 0;
            }
        } else if (mapVersion >= 2) {
            if (fscanf(f, "%d %d %d %f %f %u %u %u %f %f %f %f %f %f",
                    &newMap.sectors[i].wallStart,
                    &newMap.sectors[i].wallCount,
                    &newMap.sectors[i].boundaryCount,
                    &newMap.sectors[i].floorHeight,
                    &newMap.sectors[i].ceilHeight,
                    &fc, &cc, &glow,
                    &newMap.sectors[i].floorTexScaleX,
                    &newMap.sectors[i].floorTexScaleY,
                    &newMap.sectors[i].floorTexAngle,
                    &newMap.sectors[i].ceilTexScaleX,
                    &newMap.sectors[i].ceilTexScaleY,
                    &newMap.sectors[i].ceilTexAngle) != 14) {
                fclose(f);
                return 0;
            }
        } else {
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
        }

        newMap.sectors[i].floorColor = (uint8_t)fc;
        newMap.sectors[i].ceilColor  = (uint8_t)cc;
        newMap.sectors[i].glowlevel  = clampLightLevel((int)glow);
        newMap.sectors[i].stateFlags = (uint32_t)stateFlags;

        if (mapVersion < 4) {
            resetSectorMovePropertiesToCurrentHeights(&newMap.sectors[i]);
        }

        sanitizeSectorProperties(&newMap.sectors[i]);
    }

    fclose(f);

    /* only commit after full successful parse */
    g_edMap = newMap;
    resetUndoRedoHistory();

    /* editor state cleanup, but keep camera/zoom */
    clearDraft();
    clearMultiVertexSelection();
    clearMultiWallSelection();
    clearMultiSectorSelection();

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
    g_ed.boxSelectWalls = 0;
    clearPendingLeftMouseAction();

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
        const char magic[8] = { 'R','C','3','D','M','A','P','4' };
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
        float texScaleX    = w->texScaleX;
        float texScaleY    = w->texScaleY;

        if (fwrite(&v0,         sizeof(v0),         1, f) != 1 ||
            fwrite(&v1,         sizeof(v1),         1, f) != 1 ||
            fwrite(&neighbour,  sizeof(neighbour),  1, f) != 1 ||
            fwrite(&openBottom, sizeof(openBottom), 1, f) != 1 ||
            fwrite(&openTop,    sizeof(openTop),    1, f) != 1 ||
            fwrite(&upper,      sizeof(upper),      1, f) != 1 ||
            fwrite(&mid,        sizeof(mid),        1, f) != 1 ||
            fwrite(&lower,      sizeof(lower),      1, f) != 1 ||
            fwrite(&flags,      sizeof(flags),      1, f) != 1 ||
            fwrite(&tex_flags,  sizeof(tex_flags),  1, f) != 1 ||
            fwrite(&texScaleX,  sizeof(texScaleX),  1, f) != 1 ||
            fwrite(&texScaleY,  sizeof(texScaleY),  1, f) != 1) {
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
        uint8_t glowlevel     = clampLightLevel((int)s->glowlevel);
        int32_t tagId         = (int32_t)s->tagId;
        uint32_t stateFlags   = s->stateFlags;
        float floorMinHeight  = s->floorMinHeight;
        float floorMaxHeight  = s->floorMaxHeight;
        float ceilMinHeight   = s->ceilMinHeight;
        float ceilMaxHeight   = s->ceilMaxHeight;
        float floorFlowHeight = s->floorFlowHeight;
        float ceilFlowHeight  = s->ceilFlowHeight;

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
            fwrite(&glowlevel,      sizeof(glowlevel),      1, f) != 1 ||
            fwrite(&tagId,          sizeof(tagId),          1, f) != 1 ||
            fwrite(&stateFlags,     sizeof(stateFlags),     1, f) != 1 ||
            fwrite(&floorMinHeight, sizeof(floorMinHeight), 1, f) != 1 ||
            fwrite(&floorMaxHeight, sizeof(floorMaxHeight), 1, f) != 1 ||
            fwrite(&ceilMinHeight,  sizeof(ceilMinHeight),  1, f) != 1 ||
            fwrite(&ceilMaxHeight,  sizeof(ceilMaxHeight),  1, f) != 1 ||
            fwrite(&floorFlowHeight,sizeof(floorFlowHeight),1, f) != 1 ||
            fwrite(&ceilFlowHeight, sizeof(ceilFlowHeight), 1, f) != 1 ||
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
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u, %u, %u, %uu, %.6ff, %.6ff },\n",
            w->v0, w->v1, w->neighbour, w->openBottom, w->openTop,
            (unsigned)w->upperColor,
            (unsigned)w->midColor,
            (unsigned)w->lowerColor,
            (unsigned)w->flags,
            (unsigned)w->tex_flags,
            w->texScaleX,
            w->texScaleY);
    }
    fprintf(f, "};\n\n");

    fprintf(f, "static const RC3D_Sector g_sectors[] = {\n");
    for (int i = 0; i < g_edMap.sectorCount; i++) {
        const EdSector *s = &g_edMap.sectors[i];
        fprintf(f,
            "    { %d, %d, %d, %.6ff, %.6ff, %u, %u, %u, %d, %uu, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff },\n",
            s->wallStart, s->wallCount, s->boundaryCount,
            s->floorHeight, s->ceilHeight,
            (unsigned)s->floorColor, (unsigned)s->ceilColor,
            (unsigned)clampLightLevel((int)s->glowlevel),
            s->tagId, (unsigned)s->stateFlags,
            s->floorMinHeight, s->floorMaxHeight,
            s->ceilMinHeight, s->ceilMaxHeight,
            s->floorFlowHeight, s->ceilFlowHeight,
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
        clearWallTexFlags(&newWallsA[countA]);
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
        clearWallTexFlags(&newWallsB[countB]);
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

    const EdSector sectorProps = *sec;

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

    g_edMap.sectors[oldSelectedSector] = sectorProps;
    g_edMap.sectors[oldSelectedSector].wallStart = startA;
    g_edMap.sectors[oldSelectedSector].wallCount = countA;
    g_edMap.sectors[oldSelectedSector].boundaryCount = countA;
    sanitizeSectorProperties(&g_edMap.sectors[oldSelectedSector]);

    /* sector B is new */
    newSector = g_edMap.sectorCount;
    startB = g_edMap.wallCount;
    for (int i = 0; i < countB; i++) {
        g_edMap.walls[g_edMap.wallCount++] = newWallsB[i];
    }

    g_edMap.sectors[newSector] = sectorProps;
    g_edMap.sectors[newSector].wallStart = startB;
    g_edMap.sectors[newSector].wallCount = countB;
    g_edMap.sectors[newSector].boundaryCount = countB;
    sanitizeSectorProperties(&g_edMap.sectors[newSector]);
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
            clearWallTexFlags(w);
        }

        initializeNewSectorDefaults(&g_edMap.sectors[g_edMap.sectorCount],
                                    wallStart,
                                    g_ed.draftCount,
                                    g_ed.draftCount);

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
            clearWallTexFlags(&outerPieces[pieceCount]);
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
        clearWallTexFlags(&outerPieces[pieceCount]);
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
            clearWallTexFlags(&outerPieces[pieceCount]);
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
            clearWallTexFlags(w);
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
                clearWallTexFlags(w);
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
                clearWallTexFlags(w);
            }
        }

        initializeNewSectorDefaults(&g_edMap.sectors[newSectorIndex],
                                    newWallStart,
                                    g_ed.draftCount,
                                    g_ed.draftCount);

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
        clearWallTexFlags(w);
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
    const int oldSectorCount = g_edMap.sectorCount;

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

    remapMultiSectorSelectionFromOldToNew(sectorRemap, oldSectorCount);

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

            if (isSectorInEditSelection(owner)) {
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

    if (isSectorInEditSelection(sectorIndex)) {
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
    if (isSectorInEditSelection(sectorIndex)) {
        return 2;
    }

    if (g_ed.hoverSector == sectorIndex) {
        return 3;
    }

    return 4;
}

static int wrapTextureCoordLocal(int v, int size)
{
    if (size <= 0) {
        return 0;
    }

    v %= size;
    if (v < 0) {
        v += size;
    }

    return v;
}

static void drawTexturedSectorSpan(int x0, int x1, int y, const EdSector *sec)
{
    uint8_t *tex;
    uint8_t *dst;
    float scaleX, scaleY;
    float cosA, sinA;
    float worldStepX;
    float worldX, worldY;
    float texU, texV;
    float texUStep, texVStep;

    if (!sec) {
        return;
    }

    if ((unsigned)y >= EDIT_VIEW_PORT_HEIGHT || x1 < x0) {
        return;
    }

    tex = getTexturePtr(sec->floorColor);
    if (!tex) {
        drawLineDots(x0, y, x1, y, getSectorFillColour((int)(sec - g_edMap.sectors)));
        return;
    }

    scaleX = sec->floorTexScaleX;
    scaleY = sec->floorTexScaleY;

    if (fabsf(scaleX) < 0.001f) scaleX = 0.1f;
    if (fabsf(scaleY) < 0.001f) scaleY = 0.1f;

    cosA = cosf(sec->floorTexAngle);
    sinA = sinf(sec->floorTexAngle);

    worldStepX = 1.0f / g_ed.zoom;
    worldX = g_ed.camX + ((float)x0 - (EDIT_VIEW_PORT_WIDTH * 0.5f)) / g_ed.zoom;
    worldY = g_ed.camY + ((float)y - (EDIT_VIEW_PORT_HEIGHT * 0.5f)) / g_ed.zoom;

    texU = (((worldX * cosA) - (worldY * sinA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleX;
    texV = (((worldX * sinA) + (worldY * cosA)) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleY;

    texUStep = ((worldStepX * cosA) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleX;
    texVStep = ((worldStepX * sinA) * ED_TEXTURE_FILL_TEXELS_PER_WORLD_UNIT) / scaleY;

    dst = &fb[(y * SCREEN_W) + x0];

    for (int x = x0; x <= x1; x++) {
        const int tx = wrapTextureCoordLocal((int)floorf(texU), TEXTURE_WIDTH);
        const int ty = wrapTextureCoordLocal((int)floorf(texV), TEXTURE_HEIGHT);

        *dst++ = tex[(ty * TEXTURE_WIDTH) + tx];

        texU += texUStep;
        texV += texVStep;
    }
}

static void drawTexturedSector2DBruteForceScanline(int sectorIndex,
                                                   int minX,
                                                   int maxX,
                                                   int y,
                                                   const EdSector *sec)
{
    int runStart = -1;
    float worldX;
    float worldY;
    const float worldStepX = 1.0f / g_ed.zoom;

    if (!sec || minX > maxX) {
        return;
    }

    worldX = g_ed.camX + (((float)minX + 0.5f) - (EDIT_VIEW_PORT_WIDTH * 0.5f)) / g_ed.zoom;
    worldY = g_ed.camY + (((float)y + 0.5f) - (EDIT_VIEW_PORT_HEIGHT * 0.5f)) / g_ed.zoom;

    for (int x = minX; x <= maxX; x++) {
        const int inside = pointInSector(worldX, worldY, sectorIndex);

        if (inside) {
            if (runStart < 0) {
                runStart = x;
            }
        } else if (runStart >= 0) {
            drawTexturedSectorSpan(runStart, x - 1, y, sec);
            runStart = -1;
        }

        worldX += worldStepX;
    }

    if (runStart >= 0) {
        drawTexturedSectorSpan(runStart, maxX, y, sec);
    }
}

static void drawVectorSector2DBruteForceScanline(int sectorIndex,
                                                 int minX,
                                                 int maxX,
                                                 int y,
                                                 uint8_t fillCol)
{
    int runStart = -1;
    float worldX;
    float worldY;
    const float worldStepX = 1.0f / g_ed.zoom;

    if (minX > maxX) {
        return;
    }

    worldX = g_ed.camX + (((float)minX + 0.5f) - (EDIT_VIEW_PORT_WIDTH * 0.5f)) / g_ed.zoom;
    worldY = g_ed.camY + (((float)y + 0.5f) - (EDIT_VIEW_PORT_HEIGHT * 0.5f)) / g_ed.zoom;

    for (int x = minX; x <= maxX; x++) {
        const int inside = pointInSector(worldX, worldY, sectorIndex);

        if (inside) {
            if (runStart < 0) {
                runStart = x;
            }
        } else if (runStart >= 0) {
            drawLineDots(runStart, y, x - 1, y, fillCol);
            runStart = -1;
        }

        worldX += worldStepX;
    }

    if (runStart >= 0) {
        drawLineDots(runStart, y, maxX, y, fillCol);
    }
}

static void drawFilledSector2D(int sectorIndex)
{
    static int polyX[ED_MAX_WALLS];
    static int polyY[ED_MAX_WALLS];
    static float spanNodes[ED_MAX_WALLS];

    const EdSector *sec;
    int count;
    int minX, maxX;
    int minY, maxY;
    uint8_t fillCol;
    int stepY;
    const int useTextureFill = g_ed.bUseTextureFill != 0;
    const int useVectorFill = !useTextureFill && (g_ed.bUseVectorFill != 0);

    if (!useTextureFill && !useVectorFill) return;

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
    minX = polyX[0];
    maxX = polyX[0];

    for (int i = 1; i < count; i++) {
        if (polyY[i] < minY) minY = polyY[i];
        if (polyY[i] > maxY) maxY = polyY[i];
        if (polyX[i] < minX) minX = polyX[i];
        if (polyX[i] > maxX) maxX = polyX[i];
    }

    if (maxX < 0 || minX >= EDIT_VIEW_PORT_WIDTH) return;
    if (maxY < 0 || minY >= EDIT_VIEW_PORT_HEIGHT) return;

    minX = clampi_local(minX, 0, EDIT_VIEW_PORT_WIDTH - 1);
    maxX = clampi_local(maxX, 0, EDIT_VIEW_PORT_WIDTH - 1);
    minY = clampi_local(minY, 0, EDIT_VIEW_PORT_HEIGHT - 1);
    maxY = clampi_local(maxY, 0, EDIT_VIEW_PORT_HEIGHT - 1);

    fillCol = getSectorFillColour(sectorIndex);
    stepY   = useTextureFill ? 1 : getSectorFillStepY(sectorIndex);
    if (stepY < 1) stepY = 1;

    for (int y = minY; y <= maxY; y += stepY) {
        int nodeCount = 0;
        const float scanWorldY =
            g_ed.camY + (((float)y + 0.5f) - (EDIT_VIEW_PORT_HEIGHT * 0.5f)) / g_ed.zoom;

        for (int i = 0; i < count; i++) {
            const EdWall *w = &g_edMap.walls[sec->wallStart + i];
            const EdVec2 *a = &g_edMap.verts[w->v0];
            const EdVec2 *b = &g_edMap.verts[w->v1];

            if (fabsf(b->y - a->y) <= 0.0001f) {
                continue;
            }

            if (((a->y < scanWorldY) && (b->y >= scanWorldY)) ||
                ((b->y < scanWorldY) && (a->y >= scanWorldY))) {
                if (nodeCount < ED_MAX_WALLS) {
                    const float t = (scanWorldY - a->y) / (b->y - a->y);
                    const float hitWorldX = a->x + ((b->x - a->x) * t);

                    spanNodes[nodeCount++] =
                        ((hitWorldX - g_ed.camX) * g_ed.zoom) + (EDIT_VIEW_PORT_WIDTH * 0.5f);
                }
            }
        }

        for (int i = 1; i < nodeCount; i++) {
            const float v = spanNodes[i];
            int k = i - 1;
            while (k >= 0 && spanNodes[k] > v) {
                spanNodes[k + 1] = spanNodes[k];
                k--;
            }
            spanNodes[k + 1] = v;
        }

        if ((nodeCount & 1) != 0) {
            if (useTextureFill) {
                drawTexturedSector2DBruteForceScanline(sectorIndex, minX, maxX, y, sec);
            } else {
                drawVectorSector2DBruteForceScanline(sectorIndex, minX, maxX, y, fillCol);
            }
            continue;
        }

        for (int i = 0; i + 1 < nodeCount; i += 2) {
            int x0 = (int)ceilf(spanNodes[i]);
            int x1 = (int)floorf(spanNodes[i + 1]);

            if (x1 < minX || x0 > maxX) {
                continue;
            }

            x0 = clampi_local(x0, minX, maxX);
            x1 = clampi_local(x1, minX, maxX);

            if (x1 >= x0) {
                if (useTextureFill) {
                    drawTexturedSectorSpan(x0, x1, y, sec);
                } else {
                    drawLineDots(x0, y, x1, y, fillCol);
                }
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
    if (g_ed.tinyGridEnabled && (g_ed.zoom > 128)) {
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

    if (g_ed.isometricView) {
        drawText(340, EDIT_VIEW_PORT_HEIGHT + 4, "ISO PREVIEW [KP*] toggle  MMB pan  Wheel zoom", ED_START_COL);
    }

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

    if (hasMultiWallSelection()) {
        snprintf(buf, sizeof(buf), "Selected walls: %d", getWallEditSelectionCount());
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
                 "Sector %d  Floor %.3f  Ceil %.3f  Tag %d  State 0x%X  Walls %d  Boundary %d",
                 sector_id, s->floorHeight, s->ceilHeight, s->tagId, (unsigned)s->stateFlags,
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

        drawText(x, y, "MAP VALIDATOR ::: F9 close ::: [ / ] cycle issues", ED_VALIDATOR_TEXT);
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
                    drawRect(x, y - 2, ED_PANEL_W - 20, ED_FONT_H + 4, ED_VALIDATOR_SELECTION_BG);
                    col = ED_VALIDATOR_SELECTION_TEXT;
                }

                drawText(x+4, y, g_ed.validatorLines[i], col);
                y += ED_ROW_STEP;

                if (y > (EDIT_VIEW_PORT_HEIGHT - ED_ROW_STEP)) {
                    drawText(x+4, y, "...more issues not shown", ED_TEXT_COL);
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

        drawText(x, y, "RC3D EDITOR ::: F12 to launch the test map", ED_EXPANDED_MENU_TEXT);
        y += ED_ROW_STEP;

        drawText(x, y, "LMB drag-box vertices, RMB add draft point, MMB pan, Wheel zoom", ED_TEXT_COL);
        y += ED_ROW_STEP;

        drawText(x, y, "[ENTER] finish draft, [ESC] unselect / clear draft, [DEL] delete hovered", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[SHIFT] vertices, [CTRL] sectors, [ALT] walls", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[F6] auto-build sectors from closed inner wall loops", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "['] vector fill, [#] floor texture fill", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[KP*] toggle isometric preview (view-only)", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "CTRL+Z undo, CTRL+Y redo, F11 or CTRL+H history, [TAB] grid", ED_TEXT_COL);
        y += ED_ROW_STEP;
        drawText(x, y, "[F7] Repair Topology, [F8] clean map, [SHIFT+DRAG] drop = merge", ED_TEXT_COL);

        y += 6;//ED_ROW_STEP;

        if((g_ed.selectionType == ED_SEL_VERTEX) ||
             (g_ed.selectionType == ED_SEL_WALL) ||
             (g_ed.selectionType == ED_SEL_SECTOR) ||
            (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0) ||
            hasMultiWallSelection() ||
            hasMultiSectorSelection())
        {
            drawText(x, y, "________________________________________________________________________________________________", ED_TEXT_COL);
            y += ED_ROW_STEP;
        } else 
            y+= 10;
        
        if(g_ed.selectionType == ED_SEL_VERTEX){
            drawText(x, y, "--- VERTEX HELP", ED_EXPANDED_MENU_TEXT);
            y += ED_ROW_STEP;
            drawText(x, y, "[ARROWS] 0.01   [SHIFT+ARROWS] 0.001   [ALT+ARROWS] 0.0001", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[SHIFT+DROP] onto another vertex = merge", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(g_ed.selectionType == ED_SEL_WALL){
            drawText(x, y, "--- WALL HELP", ED_EXPANDED_MENU_TEXT);
            y += ED_ROW_STEP;
            drawText(x, y, "[NUM_1]/[NUM_2]/[NUM_3] copy wall tex/scale/rot", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[SHIFT+NUM_1]/[SHIFT+NUM_2]/[SHIFT+NUM_3] paste", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[R]/[T] bottom level,  [Y]/[U] top level", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[A]/[S] upper texture, [D]/[F] middle texture, [H]/[J] lower texture", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[CTRL+1..4] wall type, [CTRL+5] extrude, [SPACE] split", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(hasMultiWallSelection()){
            drawText(x, y, "--- MULTI WALL HELP", ED_EXPANDED_MENU_TEXT);
            y += ED_ROW_STEP;
            drawText(x, y, "[ALT+CLICK] add/remove walls, [ALT+DRAG] selected walls move / empty drag box-selects", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[NUM_1]/[NUM_2]/[NUM_3] copy from primary wall,", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[SHIFT+NUM_1]/[SHIFT+NUM_2]/[SHIFT+NUM_3] paste to all", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[R]/[T]/[Y]/[U], [Q]/[W], [A]/[S]/[D]/[F]/[H]/[J] apply to all", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "Texture browser, Copy Props, and Paste Props apply to the full selection", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(g_ed.selectionType == ED_SEL_SECTOR){
            drawText(x, y, "--- SECTOR HELP:", ED_EXPANDED_MENU_TEXT);
            y += ED_ROW_STEP;
            drawText(x, y, "[CTRL+CLICK] add/remove sectors, [CTRL+C]/[CTRL+V] copy/paste group", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[1]/[2] copy floor/ceil height, [SHIFT+1]/[SHIFT+2] paste", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "[4]/[5] copy floor/ceil texture_id, [SHIFT+4]/[SHIFT+5] paste", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "Inspector Copy Props / Paste Props copies the full sector setup", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "Use the inspector +/- buttons for sector UV, heights, glow, and mover data", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(hasMultiSectorSelection()){
            drawText(x, y, "--- MULTI SECTOR HELP:", ED_EXPANDED_MENU_TEXT);
            y += ED_ROW_STEP;
            drawText(x, y, "[CTRL+CLICK] add/remove sectors, [CTRL+C] copy, [CTRL+V] paste snapped group", ED_TEXT_COL);
            y += ED_ROW_STEP;
            drawText(x, y, "Paste Props and texture browsing apply to the whole sector selection", ED_TEXT_COL);
            y += ED_ROW_STEP;
        }
        if(g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0){
            drawText(x, y, "--- MULTI-SELECT HELP:", ED_EXPANDED_MENU_TEXT);
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
                if (isWallInEditSelection(i)) {
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
            drawRect(sx - 3, sy - 3, 7, 7, ED_COLOUR_SELECTED_SECTOR);
            drawRect(sx - 1, sy - 1, 3, 3, ED_COLOUR_SELECTED_SECTOR);
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
        drawSectorSelectionHighlight(g_ed.selectedSector);
    } else if (hasMultiSectorSelection()) {
        for (int i = 0; i < g_edMap.sectorCount; i++) {
            if (!g_ed.selectedSectors[i]) continue;
            drawSectorSelectionHighlight(i);
        }
    }

    /* -------------------------------------------------- */
    /* split preview                                      */
    /* -------------------------------------------------- */
    if (g_ed.splitPreviewValid && g_ed.selectionType == ED_SEL_WALL) {
        int sx, sy;
        worldToScreen(g_ed.splitPreviewX, g_ed.splitPreviewY, &sx, &sy);

        drawRect(sx - 3, sy - 3, 7, 7, ED_COLOUR_VERTEX_SPLIT_PREV);
        drawRect(sx - 1, sy - 1, 3, 3, ED_COLOUR_VERTEX_SPLIT_PREV);
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
    #define inspectWallYOffset 20
    /* expanded panel button rows aligned to 16px font + 6px spacing */
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_SOLID,    controloffw + (84 * 0), 194 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Solid");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_PORTAL,   controloffw + (84 * 1), 194 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Portal");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_WINDOW,   controloffw + (84 * 2), 194 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Window");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_DOOR,     controloffw + (84 * 3), 194 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Door");
    //rcguiCreateButton(&g_ui, GUI_BTN_WALL_SPLIT,    controloffw + (84 * 4), 194 + controloff, 80, ED_BTN_H, "Split");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TRANSPARENCY, controloffw + (84 * 4), 194 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Transp.");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_COPY_PROPS,    176 + controloffw,  60 + controloff + inspectWallYOffset, 96, ED_BTN_H, "Copy Props");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_PASTE_PROPS,   278 + controloffw,  60 + controloff + inspectWallYOffset, 96, ED_BTN_H, "Paste Props");
    

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_XL, controloffw + (84 * 2), 256 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Clamp XL");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_XR, controloffw + (84 * 3), 256 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Clamp XR");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_YT, controloffw + (84 * 2), 288 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Clamp YT");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_CLAMP_YB, controloffw + (84 * 3), 288 + controloff + inspectWallYOffset, 80, ED_BTN_H, "Clamp YB");

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_SX_MINUS, 136 + controloffw, 320 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_SX_PLUS,  164 + controloffw, 320 + controloff + inspectWallYOffset, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_SY_MINUS, 336 + controloffw, 320 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_SY_PLUS,  364 + controloffw, 320 + controloff + inspectWallYOffset, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 156 + controloffw, 352 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS,  184 + controloffw, 352 + controloff + inspectWallYOffset, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 216 + controloffw, 352 + controloff + inspectWallYOffset, 60, 24, "Reset");

    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_MINUS, 162 + controloffw, 384 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_PLUS,  194 + controloffw, 384 + controloff + inspectWallYOffset, 24, 24, "+");
    

    // sector inspector UI - wall
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 182 + controloffw, 88 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS,  214 + controloffw, 88 + controloff + inspectWallYOffset, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 182 + controloffw, 116 + controloff + inspectWallYOffset, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS,  214 + controloffw, 116 + controloff + inspectWallYOffset, 24, 24, "+");


    int sector_button_y_offsets = 55;

    // sector inspector UI - sector / heights
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_COPY_PROPS,  206 + controloffw,  58 + controloff, 96, ED_BTN_H, "Copy Props");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, 308 + controloffw,  58 + controloff, 96, ED_BTN_H, "Paste Props");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 136 + controloffw, 114 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS,  164 + controloffw, 114 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS,  336 + controloffw, 114 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS,   364 + controloffw, 114 + controloff, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_GLOW_MINUS,  136 + controloffw, 172 + controloff, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_GLOW_PLUS,   164 + controloffw, 172 + controloff, 24, 24, "+");

    // sector inspector UI - texture transform ceiling
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SX_MINUS, 136 + controloffw, 228 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SX_PLUS,  164 + controloffw, 228 + controloff + sector_button_y_offsets, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SY_MINUS, 336 + controloffw, 228 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_SY_PLUS,  364 + controloffw, 228 + controloff + sector_button_y_offsets, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_MINUS, 166 + controloffw, 258 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_PLUS,  194 + controloffw, 258 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CTEX_ROT_RESET, 226 + controloffw, 258 + controloff + sector_button_y_offsets, 60, 24, "Reset");

    /* sector inspector UI - texture transform floor */
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SX_MINUS, 136 + controloffw, 316 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SX_PLUS,  164 + controloffw, 316 + controloff + sector_button_y_offsets, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SY_MINUS, 336 + controloffw, 316 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_SY_PLUS,  364 + controloffw, 316 + controloff + sector_button_y_offsets, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_MINUS, 166 + controloffw, 348 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_PLUS,  194 + controloffw, 348 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FTEX_ROT_RESET, 226 + controloffw, 348 + controloff + sector_button_y_offsets, 60, 24, "Reset");


    // sector inspect UI - sector moving parts bits ;)
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_TAG_MINUS,   136 + controloffw, 406 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_TAG_PLUS,    164 + controloffw, 406 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_STATE_MINUS, 336 + controloffw, 406 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_STATE_PLUS,  364 + controloffw, 406 + controloff + sector_button_y_offsets, 24, 24, "+");

    
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_MINUS, 136 + controloffw, 436 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_PLUS,  164 + controloffw, 436 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_MINUS, 336 + controloffw, 436 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_PLUS,  364 + controloffw, 436 + controloff + sector_button_y_offsets, 24, 24, "+");

    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_MINUS,  136 + controloffw, 466 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_PLUS,   164 + controloffw, 466 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_MINUS,  336 + controloffw, 466 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_PLUS,   364 + controloffw, 466 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_MINUS, 136 + controloffw, 496 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_PLUS,  164 + controloffw, 496 + controloff + sector_button_y_offsets, 24, 24, "+");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_MINUS,  336 + controloffw, 496 + controloff + sector_button_y_offsets, 24, 24, "-");
    rcguiCreateButton(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_PLUS,   364 + controloffw, 496 + controloff + sector_button_y_offsets, 24, 24, "+");




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
    int wallIndices[ED_MAX_WALLS];
    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

    if (wallCount <= 0) {
        return;
    }

    pushUndoState();

    for (int i = 0; i < wallCount; i++) {
        EdWall *w = &g_edMap.walls[wallIndices[i]];
        makeWallSolid(wallIndices[i], (w->midColor == 0) ? ED_COLOUR_WALL : w->midColor);
    }
}

static void doWallMakePortal(void)
{
    int wallIndices[ED_MAX_WALLS];
    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

    if (wallCount <= 0) {
        return;
    }

    pushUndoState();
    for (int i = 0; i < wallCount; i++) {
        tryMakeWallPortal(wallIndices[i]);
    }
}

static void doWallMakeWindow(void)
{
    int wallIndices[ED_MAX_WALLS];
    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

    if (wallCount <= 0) {
        return;
    }

    pushUndoState();

    for (int i = 0; i < wallCount; i++) {
        float ob, ot;
        EdWall *w = &g_edMap.walls[wallIndices[i]];

        ob = w->openBottom;
        ot = w->openTop;

        if (ot <= ob) {
            ob = 0.5f;
            ot = 1.4f;
        }

        makeWallWindow(wallIndices[i], ob, ot,
                       (w->upperColor == 0) ? ED_COLOUR_WALL : w->upperColor,
                       (w->lowerColor == 0) ? ED_COLOUR_WALL : w->lowerColor);
    }
}

static void doWallMakeDoor(void)
{
    int wallIndices[ED_MAX_WALLS];
    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

    if (wallCount <= 0) {
        return;
    }

    pushUndoState();

    for (int i = 0; i < wallCount; i++) {
        float ob, ot;
        EdWall *w = &g_edMap.walls[wallIndices[i]];

        ob = w->openBottom;
        ot = w->openTop;

        if (ot <= ob) {
            ob = 0.0f;
            ot = 1.6f;
        }

        makeWallDoor(wallIndices[i], ob, ot,
                     (w->midColor == 0) ? ED_COLOUR_WALL : w->midColor);
    }
}

static void doWallMakeTransparent(void)
{
    int wallIndices[ED_MAX_WALLS];
    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

    if (wallCount <= 0) {
        return;
    }

    pushUndoState();

    for (int i = 0; i < wallCount; i++) {
        EdWall *w = &g_edMap.walls[wallIndices[i]];

        if (w->flags & RC3D_WALL_TRANSPARENCY) {
            w->flags &= ~RC3D_WALL_TRANSPARENCY;
        } else {
            w->flags |= RC3D_WALL_TRANSPARENCY;
        }
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
    clearWallTexFlags(wShared);

    wSide0->v0 = v0;
    wSide0->v1 = newV0;
    wSide0->neighbour = -1;
    wSide0->openBottom = 0.0f;
    wSide0->openTop = 0.0f;
    wSide0->upperColor = g_ed.newWallUpperColor;
    wSide0->midColor   = g_ed.newWallMidColor;
    wSide0->lowerColor = g_ed.newWallLowerColor;
    wSide0->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    clearWallTexFlags(wSide0);

    wFar->v0 = newV0;
    wFar->v1 = newV1;
    wFar->neighbour = -1;
    wFar->openBottom = 0.0f;
    wFar->openTop = 0.0f;
    wFar->upperColor = g_ed.newWallUpperColor;
    wFar->midColor   = g_ed.newWallMidColor;
    wFar->lowerColor = g_ed.newWallLowerColor;
    wFar->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    clearWallTexFlags(wFar);

    wSide1->v0 = newV1;
    wSide1->v1 = v1;
    wSide1->neighbour = -1;
    wSide1->openBottom = 0.0f;
    wSide1->openTop = 0.0f;
    wSide1->upperColor = g_ed.newWallUpperColor;
    wSide1->midColor   = g_ed.newWallMidColor;
    wSide1->lowerColor = g_ed.newWallLowerColor;
    wSide1->flags = RC3D_WALL_SOLID | RC3D_WALL_MIDDLE;
    clearWallTexFlags(wSide1);

    initializeNewSectorDefaults(&g_edMap.sectors[newSectorIndex],
                                newWallStart,
                                4,
                                4);

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







static void refreshEditorUIButtonState(void)
{
    EdWall *w = 0;

    rcguiSetButtonText(&g_ui, GUI_BTN_GRID, g_ed.tinyGridEnabled ? "Grid 0.1" : "Grid 1.0");

    rcguiSetButtonDisabled(&g_ui, GUI_BTN_UNDO, (g_undoCount <= 0));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_REDO, (g_redoCount <= 0));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_FINISH, (g_ed.draftCount < 2));
    rcguiSetButtonDisabled(&g_ui, GUI_BTN_CLRDRAFT, (g_ed.draftCount <= 0));

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SOLID, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PORTAL, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_WINDOW, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_DOOR, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TRANSPARENCY, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_COPY_PROPS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PASTE_PROPS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_PLUS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 0);

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

    rcguiSetButtonVisible(&g_ui, GUI_BTN_CONFIRM_YES, g_ed.confirmVisible);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_CONFIRM_NO,  g_ed.confirmVisible);

    if (hasAnyWallEditSelection()) {
        const int primaryWall = getPrimaryWallEditIndex();

        if (primaryWall >= 0 && primaryWall < g_edMap.wallCount) {
            w = &g_edMap.walls[primaryWall];
        }

        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SOLID, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PORTAL, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_WINDOW, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_DOOR, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TRANSPARENCY, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_COPY_PROPS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PASTE_PROPS, 1);
        rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_COPY_PROPS, 0);
        rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_PASTE_PROPS, !g_ed.hasCopiedWallProps);

        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 1);

        if (hasSingleWallSelection() && w) {
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XL, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) ? "Clamp XL\x2" : "Clamp XL");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XR, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) ? "Clamp XR\x2" : "Clamp XR");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YT, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYT) ? "Clamp YT\x2" : "Clamp YT");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YB, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYB) ? "Clamp YB\x2" : "Clamp YB");
        } else {
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XL, "Clamp XL");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XR, "Clamp XR");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YT, "Clamp YT");
            rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YB, "Clamp YB");
        }

        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_PLUS, 1);
    }

    if (hasAnySectorEditSelection()) {
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, 1);
        rcguiSetButtonDisabled(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 0);
        rcguiSetButtonDisabled(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, !g_ed.hasCopiedSectorProps);
    }

    if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_PLUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_MINUS, 1);
        rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_PLUS, 1);

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
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_COPY_PROPS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PASTE_PROPS, 0);

    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_PLUS, 0);


    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_MINUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_PLUS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 0);
    rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, 0);

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
        if (hasAnyWallEditSelection()) {
            const int primaryWall = getPrimaryWallEditIndex();

            if (primaryWall >= 0 && primaryWall < g_edMap.wallCount) {
                w = &g_edMap.walls[primaryWall];
            }

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SOLID, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PORTAL, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_WINDOW, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_DOOR, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TRANSPARENCY, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_COPY_PROPS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_PASTE_PROPS, 1);
            rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_COPY_PROPS, 0);
            rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_PASTE_PROPS, !g_ed.hasCopiedWallProps);
            //rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_SPLIT, 1);

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XL, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_XR, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YT, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_CLAMP_YB, 1);

            if (hasSingleWallSelection() && w) {
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XL, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) ? "Clamp XL\x2" : "Clamp XL");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XR, (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) ? "Clamp XR\x2" : "Clamp XR");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YT, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYT) ? "Clamp YT\x2" : "Clamp YT");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YB, (w->tex_flags & RC3D_TEX_FLAG_CLAMPYB) ? "Clamp YB\x2" : "Clamp YB");
            } else {
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XL, "Clamp XL");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_XR, "Clamp XR");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YT, "Clamp YT");
                rcguiSetButtonText(&g_ui, GUI_BTN_WALL_CLAMP_YB, "Clamp YB");
            }

            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENBOT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_OPENTOP_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SX_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_SY_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_ROT_RESET, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_WALL_TEX_BRIGHT_PLUS, 1);

            //rcguiSetButtonDisabled(&g_ui, GUI_BTN_WALL_SPLIT, !g_ed.splitPreviewValid);
        }

        if (hasAnySectorEditSelection()) {
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, 1);
            rcguiSetButtonDisabled(&g_ui, GUI_BTN_SECTOR_COPY_PROPS, 0);
            rcguiSetButtonDisabled(&g_ui, GUI_BTN_SECTOR_PASTE_PROPS, !g_ed.hasCopiedSectorProps);
        }

        if (g_ed.selectedSector >= 0 && g_ed.selectedSector < g_edMap.sectorCount) {
            sec = &g_edMap.sectors[g_ed.selectedSector];

            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_GLOW_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_TAG_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_STATE_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MIN_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_MAX_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MIN_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_MAX_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_FLOOR_FLOW_PLUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_MINUS, 1);
            rcguiSetButtonVisible(&g_ui, GUI_BTN_SECTOR_CEIL_FLOW_PLUS, 1);
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
        case GUI_BTN_WALL_COPY_PROPS:
            if (hasAnyWallEditSelection()) {
                const int primaryWall = getPrimaryWallEditIndex();
                if (primaryWall >= 0) {
                    copyWallPropsToClipboard(primaryWall);
                }
            }
            break;
        case GUI_BTN_WALL_PASTE_PROPS:
            if (hasAnyWallEditSelection()) {
                if (g_ed.hasCopiedWallProps) {
                    int wallIndices[ED_MAX_WALLS];
                    const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                    char msg[128];

                    pushUndoState();
                    for (int i = 0; i < wallCount; i++) {
                        pasteWallPropsFromClipboardToWall(wallIndices[i]);
                    }

                    snprintf(msg, sizeof(msg), "Pasted wall properties to %d selected wall%s",
                             wallCount, (wallCount == 1) ? "" : "s");
                    setEditorStatus(msg);
                } else {
                    setEditorStatus("No copied wall properties");
                }
            }
            break;


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
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    toggleWallTexFlag(wallIndices[i], RC3D_TEX_FLAG_CLAMPXL);
                }
            }
            break;

        case GUI_BTN_WALL_CLAMP_XR:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    toggleWallTexFlag(wallIndices[i], RC3D_TEX_FLAG_CLAMPXR);
                }
            }
            break;

        case GUI_BTN_WALL_CLAMP_YT:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    toggleWallTexFlag(wallIndices[i], RC3D_TEX_FLAG_CLAMPYT);
                }
            }
            break;

        case GUI_BTN_WALL_CLAMP_YB:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    toggleWallTexFlag(wallIndices[i], RC3D_TEX_FLAG_CLAMPYB);
                }
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

        case GUI_BTN_SECTOR_GLOW_MINUS:
            if (sec) {
                pushUndoState();
                sec->glowlevel = clampLightLevel((int)sec->glowlevel - 1);
            }
            break;

        case GUI_BTN_SECTOR_GLOW_PLUS:
            if (sec) {
                pushUndoState();
                sec->glowlevel = clampLightLevel((int)sec->glowlevel + 1);
            }
            break;

        case GUI_BTN_SECTOR_TAG_MINUS:
            if (sec) {
                pushUndoState();
                if (sec->tagId > 0) sec->tagId--;
            }
            break;

        case GUI_BTN_SECTOR_TAG_PLUS:
            if (sec) {
                pushUndoState();
                sec->tagId++;
            }
            break;

        case GUI_BTN_SECTOR_STATE_MINUS:
            if (sec) {
                pushUndoState();
                if (sec->stateFlags > 0u) sec->stateFlags--;
            }
            break;

        case GUI_BTN_SECTOR_STATE_PLUS:
            if (sec) {
                pushUndoState();
                if (sec->stateFlags < 0xFFFFFFFFu) sec->stateFlags++;
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_MIN_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorMinHeight -= uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_MIN_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorMinHeight += uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_MAX_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorMaxHeight -= uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_MAX_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorMaxHeight += uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_CEIL_MIN_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilMinHeight -= uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_CEIL_MIN_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilMinHeight += uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_CEIL_MAX_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilMaxHeight -= uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_CEIL_MAX_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilMaxHeight += uiStep;
                sanitizeSectorProperties(sec);
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_FLOW_MINUS:
            if (sec) {
                pushUndoState();
                sec->floorFlowHeight -= uiStep;
            }
            break;

        case GUI_BTN_SECTOR_FLOOR_FLOW_PLUS:
            if (sec) {
                pushUndoState();
                sec->floorFlowHeight += uiStep;
            }
            break;

        case GUI_BTN_SECTOR_CEIL_FLOW_MINUS:
            if (sec) {
                pushUndoState();
                sec->ceilFlowHeight -= uiStep;
            }
            break;

        case GUI_BTN_SECTOR_CEIL_FLOW_PLUS:
            if (sec) {
                pushUndoState();
                sec->ceilFlowHeight += uiStep;
            }
            break;

        case GUI_BTN_SECTOR_COPY_PROPS:
            if (hasAnySectorEditSelection()) {
                const int primarySector = getPrimarySectorEditIndex();

                if (primarySector >= 0) {
                    copySectorPropertiesToClipboard(primarySector);
                }
            }
            break;

        case GUI_BTN_SECTOR_PASTE_PROPS:
            if (hasAnySectorEditSelection() && g_ed.hasCopiedSectorProps) {
                pushUndoState();
                if (!pasteSectorPropertiesFromClipboardToSelection()) {
                    performUndo();
                }
            }
            break;

        case GUI_BTN_WALL_OPENBOT_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *wall = &g_edMap.walls[wallIndices[i]];
                    wall->openBottom -= uiStep;
                    if (wall->openTop < wall->openBottom) {
                        float t = wall->openTop;
                        wall->openTop = wall->openBottom;
                        wall->openBottom = t;
                    }
                }
            }
            break;

        case GUI_BTN_WALL_OPENBOT_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *wall = &g_edMap.walls[wallIndices[i]];
                    wall->openBottom += uiStep;
                    if (wall->openTop < wall->openBottom) {
                        float t = wall->openTop;
                        wall->openTop = wall->openBottom;
                        wall->openBottom = t;
                    }
                }
            }
            break;

        case GUI_BTN_WALL_OPENTOP_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *wall = &g_edMap.walls[wallIndices[i]];
                    wall->openTop -= uiStep;
                    if (wall->openTop < wall->openBottom) {
                        float t = wall->openTop;
                        wall->openTop = wall->openBottom;
                        wall->openBottom = t;
                    }
                }
            }
            break;

        case GUI_BTN_WALL_OPENTOP_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *wall = &g_edMap.walls[wallIndices[i]];
                    wall->openTop += uiStep;
                    if (wall->openTop < wall->openBottom) {
                        float t = wall->openTop;
                        wall->openTop = wall->openBottom;
                        wall->openBottom = t;
                    }
                }
            }
            break;

        case GUI_BTN_WALL_TEX_SX_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexScaleX(wallIndices[i], -uiStep);
                }
            }
            break;

        case GUI_BTN_WALL_TEX_SX_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexScaleX(wallIndices[i], uiStep);
                }
            }
            break;

        case GUI_BTN_WALL_TEX_SY_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexScaleY(wallIndices[i], -uiStep);
                }
            }
            break;

        case GUI_BTN_WALL_TEX_SY_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexScaleY(wallIndices[i], uiStep);
                }
            }
            break;

        case GUI_BTN_WALL_TEX_ROT_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexAngle(wallIndices[i], -DEG2RAD(15.0f));
                }
            }
            break;

        case GUI_BTN_WALL_TEX_ROT_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexAngle(wallIndices[i], DEG2RAD(15.0f));
                }
            }
            break;

        case GUI_BTN_WALL_TEX_ROT_RESET:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    setWallTexAngleEx(wallIndices[i], DEG2RAD(0));
                }
            }
            break;

        case GUI_BTN_WALL_TEX_BRIGHT_MINUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexBrightness(wallIndices[i], -1);
                }
            }
            break;

        case GUI_BTN_WALL_TEX_BRIGHT_PLUS:
            if (hasAnyWallEditSelection()) {
                int wallIndices[ED_MAX_WALLS];
                const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    adjustWallTexBrightness(wallIndices[i], 1);
                }
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
        (g_ed.selectionType == ED_SEL_SECTOR) ? "SECTOR" :
        hasMultiSectorSelection() ? "MULTI SECTOR" :
        hasMultiWallSelection() ? "MULTI WALL" :
        (g_ed.selectedVertCount > 0) ? "MULTI VERT" : "NONE");
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
        const unsigned wallTexBrightness = (unsigned)getWallTexBrightness(w);

        py = 40;

        drawRect(px + 8, py, pw - 16, 430, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 430, ED_INSPECTOR_PARENT_PANELS_FRAME);
        py += 6;

        snprintf(buf, sizeof(buf), "WALL %d", g_ed.selectedWall);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Verts: %d -> %d", w->v0, w->v1);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        snprintf(buf, sizeof(buf), "Neighbour: %d", w->neighbour);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        if (g_ed.hasCopiedWallProps) {
            if (g_ed.copiedWallPropsSourceWall >= 0) {
                snprintf(buf, sizeof(buf), "Props clip: Wall %d", g_ed.copiedWallPropsSourceWall);
            } else {
                snprintf(buf, sizeof(buf), "Props clip: Ready");
            }
        } else {
            snprintf(buf, sizeof(buf), "Props clip: Empty");
        }
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
        py += 5;

        snprintf(buf, sizeof(buf), "Scale X: %.3f           Scale Y: %.3f", w->texScaleX, w->texScaleY);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;
        py += 1;

        snprintf(buf, sizeof(buf), "Tex angle: %.1f\xb0", wallTexAngleDeg);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;
        py += 1;

        snprintf(buf, sizeof(buf), "Brightness: %u / 7", wallTexBrightness);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

    }
    else if (hasMultiWallSelection()) {
        int wallIndices[ED_MAX_WALLS];
        const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
        const int primaryWall = (wallCount > 0) ? wallIndices[0] : -1;
        const EdWall *w = (primaryWall >= 0 && primaryWall < g_edMap.wallCount)
                        ? &g_edMap.walls[primaryWall]
                        : NULL;
        int neighbour = 0;
        int neighbourMixed = 0;
        float openTop = 0.0f;
        int openTopMixed = 0;
        float openBottom = 0.0f;
        int openBottomMixed = 0;
        unsigned wallFlags = 0;
        int wallFlagsMixed = 0;
        unsigned upperColor = 0;
        int upperColorMixed = 0;
        unsigned midColor = 0;
        int midColorMixed = 0;
        unsigned lowerColor = 0;
        int lowerColorMixed = 0;
        unsigned texFlags = 0;
        int texFlagsMixed = 0;
        int clampXL = 0;
        int clampXLMixed = 0;
        int clampXR = 0;
        int clampXRMixed = 0;
        int clampYT = 0;
        int clampYTMixed = 0;
        int clampYB = 0;
        int clampYBMixed = 0;
        float texScaleX = 0.0f;
        int texScaleXMixed = 0;
        float texScaleY = 0.0f;
        int texScaleYMixed = 0;
        float wallTexAngleDeg = 0.0f;
        int wallTexAngleMixed = 0;
        unsigned wallTexBrightness = 0;
        int wallTexBrightnessMixed = 0;
        char upperBuf[16];
        char midBuf[16];
        char lowerBuf[16];
        const char *clampXLText = " ";
        const char *clampXRText = " ";
        const char *clampYTText = " ";
        const char *clampYBText = " ";

        if (w) {
            neighbour = w->neighbour;
            openTop = w->openTop;
            openBottom = w->openBottom;
            wallFlags = w->flags;
            upperColor = w->upperColor;
            midColor = w->midColor;
            lowerColor = w->lowerColor;
            texFlags = w->tex_flags;
            clampXL = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXL) != 0;
            clampXR = (w->tex_flags & RC3D_TEX_FLAG_CLAMPXR) != 0;
            clampYT = (w->tex_flags & RC3D_TEX_FLAG_CLAMPYT) != 0;
            clampYB = (w->tex_flags & RC3D_TEX_FLAG_CLAMPYB) != 0;
            texScaleX = w->texScaleX;
            texScaleY = w->texScaleY;
            wallTexAngleDeg = RAD2DEG(getWallTexAngle(w));
            wallTexBrightness = (unsigned)getWallTexBrightness(w);

            for (int i = 1; i < wallCount; i++) {
                const EdWall *other = &g_edMap.walls[wallIndices[i]];
                const int otherClampXL = (other->tex_flags & RC3D_TEX_FLAG_CLAMPXL) != 0;
                const int otherClampXR = (other->tex_flags & RC3D_TEX_FLAG_CLAMPXR) != 0;
                const int otherClampYT = (other->tex_flags & RC3D_TEX_FLAG_CLAMPYT) != 0;
                const int otherClampYB = (other->tex_flags & RC3D_TEX_FLAG_CLAMPYB) != 0;
                const float otherTexAngleDeg = RAD2DEG(getWallTexAngle(other));

                if (other->neighbour != neighbour) neighbourMixed = 1;
                if (fabsf(other->openTop - openTop) > ED_EPSILON) openTopMixed = 1;
                if (fabsf(other->openBottom - openBottom) > ED_EPSILON) openBottomMixed = 1;
                if (other->flags != wallFlags) wallFlagsMixed = 1;
                if (other->upperColor != upperColor) upperColorMixed = 1;
                if (other->midColor != midColor) midColorMixed = 1;
                if (other->lowerColor != lowerColor) lowerColorMixed = 1;
                if (other->tex_flags != texFlags) texFlagsMixed = 1;
                if (otherClampXL != clampXL) clampXLMixed = 1;
                if (otherClampXR != clampXR) clampXRMixed = 1;
                if (otherClampYT != clampYT) clampYTMixed = 1;
                if (otherClampYB != clampYB) clampYBMixed = 1;
                if (fabsf(other->texScaleX - texScaleX) > ED_EPSILON) texScaleXMixed = 1;
                if (fabsf(other->texScaleY - texScaleY) > ED_EPSILON) texScaleYMixed = 1;
                if (fabsf(otherTexAngleDeg - wallTexAngleDeg) > 0.05f) wallTexAngleMixed = 1;
                if ((unsigned)getWallTexBrightness(other) != wallTexBrightness) wallTexBrightnessMixed = 1;
            }
        }

        if (upperColorMixed) snprintf(upperBuf, sizeof(upperBuf), "Mixed");
        else snprintf(upperBuf, sizeof(upperBuf), "%u", upperColor);

        if (midColorMixed) snprintf(midBuf, sizeof(midBuf), "Mixed");
        else snprintf(midBuf, sizeof(midBuf), "%u", midColor);

        if (lowerColorMixed) snprintf(lowerBuf, sizeof(lowerBuf), "Mixed");
        else snprintf(lowerBuf, sizeof(lowerBuf), "%u", lowerColor);

        clampXLText = clampXLMixed ? "Mix" : (clampXL ? "\x2" : " ");
        clampXRText = clampXRMixed ? "Mix" : (clampXR ? "\x2" : " ");
        clampYTText = clampYTMixed ? "Mix" : (clampYT ? "\x2" : " ");
        clampYBText = clampYBMixed ? "Mix" : (clampYB ? "\x2" : " ");

        py = 40;

        drawRect(px + 8, py, pw - 16, 438, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 438, ED_INSPECTOR_PARENT_PANELS_FRAME);
        py += 6;

        snprintf(buf, sizeof(buf), "WALLS (%d)", wallCount);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "First wall: %d", primaryWall);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        if (g_ed.hasCopiedWallProps) {
            if (g_ed.copiedWallPropsSourceWall >= 0) {
                snprintf(buf, sizeof(buf), "Props clip: Wall %d", g_ed.copiedWallPropsSourceWall);
            } else {
                snprintf(buf, sizeof(buf), "Props clip: Ready");
            }
        } else {
            snprintf(buf, sizeof(buf), "Props clip: Empty");
        }
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        if (neighbourMixed) snprintf(buf, sizeof(buf), "Neighbour: Mixed");
        else snprintf(buf, sizeof(buf), "Neighbour: %d", neighbour);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;
        py += 8;

        if (openTopMixed) snprintf(buf, sizeof(buf), "Open Top: Mixed");
        else snprintf(buf, sizeof(buf), "Open Top: %.3f", openTop);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        if (openBottomMixed) snprintf(buf, sizeof(buf), "Open Bottom: Mixed");
        else snprintf(buf, sizeof(buf), "Open Bottom: %.3f", openBottom);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        if (wallFlagsMixed) snprintf(buf, sizeof(buf), "Flags: Mixed");
        else snprintf(buf, sizeof(buf), "Flags: %u", wallFlags);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        snprintf(buf, sizeof(buf), "Upper: %s   Mid: %s   Lower: %s", upperBuf, midBuf, lowerBuf);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        py += 30;

        if (texFlagsMixed) snprintf(buf, sizeof(buf), "Tex flags: Mixed");
        else snprintf(buf, sizeof(buf), "Tex flags: 0x%08X", texFlags);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Clamp X  L[%s] R[%s]", clampXLText, clampXRText);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "Clamp Y  T[%s] B[%s]", clampYTText, clampYBText);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        if (texScaleXMixed && texScaleYMixed) {
            snprintf(buf, sizeof(buf), "Scale X: Mixed          Scale Y: Mixed");
        } else if (texScaleXMixed) {
            snprintf(buf, sizeof(buf), "Scale X: Mixed          Scale Y: %.3f", texScaleY);
        } else if (texScaleYMixed) {
            snprintf(buf, sizeof(buf), "Scale X: %.3f           Scale Y: Mixed", texScaleX);
            snprintf(buf, sizeof(buf), "Scale X: %.3f           Scale Y: %.3f", w->texScaleX, w->texScaleY);
        } else {
            snprintf(buf, sizeof(buf), "Scale X: %.3f           Scale Y: %.3f", texScaleX, texScaleY);
        }
        py += 5;
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;
        py += 1;

        if (wallTexAngleMixed) snprintf(buf, sizeof(buf), "Tex angle: Mixed");
        else snprintf(buf, sizeof(buf), "Tex angle: %.1f\xb0", wallTexAngleDeg);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 30;

        py += 1;

        if (wallTexBrightnessMixed) snprintf(buf, sizeof(buf), "Brightness: Mixed");
        else snprintf(buf, sizeof(buf), "Brightness: %u / 7", wallTexBrightness);
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

    else if (hasMultiSectorSelection()) {
        const int primarySector = getPrimarySectorEditIndex();
        const int clipSectorCount = g_ed.hasCopiedSectorGeometry ? g_ed.copiedSectorGeometry.sectorCount : 0;

        py = 40;

        drawRect(px + 8, py, pw - 16, 180, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 180, ED_INSPECTOR_PARENT_PANELS_FRAME);
        py += 8;

        snprintf(buf, sizeof(buf), "SECTORS (%d)", g_ed.selectedSectorCount);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL); py += 24;

        snprintf(buf, sizeof(buf), "Primary sector: %d", primarySector);
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        if (g_ed.hasCopiedSectorGeometry) {
            snprintf(buf, sizeof(buf), "Geom clip: %d sector%s",
                     clipSectorCount, (clipSectorCount == 1) ? "" : "s");
        } else {
            snprintf(buf, sizeof(buf), "Geom clip: Empty");
        }
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        if (g_ed.hasCopiedSectorProps) {
            if (g_ed.copiedSectorPropsSourceSector >= 0) {
                snprintf(buf, sizeof(buf), "Props clip: Sector %d", g_ed.copiedSectorPropsSourceSector);
            } else {
                snprintf(buf, sizeof(buf), "Props clip: Ready");
            }
        } else {
            snprintf(buf, sizeof(buf), "Props clip: Empty");
        }
        drawText(px + 16, py, buf, ED_TEXT_COL); py += 20;

        drawText(px + 16, py, "CTRL+Click adds/removes sectors", ED_TEXT_COL); py += 20;
        drawText(px + 16, py, "CTRL+C copies the full group", ED_TEXT_COL); py += 20;
        drawText(px + 16, py, "Paste Props and the texture browser hit the full selection", ED_TEXT_COL); py += 20;
        drawText(px + 16, py, "CTRL+V pastes to a snapped group anchor", ED_TEXT_COL); py += 20;
    }
    /// SELECTED SECTORS ///////////////////////////////////////////////////////
    
    else if (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector >= 0) {
        const EdSector *sec = &g_edMap.sectors[g_ed.selectedSector];
        py = 40;

        drawRect(px + 8, py, pw - 16, 580, ED_INSPECTOR_PARENT_PANELS_BG);
        drawRectL(px + 8, py, pw - 16, 580, ED_INSPECTOR_PARENT_PANELS_FRAME);

        py += 8;

        snprintf(buf, sizeof(buf), "SECTOR %d", g_ed.selectedSector);
        drawText(px + 16, py, buf, ED_INSPECTOR_TEXT_COL);  py += 20;
        py += 4;

        snprintf(buf, sizeof(buf), "Walls: start %d   count %d   boundary %d",
                 sec->wallStart, sec->wallCount, sec->boundaryCount);
        drawText(px + 16, py, buf, ED_TEXT_COL);  py += 20;

        if (g_ed.hasCopiedSectorProps) {
            if (g_ed.copiedSectorPropsSourceSector >= 0) {
                snprintf(buf, sizeof(buf), "Props clip: Sector %d", g_ed.copiedSectorPropsSourceSector);
            } else {
                snprintf(buf, sizeof(buf), "Props clip: Ready");
            }
        } else {
            snprintf(buf, sizeof(buf), "Props clip: Empty");
        }
        drawText(px + 16, py, buf, ED_TEXT_COL);  py += 20;

        /*
        if (g_ed.hasCopiedSectorGeometry) {
            snprintf(buf, sizeof(buf), "Geom clip: %d sector%s",
                     g_ed.copiedSectorGeometry.sectorCount,
                     (g_ed.copiedSectorGeometry.sectorCount == 1) ? "" : "s");
        } else {
            snprintf(buf, sizeof(buf), "Geom clip: Empty");
        }
        drawText(px + 16, py, buf, ED_TEXT_COL);  py += 20;
        */
        py += 8;

        // heights ////////////////////////////////
        drawRect(px + 12,  py, pw - 24, 58, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 58, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "HEIGHTS", ED_INSPECTOR_PANELS_HEADER_TEXT);      py += 20;

        py += 6;
        snprintf(buf, sizeof(buf), "Floor: %.3f", sec->floorHeight);
        drawText(px + 18,  py, buf, ED_TEXT_COL);   

        snprintf(buf, sizeof(buf), "Ceiling : %.3f", sec->ceilHeight);
        drawText(px + 218,  py, buf, ED_TEXT_COL);   py += 30;

        py += 2;
        drawRect(px + 12,  py, pw - 24, 48, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 48, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "LIGHTING", ED_INSPECTOR_PANELS_HEADER_TEXT);     py += 20;

        snprintf(buf, sizeof(buf), "Glow: %u / 7", (unsigned)clampLightLevel((int)sec->glowlevel));
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 22;

        py += 4;
        // TEXTURES ///////////////////////////////
        
        drawRect(px + 12,  py, pw - 24, 48, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 48, ED_INSPECTOR_PANELS_PANELFRAME);
        py += 6;
        drawText(px + 18,  py, "TEXTURES", ED_INSPECTOR_PANELS_HEADER_TEXT);     py += 20;

        snprintf(buf, sizeof(buf), "Floor tex: %u   Ceil tex: %u",
                 (unsigned)sec->floorColor, (unsigned)sec->ceilColor);
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 20;

        // CEILING Texture UVs ////////////////////////////
        py += 6;
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
        drawText(px + 18,  py, buf, ED_TEXT_COL);  py += 20;


        // FLOOR Texture UVs //////////////////////
        py += 8;
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


        // Sector mover!!
        py += 8;
        drawRect(px + 12,  py, pw - 24, 148, ED_INSPECTOR_PANELS_BACKPANEL);
        drawRectL(px + 12, py, pw - 24, 148, ED_INSPECTOR_PANELS_PANELFRAME);

        
        py += 6;
        drawText(px + 18,  py, "SECTOR MOVER", ED_INSPECTOR_PANELS_HEADER_TEXT); py += 20;

        py += 6;
        snprintf(buf, sizeof(buf), "Tag ID: %d", sec->tagId);
        drawText(px + 18, py, buf, ED_TEXT_COL);
        snprintf(buf, sizeof(buf), "State: 0x%04X", (unsigned)sec->stateFlags);
        drawText(px + 218, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "F_min: %.3f", sec->floorMinHeight);
        drawText(px + 18, py, buf, ED_TEXT_COL);
        snprintf(buf, sizeof(buf), "F_max: %.3f", sec->floorMaxHeight);
        drawText(px + 218, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "C_min: %.3f", sec->ceilMinHeight);
        drawText(px + 18, py, buf, ED_TEXT_COL);
        snprintf(buf, sizeof(buf), "C_max: %.3f", sec->ceilMaxHeight);
        drawText(px + 218, py, buf, ED_TEXT_COL); py += 30;

        snprintf(buf, sizeof(buf), "F_flow: %.3f", sec->floorFlowHeight);
        drawText(px + 18, py, buf, ED_TEXT_COL);
        snprintf(buf, sizeof(buf), "C_flow: %.3f", sec->ceilFlowHeight);
        drawText(px + 218, py, buf, ED_TEXT_COL); py += 24;

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

    if (keyPressedOnce(keys, SDL_SCANCODE_KP_MULTIPLY) ||
        (g_ed.isometricView && keyPressedOnce(keys, SDL_SCANCODE_ESCAPE))) {
        g_ed.isometricView = 1 - g_ed.isometricView;
        g_ed.draggingPan = 0;
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
        g_ed.draggingMultiVertex = 0;
        g_ed.boxSelecting = 0;
        g_ed.textureScrollbarDragging = 0;
        g_ed.hoverVert = -1;
        g_ed.hoverWall = -1;
        g_ed.hoverSector = -1;
        clearPendingLeftMouseAction();

        if (g_ed.isometricView) {
            setEditorStatus("Isometric preview enabled (* toggles back, view-only)");
        } else {
            setEditorStatus("Isometric preview disabled");
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
            float beforeX;
            float beforeY;

            if (g_ed.isometricView) {
                screenToIsoWorldOnPlane(mouseX, mouseY, 0.0f, &beforeX, &beforeY);
            } else {
                beforeX = worldX;
                beforeY = worldY;
            }

            g_ed.zoom *= (mouseWheelY > 0) ? 1.15f : (1.0f / 1.15f);
            g_ed.zoom = clampf_local(g_ed.zoom, 4.0f, 512.0f);

            {
                float afterX, afterY;
                if (g_ed.isometricView) {
                    screenToIsoWorldOnPlane(mouseX, mouseY, 0.0f, &afterX, &afterY);
                } else {
                    screenToWorld(mouseX, mouseY, &afterX, &afterY);
                }
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
        if (g_ed.isometricView) {
            panIsoCameraByScreenDelta(dx, dy);
        } else {
            g_ed.camX -= (float)dx / g_ed.zoom;
            g_ed.camY -= (float)dy / g_ed.zoom;
        }
    }

    if (g_ed.isometricView) {
        g_ed.hoverVert = -1;
        g_ed.hoverWall = -1;
        g_ed.hoverSector = -1;
        g_ed.splitPreviewValid = 0;
        g_ed.uiMouseCaptured = 0;
        finishEditorInputFrame(keys, leftDown, rightDown, middleDown, mouseX, mouseY);
        return;
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
        const int shiftDown = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 1 : 0;
        const int ctrlDown = (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) ? 1 : 0;
        const int altDown = (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]) ? 1 : 0;

        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
        g_ed.draggingMultiVertex = 0;
        g_ed.boxSelecting = 0;
        clearPendingLeftMouseAction();

        g_ed.pendingLeftMouseDown = 1;
        g_ed.pendingLeftMouseX = mouseX;
        g_ed.pendingLeftMouseY = mouseY;
        g_ed.pendingLeftWorldX = worldX;
        g_ed.pendingLeftWorldY = worldY;
        g_ed.pendingLeftCtrlDown = ctrlDown;
        g_ed.pendingLeftAltDown = altDown;
        g_ed.pendingLeftBoxSelectWalls = 0;

        if (ctrlDown) {
            if (g_ed.hoverSector >= 0) {
                g_ed.pendingLeftTargetIndex = g_ed.hoverSector;
                g_ed.pendingLeftAction =
                    (g_ed.selectionType == ED_SEL_SECTOR && g_ed.selectedSector == g_ed.hoverSector)
                        ? ED_PENDING_LEFT_SECTOR_DRAG
                        : ED_PENDING_LEFT_SECTOR_CLICK_OR_BOX;
            } else {
                g_ed.pendingLeftAction = ED_PENDING_LEFT_EMPTY_CLICK;
            }
        }
        else if (altDown) {
            g_ed.pendingLeftBoxSelectWalls = 1;

            if (g_ed.hoverWall >= 0) {
                g_ed.pendingLeftTargetIndex = g_ed.hoverWall;
                g_ed.pendingLeftAction =
                    (g_ed.selectionType == ED_SEL_WALL && g_ed.selectedWall == g_ed.hoverWall)
                        ? ED_PENDING_LEFT_WALL_DRAG
                        : ED_PENDING_LEFT_WALL_CLICK_OR_BOX;
            } else {
                g_ed.pendingLeftAction = ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX;
            }
        }
        else if (shiftDown) {
            if (g_ed.selectionType == ED_SEL_NONE && g_ed.selectedVertCount > 0 &&
                findSelectedVertexNearMouse(mouseX, mouseY) >= 0) {
                g_ed.pendingLeftAction = ED_PENDING_LEFT_MULTI_DRAG;
            }
            else if (g_ed.hoverVert >= 0) {
                g_ed.pendingLeftTargetIndex = g_ed.hoverVert;
                g_ed.pendingLeftAction = ED_PENDING_LEFT_VERTEX;
            }
            else {
                g_ed.pendingLeftAction = ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX;
            }
        }
        else {
            g_ed.pendingLeftAction = ED_PENDING_LEFT_EMPTY_CLICK_OR_BOX;
        }
    }

    if (leftDown && g_ed.pendingLeftMouseDown && pendingLeftExceededDragTolerance(mouseX, mouseY)) {
        promotePendingLeftMouseAction(mouseX, mouseY);
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

    /* drag selected wall(s) */
    if (leftDown && g_ed.draggingWall) {
        dragSelectedWallTo(worldX, worldY);
        updateHover(worldX, worldY, mouseX, mouseY);
    }

    

    if (leftDown && g_ed.draggingSector) {
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
        if (g_ed.pendingLeftMouseDown) {
            commitPendingLeftMouseClick();
        }
        else if (g_ed.boxSelecting) {
            finalizeBoxSelect();
        }
    }

    if (!leftDown) {
        g_ed.draggingVertex = 0;
        g_ed.draggingWall = 0;
        g_ed.draggingSector = 0;
        g_ed.draggingMultiVertex = 0;
        g_ed.textureScrollbarDragging = 0;
        if (g_ed.pendingLeftMouseDown) {
            clearPendingLeftMouseAction();
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_DELETE)) {
        if (g_ed.selectedVert >= 0) {
            pushUndoState();
            deleteVertexByIndex(g_ed.selectedVert);
        } else if (hasMultiWallSelection()) {
            int wallIndices[ED_MAX_WALLS];
            const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);

            if (wallCount > 0) {
                pushUndoState();
                for (int i = wallCount - 1; i >= 0; i--) {
                    deleteWallByIndex(wallIndices[i]);
                }
            }
        } else if (g_ed.selectedWall >= 0) {
            pushUndoState();
            deleteWallByIndex(g_ed.selectedWall);
        } else if (hasMultiSectorSelection()) {
            int sectorIndices[ED_MAX_SECTORS];
            const int sectorCount = collectSectorEditSelectionIndices(sectorIndices, ED_MAX_SECTORS);

            if (sectorCount > 0) {
                pushUndoState();
                for (int i = sectorCount - 1; i >= 0; i--) {
                    deleteSectorByIndex(sectorIndices[i]);
                }
            }
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

    if (keyPressedOnce(keys, SDL_SCANCODE_APOSTROPHE)) {
        g_ed.bUseVectorFill = 1 - g_ed.bUseVectorFill;
        if (g_ed.bUseVectorFill) {
            g_ed.bUseTextureFill = 0;
            setEditorStatus("Vector sector fill enabled");
        } else {
            setEditorStatus("Vector sector fill disabled");
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_SEMICOLON) || ((keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) && keyPressedOnce(keys, SDL_SCANCODE_3))) {
        g_ed.bUseTextureFill = 1 - g_ed.bUseTextureFill;
        if (g_ed.bUseTextureFill) {
            g_ed.bUseVectorFill = 0;
            setEditorStatus("Floor texture fill enabled");
        } else {
            setEditorStatus("Floor texture fill disabled");
        }
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
        if (hasAnyActiveSelection()) {
            cancelActiveSelection();
        } else {
            executeEditorAction(ED_ACT_CLEAR_DRAFT, worldX, worldY);
        }
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) && keyPressedOnce(keys, SDL_SCANCODE_Z)) {
        executeEditorAction(ED_ACT_UNDO, worldX, worldY);
    }

    if ((keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) && keyPressedOnce(keys, SDL_SCANCODE_Y)) {
        executeEditorAction(ED_ACT_REDO, worldX, worldY);
    }

    if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_C)) {
        if (hasAnySectorEditSelection()) {
            copySelectedSectorGeometryToClipboard();
        } else {
            setEditorStatus("Select at least one sector to copy");
        }
    }

    if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_V)) {
        if (g_ed.hasCopiedSectorGeometry) {
            pushUndoState();
            if (!pasteSectorGeometryFromClipboard(worldX, worldY)) {
                performUndo();
            } else {
                updateHover(worldX, worldY, mouseX, mouseY);
            }
        } else {
            setEditorStatus("No copied sector geometry");
        }
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_TAB)) {
        executeEditorAction(ED_ACT_TOGGLE_GRID, worldX, worldY);
    }

    if (keyPressedOnce(keys, SDL_SCANCODE_RETURN)) {
        executeEditorAction(ED_ACT_FINISH_DRAFT, worldX, worldY);
    }

    /* sector defaults for NEW drafted sectors only when nothing is selected */
    if (!hasAnyWallEditSelection() &&
        !hasMultiSectorSelection() &&
        g_ed.selectedSector < 0 &&
        g_ed.selectionType != ED_SEL_VERTEX) {
        if (keyPressedOnce(keys, SDL_SCANCODE_F)) g_ed.sectorFloor -= 0.1f;
        if (keyPressedOnce(keys, SDL_SCANCODE_G)) g_ed.sectorFloor += 0.1f;
        if (!ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_C)) g_ed.sectorCeil  -= 0.1f;
        if (!ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_V)) g_ed.sectorCeil  += 0.1f;

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
        const int shiftDown = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 1 : 0;
        const int copyFloorHeightPressed = !shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_1) || keyPressedOnce(keys, SDL_SCANCODE_KP_1));
        const int copyCeilHeightPressed = !shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_2) || keyPressedOnce(keys, SDL_SCANCODE_KP_2));
        const int pasteFloorHeightPressed = shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_1) || keyPressedOnce(keys, SDL_SCANCODE_KP_1));
        const int pasteCeilHeightPressed = shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_2) || keyPressedOnce(keys, SDL_SCANCODE_KP_2));
        const int copyFloorTexPressed = !shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_4) || keyPressedOnce(keys, SDL_SCANCODE_KP_4));
        const int copyCeilTexPressed = !shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_5) || keyPressedOnce(keys, SDL_SCANCODE_KP_5));
        const int pasteFloorTexPressed = shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_4) || keyPressedOnce(keys, SDL_SCANCODE_KP_4));
        const int pasteCeilTexPressed = shiftDown &&
            (keyPressedOnce(keys, SDL_SCANCODE_5) || keyPressedOnce(keys, SDL_SCANCODE_KP_5));

        if (copyFloorHeightPressed) {
            char msg[128];
            g_ed.copiedSectorFloor = sec->floorHeight;
            g_ed.hasCopiedSectorFloor = 1;
            snprintf(msg, sizeof(msg), "Copied sector floor height: %.2f", sec->floorHeight);
            setEditorStatus(msg);
        }

        if (copyCeilHeightPressed) {
            char msg[128];
            g_ed.copiedSectorCeil = sec->ceilHeight;
            g_ed.hasCopiedSectorCeil = 1;
            snprintf(msg, sizeof(msg), "Copied sector ceiling height: %.2f", sec->ceilHeight);
            setEditorStatus(msg);
        }

        if (pasteFloorHeightPressed) {
            if (g_ed.hasCopiedSectorFloor) {
                char msg[128];
                pushUndoState();
                sec->floorHeight = g_ed.copiedSectorFloor;
                changed = 1;
                snprintf(msg, sizeof(msg), "Pasted sector floor height: %.2f", sec->floorHeight);
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied sector floor height");
            }
        }

        if (pasteCeilHeightPressed) {
            if (g_ed.hasCopiedSectorCeil) {
                char msg[128];
                pushUndoState();
                sec->ceilHeight = g_ed.copiedSectorCeil;
                changed = 1;
                snprintf(msg, sizeof(msg), "Pasted sector ceiling height: %.2f", sec->ceilHeight);
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied sector ceiling height");
            }
        }

        if (copyFloorTexPressed) {
            char msg[128];
            g_ed.copiedSectorFloorColor = sec->floorColor;
            g_ed.hasCopiedSectorFloorColor = 1;
            snprintf(msg, sizeof(msg), "Copied sector floor texture id: %u", (unsigned)sec->floorColor);
            setEditorStatus(msg);
        }

        if (copyCeilTexPressed) {
            char msg[128];
            g_ed.copiedSectorCeilColor = sec->ceilColor;
            g_ed.hasCopiedSectorCeilColor = 1;
            snprintf(msg, sizeof(msg), "Copied sector ceiling texture id: %u", (unsigned)sec->ceilColor);
            setEditorStatus(msg);
        }

        if (pasteFloorTexPressed) {
            if (g_ed.hasCopiedSectorFloorColor) {
                char msg[128];
                pushUndoState();
                sec->floorColor = g_ed.copiedSectorFloorColor;
                changed = 1;
                snprintf(msg, sizeof(msg), "Pasted sector floor texture id: %u", (unsigned)sec->floorColor);
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied sector floor texture id");
            }
        }

        if (pasteCeilTexPressed) {
            if (g_ed.hasCopiedSectorCeilColor) {
                char msg[128];
                pushUndoState();
                sec->ceilColor = g_ed.copiedSectorCeilColor;
                changed = 1;
                snprintf(msg, sizeof(msg), "Pasted sector ceiling texture id: %u", (unsigned)sec->ceilColor);
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied sector ceiling texture id");
            }
        }

        if (shiftDown) {
            if (keyPressedOnce(keys, SDL_SCANCODE_F)) { pushUndoState(); sec->floorHeight -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_G)) { pushUndoState(); sec->floorHeight += 0.1f; changed = 1; }
            if (!ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_C)) { pushUndoState(); sec->ceilHeight  -= 0.1f; changed = 1; }
            if (!ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_V)) { pushUndoState(); sec->ceilHeight  += 0.1f; changed = 1; }

            if (keyPressedOnce(keys, SDL_SCANCODE_J)) { pushUndoState(); sec->floorColor--; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_K)) { pushUndoState(); sec->floorColor++; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_N)) { pushUndoState(); sec->ceilColor--;  changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_M)) { pushUndoState(); sec->ceilColor++;  changed = 1; }
        } else {
            if (keyPressedOnce(keys, SDL_SCANCODE_Q)) { pushUndoState(); sec->ceilTexScaleX -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_W)) { pushUndoState(); sec->ceilTexScaleX += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_E)) { pushUndoState(); sec->ceilTexScaleY -= 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_R)) { pushUndoState(); sec->ceilTexScaleY += 0.1f; changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_T)) { pushUndoState(); sec->ceilTexAngle  -= DEG2RAD(15.0f); changed = 1; }
            if (keyPressedOnce(keys, SDL_SCANCODE_Y)) { pushUndoState(); sec->ceilTexAngle  += DEG2RAD(15.0f); changed = 1; }
        }

        if (changed) {
            sanitizeSectorProperties(sec);
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
    if (hasAnyWallEditSelection()) {
        int wallIndices[ED_MAX_WALLS];
        const int wallCount = collectWallEditSelectionIndices(wallIndices, ED_MAX_WALLS);
        const int shiftDown = (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 1 : 0;
        const int copyWallTexturePressed = !ctrlDown && !shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_1);
        const int copyWallScalePressed = !ctrlDown && !shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_2);
        const int copyWallRotationPressed = !ctrlDown && !shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_3);
        const int pasteWallTexturePressed = !ctrlDown && shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_1);
        const int pasteWallScalePressed = !ctrlDown && shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_2);
        const int pasteWallRotationPressed = !ctrlDown && shiftDown &&
            keyPressedOnce(keys, SDL_SCANCODE_KP_3);

        if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_1)) executeEditorAction(ED_ACT_WALL_SOLID, worldX, worldY);
        if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_2)) executeEditorAction(ED_ACT_WALL_PORTAL, worldX, worldY);
        if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_3)) executeEditorAction(ED_ACT_WALL_WINDOW, worldX, worldY);
        if (ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_4)) executeEditorAction(ED_ACT_WALL_DOOR, worldX, worldY);
        if (hasSingleWallSelection() && ctrlDown && keyPressedOnce(keys, SDL_SCANCODE_5)) {
            executeEditorAction(ED_ACT_WALL_EXTRUDE, worldX, worldY);
        }

        if (copyWallTexturePressed) {
            const int primaryWall = getPrimaryWallEditIndex();
            char msg[128];
            if (primaryWall >= 0) {
                const EdWall *src = &g_edMap.walls[primaryWall];
                g_ed.copiedWallUpperColor = src->upperColor;
                g_ed.copiedWallMidColor = src->midColor;
                g_ed.copiedWallLowerColor = src->lowerColor;
                g_ed.hasCopiedWallTexture = 1;
                snprintf(msg, sizeof(msg), "Copied wall textures from wall %d", primaryWall);
                setEditorStatus(msg);
            }
        }

        if (copyWallScalePressed) {
            const int primaryWall = getPrimaryWallEditIndex();
            char msg[128];
            if (primaryWall >= 0) {
                const EdWall *src = &g_edMap.walls[primaryWall];
                g_ed.copiedWallTexScaleX = src->texScaleX;
                g_ed.copiedWallTexScaleY = src->texScaleY;
                g_ed.hasCopiedWallScale = 1;
                snprintf(msg, sizeof(msg), "Copied wall scale from wall %d", primaryWall);
                setEditorStatus(msg);
            }
        }

        if (copyWallRotationPressed) {
            const int primaryWall = getPrimaryWallEditIndex();
            char msg[128];
            if (primaryWall >= 0) {
                const EdWall *src = &g_edMap.walls[primaryWall];
                g_ed.copiedWallTexAngle = getWallTexAngle(src);
                g_ed.hasCopiedWallRotation = 1;
                snprintf(msg, sizeof(msg), "Copied wall rotation from wall %d", primaryWall);
                setEditorStatus(msg);
            }
        }

        if (pasteWallTexturePressed) {
            if (g_ed.hasCopiedWallTexture) {
                char msg[128];
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *w = &g_edMap.walls[wallIndices[i]];
                    w->upperColor = g_ed.copiedWallUpperColor;
                    w->midColor = g_ed.copiedWallMidColor;
                    w->lowerColor = g_ed.copiedWallLowerColor;
                }
                snprintf(msg, sizeof(msg), "Pasted wall textures to %d selected wall%s",
                         wallCount, (wallCount == 1) ? "" : "s");
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied wall textures");
            }
        }

        if (pasteWallScalePressed) {
            if (g_ed.hasCopiedWallScale) {
                char msg[128];
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    EdWall *w = &g_edMap.walls[wallIndices[i]];
                    setWallTexScaleX(w, g_ed.copiedWallTexScaleX);
                    setWallTexScaleY(w, g_ed.copiedWallTexScaleY);
                }
                snprintf(msg, sizeof(msg), "Pasted wall scale to %d selected wall%s",
                         wallCount, (wallCount == 1) ? "" : "s");
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied wall scale");
            }
        }

        if (pasteWallRotationPressed) {
            if (g_ed.hasCopiedWallRotation) {
                char msg[128];
                pushUndoState();
                for (int i = 0; i < wallCount; i++) {
                    setWallTexAngleEx(wallIndices[i], g_ed.copiedWallTexAngle);
                }
                snprintf(msg, sizeof(msg), "Pasted wall rotation to %d selected wall%s",
                         wallCount, (wallCount == 1) ? "" : "s");
                setEditorStatus(msg);
            } else {
                setEditorStatus("No copied wall rotation");
            }
        }

        for (int i = 0; i < wallCount; i++) {
            EdWall *w = &g_edMap.walls[wallIndices[i]];

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
        }

        if (keyPressedOnce(keys, SDL_SCANCODE_Q)) {
            pushUndoState();
            for (int i = 0; i < wallCount; i++) {
                adjustWallTexAngle(wallIndices[i], -DEG2RAD(15.0f));
            }
        }
        if (keyPressedOnce(keys, SDL_SCANCODE_W)) {
            pushUndoState();
            for (int i = 0; i < wallCount; i++) {
                adjustWallTexAngle(wallIndices[i], DEG2RAD(15.0f));
            }
        }

        if (hasSingleWallSelection() && keyPressedOnce(keys, SDL_SCANCODE_SPACE)) {
            executeEditorAction(ED_ACT_WALL_SPLIT, worldX, worldY);
        }

        for (int i = 0; i < wallCount; i++) {
            if (g_edMap.walls[wallIndices[i]].flags & RC3D_WALL_PORTAL) {
                refreshSelectedPortalFromReverse(wallIndices[i]);
            }
        }
    }
    if (!hasAnyWallEditSelection()) {
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
        !hasAnyWallEditSelection() &&
        !hasMultiSectorSelection() &&
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






    if (hasAnyWallEditSelection()) {
        if (g_ed.textureBrowserTarget != TEX_TARGET_WALL_UPPER &&
            g_ed.textureBrowserTarget != TEX_TARGET_WALL_MIDDLE &&
            g_ed.textureBrowserTarget != TEX_TARGET_WALL_LOWER) {
            g_ed.textureBrowserTarget = TEX_TARGET_WALL_MIDDLE;
        }
    }
    else if (hasAnySectorEditSelection()) {
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

    refreshEditorUIButtonState();
    finishEditorInputFrame(keys, leftDown, rightDown, middleDown, mouseX, mouseY);
}

void rc3dEditRender(void)
{
    if (g_ed.isometricView) {
        drawIsometricPreview();
    } else {
        drawGrid();
        drawMapGeometry();
        drawStartMarker();
    }

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

    drawTextureBrowser();
    drawInspectorPanel();
    drawHoverPanel();
    rcguiDraw(&g_ui);
    drawUndoHistoryPopup();
}
