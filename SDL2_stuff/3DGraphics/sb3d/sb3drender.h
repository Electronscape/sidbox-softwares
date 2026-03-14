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


int projectPoint(Vec3 p, Vec2 *out);
int clipLineToNearPlane(Vec3 *a, Vec3 *b);
void resetRenderList(void);
void Render3D(const Camera *cam);

// sorting
void sortEntitiesByDepth(Entity *entities, int count, const Camera *cam);

void submitWorldEntities(const Camera *cam);
void submitEntitySolid(const Entity *ent, const Camera *cam);
void drawEntitySolid(const Entity *ent, const Camera *cam);
void drawEntity(const Entity *ent, const Camera *cam, uint8_t color);
void drawWorldLine(Vec3 a, Vec3 b, const Camera *cam, uint8_t color);

#endif