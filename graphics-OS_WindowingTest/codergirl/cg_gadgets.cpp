//// SBX_GADGETS.CPP //////

#include <stdio.h>
#include <string.h>
#include "cg_gadgets.h"
#include "cg_windowex.h"
#include "cg_msghandler.h"

// APIs
#include "cg_gad_bitmapview.h"
#include "cg_gad_button.h"
#include "cg_gad_canvas.h"
#include "cg_gad_checkbox.h"
#include "cg_gad_gridselect.h"
#include "cg_gad_label.h"
#include "cg_gad_listbox.h"
#include "cg_gad_progbar.h"
#include "cg_gad_radio.h"
#include "cg_gad_scrollbar.h"

#include "lib_filerequest.h"
#include "lib_msgbox.h"



// ---------------- POOLS ----------------
static GADGET_BASE_T g_basePool[MAX_GADGETS];
GAD_BITMAPVIEW_T g_bvPool   [MAX_BITMAPVIEWS];
GAD_BUTTON_T     g_btnPool  [MAX_BUTTONS];
GAD_CANVAS_T     g_cnPool   [MAX_CANVASES];
GAD_CHECKBOX_T   g_chkPool  [MAX_CHECKBOXES];
GAD_GRIDSELECT_T g_gsPool   [MAX_GRIDSELECTS];
GAD_LABEL_T      g_lblPool  [MAX_LABELS];
GAD_LISTBOX_T    g_lbPool   [MAX_LISTBOXES];
GAD_PROGBAR_T    g_pbPool   [MAX_PROGBARS];
GAD_RADIO_T      g_radPool  [MAX_RADIOS];
GAD_SCROLLBAR_T  g_sbPool   [MAX_SCROLLBARS];






typedef struct {
    size_t basePool, btnPool, cnPool, chkPool, radPool, sbPool, bvPool, lbPool, lblPool, pbPool, gsPool, msgPool, frqPool;
} SBOS_GadgetPoolBytes;

SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void){
    SBOS_GadgetPoolBytes b;
    b.basePool = sizeof(g_basePool);
    b.btnPool  = sizeof(g_btnPool);
    b.chkPool  = sizeof(g_chkPool);
    b.cnPool   = sizeof(g_cnPool);
    b.radPool  = sizeof(g_radPool);
    b.sbPool   = sizeof(g_sbPool);
    b.bvPool   = sizeof(g_bvPool);
    b.lbPool   = sizeof(g_lbPool);
    b.lblPool  = sizeof(g_lblPool);
    b.pbPool   = sizeof(g_pbPool);
    b.gsPool   = sizeof(g_gsPool);


    b.msgPool = SBOS_msgbox_poolsize();
    b.frqPool = SBOS_filerq_poolsize();

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
        //if (g_basePool[i].gadget != NULL) c.base_used++;   // <-- only if this exists
        if (g_basePool[i].gadgetSlotUsed) c.base_used++;
    }

    // Per-type gadget pools
    for (int i = 0; i < MAX_BUTTONS; i++)     if (g_btnPool[i].used)  c.btn_used++;
    for (int i = 0; i < MAX_CHECKBOXES; i++)  if (g_chkPool[i].used)  c.chk_used++;
    for (int i = 0; i < MAX_RADIOS; i++)      if (g_radPool[i].used)  c.rad_used++;
    for (int i = 0; i < MAX_SCROLLBARS; i++)  if (g_sbPool[i].used)   c.sb_used++;
    for (int i = 0; i < MAX_BITMAPVIEWS; i++) if (g_bvPool[i].used)   c.bv_used++;
    for (int i = 0; i < MAX_LABELS; i++)      if (g_lblPool[i].used)  c.lbl_used++;
    for (int i = 0; i < MAX_LISTBOXES; i++)   if (g_lbPool[i].used)   c.lb_used++;
    for (int i = 0; i < MAX_PROGBARS; i++)    if (g_pbPool[i].used)   c.pb_used++;

    for (int i = 0; i < MAX_GRIDSELECTS; i++) if (g_gsPool[i].used)   c.gs_used++;
    for (int i = 0; i < MAX_CANVASES; i++)    if (g_cnPool[i].used)   c.cn_used++;

    c.filerq_used = SBOS_filerq_used_count();
    c.msgbox_used = SBOS_msgbox_used_count();

    //c.filerq_cap  = SBOS_filerq_capacity();
    //c.msgbox_cap  = SBOS_msgbox_capacity();

    return c;
}



