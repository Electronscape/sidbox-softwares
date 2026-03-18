#ifndef _SIDBOX_3D_MATERIALS_H_
#define _SIDBOX_3D_MATERIALS_H_

#include <stdint.h>

// meshSetMaterial(&textMesh, 0.00f, 0.55f, 0.0f, 1.50f, 64.0f);   // shiny metal
/*
    Material guide:

    ambient          = base light even in darkness
    diffuse          = how strongly it reacts to light
    specularStrength = strength of highlight
    shininess        = size/tightness of highlight
                       low  = broad dull highlight
                       high = small sharp highlight
    emissive         = self-lit glow amount
*/

typedef struct {
    float ambient;           // 0.0 .. 1.0
    float diffuse;           // 0.0 .. 2.0
    float specularStrength;  // 0.0 .. 2.0
    float shininess;         // e.g. 4, 8, 16, 32
    float emissive;          // 0.0 .. 1.0
} Material;



/*
    Example materials:

    // dull matte / chalk / unpolished surface
    meshSetMaterial(&mesh, 0.10f, 0.90f, 0.00f, 0.00f, 4.0f);

    // plastic
    meshSetMaterial(&mesh, 0.08f, 0.85f, 0.25f, 8.0f, 0.00f);

    // glossy plastic
    meshSetMaterial(&mesh, 0.06f, 0.90f, 0.55f, 16.0f, 0.00f);

    // rubber / tyre
    meshSetMaterial(&mesh, 0.04f, 0.55f, 0.08f, 4.0f, 0.00f);

    // brushed metal
    meshSetMaterial(&mesh, 0.03f, 0.65f, 0.80f, 16.0f, 0.00f);

    // shiny metal
    meshSetMaterial(&mesh, 0.00f, 0.55f, 1.50f, 64.0f, 0.00f);

    // chrome / polished metal
    meshSetMaterial(&mesh, 0.00f, 0.45f, 2.00f, 96.0f, 0.00f);

    // dull stone
    meshSetMaterial(&mesh, 0.14f, 0.75f, 0.03f, 4.0f, 0.00f);

    // ceramic
    meshSetMaterial(&mesh, 0.10f, 0.80f, 0.35f, 12.0f, 0.00f);

    // glassy / crystal-ish fake
    meshSetMaterial(&mesh, 0.02f, 0.35f, 1.20f, 64.0f, 0.00f);

    // glowing panel / UI / engine light
    meshSetMaterial(&mesh, 0.00f, 0.20f, 0.00f, 4.0f, 0.55f);

    // strong emissive glow
    meshSetMaterial(&mesh, 0.00f, 0.10f, 0.00f, 4.0f, 1.00f);

    // dark spaceship hull
    meshSetMaterial(&mesh, 0.03f, 0.70f, 0.20f, 8.0f, 0.00f);

    // painted ship hull
    meshSetMaterial(&mesh, 0.06f, 0.95f, 0.30f, 12.0f, 0.00f);

    // old worn metal
    meshSetMaterial(&mesh, 0.05f, 0.70f, 0.35f, 8.0f, 0.00f);
*/






#endif