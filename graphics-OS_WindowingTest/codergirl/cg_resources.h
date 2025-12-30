#ifndef CG_RESOURCES_H
#define CG_RESOURCES_H

#include <stdint.h>
#include <stddef.h>
#include "cg_windowex.h"




typedef struct {
    size_t basePool, btnPool, chkPool, radPool, sbPool, bvPool, lbPool;
} SBOS_GadgetPoolBytes;

SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void);
void SBOS_print_reserved_ui_memory(void);

#endif // CG_RESOURCES_H
