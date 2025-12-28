//// SBX_GADGETS.CPP //////

#include <string.h>
#include "sbx_gadgets.h"
#include "sbx_windowex.h"

// ---------------- POOLS ----------------
static GADGET_BASE_T    g_basePool[MAX_GADGETS];
static GAD_BUTTON_T     g_btnPool [MAX_BUTTONS];
static GAD_CHECKBOX_T   g_chkPool [MAX_CHECKBOXES];
static GAD_RADIO_T      g_radPool [MAX_RADIOS];



// ---------------- INTERNAL HELPERS ----------------
static int find_free_window_slot(sbx_window_t *w){
    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
        if (w->GADGETS[i] == NULL) return i;
    }
    return -1;
}

static GADGET_BASE_T* base_alloc(void){
    for (int i = 0; i < MAX_GADGETS; i++){
        if (!g_basePool[i].gadgetSlotUsed){
            g_basePool[i].gadgetSlotUsed = 1;
            g_basePool[i].handleGen++;
            return &g_basePool[i];
        }
    }
    return NULL;
}


// BITS FOR THE GADGETS //////////// the helpers /////////////

static void base_free(GADGET_BASE_T *g){
    if (!g) return;
    g->gadgetSlotUsed = 0;
    g->gadgetType = GAD_NULL;
    g->gadget = NULL;
    g->winhnd = 0;
}

////////////////// allocations area /////////////////////////

// ---------- button ---------------------
static GAD_BUTTON_T* btn_alloc(void){
    for (int i = 0; i < MAX_BUTTONS; i++){
        if (!g_btnPool[i].used){
            g_btnPool[i].used = 1;

            // sane defaults
            g_btnPool[i].h.enabled = 1;
            g_btnPool[i].h.visible = 1;
            g_btnPool[i].h.down = 0;
            g_btnPool[i].h.flags = GAD_TOOL_DEFAULT;

            g_btnPool[i].text[0] = '\0';
            return &g_btnPool[i];
        }
    }
    return NULL;
}

static GAD_CHECKBOX_T* chk_alloc(void){
    for (int i = 0; i < MAX_CHECKBOXES; i++){
        if (!g_chkPool[i].used){
            g_chkPool[i].used = 1;

            g_chkPool[i].h.enabled = 1;
            g_chkPool[i].h.visible = 1;
            g_chkPool[i].h.down    = 0;
            g_chkPool[i].h.flags   = GAD_TOOL_DEFAULT;

            g_chkPool[i].checked = 0;
            g_chkPool[i].text[0] = '\0';
            return &g_chkPool[i];
        }
    }
    return NULL;
}


static GAD_RADIO_T* rad_alloc(void){
    for (int i = 0; i < MAX_RADIOS; i++){
        if (!g_radPool[i].used){
            g_radPool[i].used = 1;

            g_radPool[i].h.enabled = 1;
            g_radPool[i].h.visible = 1;
            g_radPool[i].h.down    = 0;
            g_radPool[i].h.flags   = GAD_TOOL_DEFAULT;

            g_radPool[i].group   = 0;
            g_radPool[i].checked = 0;
            g_radPool[i].text[0] = '\0';
            return &g_radPool[i];
        }
    }
    return NULL;
}





/////////////////////////////////////////////////////////////////////////////////////////

// free gadget types


static void chk_free(GAD_CHECKBOX_T *c){
    if (!c) return;
    c->used = 0;
}

static void btn_free(GAD_BUTTON_T *b){
    if (!b) return;
    b->used = 0;
}

static void rad_free(GAD_RADIO_T *r){
    if (!r) return;
    r->used = 0;
}


//
// Convert a base pointer to stable handle (idx+gen)
static SBControlHandle base_to_handle(GADGET_BASE_T *g){
    uint16_t idx = (uint16_t)(g - &g_basePool[0]);
    return SBCTL_MAKE(g->handleGen, idx);
}




















// Validate handle -> base pointer (stale handle detection)
GADGET_BASE_T* SBOS_gadgetFromHandle(SBControlHandle h){
    if (h == SBCTL_INVALID) return NULL;

    uint16_t idx = SBCTL_IDX(h);
    uint16_t gen = SBCTL_GEN(h);

    if (idx >= MAX_GADGETS) return NULL;

    GADGET_BASE_T *g = &g_basePool[idx];
    if (!g->gadgetSlotUsed) return NULL;
    if (g->handleGen != gen) return NULL;

    return g;
}

