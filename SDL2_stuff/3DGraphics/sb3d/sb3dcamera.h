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


    // projector
    float projF;
    float halfW, halfH, halfOverW;
} Camera;


Camera cameraCreate(void);  //
void cameraNormalize(Camera *cam);
Vec3 worldToCamera(Vec3 p, Camera cam);
void cameraSetRange(Camera *cam, float nearPlane, float farPlane);
void cameraSetPosition(Camera *cam, Vec3 pos);
void cameraMove(Camera *cam, float x, float y, float z);
void cameraRotate(Camera *cam, float yaw, float pitch, float roll);
void cameraTurn(Camera *cam, float x, float y, float z, uint8_t global);


#endif