#ifndef RC3D_MAP_H
#define RC3D_MAP_H

#include <stdint.h>


#define RC3D_SECTOR_STATE_NONE                  0u
#define RC3D_SECTOR_STATE_RAISE_FLOOR           0x01u
#define RC3D_SECTOR_STATE_LOWER_FLOOR           0x02u
#define RC3D_SECTOR_STATE_LOWER_CEILING         0x04u
#define RC3D_SECTOR_STATE_RAISE_CEILING         0x08u


#define RC3D_SECTOR_FLAGS_FLICKERING_LIGHTS     0x100
#define RC3D_SECTOR_FLAGS_PULSATING_LIGHT       0x200
#define RC3D_SECTOR_FLAGS_FULLBRIGHT            0x400
#define RC3D_SECTOR_FLAGS_EFFECTWALLS           0x800
#define RC3D_SECTOR_FLAGS_DAMAGEZONE            0x1000   /// YEAH, STAY out of these zones if you can ;)


#define RC3D_WALL_PORTAL            0x0001u
#define RC3D_WALL_UPPER             0x0002u
#define RC3D_WALL_MIDDLE            0x0004u
#define RC3D_WALL_LOWER             0x0008u
#define RC3D_WALL_SOLID             0x0010u
#define RC3D_WALL_MANUAL_TARGET     0x0020u
#define RC3D_WALL_TRANSPARENCY      0x0040u
#define RC3D_WALL_DOUBLESIDED       0x0080u
#define RC3D_WALL_CLICKABLE         0x0100u
#define RC3D_WALL_CLICK_ENABLE      0x0200u
#define RC3D_WALL_CLICK_ACT_ENTER   0x0400u
#define RC3D_WALL_CLICK_ACT_EXIT    0x0800u



#define RC3D_TEX_FLAG_DEFAULT       0x00u
#define RC3D_TEX_FLAG_CLAMPXL       0x01u
#define RC3D_TEX_FLAG_CLAMPXR       0x02u
#define RC3D_TEX_FLAG_CLAMPYT       0x04u
#define RC3D_TEX_FLAG_CLAMPYB       0x08u
#define RC3D_TEX_WALL_ANGLE_SHIFT   4u
#define RC3D_TEX_WALL_ANGLE_MASK    (0xFFFFu << RC3D_TEX_WALL_ANGLE_SHIFT)
#define RC3D_TEX_WALL_BRIGHT_SHIFT  20u
#define RC3D_TEX_WALL_BRIGHT_MASK   (0x0Fu << RC3D_TEX_WALL_BRIGHT_SHIFT)
#define RC3D_TEX_FLAG_FLIPX         0x1000000u
#define RC3D_TEX_FLAG_FLIPY         0x2000000u


typedef struct {
    float x;
    float y;
} RC3D_Vec2;

typedef struct {
    int v0;
    int v1;

    int neighbour;          /* -1 = no adjoining sector */

    float openBottom;       /* opening lower Z */
    float openTop;          /* opening upper Z */

    uint8_t upperColor;     /* above opening */
    uint8_t midColor;       /* middle slab */
    uint8_t lowerColor;     /* below opening */
    uint8_t targetObjID;    /* use this to trigger */

    uint16_t flags;          /* wall flags */
    uint16_t _pad2;
    uint32_t texture_flags; /* low bits: clamp flags, bits 4..19: angle, bits 20..23: brightness */
    float texScaleX;        /* wall texture UV scale X */
    float texScaleY;        /* wall texture UV scale Y */
    float texOffsetX;       /* wall texture UV offset X */
    float texOffsetY;       /* wall texture UV offset X */

} RC3D_Wall;


#define RC3D_SECTORTEX_CLAMPX1  0x01
#define RC3D_SECTORTEX_CLAMPX2  0x02
#define RC3D_SECTORTEX_CLAMPY1  0x04
#define RC3D_SECTORTEX_CLAMPY2  0x08


typedef struct {
    int wallStart;
    int wallCount;
    int boundaryCount;

    float floorHeight;
    float ceilHeight;

    uint8_t floorColor;
    uint8_t ceilColor;
    uint8_t glowlevel;      /* 0 = normal lighting, 1..7 = brighter */
    uint8_t texFlags;       // texture clamping flags  ;)

    int32_t tagId;          /* runtime trigger/tag id */
    uint32_t stateFlags;    /* runtime-controlled sector state flags */
    uint32_t sectorFlags;   /* runtime-stuff the sectors can do */
    float floorMinHeight;   /* runtime lower bound for floor */
    float floorMaxHeight;   /* runtime upper bound for floor */
    float ceilMinHeight;    /* runtime lower bound for ceiling */
    float ceilMaxHeight;    /* runtime upper bound for ceiling */
    float floorFlowHeight;  /* runtime floor step/speed amount */
    float ceilFlowHeight;   /* runtime ceiling step/speed amount */

    float floorTexScaleX;
    float floorTexScaleY;
    float floorTexAngle;

    float ceilTexScaleX;
    float ceilTexScaleY;
    float ceilTexAngle;
} RC3D_Sector;


typedef enum {
    RC3D_OBJTYPE_SPRITE = 0u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_INOUT = 1u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_ENTER = 2u,
    RC3D_OBJTYPE_SECTOR_TRIGGER_EXIT  = 3u,
    RC3D_OBJTYPE_ROUTE_PREVIEW        = 4u,
    RC3D_OBJTYPE_BAKED_ROUTE_NODE     = 5u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_INOUT = 6u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_ENTER = 7u,
    RC3D_OBJTYPE_OBJECT_TRIGGER_EXIT  = 8u,
    RC3D_OBJTYPE_OBJECT_TELEPORTER    = 9u,
    RC3D_OBJTYPE_OBJECT_CLICKABLE     = 10u,
    RC3D_OBJTYPE_OBJECT_ACTIOBUTTON   = 11u,
    RC3D_OBJTYPE_OBJECT_GENERIC_USER  = 12u,    // add stuff after this!!
    RC3D_OBJTYPE_OBJECT_ENEMY_1       = 13u,
    RC3D_OBJTYPE_OBJECT_FRIENDLY_1    = 14u,
    RC3D_OBJTYPE_ENDLIST          
} RC3D_ObjectType;


typedef struct {
    float x;
    float y;
    float z;
    int tagId;
    int targetTagId;
    uint32_t flags;
    uint32_t type;    /* RC3D_ObjectType built-ins, higher values are runtime-defined */
    float radius;
    uint8_t textureId;
    uint8_t inFlag;     // when inside the boundery set the targetTagIds flag to this flag
    uint8_t outFlag;    // when outside the boundery set the targetTagIds flag to this flag
    float scalex;
    float scaley;
    float angle;      /* radians, used for teleporter exit facing and editor direction markers */
} RC3D_Object;


typedef struct {
    const RC3D_Vec2   *verts;
    int                vertCount;

    const RC3D_Wall   *walls;
    int                wallCount;

    const RC3D_Sector *sectors;
    int                sectorCount;

    const RC3D_Object *objects;
    int objectCount;

    int startSector;
    float startX;
    float startY;
    float startAngle;
} RC3D_Map;

extern const RC3D_Map g_rc3dDemoMap;

#endif
