#ifndef RC3D_MAP_H
#define RC3D_MAP_H

#include <stdint.h>

#define RC3D_WALL_PORTAL   0x01
#define RC3D_WALL_UPPER    0x02
#define RC3D_WALL_MIDDLE   0x04
#define RC3D_WALL_LOWER    0x08
#define RC3D_WALL_SOLID    0x10
#define RC3D_WALL_MANUAL_TARGET 0x20
#define RC3D_WALL_TRANSPARENCY 0x40



// texture flags
#define RC3D_TEX_FLAG_DEFAULT   0x00000000u
#define RC3D_TEX_FLAG_CLAMPXL   0x00000001u
#define RC3D_TEX_FLAG_CLAMPXR   0x00000002u
#define RC3D_TEX_FLAG_CLAMPYT   0x00000004u
#define RC3D_TEX_FLAG_CLAMPYB   0x00000008u


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

    uint8_t flags;
    uint32_t tex_flags;  // texture clamp/uv behaviour flags
} RC3D_Wall;

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

/* runtime binary map loading */
int rc3dMapLoadBinary(const char *path, RC3D_Map *outMap);
void rc3dMapFreeBinary(RC3D_Map *map);

#endif
