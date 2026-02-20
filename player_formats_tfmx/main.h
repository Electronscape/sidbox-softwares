#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

#include "sidbox/players/player.h"

#ifdef __cplusplus
extern "C" {
#endif




uint16_t tfmx_be16(uint16_t v);
uint32_t tfmx_be32(uint32_t v);


extern int singleFile;
extern int dosExt;

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
