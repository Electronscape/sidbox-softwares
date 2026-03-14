#include <stdint.h>
#include <math.h>

#include "sb3d.h"


Vec3 entityLocalToWorld(const Entity *e, Vec3 v)
{
    Vec3 world = e->pos;

    world = vec3Add(world, vec3Scale(e->right,   v.x));
    world = vec3Add(world, vec3Scale(e->up,      v.y));
    world = vec3Add(world, vec3Scale(e->forward, v.z));

    return world;
}


float meshComputeBoundsRadius(const Mesh *mesh)
{
    float maxDist2 = 0.0f;

    for (int i = 0; i < mesh->vertCount; i++) {
        Vec3 v = mesh->verts[i];
        float d2 = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);

        if (d2 > maxDist2) {
            maxDist2 = d2;
        }
    }

    return sqrtf(maxDist2);
}


int entityCreate(Mesh *mesh, Vec3 pos)
{
    int id;

    if (worldEntityCount >= WORLD_MAX) {
        return -1;
    }

    id = worldEntityCount;
    worldEntityCount++;

    worldEntities[id].mesh = mesh;
    worldEntities[id].pos = pos;

    worldEntities[id].right   = (Vec3){ 1.0f, 0.0f, 0.0f };
    worldEntities[id].up      = (Vec3){ 0.0f, 1.0f, 0.0f };
    worldEntities[id].forward = (Vec3){ 0.0f, 0.0f, 1.0f };

    //worldEntities[id].mesh->boundsRadius = meshComputeBoundsRadius(mesh);

    return id;
}


void entitySetPosition(int id, Vec3 pos)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = pos;
}

void entityMove(int id, Vec3 delta)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(worldEntities[id].pos, delta);
}

void entityMoveForward(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].forward, dist)
    );
}

void entityMoveRight(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].right, dist)
    );
}

void entityMoveUp(int id, float dist)
{
    if (id < 0 || id >= worldEntityCount) return;
    worldEntities[id].pos = vec3Add(
        worldEntities[id].pos,
        vec3Scale(worldEntities[id].up, dist)
    );
}



void normalizeEntity(Entity *e)
{
    e->forward = vec3Normalize(e->forward);
    e->right   = vec3Normalize(e->right);

    e->up = vec3Cross(e->forward, e->right);
    e->up = vec3Normalize(e->up);

    e->right = vec3Cross(e->up, e->forward);
    e->right = vec3Normalize(e->right);
}

void entityResetAxes(Entity *e)
{
    e->right   = (Vec3){ 1.0f, 0.0f, 0.0f };
    e->up      = (Vec3){ 0.0f, 1.0f, 0.0f };
    e->forward = (Vec3){ 0.0f, 0.0f, 1.0f };
}


void entityTurnLocal(int id, float yaw, float pitch, float roll)
{
    Entity *e;

    if (id < 0 || id >= worldEntityCount) return;
    e = &worldEntities[id];

    if (yaw != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, e->up, yaw);
        e->right   = rotateAroundAxis(e->right,   e->up, yaw);
    }

    if (pitch != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, e->right, pitch);
        e->up      = rotateAroundAxis(e->up,      e->right, pitch);
    }

    if (roll != 0.0f) {
        e->right = rotateAroundAxis(e->right,   e->forward, roll);
        e->up    = rotateAroundAxis(e->up,      e->forward, roll);
    }

    normalizeEntity(e);
}

void entityTurnGlobal(int id, float yaw, float pitch, float roll)
{
    Entity *e;
    Vec3 worldX = { 1.0f, 0.0f, 0.0f };
    Vec3 worldY = { 0.0f, 1.0f, 0.0f };
    Vec3 worldZ = { 0.0f, 0.0f, 1.0f };

    if (id < 0 || id >= worldEntityCount) return;
    e = &worldEntities[id];

    if (yaw != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldY, yaw);
        e->right   = rotateAroundAxis(e->right,   worldY, yaw);
        e->up      = rotateAroundAxis(e->up,      worldY, yaw);
    }

    if (pitch != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldX, pitch);
        e->right   = rotateAroundAxis(e->right,   worldX, pitch);
        e->up      = rotateAroundAxis(e->up,      worldX, pitch);
    }

    if (roll != 0.0f) {
        e->forward = rotateAroundAxis(e->forward, worldZ, roll);
        e->right   = rotateAroundAxis(e->right,   worldZ, roll);
        e->up      = rotateAroundAxis(e->up,      worldZ, roll);
    }

    normalizeEntity(e);
}