// Handy: get header for hit-test / common flags
GAD_HDR_T* SBOS_gadgetHdr(GADGET_BASE_T *g){
    if (!g || !g->gadget) return NULL;
    return (GAD_HDR_T*)g->gadget; // safe because GAD_HDR_T is first in payload
}

// ---------------- PUBLIC API ----------------

void SBOS_gadgetsInit(void){
    memset(g_basePool, 0, sizeof(g_basePool));
    memset(g_btnPool,  0, sizeof(g_btnPool));
    memset(g_chkPool,  0, sizeof(g_chkPool));
    memset(g_radPool,  0, sizeof(g_radPool));
}

SBControlHandle SBOS_addButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags) {
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_BUTTON_T *b = btn_alloc();
    if (!b){
        base_free(g);
        return SBCTL_INVALID;
    }

    // base host
    g->winhnd = win;
    g->gadgetType = GAD_BUTTON;
    g->gadget = b;

    // payload header
    b->h.rect.x = x; b->h.rect.y = y; b->h.rect.w = w; b->h.rect.h = h;
    b->h.flags = flags;

    // payload data
    if (text){
        int i = 0;
        for (; text[i] && i < (DEF_GADGET_TEXT_SIZE - 1); i++) b->text[i] = text[i];
        b->text[i] = '\0';
    } else {
        b->text[0] = '\0';
    }

    // attach to window
    W->GADGETS[slot] = g;

    // window might need to know about docking later
    if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        W->hasDockedGadget |= flags;
    }

    return base_to_handle(g);
}


SBControlHandle SBOS_addCheckbox(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t initial_checked, GAD_TOOL_FLAGS flags) {
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_CHECKBOX_T *c = chk_alloc();
    if (!c){
        base_free(g);
        return SBCTL_INVALID;
    }

    g->winhnd = win;
    g->gadgetType = GAD_CHECKBOX;
    g->gadget = c;

    c->h.rect.x = x; c->h.rect.y = y; c->h.rect.w = w; c->h.rect.h = h;
    c->h.flags = flags;

    c->checked = initial_checked ? 1 : 0;

    if (text){
        int i = 0;
        for (; text[i] && i < (DEF_GADGET_TEXT_SIZE - 1); i++) c->text[i] = text[i];
        c->text[i] = '\0';
    } else {
        c->text[0] = '\0';
    }

    W->GADGETS[slot] = g;

    if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        W->hasDockedGadget |= flags;
    }

    return base_to_handle(g);
}



SBControlHandle SBOS_addRadioButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h,
                                    const char *text, uint8_t group, uint8_t checked, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_RADIO_T *r = rad_alloc();
    if (!r){
        base_free(g);
        return SBCTL_INVALID;
    }

    g->winhnd = win;
    g->gadgetType = GAD_RADIO;
    g->gadget = r;

    r->h.rect.x = x; r->h.rect.y = y; r->h.rect.w = w; r->h.rect.h = h;
    r->h.flags = flags;

    r->group = group;
    r->checked = checked ? 1 : 0;

    if (text){
        int i = 0;
        for (; text[i] && i < (DEF_GADGET_TEXT_SIZE - 1); i++) r->text[i] = text[i];
        r->text[i] = '\0';
    } else {
        r->text[0] = '\0';
    }

    W->GADGETS[slot] = g;

    if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        W->hasDockedGadget |= flags;
    }

    // Optional “only one checked per group” policy on add:
    if (r->checked) {
        for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
            GADGET_BASE_T *og = W->GADGETS[i];
            if (!og || og == g || og->gadgetType != GAD_RADIO) continue;
            GAD_RADIO_T *ort = (GAD_RADIO_T*)og->gadget;
            if (ort && ort->group == group) ort->checked = 0;
        }
    }

    return base_to_handle(g);
}




//////////////////// destroy gadgets ///////////////////////////////////////////////





// Optional: destroy gadget by handle
void SBOS_destroyGadget(SBControlHandle h){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return;

    // detach from window slots
    sbx_window_t *W = SBOS_getWindow(g->winhnd);
    if (W){
        for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
            if (W->GADGETS[i] == g){
                W->GADGETS[i] = NULL;
                break;
            }
        }
    }

    // free payload by type
    if (g->gadgetType == GAD_BUTTON)    btn_free((GAD_BUTTON_T*)g->gadget);
    if (g->gadgetType == GAD_CHECKBOX)  chk_free((GAD_CHECKBOX_T*)g->gadget);
    if (g->gadgetType == GAD_RADIO)     rad_free((GAD_RADIO_T*)g->gadget);


    base_free(g);
}




