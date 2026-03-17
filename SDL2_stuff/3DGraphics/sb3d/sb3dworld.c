#include <stdint.h>
#include <string.h>

#include "sb3d.h"


Entity worldEntities[WORLD_MAX];

void worldClear(void){
    memset(worldEntities, 0, sizeof(worldEntities));
}