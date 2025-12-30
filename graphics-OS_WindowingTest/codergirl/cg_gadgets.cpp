//// SBX_GADGETS.CPP //////

#include <stdio.h>
#include <string.h>
#include "cg_gadgets.h"
#include "cg_windowex.h"


// APIs
#include "cg_gad_button.h"
#include "cg_gad_checkbox.h"
#include "cg_gad_radio.h"
#include "cg_gad_bitmapview.h"
#include "cg_gad_scrollbar.h"
#include "cg_gad_listbox.h"


// ---------------- POOLS ----------------
static GADGET_BASE_T    g_basePool[MAX_GADGETS];
GAD_BUTTON_T     g_btnPool [MAX_BUTTONS];
GAD_CHECKBOX_T   g_chkPool [MAX_CHECKBOXES];
GAD_RADIO_T      g_radPool [MAX_RADIOS];
GAD_BITMAPVIEW_T g_bvPool  [MAX_BITMAPVIEWS];
GAD_SCROLLBAR_T  g_sbPool  [MAX_SCROLLBARS];
GAD_LISTBOX_T    g_lbPool  [MAX_LISTBOXES];



typedef struct {
    size_t basePool, btnPool, chkPool, radPool, sbPool, bvPool, lbPool;
} SBOS_GadgetPoolBytes;

SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void){
    SBOS_GadgetPoolBytes b;
    b.basePool = sizeof(g_basePool);
    b.btnPool  = sizeof(g_btnPool);
    b.chkPool  = sizeof(g_chkPool);
    b.radPool  = sizeof(g_radPool);
    b.sbPool   = sizeof(g_sbPool);
    b.bvPool   = sizeof(g_bvPool);
    b.lbPool   = sizeof(g_lbPool);
    return b;
}

uint32_t SBOS_GetBasePoolSize(){
    return sizeof(g_basePool[0]);
}


SBOS_UiUsageCounts SBOS_get_ui_usage_counts(void){
    SBOS_UiUsageCounts c = {0};
    // Windows in use
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (gui_used[i]) c.win_used++;
    }

    // Base gadget handles in use (depends on your base pool layout)
    // If GADGET_BASE_T has a "used" field, count it; otherwise skip.
    for (int i = 0; i < MAX_GADGETS; i++) {
        if (g_basePool[i].gadget != NULL) c.base_used++;   // <-- only if this exists
    }

    // Per-type gadget pools
    for (int i = 0; i < MAX_BUTTONS; i++)    if (g_btnPool[i].used) c.btn_used++;
    for (int i = 0; i < MAX_CHECKBOXES; i++) if (g_chkPool[i].used) c.chk_used++;
    for (int i = 0; i < MAX_RADIOS; i++)     if (g_radPool[i].used) c.rad_used++;
    for (int i = 0; i < MAX_SCROLLBARS; i++) if (g_sbPool[i].used)  c.sb_used++;
    for (int i = 0; i < MAX_BITMAPVIEWS; i++)if (g_bvPool[i].used)  c.bv_used++;
    for (int i = 0; i < MAX_LISTBOXES; i++)  if (g_lbPool[i].used)  c.lb_used++;

    return c;
}

// --------------- DEFAULT EventSubmitters -----------------------


void onScrollEmitEvent(void *s){
    GAD_SCROLLBAR_T *sb = (GAD_SCROLLBAR_T*)s;  // cast FIRST
    if (!sb) return;

    printf("From ScrB CB: %d\n", sb->value);
}











