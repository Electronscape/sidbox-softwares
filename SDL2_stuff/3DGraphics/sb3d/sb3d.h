#ifndef _SIDBOX_3D_LIB_H_
#define _SIDBOX_3D_LIB_H_

#include "sb3d/sb3dmath.h"
#include "sb3d/sb3dcamera.h"

#include "sb3d/sb3dmaterial.h"

#include "sb3d/sb3dworld.h"
#include "sb3d/sb3dlights.h"
#include "sb3d/sb3dentity.h"
#include "sb3d/sb3dparticles.h"


#include "sb3d/sb3drender.h"



typedef struct {
    uint8_t hit;
    int entityId;
    int triIndex;
    float distance;

    Vec3 point;
    Vec3 normal;

    Vec3 right;
    Vec3 up;
    Vec3 forward;

    float yaw;
    float pitch;
    float roll;
} SB3DRaycastHit;



int sb3dRaycastWorld(
    Vec3 rayOrig,
    Vec3 rayDir,
    float maxDist,
    SB3DRaycastHit *outHit
);

int sb3dRaycastFromCamera(
    const Camera *cam,
    float maxDist,
    SB3DRaycastHit *outHit
);


int sb3dRaycastFromCamera(const Camera *cam, float maxDist, SB3DRaycastHit *outHit);
int sb3dRaycastWorld(Vec3 rayOrig, Vec3 rayDir, float maxDist, SB3DRaycastHit *outHit);

void entityAlignToHit(int id, const SB3DRaycastHit *hit);

#endif