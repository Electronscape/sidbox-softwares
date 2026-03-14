#ifndef _SIDBOX_3D_ENTITIES_H_
#define _SIDBOX_3D_ENTITIES_H_

#include <stdint.h>
#include "sb3dworld.h"
#include "sb3dmath.h"



typedef struct {
    Mesh *mesh;
    Vec3 pos;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
} Entity;


extern Entity worldEntities[WORLD_MAX];

//Entity entityCreate(Mesh *mesh, Vec3 pos);

int entityCreate(Mesh *mesh, Vec3 pos);
float meshComputeBoundsRadius(const Mesh *mesh);

void entitySetPosition(int id, Vec3 pos);
void entityMove(int id, Vec3 delta);

void entityMoveForward(int id, float dist);
void entityMoveRight(int id, float dist);
void entityMoveUp(int id, float dist);

void normalizeEntity(Entity *e);
void entityResetAxes(Entity *e);

void entityTurnLocal(int id, float yaw, float pitch, float roll);
void entityTurnGlobal(int id, float yaw, float pitch, float roll);

Vec3 entityLocalToWorld(const Entity *e, Vec3 v);

#endif