#ifndef _SIDBOX_3D_ENTITIES_H_
#define _SIDBOX_3D_ENTITIES_H_

#include <stdint.h>
#include "sb3dworld.h"
#include "sb3dmath.h"

// Default colour offsets
#define DEFAULT_COLOUR_BOTTOM  (COLOUR_OFFSET + 2)
#define DEFAULT_COLOUR_TOP     (COLOUR_OFFSET + 3)
#define DEFAULT_COLOUR_SIDE1   (COLOUR_OFFSET + 4)
#define DEFAULT_COLOUR_SIDE2   (COLOUR_OFFSET + 5)
#define DEFAULT_COLOUR_SIDE3   (COLOUR_OFFSET + 2)
#define DEFAULT_COLOUR_SIDE4   (COLOUR_OFFSET + 3)
#define DEFAULT_COLOUR         (COLOUR_OFFSET + 1)

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


/////// primatives //////////
Mesh createBox(float width, float height, float depth);
Mesh createSphere(float radius, int stacks, int slices);
Mesh createPlane(float sizeX, float sizeZ, int divisions);
Mesh createCylinder(float radius, float height, int segments);
Mesh createCone(float radius, float height, int segments);
Mesh createPyramid(float width, float height);
Mesh createTorus(float majorRadius, float minorRadius, int majorSegs, int minorSegs);




// entity changes/
void entityColour(int id, uint8_t colour);
void entityColourFace(int id, int faceId, uint8_t colour);

// materials //
void meshSetDefaultMaterial(Mesh *mesh);
void meshSetMaterial(Mesh *mesh, float ambient, float diffuse, float emissive, float specularStrength, float shininess);




void meshSetVertex(Mesh *mesh, int index, Vec3 v);
void meshOffsetVertex(Mesh *mesh, int index, Vec3 delta);
Vec3 meshGetVertex(const Mesh *mesh, int index);

void meshSetVertexRecalc(Mesh *mesh, int index, Vec3 v);
void meshOffsetVertexRecalc(Mesh *mesh, int index, Vec3 delta);
void meshResetFromSource(Mesh *dst, const Mesh *src);

void meshDeformWaveY(Mesh *mesh, float time, float amount, float freq);
void meshDeformWavePlaneY(Mesh *mesh, float time, float amp, float freqX, float freqZ, float speed);


Mesh copyMesh(const Mesh *src);


void entityFollowCameraXZ(int id, const Camera *cam, float worldY, float snap);


void meshUpdateInfinitePlaneY(
    Mesh *mesh,
    const Mesh *src,
    Vec3 planeOrigin,
    float time,
    float amp,
    float freqX,
    float freqZ,
    float speed
);





#endif