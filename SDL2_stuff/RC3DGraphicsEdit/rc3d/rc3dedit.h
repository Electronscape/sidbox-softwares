#ifndef RC3D_EDIT_H
#define RC3D_EDIT_H

#include <stdint.h>


//// EDITOR DEFAULTS
#define ED_DEFAULT_WALL_UPPER_TEX_ID        1   // brick (though should be "no texture")
#define ED_DEFAULT_WALL_MED_TEX_ID          1
#define ED_DEFAULT_WALL_LOWER_TEX_ID        1
#define ED_DEFAULT_SECTOR_FLOOR_TEX_ID      3
#define ED_DEFAULT_SECTOR_CEILING_TEX_ID    255


#define RC3D_SECTOR_STATE_NONE           0u
#define RC3D_SECTOR_STATE_RAISE_FLOOR    0x01u
#define RC3D_SECTOR_STATE_LOWER_FLOOR    0x02u
#define RC3D_SECTOR_STATE_LOWER_CEILING  0x04u
#define RC3D_SECTOR_STATE_RAISE_CEILING  0x08u


// colours 

// colour settings
#define ED_INSPECTOR_TITLE_COL  7
#define ED_INSPECTOR_TEXT_COL   2
#define ED_TEXT_COL             6

#define ED_START_COL            2   // white
#define ED_HOME_GRID_COL        1

#define ED_GRID_MAJOR_COL       3  // blue
#define ED_GRID_MINOR_COL       5  // darker blue
#define ED_GRID_MICRO_COL       5

//#define ED_VERT_COL              3
//#define ED_WALL_COL              2
#define ED_PORTAL_COL            8  // green


#define ED_COLOUR_SCROLL_BAR_BG 5
#define ED_COLOUR_SCROLL_BAR    3
#define ED_COLOUR_SCROLL_BAR_FRAME  1

#define ED_UI_BG                5
#define ED_UI_BORDER            2

#define ED_COLOUR_SELECTED_SECTOR   7  // yellow



#define ED_CURSOR_COL           11

#define ED_COLOUR_BTN_TEXT              2
#define ED_COLOUR_BTN_BG                9
#define ED_COLOUR_BTN_BG_ACTIVE         8
#define ED_COLOUR_BTN_FRAME             1
#define ED_COLOUR_BTN_TXT_DISABLED      15
#define ED_COLOUR_BTN_BG_DISABLED       1
#define ED_COLOUR_BTN_FRAME_DISABLED    15
#define ED_COLOUR_BTN_ACTIVE            6
#define ED_COLOUR_BTN_TEXT_HOVER        16


#define ED_COLOUR_TOOLTIP_BG        15
#define ED_COLOUR_TOOLTIP_BORDER    14
#define ED_COLOUR_TOOLTIP_TEXT      7

#define ED_COLOUR_TEXT_BAR_BG       4
#define ED_COLOUR_TEXT_BAR_BORDER   3

#define ED_OBJECT_TEXT_NOTE_BG      16
#define ED_OBJECT_TEXT_NOTE_BORDER  4
#define ED_OBJECT_TEXT_TEXT         3

// map validator
#define ED_VALIDATOR_TEXT                   2
#define ED_VALIDATOR_SELECTION_BG           3
#define ED_VALIDATOR_SELECTION_TEXT         14

// expanded panels
#define ED_EXPANDED_MENU_TEXT               2

// inspectors
#define ED_INSPECTOR_PANELS_HEADER_TEXT     7
#define ED_INSPECTOR_PARENT_PANELS_BG       16
#define ED_INSPECTOR_PARENT_PANELS_FRAME    6
#define ED_INSPECTOR_PANELS_BACKPANEL       17
#define ED_INSPECTOR_PANELS_PANELFRAME      3

// texture explorer
#define ED_TEXTURE_EXPLORER_HEADER_TEXT     2
#define ED_TEXTURE_EXPLORER_SELECT_FRAME    8

// DRAFTING colours
// walls
#define ED_COLOUR_DRAFTWALL         11
#define ED_COLOUR_HOVER_WALL        1
#define ED_COLOUR_SELECTED_WALL     8  
#define ED_COLOUR_WALL              26//11
#define ED_COLOUR_WALLO             8
#define ED_COLOUR_WALL_SPECIAL      14

// vertex
#define ED_COLOUR_VERTEX            11
#define ED_COLOUR_VERTEX_HOVER      10
#define ED_COLOUR_VERTEX_SELECTED   2

#define ED_COLOUR_VERTEX_SPLIT_PREV 7
#define ED_COLOUR_MULTI_SELECT_RANGE_BOX    1


#define RC3D_SECTOR_FLAGS_FLICKERING_LIGHTS 0x100
#define RC3D_SECTOR_FLAGS_PULSATING_LIGHT   0x200
#define RC3D_SECTOR_FLAGS_FULLBRIGHT        0x400

void rc3dEditInit(void);
void rc3dEditUpdate(float dt,
                    const uint8_t *keys,
                    int mouseX,
                    int mouseY,
                    uint32_t mouseButtons,
                    int mouseWheelY);
int rc3dEditHandleTextInput(const char *text);


void rc3dEditRender(void);
int rc3dGuiCheckDirty();

#endif
