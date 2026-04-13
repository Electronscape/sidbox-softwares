#ifndef RC3D_MAP_H
#define RC3D_MAP_H

#include <stdint.h>

#define RC3D_WALL_PORTAL   0x01
#define RC3D_WALL_UPPER    0x02
#define RC3D_WALL_MIDDLE   0x04
#define RC3D_WALL_LOWER    0x08
#define RC3D_WALL_SOLID    0x10

#define RC3D_TEX_FLAG_DEFAULT       0x00000000u
#define RC3D_TEX_FLAG_CLAMPXL       0x00000001u
#define RC3D_TEX_FLAG_CLAMPXR       0x00000002u
#define RC3D_TEX_FLAG_CLAMPYT       0x00000004u
#define RC3D_TEX_FLAG_CLAMPYB       0x00000008u
#define RC3D_TEX_WALL_ANGLE_SHIFT   4u
#define RC3D_TEX_WALL_ANGLE_MASK    (0xFFFFu << RC3D_TEX_WALL_ANGLE_SHIFT)
#define RC3D_TEX_WALL_BRIGHT_SHIFT  20u
#define RC3D_TEX_WALL_BRIGHT_MASK   (0x0Fu << RC3D_TEX_WALL_BRIGHT_SHIFT)


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

    uint8_t flags;          /* wall flags */
    uint32_t texture_flags; /* low bits: clamp flags, bits 4..19: angle, bits 20..23: brightness */
    float texScaleX;        /* wall texture UV scale X */
    float texScaleY;        /* wall texture UV scale Y */

} RC3D_Wall;

typedef struct {
    int wallStart;
    int wallCount;
    int boundaryCount;

    float floorHeight;
    float ceilHeight;

    uint8_t floorColor;
    uint8_t ceilColor;
    uint8_t glowlevel;      /* 0 = normal lighting, 1..7 = brighter */

    int32_t tagId;          /* runtime trigger/tag id */
    uint32_t stateFlags;    /* runtime-controlled sector state flags */
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

typedef struct {
    const RC3D_Vec2   *verts;
    int                vertCount;

    const RC3D_Wall   *walls;
    int                wallCount;

    const RC3D_Sector *sectors;
    int                sectorCount;

    int startSector;
    float startX;
    float startY;
    float startAngle;
} RC3D_Map;

extern const RC3D_Map g_rc3dDemoMap;

#endif
