#ifndef RENDER3D_H
#define RENDER3D_H

#include "sb3dworld.h"

#define CLIP_MAX_VERTS 8
#define PROJ_F 200.0f

typedef enum {
    PLANE_NEAR = 0,
    PLANE_LEFT,
    PLANE_RIGHT,
    PLANE_TOP,
    PLANE_BOTTOM
} ClipPlane;


typedef struct {
    float x;
    float y;
    float camz;
} ScreenVert;



void setDefaultRenderMode();
void enableZOrdering(int enable);
void enableFlatMode(int en);
void enableTwoShade(int en);
void enableWireFrame(int en);


int projectPoint(Vec3 p, const Camera *cam, Vec2 *out);
int clipLineToNearPlane(Vec3 *a, Vec3 *b, const Camera *cam);
void resetRenderList(void);

int getRenderTriCount(void);
void Render3D(const Camera *cam);

// sorting
void sortEntitiesByDepth(Entity *entities, int count, const Camera *cam);

void submitWorldEntities(const Camera *cam);
void submitEntitySolid(const Entity *ent, const Camera *cam);
void drawEntitySolid(const Entity *ent, const Camera *cam);
void drawEntity(const Entity *ent, const Camera *cam, uint8_t color);
void drawWorldLine(Vec3 a, Vec3 b, const Camera *cam, uint8_t color);


void drawFakeHorizonDots(const Camera *cam, uint8_t dotCol, int spacing, float ylevel, uint8_t density);
void drawFakeHorizon(const Camera *cam, uint8_t skyCol, uint8_t groundCol, uint8_t lineCol, float ylevel);

#endif