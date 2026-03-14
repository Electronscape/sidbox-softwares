#include <stdint.h>

#include "sb3d.h"


Entity worldEntities[WORLD_MAX];
int worldEntityCount = 0;

void worldClear(void)
{
    worldEntityCount = 0;
}