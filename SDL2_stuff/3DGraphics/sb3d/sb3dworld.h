#ifndef _SIDBOX_3D_WORLDSPACE_H_
#define _SIDBOX_3D_WORLDSPACE_H_

#include <stdint.h>
#include "sb3dmaterial.h"


#define NEAR_Z 0.1f



#define WORLD_MAX   256
extern int worldEntityCount;
void worldClear(void);

#define M_PI    3.14159265358979323846f

#define MAX_RENDER_TRIS 1024 * 8    
#define MAX_LIGHTS 8


#define COLOUR_OFFSET   32  // base default colouring system


typedef struct {
    float x;
    float y;
    float z;
} Vec3;

typedef struct {
    int x;
    int y;
} Vec2;




typedef enum {
    LIGHT_POINT = 0,
    LIGHT_DIRECTIONAL = 1
} LightType;

typedef struct {
    int a;
    int b;
} Edge;

typedef struct {
    int a;
    int b;
    int c;
    uint8_t color;
    uint8_t emission;
    uint8_t transparency;
} Tri;






typedef struct {
    Vec3 *verts;
    int vertCount;

    Edge *edges;
    int edgeCount;

    Tri *tris;
    int triCount;

    float boundsRadius;

    Material material;
} Mesh;



typedef struct {
    Vec2 p0;
    Vec2 p1;
    Vec2 p2;
    float depth;
    float shadeF;

    float shade0F;
    float shade1F;
    float shade2F;

    uint16_t z0;
    uint16_t z1;
    uint16_t z2;

    float camz0;
    float camz1;
    float camz2;

    uint8_t color;
    uint8_t emission;
    uint8_t transparency;

    int16_t minY;
    int16_t maxY;
} RenderTri;

#endif