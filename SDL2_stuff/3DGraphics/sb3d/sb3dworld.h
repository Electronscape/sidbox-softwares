#ifndef _SIDBOX_3D_WORLDSPACE_H_
#define _SIDBOX_3D_WORLDSPACE_H_

#include <stdint.h>


#define NEAR_Z 0.1f



#define WORLD_MAX   256
extern int worldEntityCount;
void worldClear(void);


#define MAX_RENDER_TRIS 4096    
#define MAX_LIGHTS 8



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
} Tri;

typedef struct {
    Vec3 pos;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
} Camera;


typedef struct {
    LightType type;
    Vec3  pos;
    Vec3  dir;
    float intensity;
    int   enabled;
} Light;

typedef struct {
    Vec3 *verts;
    int vertCount;

    Edge *edges;
    int edgeCount;

    Tri *tris;
    int triCount;

    float boundsRadius;
} Mesh;



typedef struct {
    Vec2 p0;
    Vec2 p1;
    Vec2 p2;

    uint8_t color;
    float shadeF;
    float depth;
} RenderTri;

#endif