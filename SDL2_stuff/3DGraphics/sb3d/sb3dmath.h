#ifndef _SIDBOX_3D_MATH_H_
#define _SIDBOX_3D_MATH_H_

#include "sb3dworld.h"






Vec3 vec3Add(Vec3 a, Vec3 b);
Vec3 vec3Sub(Vec3 a, Vec3 b);
Vec3 vec3Scale(Vec3 v, float s);
float vec3Dot(Vec3 a, Vec3 b);
Vec3 vec3Cross(Vec3 a, Vec3 b);
Vec3 vec3Normalize(Vec3 v);
Vec3 triangleCenter(Vec3 a, Vec3 b, Vec3 c);

Vec3 rotateAroundAxis(Vec3 v, Vec3 axis, float angle);








#endif