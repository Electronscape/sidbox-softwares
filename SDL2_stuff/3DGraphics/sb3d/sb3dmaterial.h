#ifndef _SIDBOX_3D_MATERIALS_H_
#define _SIDBOX_3D_MATERIALS_H_

#include <stdint.h>


typedef struct {
    float ambient;           // 0.0 .. 1.0
    float diffuse;           // 0.0 .. 2.0
    float specularStrength;  // 0.0 .. 2.0
    float shininess;         // e.g. 4, 8, 16, 32
    float emissive;          // 0.0 .. 1.0
} Material;










#endif