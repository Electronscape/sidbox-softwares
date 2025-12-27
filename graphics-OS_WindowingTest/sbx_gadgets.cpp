// nothing yet

#include <stdio.h>

#include <stddef.h>
#include "sbx_gadgets.h"
#include "sbx_windowex.h"

GADGET_BASE_T   GADGET_RESOURCE_POOL[MAX_GADGETS];

static GADGET_BASE_T* gad_alloc(void){
    for (int i = 0; i < MAX_GADGETS; i++){
        if (!GADGET_RESOURCE_POOL[i].gadgetSlotUsed){
            GADGET_RESOURCE_POOL[i].gadgetSlotUsed = 1;
            GADGET_RESOURCE_POOL[i].handleGen++;
            GADGET_RESOURCE_POOL[i].enabled = 1;
            GADGET_RESOURCE_POOL[i].visible = 1;
            GADGET_RESOURCE_POOL[i].down = 0;
            return &GADGET_RESOURCE_POOL[i];
        }
    }
    return NULL;
}


SBControlHandle SBOS_addButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags){
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    // find free gadget slot in window
    int slot = -1;
    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
        if (W->GADGETS[i] == NULL) { slot = i; break; }
    }
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = gad_alloc();
    if (!g) return SBCTL_INVALID;

    g->winhnd = win;
    g->gadgetType = GAD_BUTTON;
    g->rect.x = x; g->rect.y = y; g->rect.w = w; g->rect.h = h;

    // copy text
    int i = 0;
    if (text){
        for (; text[i] && i < DEF_GADGET_TEXT_SIZE-1; i++) g->text[i] = text[i];
    }
    g->text[i] = '\0';
    g->flags = flags;

    W->GADGETS[slot] = g;

    if((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM) ) {
        W->hasDockedGadget |= flags;
        printf("docked item\n");
    }

    // You’ll want a stable handle: “index in pool + generation”
    uint16_t idx = (uint16_t)(g - &GADGET_RESOURCE_POOL[0]);
    return SBCTL_MAKE(g->handleGen, idx);
}
