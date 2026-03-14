#include <stdint.h>
#include <math.h>


#include "sb3d.h"


Vec3 vec3Add(Vec3 a, Vec3 b)
{
    Vec3 out = { a.x + b.x, a.y + b.y, a.z + b.z };
    return out;
}

Vec3 vec3Sub(Vec3 a, Vec3 b)
{
    Vec3 out = { a.x - b.x, a.y - b.y, a.z - b.z };
    return out;
}

Vec3 vec3Scale(Vec3 v, float s)
{
    Vec3 out = { v.x * s, v.y * s, v.z * s };
    return out;
}

float vec3Dot(Vec3 a, Vec3 b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

Vec3 vec3Cross(Vec3 a, Vec3 b)
{
    Vec3 out = {
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x)
    };
    return out;
}

Vec3 vec3Normalize(Vec3 v)
{
    float len = sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));

    if (len <= 0.000001f) {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        return zero;
    }

    return vec3Scale(v, 1.0f / len);
}


Vec3 rotateAroundAxis(Vec3 v, Vec3 axis, float angle)
{
    axis = vec3Normalize(axis);

    float c = cosf(angle);
    float s = sinf(angle);

    Vec3 term1 = vec3Scale(v, c);
    Vec3 term2 = vec3Scale(vec3Cross(axis, v), s);
    Vec3 term3 = vec3Scale(axis, vec3Dot(axis, v) * (1.0f - c));

    return vec3Add(vec3Add(term1, term2), term3);
}




Vec3 triangleCenter(Vec3 a, Vec3 b, Vec3 c)
{
    Vec3 out;
    out.x = (a.x + b.x + c.x) / 3.0f;
    out.y = (a.y + b.y + c.y) / 3.0f;
    out.z = (a.z + b.z + c.z) / 3.0f;
    return out;
}
