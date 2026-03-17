#ifndef _SIDBOX_3D_CAMERA_H_
#define _SIDBOX_3D_CAMERA_H_

#include "sb3dworld.h"
#include "sb3dmath.h"

typedef struct {
    Vec3 pos;
    Vec3 rotation;

    Vec3 right;
    Vec3 up;
    Vec3 forward;

    float nearPlane;
    float farPlane;
} Camera;


Camera createCamera(void);
void normalizeCamera(Camera *cam);
Vec3 worldToCamera(Vec3 p, Camera cam);
void setCameraRange(Camera *cam, float nearPlane, float farPlane);

void positionCamera(Camera *cam, Vec3 pos);
void translateCamera(Camera *cam, float x, float y, float z);
void moveCamera(Camera *cam, float x, float y, float z);
void rotateCamera(Camera *cam, float yaw, float pitch, float roll);
//void turnCamera(Camera *cam, float yaw, float pitch, float roll);
void turnCamera(Camera *cam, float x, float y, float z, uint8_t global);


#endif