/////// DEFAULT EventEmitters /////////////////////////////////////////////////////////////////

void onButtonClickEmitEvent(void *g){
    GAD_BUTTON_T *bt = (GAD_BUTTON_T *)g;
    if (!bt) return;
    CG_PostGadgetMsg(bt->h.winhnd, bt->h.self, CGEVT_GAD_BUTTON_HIT, bt->current_option, 0, 0, 0);
}

void onCheckBoxClickEmitEvent(void *g){
    GAD_CHECKBOX_T *cb = (GAD_CHECKBOX_T *)g;   // get the button
    if(!cb) return;
    CG_PostGadgetMsg(cb->h.winhnd, cb->h.self, CGEVT_GAD_CHECK_CHANGED, cb->checked ? 1 : 0, 0, 0, 0);
}

void onGridSelectEmitEvent(void *g){
    GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T*)g;
    if (!gs) return;
    CG_PostGadgetMsg(gs->h.winhnd, gs->h.self, CGEVT_GAD_GRIDSEL_CHANGED, gs->selected_idx, 0, 0, 0);
}

void onListBoxEmitEvent(void *g){
    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g;
    if (!lb) return;
    CG_PostGadgetMsg(lb->h.winhnd, lb->h.self, CGEVT_GAD_LISTBOX_CHANGED, lb->sel, lb->top, 0, 0);
}

void onRadioClickEmitEvent(void *g){
    GAD_RADIO_T *ra = (GAD_RADIO_T *)g;   // get the button
    if(!ra) return;
    CG_PostGadgetMsg(ra->h.winhnd, ra->h.self, CGEVT_GAD_RADIO_CHANGED, ra->checked ? 1 : 0, ra->group, 0, 0);
}

void onScrollEmitEvent(void *g){
    GAD_SCROLLBAR_T *sb = (GAD_SCROLLBAR_T*)g;  // cast FIRST
    if (!sb) return;
    CG_PostGadgetMsg(sb->h.winhnd, sb->h.self, CGEVT_GAD_SCROLL_CHANGED, sb->value, 0, 0, 0);
}


// ---------------- INTERNAL HELPERS ----------------

/////// internal supports
static int find_free_window_slot(sbx_window_t *w){
    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
        if (w->GADGETS[i] == NULL) return i;
    }
    SBOS_EmergencyError("Out of gadget slots for window");
    return -1;
}

static GADGET_BASE_T* alloc_base(void){
    for (uint16_t i = 0; i < MAX_GADGETS; i++){
        if (!g_basePool[i].gadgetSlotUsed){
            GADGET_BASE_T *g = &g_basePool[i];
            g->gadgetSlotUsed = 1;
            g->handleGen++;          // new lifetime
            if (g_basePool[i].handleGen == 0) g_basePool[i].handleGen = 1;
            g->slotIndex = i;        // <-- key for Option B
            return g;
        }
    }
    return NULL;
}


// BITS FOR THE GADGETS //////////// the helpers /////////////
/*
static void base_free(GADGET_BASE_T *g){
    if (!g) return;
    g->gadgetSlotUsed = 0;
    g->gadgetType = GAD_NULL;
    g->gadget = NULL;
    //g->winhnd = 0;
}
*/
static void free_base(GADGET_BASE_T *g){
    if (!g) return;

    g->handleGen++;             //invalidate old handles NOW
    g->gadgetSlotUsed = 0;
    g->gadgetType = GAD_NULL;
    g->gadget = NULL;
}

////////////////// allocations area /////////////////////////

