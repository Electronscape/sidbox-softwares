#include <stdint.h>
#include <math.h>

#include "sb3d.h"



static void rebuildCameraBasis(Camera *cam)
{
    float cy = cosf(cam->rotation.x);
    float sy = sinf(cam->rotation.x);

    float cp = cosf(cam->rotation.y);
    float sp = sinf(cam->rotation.y);

    //float cr = cosf(cam->rotation.z);
    //float sr = sinf(cam->rotation.z);

    // Forward from yaw/pitch
    cam->forward.x = sy * cp;
    cam->forward.y = sp;
    cam->forward.z = cy * cp;
    cam->forward = vec3Normalize(cam->forward);

    // Build a base right/up from world up
    {
        Vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);

        cam->right = vec3Cross(worldUp, cam->forward);
        if (vec3Dot(cam->right, cam->right) < 0.00000001f) {
            cam->right = vec3(1.0f, 0.0f, 0.0f);
        } else {
            cam->right = vec3Normalize(cam->right);
        }

        cam->up = vec3Cross(cam->forward, cam->right);
        cam->up = vec3Normalize(cam->up);
    }

    // Apply roll around forward axis
    if (cam->rotation.z != 0.0f) {
        cam->right = rotateAroundAxis(cam->right, cam->forward, cam->rotation.z);
        cam->up    = rotateAroundAxis(cam->up,    cam->forward, cam->rotation.z);
    }

    cameraNormalize(cam);
}


Camera cameraCreate(void)
{
    Camera cam = {
        .pos       = { 0.0f, 0.0f, 0.0f },
        .rotation  = { 0.0f, 0.0f, 0.0f },
        .right     = { 1.0f, 0.0f, 0.0f },
        .up        = { 0.0f, 1.0f, 0.0f },
        .forward   = { 0.0f, 0.0f, 1.0f },
        .nearPlane = 0.01f,
        .farPlane  = 1000.0f
    };

    return cam;
}


void cameraSetPosition(Camera *cam, Vec3 pos)
{
    if (!cam) return;
    cam->pos = pos;
}

void cameraRotate(Camera *cam, float yaw, float pitch, float roll){
    if (!cam) return;
    cam->rotation.x = yaw;
    cam->rotation.y = pitch;
    cam->rotation.z = roll;
    rebuildCameraBasis(cam);
}




void cameraTurn(Camera *cam, float x, float y, float z, uint8_t global)
{
    Vec3 axisX;
    Vec3 axisY;
    Vec3 axisZ;

    if (!cam) return;

    if (global) {
        axisX = vec3(1.0f, 0.0f, 0.0f);
        axisY = vec3(0.0f, 1.0f, 0.0f);
        axisZ = vec3(0.0f, 0.0f, 1.0f);
    } else {
        axisX = cam->right;    /* pitch */
        axisY = cam->up;       /* yaw   */
        axisZ = cam->forward;  /* roll  */
    }

    if (y != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, axisY, y);
        cam->right   = rotateAroundAxis(cam->right,   axisY, y);
        cam->up      = rotateAroundAxis(cam->up,      axisY, y);
    }

    if (x != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, axisX, x);
        cam->right   = rotateAroundAxis(cam->right,   axisX, x);
        cam->up      = rotateAroundAxis(cam->up,      axisX, x);
    }

    if (z != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, axisZ, z);
        cam->right   = rotateAroundAxis(cam->right,   axisZ, z);
        cam->up      = rotateAroundAxis(cam->up,      axisZ, z);
    }

    cameraNormalize(cam);
}

void cameraNormalize(Camera *cam)
{
    cam->forward = vec3Normalize(cam->forward);
    cam->right   = vec3Normalize(cam->right);

    cam->up = vec3Cross(cam->forward, cam->right);
    cam->up = vec3Normalize(cam->up);

    cam->right = vec3Cross(cam->up, cam->forward);
    cam->right = vec3Normalize(cam->right);
}


Vec3 worldToCamera(Vec3 p, Camera cam)
{
    Vec3 d;
    d.x = p.x - cam.pos.x;
    d.y = p.y - cam.pos.y;
    d.z = p.z - cam.pos.z;

    Vec3 out;
    out.x = vec3Dot(d, cam.right);
    out.y = vec3Dot(d, cam.up);
    out.z = vec3Dot(d, cam.forward);

    return out;
}


void cameraSetRange(Camera *cam, float nearPlane, float farPlane)
{
    if (!cam) return;

    if (nearPlane < 0.001f) {
        nearPlane = 0.001f;
    }

    if (farPlane <= nearPlane) {
        farPlane = nearPlane + 1.0f;
    }

    cam->nearPlane = nearPlane;
    cam->farPlane  = farPlane;
}



void cameraMove(Camera *cam, float x, float y, float z)
{
    if (!cam) return;

    cam->pos.x += cam->right.x   * x;
    cam->pos.y += cam->right.y   * x;
    cam->pos.z += cam->right.z   * x;

    cam->pos.x += cam->up.x      * y;
    cam->pos.y += cam->up.y      * y;
    cam->pos.z += cam->up.z      * y;

    cam->pos.x += cam->forward.x * z;
    cam->pos.y += cam->forward.y * z;
    cam->pos.z += cam->forward.z * z;
}