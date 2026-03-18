#ifndef _SIDBOX_3D_ENTITIES_H_
#define _SIDBOX_3D_ENTITIES_H_

#include <stdint.h>

#include "sb3dworld.h"
#include "sb3dmath.h"


//meshSetMaterial(&theTorsu, 0.00f, 0.55f, 0.0f, 1.50f, 64.0f);   // shiny metal

/* -------------------------------------------------------------------------- */
/* Default colour offsets                                                     */
/* -------------------------------------------------------------------------- */

#define DEFAULT_COLOUR_BOTTOM  (COLOUR_OFFSET + 2)
#define DEFAULT_COLOUR_TOP     (COLOUR_OFFSET + 3)
#define DEFAULT_COLOUR_SIDE1   (COLOUR_OFFSET + 4)
#define DEFAULT_COLOUR_SIDE2   (COLOUR_OFFSET + 5)
#define DEFAULT_COLOUR_SIDE3   (COLOUR_OFFSET + 2)
#define DEFAULT_COLOUR_SIDE4   (COLOUR_OFFSET + 3)
#define DEFAULT_COLOUR         (COLOUR_OFFSET + 1)
#define DEFAULT_EMISSION       0


/* -------------------------------------------------------------------------- */
/* Entity                                                                     */
/* -------------------------------------------------------------------------- */


typedef struct {
    Vec3 pos;
    Mesh *mesh;
    
    Vec3 forward;
    Vec3 right;
    Vec3 up;

    uint8_t active;
} Entity;

extern Entity worldEntities[WORLD_MAX];


/* -------------------------------------------------------------------------- */
/* Entity world management                                                    */
/* -------------------------------------------------------------------------- */

int  entityWorldSpawn(Mesh *mesh, Vec3 pos);
void entityWorldDestroy(int *id);


/* -------------------------------------------------------------------------- */
/* Entity transforms / movement                                               */
/* -------------------------------------------------------------------------- */

Mesh copyMesh(const Mesh *src);
void entitySetPosition(int id, Vec3 pos);
Vec3 entityGetPosition(int id);

void entityMove(int id, Vec3 delta);
void entityMoveForward(int id, float dist);
void entityMoveRight(int id, float dist);
void entityMoveUp(int id, float dist);

void entityTurnLocal(int id, float yaw, float pitch, float roll);
void entityTurnGlobal(int id, float yaw, float pitch, float roll);

void entityRotation(int id, float yaw, float pitch, float roll);

void normalizeEntity(Entity *e);
void entityResetAxes(Entity *e);

Vec3 entityLocalToWorld(const Entity *e, Vec3 v);
void entityFollowCameraXZ(int id, const Camera *cam, float worldY, float snap);

Vec3 entityLookAt(int aId, int bId, uint8_t rotate);


/* -------------------------------------------------------------------------- */
/* Entity / mesh colour helpers                                               */
/* -------------------------------------------------------------------------- */

void entityColour(int id, uint8_t colour);
void entityColourFace(int id, int faceId, uint8_t colour);


/* -------------------------------------------------------------------------- */
/* Mesh material                                                              */
/* -------------------------------------------------------------------------- */

void meshSetDefaultMaterial(
    Mesh *mesh
);

void meshSetMaterial(
    Mesh *mesh,
    float ambient,
    float diffuse,
    float emissive,
    float specularStrength,
    float shininess
);


/* -------------------------------------------------------------------------- */
/* Mesh utilities                                                             */
/* -------------------------------------------------------------------------- */

float meshComputeBoundsRadius(const Mesh *mesh);

void meshSetVertex(Mesh *mesh, int index, Vec3 v);
void meshOffsetVertex(Mesh *mesh, int index, Vec3 delta);
Vec3 meshGetVertex(const Mesh *mesh, int index);

void meshSetVertexRecalc(Mesh *mesh, int index, Vec3 v);
void meshOffsetVertexRecalc(Mesh *mesh, int index, Vec3 delta);
void meshResetFromSource(Mesh *dst, const Mesh *src);

Mesh copyMesh(const Mesh *src);



/* -------------------------------------------------------------------------- */
/* Primitive mesh factories                                                   */
/* -------------------------------------------------------------------------- */

Mesh createBox(float width, float height, float depth);
Mesh createSphere(float radius, int stacks, int slices);
Mesh createPlane(float sizeX, float sizeZ, int divisions);
Mesh createCylinder(float radius, float height, int segments);
Mesh createCone(float radius, float height, int segments);
Mesh createPyramid(float width, float height);
Mesh createTorus(float majorRadius, float minorRadius, int majorSegs, int minorSegs);


int entityBuildWorldCache(Entity *ent);

#endif

