#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "sb3d.h"





void meshSetDefaultMaterial(Mesh *mesh)
{
    if (!mesh) return;

    mesh->material.ambient          = 0.00f;
    mesh->material.diffuse          = 1.00f;
    mesh->material.emissive         = 0.00f;
    mesh->material.specularStrength = 0.00f;
    mesh->material.shininess        = 8.0f;
}

void meshSetMaterial(
    Mesh *mesh,
    float ambient,
    float diffuse,
    float emissive,
    float specularStrength,
    float shininess
)
{
    if (!mesh) return;

    mesh->material.ambient          = ambient;
    mesh->material.diffuse          = diffuse;
    mesh->material.emissive         = emissive;
    mesh->material.specularStrength = specularStrength;
    mesh->material.shininess        = shininess;
}