static GAD_BITMAPVIEW_T* alloc_bv(void){
    for (int i = 0; i < MAX_BITMAPVIEWS; i++){
        if (!g_bvPool[i].used){
            memset(&g_bvPool[i], 0, sizeof(g_bvPool[i]));
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

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: bitmapView");
    return NULL;
}

static GAD_BUTTON_T* alloc_btn(void){
    for (int i = 0; i < MAX_BUTTONS; i++){
        if (!g_btnPool[i].used){
            memset(&g_btnPool[i], 0, sizeof(g_btnPool[i]));

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

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: buttons");
    return NULL;
}

static GAD_CANVAS_T* alloc_canv(void){
    for (int i = 0; i < MAX_CANVASES; i++){
        if (!g_cnPool[i].used){
            memset(&g_cnPool[i], 0, sizeof(g_cnPool[i]));

            g_cnPool[i].used = 1;

            // sane defaults
            g_cnPool[i].h.enabled = 1;
            g_cnPool[i].h.visible = 1;
            g_cnPool[i].h.down = 0;
            g_cnPool[i].h.flags = GAD_TOOL_DEFAULT;

            return &g_cnPool[i];
        }
    }

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: canvas");
    return NULL;
}



static GAD_CHECKBOX_T* alloc_chk(void){
    for (int i = 0; i < MAX_CHECKBOXES; i++){
        if (!g_chkPool[i].used){
            memset(&g_chkPool[i], 0, sizeof(g_chkPool[i]));
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
    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: checkBox");
    return NULL;
}


static GAD_GRIDSELECT_T* alloc_gsl(void){
    for (int i = 0; i < MAX_GRIDSELECTS; i++){
        if (!g_gsPool[i].used){
            memset(&g_gsPool[i], 0, sizeof(g_gsPool[i]));
            g_gsPool[i].used = 1;

            g_gsPool[i].h.enabled = 1;
            g_gsPool[i].h.visible = 1;
            g_gsPool[i].h.down    = 0;
            g_gsPool[i].h.flags   = GAD_TOOL_DEFAULT;
            g_gsPool[i].flags     = GAD_GRIDSEL_JUST_ONE;

            return &g_gsPool[i];
        }
    }

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: gridSelect");
    return NULL;
}

static GAD_LABEL_T* alloc_lbl(void){
    for (int i = 0; i < MAX_LABELS; i++){
        if (!g_lblPool[i].used){
            memset(&g_lblPool[i], 0, sizeof(g_lblPool[i]));
            g_lblPool[i].used = 1;

            // sane defaults
            g_lblPool[i].h.enabled = 1;
            g_lblPool[i].h.visible = 1;
            g_lblPool[i].h.down = 0;
            g_lblPool[i].h.flags = GAD_TOOL_DEFAULT;

            g_lblPool[i].text[0] = '\0';
            return &g_lblPool[i];
        }
    }
    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: labels");
    return NULL;
}

static GAD_LISTBOX_T* alloc_lb(void){
    for (int i = 0; i < MAX_LISTBOXES; i++){
        if (!g_lbPool[i].used){
            memset(&g_lbPool[i], 0, sizeof(g_lbPool[i]));
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
    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: listBox");
    return NULL;
}


static GAD_PROGBAR_T* alloc_pb(void){
    for (int i = 0; i < MAX_PROGBARS; i++){
        if (!g_pbPool[i].used){
            memset(&g_pbPool[i], 0, sizeof(g_pbPool[i]));
            g_pbPool[i].used = 1;

            g_pbPool[i].h.enabled = 1;
            g_pbPool[i].h.visible = 1;
            g_pbPool[i].h.down    = 0;
            g_pbPool[i].h.flags   = GAD_TOOL_DEFAULT;

            return &g_pbPool[i];
        }
    }

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: progressBar");
    return NULL;
}


static GAD_RADIO_T* alloc_rad(void){
    for (int i = 0; i < MAX_RADIOS; i++){
        if (!g_radPool[i].used){
            memset(&g_radPool[i], 0, sizeof(g_radPool[i]));
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

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: radioButton");
    return NULL;
}


static GAD_SCROLLBAR_T* alloc_sb(void){
    for (int i = 0; i < MAX_SCROLLBARS; i++){
        if (!g_sbPool[i].used){
            memset(&g_sbPool[i], 0, sizeof(g_sbPool[i]));
            g_sbPool[i].used = 1;

            g_sbPool[i].h.enabled = 1;
            g_sbPool[i].h.visible = 1;
            g_sbPool[i].h.down    = 0;
            g_sbPool[i].h.flags   = GAD_TOOL_DEFAULT;

            g_sbPool[i].orient = SB_ORIENT_VERT;
            g_sbPool[i].min = 0;
            g_sbPool[i].max = 100;
            g_sbPool[i].step_large = 1;
            g_sbPool[i].step_small = 1;

            g_sbPool[i].value = 0;

            g_sbPool[i].dragging = 0;
            g_sbPool[i].drag_off = 0;

            return &g_sbPool[i];
        }
    }

    SBOS_EmergencyError("Out of gadget slots:\n"
                        "Resource: scrollBar");
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////

// free gadget types


static void free_bv(GAD_BITMAPVIEW_T *bv){
    if (!bv) return;
    bv->used = 0;
}

static void free_btn(GAD_BUTTON_T *b){
    if (!b) return;
    b->used = 0;
}

static void free_canv(GAD_CANVAS_T *b){
    if (!b) return;
    b->used = 0;
}


static void free_chk(GAD_CHECKBOX_T *c){
    if (!c) return;
    c->used = 0;
}

static void free_gs(GAD_GRIDSELECT_T *c){
    if (!c) return;
    c->used = 0;
}

static void free_lbl(GAD_LABEL_T *lb){
    if (!lb) return;
    lb->used = 0;
}

static void free_pb(GAD_PROGBAR_T *pb){
    if (!pb) return;
    pb->used = 0;
}


static void free_lb(GAD_LISTBOX_T *lb){
    // Listbox's dont OWN the list attached.
    if (!lb) return;
    lb->items = NULL;   // detach; owner frees the list
    lb->used = 0;
    /*
    if (!lb) return;
    if (lb->items) {
        SBOS_destroyItemList(lb->items);
        lb->items = NULL;
    }
    lb->used = 0;
    */
}

static void free_rad(GAD_RADIO_T *r){
    if (!r) return;
    r->used = 0;
}

static void free_sb(GAD_SCROLLBAR_T *s){
    if (!s) return;
    s->used = 0;
}


//
// Convert a base pointer to stable handle (idx+gen)
/*
CGGadgetHandle base_to_handle(GADGET_BASE_T *g){
    uint16_t idx = (uint16_t)(g - &g_basePool[0]);
    return SBCTL_MAKE(g->handleGen, idx);
}
*/

/*
CGGadgetHandle base_to_handle(GADGET_BASE_T *g){
    if (!g) return SBCTL_INVALID;

    if (g < &g_basePool[0] || g >= &g_basePool[MAX_GADGETS]) {
        return SBCTL_INVALID; // not from base pool -> don't mint nonsense
    }

    uint16_t idx = (uint16_t)(g - &g_basePool[0]);
    return SBCTL_MAKE(g->handleGen, idx);
}
*/

CGGadgetHandle base_to_handle(GADGET_BASE_T *g){
    if (!g) return SBCTL_INVALID;
    if (!g->gadgetSlotUsed) return SBCTL_INVALID;  // optional paranoia
    return SBCTL_MAKE(g->handleGen, g->slotIndex);
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
GADGET_BASE_T* SBOS_gadgetFromHandle(CGGadgetHandle h){
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

// keep for tight loops
static inline GAD_HDR_T* gad_hdr(GADGET_BASE_T *b){
    return (b && b->gadget) ? (GAD_HDR_T*)b->gadget : NULL;
}


// global gadget tools
void SBOS_moveGadget(CGGadgetHandle hnd, int16_t newx, int16_t newy){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(hnd);
    GAD_HDR_T *gad = SBOS_gadgetHdr(g);

    gad->rect.x = newx;
    gad->rect.y = newy;
}

void SBOS_resizeGadget(CGGadgetHandle hnd, int16_t neww, int16_t newh){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(hnd);
    GAD_HDR_T *gad = SBOS_gadgetHdr(g);

    gad->rect.w = neww;
    gad->rect.h = newh;
}

void SBOS_setGadgetFPen(CGGadgetHandle hnd, uint8_t fpen){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(hnd);
    GAD_HDR_T *gad = SBOS_gadgetHdr(g);

    gad->FPen = fpen;
}

void SBOS_setGadgetBPen(CGGadgetHandle hnd, uint8_t bpen){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(hnd);
    GAD_HDR_T *gad = SBOS_gadgetHdr(g);

    gad->BPen = bpen;
}

void SBOS_setGadgetHPen(CGGadgetHandle hnd, uint8_t hpen){  // select colour pen
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(hnd);
    GAD_HDR_T *gad = SBOS_gadgetHdr(g);

    gad->HPen = hpen;
}



// ---------------- PUBLIC API ----------------
void SBOS_gadgetsInit(void){
    memset(g_basePool, 0, sizeof(g_basePool));  // base gadgets ________________________________

    memset(g_bvPool ,  0, sizeof( g_bvPool  ));   // bitmapview gadgets (THIS one is adventureous)
    memset(g_btnPool,  0, sizeof( g_btnPool ));   // button gadgets
    memset(g_chkPool,  0, sizeof( g_chkPool ));   // check box gadgets
    memset(g_gsPool ,  0, sizeof( g_gsPool  ));   // grid select gadgets
    memset(g_lblPool,  0, sizeof( g_lblPool ));   // labels gadgets
    memset(g_lbPool ,  0, sizeof( g_lbPool  ));   // listbox gadgets
    memset(g_pbPool ,  0, sizeof( g_pbPool  ));   // progbar gadgets
    memset(g_radPool,  0, sizeof( g_radPool ));   // radio buttons gadgets
    memset(g_sbPool ,  0, sizeof( g_sbPool  ));   // scrollbar gadgets
}


CGGadgetHandle SBOS_CreateBitmapView(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *pixels, int16_t bmp_w, int16_t bmp_h, int16_t bmp_stride, uint32_t bv_flags, uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_BITMAPVIEW_T *bv = alloc_bv();
    if (!bv){
        free_base(g);
        return SBCTL_INVALID;
    }

    // base host
    //g->winhnd = win;
    g->gadgetType = GAD_BITMAPVIEW;
    g->gadget = bv;

    // header
    bv->h.rect.x = x; bv->h.rect.y = y; bv->h.rect.w = w; bv->h.rect.h = h;
    bv->h.flags = flags;
    bv->h.winhnd = win;
    bv->h.BPen = PEN_WIN_BG;
    bv->h.FPen = PEN_TEXT;

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

    CGGadgetHandle gHndle = base_to_handle(g);
    bv->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

CGGadgetHandle SBOS_CreateButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_BUTTON_T *b = alloc_btn();
    if (!b){
        free_base(g);
        return SBCTL_INVALID;
    }

    // base host
    //g->winhnd = win;
    g->gadgetType = GAD_BUTTON;
    g->gadget = b;
    b->h.callbackRouteA = onButtonClickEmitEvent;  // basic button clicky
    b->h.winhnd = win;
    b->h.BPen = PEN_BUTTON_FACE;
    b->h.FPen = PEN_TEXT;

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

    CGGadgetHandle gHndle = base_to_handle(g);
    b->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;

}

CGGadgetHandle SBOS_CreateCanvas(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, CNV_FLAGS_T drawtype, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_CANVAS_T *b = alloc_canv();
    if (!b){
        free_base(g);
        return SBCTL_INVALID;
    }

    // base host
    //g->winhnd = win;
    g->gadgetType = GAD_CANVAS;
    g->gadget = b;
    //b->h.callbackRouteA = onButtonClickEmitEvent;  // basic button clicky
    b->h.winhnd = win;
    b->h.BPen = PEN_WIN_BG;
    b->h.FPen = PEN_TEXT;

    // payload header
    b->h.rect.x = x; b->h.rect.y = y; b->h.rect.w = w; b->h.rect.h = h;
    b->h.flags = flags;
    b->drawtype = drawtype;

    // attach to window
    W->GADGETS[slot] = g;

    // window might need to know about docking later
    //if ((flags & GAD_TOOL_DOCKED_RIGHT) || (flags & GAD_TOOL_DOCKED_BOTTOM)) {
    //W->flags |= flags;
    //}

    CGGadgetHandle gHndle = base_to_handle(g);
    b->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

CGGadgetHandle SBOS_CreateCheckbox(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t initial_checked, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_CHECKBOX_T *c = alloc_chk();
    if (!c){
        free_base(g);
        return SBCTL_INVALID;
    }

    //g->winhnd = win;
    g->gadgetType = GAD_CHECKBOX;
    g->gadget = c;

    c->h.rect.x = x; c->h.rect.y = y; c->h.rect.w = w; c->h.rect.h = h;
    c->h.flags = flags;
    c->h.winhnd = win;
    c->h.BPen = PEN_CHECKBOX_FACE;
    c->h.FPen = PEN_TEXT;
    c->h.HPen = PEN_WIN_BORDER_ACTIVE;
    c->checked = initial_checked ? 1 : 0;

    c->h.callbackRouteA = onCheckBoxClickEmitEvent;

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

    CGGadgetHandle gHndle = base_to_handle(g);
    c->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

CGGadgetHandle SBOS_CreateGridSelect(SBXWindowId win, int16_t x, int16_t y, int16_t cell_size_x, int16_t cell_size_y, uint8_t cells_x, uint8_t cells_y, uint32_t gridflags, GAD_TOOL_FLAGS flags)
{

    // check before we buy!
    uint16_t cellcount = (uint16_t)cells_x * (uint16_t)cells_y;
    if (cellcount > 256) return SBCTL_INVALID;


    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_GRIDSELECT_T *c = alloc_gsl();
    if (!c){
        free_base(g);
        return SBCTL_INVALID;
    }

    c->h.BPen = PEN_WIN_BG;
    c->h.FPen = PEN_TEXT;
    c->h.HPen = PEN_WIN_BORDER_ACTIVE;

    for(int i = 0; i < 256; i++){
        c->cellColour[i] = c->h.BPen;   // default all to window background
        memset(c->cellText[i], 0x00, c->h.FPen);
    }

    g->gadgetType = GAD_GRIDSELECT;
    g->gadget = c;
    c->h.callbackRouteA = onGridSelectEmitEvent;

    int16_t border = 2;
    if(flags & GAD_TOOL_NOBORDER)
        border = 0;

    c->flags = gridflags;

    c->h.rect.x = x; c->h.rect.y = y; c->h.rect.w = cell_size_x * cells_x + border; c->h.rect.h = cell_size_y * cells_y + border;
    c->h.flags = flags;
    c->h.winhnd = win;

    c->down_idx  = -1;
    c->selected_idx = -1;

    c->cells_x = cells_x;
    c->cells_y = cells_y;

    W->GADGETS[slot] = g;

    CGGadgetHandle gHndle = base_to_handle(g);
    c->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

CGGadgetHandle SBOS_CreateLabel(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_LABEL_T *b = alloc_lbl();
    if (!b){
        free_base(g);
        return SBCTL_INVALID;
    }

    // base host
    //g->winhnd = win;
    g->gadgetType = GAD_LABEL;
    g->gadget = b;
    b->h.winhnd = win;
    b->h.BPen = W->backColour;   // in herrite the background window colour
    b->h.FPen = PEN_WIN_TITLE;

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

    CGGadgetHandle gHndle = base_to_handle(g);
    b->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

// NOTE: items is caller-owned. ListBox will NOT free it.
// Caller must keep it alive while ListBox is using it and free it after destroying the ListBox/window.
CGGadgetHandle SBOS_CreateListBox(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, ItemLists_t *items, uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_LISTBOX_T *lb = alloc_lb();
    if (!lb){ free_base(g); return SBCTL_INVALID; }

    //g->winhnd = win;
    g->gadgetType = GAD_LISTBOX;
    g->gadget = lb;


    lb->h.rect.x = x; lb->h.rect.y = y; lb->h.rect.w = w; lb->h.rect.h = h;
    lb->h.flags = flags;
    lb->h.winhnd = win;
    lb->h.BPen = PEN_WIN_BG;
    lb->h.FPen = PEN_TEXT;
    lb->h.HPen = PEN_WIN_BORDER_ACTIVE;


    lb->items = items;
    lb->row_h = DEF_LISTBOX_TEXT_HEIGHT;

    lb->h.callbackRouteA = onListBoxEmitEvent;

    W->GADGETS[slot] = g;

    CGGadgetHandle gHndle = base_to_handle(g);
    lb->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}


CGGadgetHandle SBOS_CreateProgBar(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_PROGBAR_T *b = alloc_pb();
    if (!b){
        free_base(g);
        return SBCTL_INVALID;
    }

    // base host
    //g->winhnd = win;
    g->gadgetType = GAD_PROGBAR;
    g->gadget = b;
    b->h.callbackRouteA = NULL;  // shouldnt ever HAVE a call back
    b->h.winhnd = win;
    b->h.BPen = PEN_WIN_BG;
    b->h.FPen = PEN_WIN_BORDER_ACTIVE;


    // payload header
    b->h.rect.x = x; b->h.rect.y = y; b->h.rect.w = w; b->h.rect.h = h;
    b->h.flags = flags;

    // attach to window
    W->GADGETS[slot] = g;

    CGGadgetHandle gHndle = base_to_handle(g);
    b->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;

}


CGGadgetHandle SBOS_CreateRadioButton(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, const char *text, uint8_t group, uint8_t checked, GAD_TOOL_FLAGS flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_RADIO_T *r = alloc_rad();
    if (!r){
        free_base(g);
        return SBCTL_INVALID;
    }

    //g->winhnd = win;
    g->gadgetType = GAD_RADIO;
    g->gadget = r;


    r->h.winhnd = win;
    r->h.BPen = PEN_CHECKBOX_FACE;
    r->h.FPen = PEN_TEXT;
    r->h.HPen = PEN_WIN_BORDER_ACTIVE;

    r->h.rect.x = x; r->h.rect.y = y; r->h.rect.w = w; r->h.rect.h = h;
    r->h.flags = flags;
    r->h.callbackRouteA = onRadioClickEmitEvent;


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

    CGGadgetHandle gHndle = base_to_handle(g);
    r->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}

CGGadgetHandle SBOS_CreateScrollbar(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t orient, int16_t min, int16_t max, int16_t step_small, int16_t step_large, uint32_t flags)
{
    sbx_window_t *W = SBOS_getWindow(win);
    if (!W) return SBCTL_INVALID;

    int slot = find_free_window_slot(W);
    if (slot < 0) return SBCTL_INVALID;

    GADGET_BASE_T *g = alloc_base();
    if (!g) return SBCTL_INVALID;

    GAD_SCROLLBAR_T *s = alloc_sb();
    if (!s){
        free_base(g);
        return SBCTL_INVALID;
    }

    // host base
    //g->winhnd = win;
    g->gadgetType = GAD_SCROLLBAR;
    g->gadget = s;

    // containment field
    s->h.rect.x = x; s->h.rect.y = y; s->h.rect.w = w; s->h.rect.h = h;
    s->h.flags = flags;
    s->h.winhnd = win;
    // doubt this will used, but set anyway
    s->h.BPen = PEN_WIN_BG;
    s->h.FPen = PEN_TEXT;
    s->h.HPen = PEN_WIN_BORDER_ACTIVE;


    s->orient = orient;
    s->min = min;
    s->max = max;
    s->step_large = (step_large <= 0) ? 1 : step_large;
    s->step_small = (step_small <= 0) ? 1 : step_small;

    if (step_small > step_large-1)  // more a rule of the OS than being "right"
        s->step_small /= 2;

    s->value = 0;
    s->h.callbackRouteA = onScrollEmitEvent;

    // Set flag for arrows based on the provided flags
    s->show_arrows = (flags & GAD_TOOL_SCROLLARROWS) ? 1 : 0;

    W->GADGETS[slot] = g;

    // propagate dock flags into window flags so layoutWindow/layoutDockedControls knows
    if (flags & GAD_TOOL_DOCKED_RIGHT)  W->flags |= SBX_WF_DOCKRIGHT;
    if (flags & GAD_TOOL_DOCKED_BOTTOM) W->flags |= SBX_WF_DOCKBOTTOM;


    CGGadgetHandle gHndle = base_to_handle(g);
    s->h.self = gHndle;   // for gridselect/label/button etc.
    return gHndle;
}


uint32_t commitGadgetRelease(sbx_window_t *gw, GADGET_BASE_T *g)
{
    switch (g->gadgetType) {
    case GAD_BUTTON: {
        GAD_BUTTON_T *b = (GAD_BUTTON_T*) g->gadget;

        b->current_option++;
        if (b->current_option > b->max_options - 1)
            b->current_option = 0;

        //printf("BUTTON CLICK: %s (cycle %d)\r\n", b->text,
        //b->current_option);
        if(b->h.callbackRouteA)
            b->h.callbackRouteA(b);
    }
    break;
    case GAD_CHECKBOX: {
        GAD_CHECKBOX_T *c = (GAD_CHECKBOX_T*) g->gadget;
        c->checked ^= 1;
        //printf("CHECKBOX TOGGLE: %s => %d\r\n", c->text, c->checked);
        if(c->h.callbackRouteA)
            c->h.callbackRouteA(c);  // call any function attached

    }
    break;
    case GAD_RADIO: {
        GAD_RADIO_T *r = (GAD_RADIO_T*) g->gadget;
        if (gw) {
            // clear all radios in same group in this window
            for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++) {
                GADGET_BASE_T *og = gw->GADGETS[i];
                if (!og || og->gadgetType != GAD_RADIO
                    || !og->gadget)
                    continue;

                GAD_RADIO_T *ort = (GAD_RADIO_T*) og->gadget;
                if (ort->group == r->group)
                    ort->checked = 0;
            }
            r->checked = 1;
        }
        if(r->h.callbackRouteA)
            r->h.callbackRouteA(r);
    }
    break;

    case GAD_LISTBOX: {
        // this should be handled on the onMouseReleaseListBox
    }
    break;

    case GAD_GRIDSELECT: {
        GAD_GRIDSELECT_T *gs = (GAD_GRIDSELECT_T *) g->gadget;
        if (gw) {
            if(gs->h.callbackRouteA){
                gs->h.callbackRouteA(gs);
            }
        }
    }


    default:
        break;
    }
    return (0x00);/// this doesnt really need to worry about anything
}


void SBOS_enableGadget(CGGadgetHandle h, uint8_t enable){
    GADGET_BASE_T *g = SBOS_gadgetFromHandle(h);
    if (!g) return;

    GAD_HDR_T *hdr = SBOS_gadgetHdr(g);
    if (!hdr) return;

    hdr->enabled = !!enable;
    if (!hdr->enabled)
        hdr->down = 0;   // prevent stuck-pressed visuals

    // eventually have paintGadget(g);

}


//////////////////// destroy gadgets ///////////////////////////////////////////////




void SBOS_destroyGadget(CGGadgetHandle h){
    GADGET_BASE_T *b = SBOS_gadgetFromHandle(h);
    if (!b || !b->gadget) return;

    GAD_HDR_T *hdr = (GAD_HDR_T*)b->gadget;   // requires "header is first field" rule
    sbx_window_t *W = SBOS_getWindow(hdr->winhnd);

    // detach from window slots
    if (W){
        for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
            if (W->GADGETS[i] == b){
                W->GADGETS[i] = NULL;
                break;
            }
        }
    }

    // free payload by type
    switch (b->gadgetType){
        case GAD_BITMAPVIEW: free_bv  ((GAD_BITMAPVIEW_T* )b->gadget); break;
        case GAD_BUTTON:     free_btn ((GAD_BUTTON_T*     )b->gadget); break;
        case GAD_CANVAS:     free_canv((GAD_CANVAS_T*     )b->gadget); break;
        case GAD_CHECKBOX:   free_chk ((GAD_CHECKBOX_T*   )b->gadget); break;
        case GAD_GRIDSELECT: free_gs  ((GAD_GRIDSELECT_T* )b->gadget); break;
        case GAD_LABEL:      free_lbl ((GAD_LABEL_T*      )b->gadget); break;
        case GAD_LISTBOX:    free_lb  ((GAD_LISTBOX_T*    )b->gadget); break;
        case GAD_PROGBAR:    free_pb  ((GAD_PROGBAR_T*    )b->gadget); break;
        case GAD_RADIO:      free_rad ((GAD_RADIO_T*      )b->gadget); break;
        case GAD_SCROLLBAR:  free_sb  ((GAD_SCROLLBAR_T*  )b->gadget); break;

        default: break;
    }

    free_base(b);
}



