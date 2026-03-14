#include <stdint.h>

#include "sb3d.h"


void normalizeCamera(Camera *cam)
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



void turnCameraLocal(Camera *cam, float yaw, float pitch, float roll)
{
    if (yaw != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, cam->up, yaw);
        cam->right   = rotateAroundAxis(cam->right,   cam->up, yaw);
    }

    if (pitch != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, cam->right, pitch);
        cam->up      = rotateAroundAxis(cam->up,      cam->right, pitch);
    }

    if (roll != 0.0f) {
        cam->right = rotateAroundAxis(cam->right, cam->forward, roll);
        cam->up    = rotateAroundAxis(cam->up,    cam->forward, roll);
    }

    normalizeCamera(cam);
}

void turnCameraGlobal(Camera *cam, float yaw, float pitch, float roll)
{
    Vec3 worldX = {1.0f, 0.0f, 0.0f};
    Vec3 worldY = {0.0f, 1.0f, 0.0f};
    Vec3 worldZ = {0.0f, 0.0f, 1.0f};

    if (yaw != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, worldY, yaw);
        cam->right   = rotateAroundAxis(cam->right,   worldY, yaw);
        cam->up      = rotateAroundAxis(cam->up,      worldY, yaw);
    }

    if (pitch != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, worldX, pitch);
        cam->right   = rotateAroundAxis(cam->right,   worldX, pitch);
        cam->up      = rotateAroundAxis(cam->up,      worldX, pitch);
    }

    if (roll != 0.0f) {
        cam->forward = rotateAroundAxis(cam->forward, worldZ, roll);
        cam->right   = rotateAroundAxis(cam->right,   worldZ, roll);
        cam->up      = rotateAroundAxis(cam->up,      worldZ, roll);
    }

    normalizeCamera(cam);
}