// ---------------- INTERNAL HELPERS ----------------
static inline int16_t clamp_i16_local(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/////// internal supports
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

//-------- LISTBOX ------------------------------------------
static GAD_LISTBOX_T* lb_alloc(void){
    for (int i = 0; i < MAX_LISTBOXES; i++){
        if (!g_lbPool[i].used){
            g_lbPool[i].used = 1;

            g_lbPool[i].h.enabled = 1;
            g_lbPool[i].h.visible = 1;
            g_lbPool[i].h.down    = 0;
            g_lbPool[i].h.flags   = GAD_TOOL_DEFAULT;

            g_lbPool[i].items = NULL;
            g_lbPool[i].row_h = 16;
            g_lbPool[i].padding_x = 4;
            g_lbPool[i].padding_y = 2;

            return &g_lbPool[i];
        }
    }
    return NULL;
}

static void lb_free(GAD_LISTBOX_T *lb){
    if (!lb) return;
    lb->used = 0;
    lb->items = NULL;
}


//-------- BITMAP VIEWS -------------------------------------
static GAD_BITMAPVIEW_T* bv_alloc(void){
    for (int i = 0; i < MAX_BITMAPVIEWS; i++){
        if (!g_bvPool[i].used){
            g_bvPool[i].used = 1;

            g_bvPool[i].h.enabled = 1;
            g_bvPool[i].h.visible = 1;
            g_bvPool[i].h.down    = 0;
            g_bvPool[i].h.flags   = GAD_TOOL_DEFAULT;

            // sensible defaults
            g_bvPool[i].pixels = NULL;
            g_bvPool[i].bmp_w = g_bvPool[i].bmp_h = 0;
            g_bvPool[i].bmp_stride = 0;
            g_bvPool[i].scroll_x = g_bvPool[i].scroll_y = 0;
            g_bvPool[i].panning = 0;
            g_bvPool[i].bv_flags = 0;

            return &g_bvPool[i];
        }
    }
    return NULL;
}

static void bv_free(GAD_BITMAPVIEW_T *bv){
    if (!bv) return;
    bv->used = 0;
}





//-------- BUTTONS ------------------------------------------
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

//-------- RADIO BUTTONS ------------------------------------
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


static GAD_SCROLLBAR_T* sb_alloc(void){
    for (int i = 0; i < MAX_SCROLLBARS; i++){
        if (!g_sbPool[i].used){
            g_sbPool[i].used = 1;

            g_sbPool[i].h.enabled = 1;
            g_sbPool[i].h.visible = 1;
            g_sbPool[i].h.down    = 0;
            g_sbPool[i].h.flags   = GAD_TOOL_DEFAULT;

            g_sbPool[i].orient = SB_ORIENT_VERT;
            g_sbPool[i].min = 0;
            g_sbPool[i].max = 100;
            g_sbPool[i].step = 1;

            g_sbPool[i].value = 0;

            g_sbPool[i].dragging = 0;
            g_sbPool[i].drag_off = 0;

            return &g_sbPool[i];
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


static void sb_free(GAD_SCROLLBAR_T *s){
    if (!s) return;
    s->used = 0;
}

//
// Convert a base pointer to stable handle (idx+gen)
SBControlHandle base_to_handle(GADGET_BASE_T *g){
    uint16_t idx = (uint16_t)(g - &g_basePool[0]);
    return SBCTL_MAKE(g->handleGen, idx);
}

GADGET_RECT_T r16(int16_t x, int16_t y, int16_t w, int16_t h){
    GADGET_RECT_T r = {x,y,w,h};
    return r;
}

uint8_t pt_in_r16(int16_t px, int16_t py, const GADGET_RECT_T *r){
    return (px >= r->x) && (py >= r->y) && (px < (int16_t)(r->x + r->w)) && (py < (int16_t)(r->y + r->h));
}


uint8_t gadget_mouse_inside(const sbx_window_t *w, const GADGET_BASE_T *g, int16_t mx, int16_t my){
    if (!w || !g || !g->gadget) return 0;

    int16_t lx = (int16_t)(mx - w->clientrect.x);
    int16_t ly = (int16_t)(my - w->clientrect.y);

    GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
    GADGET_RECT_T r = r16(h->rect.x, h->rect.y, h->rect.w, h->rect.h);
    return pt_in_r16(lx, ly, &r);
}


uint8_t mousept_in_rect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h){
    GADGET_RECT_T r = r16(x,y,w,h);
    return pt_in_r16(px,py,&r);
}

GADGET_BASE_T* hittest_gadget(sbx_window_t *w, int16_t mx, int16_t my){
    if (!w) return NULL;

    int16_t lx = (int16_t)(mx - w->clientrect.x);
    int16_t ly = (int16_t)(my - w->clientrect.y);

    for (int i = MAX_GADGETS_PER_WINDOW - 1; i >= 0; i--){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g || !g->gadget) continue;

        GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
        if (!h->visible || !h->enabled) continue;

        if (g->gadgetType == GAD_SCROLLBAR) {
            GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g->gadget;
            if (s->h.flags & (GAD_TOOL_DOCKED_RIGHT | GAD_TOOL_DOCKED_BOTTOM)) {
                int16_t ts=0, tl=0, tr=0;
                SBPart part = hittest_scrollbar_part(w, s, mx, my, &ts, &tl, &tr);
                if (part != SB_PART_NONE) return g;
                continue;
            }
        }

        GADGET_RECT_T r = r16(h->rect.x, h->rect.y, h->rect.w, h->rect.h);
        if (pt_in_r16(lx, ly, &r)) return g;
    }
    return NULL;
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
    memset(g_basePool, 0, sizeof(g_basePool));  // base gadgets
    memset(g_btnPool,  0, sizeof(g_btnPool));   // button gadgets
    memset(g_chkPool,  0, sizeof(g_chkPool));   // check box gadgets
    memset(g_radPool,  0, sizeof(g_radPool));   // radio buttons gadgets
    memset(g_sbPool,   0, sizeof(g_sbPool));    // scrollbar gadgets
    memset(g_bvPool,   0, sizeof(g_bvPool));    // bitmapview gadgets (THIS one is adventureous)
    memset(g_lbPool,   0, sizeof(g_lbPool));    // listbox gadgets
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

    b->current_option = 0;      // for now init the value to 0
    b->options[0] = b->text;    // first text;
    if (flags & GAD_TOOL_CYCLEBUTTON) {
        int scanIdx = 0;
        int optionCount = 1;  // Start counting from 1 because the first option is already set

        int strinLen = strlen(b->text); // Get the length of the text

        for (scanIdx = 0; scanIdx < strinLen; scanIdx++) {
            if (b->text[scanIdx] == '|') {  // Found a pipe (|)
                b->text[scanIdx] = '\0';    // Replace pipe with null terminator
                b->options[optionCount++] = &b->text[scanIdx + 1];  // Store pointer to the next option
            }
        }
        b->max_options = optionCount;
        printf("Found options; %d\n", optionCount);
    }


    // attach to window
    W->GADGETS[slot] = g;

    // window might need to know about docking later
    //if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        //W->flags |= flags;
    //}

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

    //if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        //W->hasDockedGadget |= flags;
    //}

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

    //if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
        //W->hasDockedGadget |= flags;
    //}

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


SBControlHandle SBOS_addScrollbar(SBXWindowId win,
                                  int16_t x, int16_t y, int16_t w, int16_t h,
                                  uint8_t orient,
                                  int16_t min, int16_t max,
                                  int16_t step,
                                  int16_t initial_pct,
                                  uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_SCROLLBAR_T *s = sb_alloc();
    if (!s){
        base_free(g);
        return SBCTL_INVALID;
    }

    // host base
    g->winhnd = win;
    g->gadgetType = GAD_SCROLLBAR;
    g->gadget = s;

    // containment field
    s->h.rect.x = x; s->h.rect.y = y; s->h.rect.w = w; s->h.rect.h = h;
    s->h.flags = flags;


    s->orient = orient;
    s->min = min;
    s->max = max;
    s->step = (step <= 0) ? 1 : step;
    s->value = initial_pct;
    s->onScrollCallBack = onScrollEmitEvent;

    // Set flag for arrows based on the provided flags
    s->show_arrows = (flags & GAD_TOOL_SCROLLARROWS) ? 1 : 0;

    W->GADGETS[slot] = g;

    // propagate dock flags into window flags so layoutWindow/layoutDockedControls knows
    if (flags & GAD_TOOL_DOCKED_RIGHT)  W->flags |= SBX_WF_DOCKRIGHT;
    if (flags & GAD_TOOL_DOCKED_BOTTOM) W->flags |= SBX_WF_DOCKBOTTOM;


    return base_to_handle(g);
}

SBControlHandle SBOS_addBitmapView(
    SBXWindowId win,
    int16_t x, int16_t y, int16_t w, int16_t h,
    const uint8_t *pixels, int16_t bmp_w, int16_t bmp_h, int16_t bmp_stride,
    uint32_t bv_flags,
    uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_BITMAPVIEW_T *bv = bv_alloc();
    if (!bv){
        base_free(g);
        return SBCTL_INVALID;
    }

    // base host
    g->winhnd = win;
    g->gadgetType = GAD_BITMAPVIEW;
    g->gadget = bv;

    // header
    bv->h.rect.x = x; bv->h.rect.y = y; bv->h.rect.w = w; bv->h.rect.h = h;
    bv->h.flags = flags;

    // payload
    bv->pixels = pixels;
    bv->bmp_w = bmp_w;
    bv->bmp_h = bmp_h;
    bv->bmp_stride = bmp_stride;

    bv->scroll_x = 0;
    bv->scroll_y = 0;
    bv->panning = 0;
    bv->bv_flags = bv_flags;

    // attach
    W->GADGETS[slot] = g;

    // (optional) docking propagation if you want it, same as scrollbar:
    if (flags & GAD_TOOL_DOCKED_RIGHT)  W->flags |= SBX_WF_DOCKRIGHT;
    if (flags & GAD_TOOL_DOCKED_BOTTOM) W->flags |= SBX_WF_DOCKBOTTOM;

    return base_to_handle(g);
}

SBControlHandle SBOS_addListBox(SBXWindowId win,
                                int16_t x, int16_t y, int16_t w, int16_t h,
                                ItemLists_t *items,
                                uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = base_alloc();
    if (!g) return SBCTL_INVALID;

    GAD_LISTBOX_T *lb = lb_alloc();
    if (!lb){ base_free(g); return SBCTL_INVALID; }

    g->winhnd = win;
    g->gadgetType = GAD_LISTBOX;
    g->gadget = lb;

    lb->h.rect.x = x; lb->h.rect.y = y; lb->h.rect.w = w; lb->h.rect.h = h;
    lb->h.flags = flags;

    lb->items = items;

    W->GADGETS[slot] = g;
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
    if (g->gadgetType == GAD_BUTTON)        btn_free((GAD_BUTTON_T*)     g->gadget);
    if (g->gadgetType == GAD_CHECKBOX)      chk_free((GAD_CHECKBOX_T*)   g->gadget);
    if (g->gadgetType == GAD_RADIO)         rad_free((GAD_RADIO_T*)      g->gadget);
    if (g->gadgetType == GAD_SCROLLBAR)     sb_free ((GAD_SCROLLBAR_T*)  g->gadget);
    if (g->gadgetType == GAD_BITMAPVIEW)    bv_free ((GAD_BITMAPVIEW_T*) g->gadget);
    if (g->gadgetType == GAD_LISTBOX)       lb_free((GAD_LISTBOX_T*)     g->gadget);





    base_free(g);
}




