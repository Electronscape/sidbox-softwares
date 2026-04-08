#ifndef RC3D_EDIT_H
#define RC3D_EDIT_H

#include <stdint.h>

void rc3dEditInit(void);
void rc3dEditUpdate(float dt,
                    const uint8_t *keys,
                    int mouseX,
                    int mouseY,
                    uint32_t mouseButtons,
                    int mouseWheelY);
void rc3dEditRender(void);
int rc3dGuiCheckDirty();

#endif
