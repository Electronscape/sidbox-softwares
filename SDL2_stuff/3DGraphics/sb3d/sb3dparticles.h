#ifndef _SIDBOX_3D_PARTICALS_H_
#define _SIDBOX_3D_PARTICALS_H_


#include <stdint.h>
#include "sb3dmath.h"
#include "sb3dcamera.h"


#ifndef SB3D_MAX_QUAD_PARTICLES
#define SB3D_MAX_QUAD_PARTICLES 256
#endif


// particle billboard stuff
typedef struct {
    Vec3 pos;
    float size;
    float shadeF;
    float lightStrength;
    uint8_t color;
    uint8_t emission;
    uint8_t active;
} SB3DQuadParticle;

void sb3dParticlesClear(void);

int sb3dParticleSpawnQuad(
    Vec3 pos,
    float size,
    uint8_t color,
    float shadeF,
    uint8_t emission,
    float lightstrength
);

void sb3dParticleSetPosition(int id, Vec3 pos);
void sb3dParticleSetSize(int id, float size);
void sb3dParticleSetShade(int id, float shadeF);
void sb3dParticleSetLightStrength(int id, float lightStrength);
void sb3dParticleSetColor(int id, uint8_t color);
void sb3dParticleSetEmission(int id, uint8_t emission);
void sb3dParticleEnable(int id, uint8_t enable);

void sb3dParticlesRender(const Camera *cam);



#endif