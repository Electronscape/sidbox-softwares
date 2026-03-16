#ifndef _SIDBOX_3D_FILE_LOADER_OBJ_H_
#define _SIDBOX_3D_FILE_LOADER_OBJ_H_

#include <stdint.h>
#include "sb3d.h"

int loadMeshOBJ(const char *filename, Mesh *mesh, uint8_t colour, float scale);
int loadMeshSB3D(const char *filename, Mesh *mesh, float scale);
void freeMesh(Mesh *mesh);



#endif