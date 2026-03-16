#ifndef _SIDBOX_3D_CAMERA_H_
#define _SIDBOX_3D_CAMERA_H_

#include "sb3dworld.h"
#include "sb3dmath.h"

typedef struct {
    Vec3 pos;
    Vec3 right;
    Vec3 up;
    Vec3 forward;
    float nearPlane;
    float farPlane;
} Camera;


void normalizeCamera(Camera *cam);
Vec3 worldToCamera(Vec3 p, Camera cam);
void setCameraRange(Camera *cam, float nearPlane, float farPlane);

void turnCameraLocal(Camera *cam, float yaw, float pitch, float roll);
void turnCameraGlobal(Camera *cam, float yaw, float pitch, float roll);

#endif