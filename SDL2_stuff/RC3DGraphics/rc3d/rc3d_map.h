#ifndef RC3D_MAP_H
#define RC3D_MAP_H

#include <stdint.h>

#define RC3D_WALL_PORTAL        0x01
#define RC3D_WALL_UPPER         0x02
#define RC3D_WALL_MIDDLE        0x04
#define RC3D_WALL_LOWER         0x08
#define RC3D_WALL_SOLID         0x10
#define RC3D_WALL_MANUAL_TARGET 0x20
#define RC3D_WALL_TRANSPARENCY  0x40
#define RC3D_WALL_DOUBLESIDED   0x80


#define RC3D_SECTORTEX_CLAMPX1  0x01
#define RC3D_SECTORTEX_CLAMPX2  0x02
#define RC3D_SECTORTEX_CLAMPY1  0x04
#define RC3D_SECTORTEX_CLAMPY2  0x08


// texture flags
#define RC3D_TEX_FLAG_DEFAULT   0x00000000u
#define RC3D_TEX_FLAG_CLAMPXL   0x00000001u
#define RC3D_TEX_FLAG_CLAMPXR   0x00000002u
#define RC3D_TEX_FLAG_CLAMPYT   0x00000004u
#define RC3D_TEX_FLAG_CLAMPYB   0x00000008u
#define RC3D_TEX_WALL_GLOW_SHIFT 20u
#define RC3D_TEX_WALL_GLOW_MAX   7u
#define RC3D_TEX_WALL_GLOW_MASK  (RC3D_TEX_WALL_GLOW_MAX << RC3D_TEX_WALL_GLOW_SHIFT)
#define RC3D_TEX_WALL_GLOW(level) \ ((((uint32_t)(level)) & RC3D_TEX_WALL_GLOW_MAX) << RC3D_TEX_WALL_GLOW_SHIFT)
#define RC3D_TEX_FLAG_FLIPX     0x1000000u
#define RC3D_TEX_FLAG_FLIPY     0x2000000u


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
    uint8_t texFlags;       // texture clamping flags  ;)

    int32_t tagId;          /* runtime trigger/tag id */
    uint32_t stateFlags;    /* runtime-controlled sector state flags */
    uint32_t sectorFlags;   /* runtime-sector flags, they do things! */
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

    // Runtime Memory;      
    uint8_t originalLightLevel;
    int8_t PulsatingLightTimeDir;
} RC3D_Sector;


typedef struct {
float x;
    float y;
    float z;
    int tagId;
    int targetTagId;
    uint32_t flags;
    uint32_t type;
    float radius;
    uint8_t textureId;
    uint8_t inFlag;     // when inside the boundery set the targetTagIds flag to this flag
    uint8_t outFlag;    // when outside the boundery set the targetTagIds flag to this flag
    float scalex;       // texture scale x
    float scaley;       // texture scale y
    
    // runtime side only
    uint8_t trigger;    // used to savestate
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

extern RC3D_Map g_rc3dDemoMap;

/* runtime binary map loading */
int rc3dMapLoadBinary(const char *path, RC3D_Map *outMap);
void rc3dMapFreeBinary(RC3D_Map *map);

#endif
