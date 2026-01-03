//// SBX_WINDOWEX.CPP //////


#include "stdio.h"
#include <string.h>
#include "stdint.h"

#include "../fastram.h"
#include "cg_renderer.h"
#include "cg_input.h"
#include "cg_windowex.h"
#include "cg_gadgetrender.h"


#include "cg_glyphs.h"

#include "cg_gadgets.h"

// GADGET API's
#include "cg_gad_bitmapview.h"
//#include "cg_gad_button.h"
//#include "cg_gad_checkbox.h"
#include "cg_gad_gridselect.h"
//#include "cg_gad_radio.h"
//#include "cg_gad_label.h"
#include "cg_gad_listbox.h"
#include "cg_gad_scrollbar.h"

#include "cg_msghandler.h"

#include "cg_resources.h"



// globals ONLY for windows!! everything else needs to be in a contained
static SBXWindowId      g_focusWin   = SBW_INVALID_ID;


static uint32_t (*callMouseMoveEvt)   (sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my) = NULL;
static uint32_t (*callMouseReleaseEvt)(GADGET_BASE_T *g, int16_t *mx, int16_t *my) = NULL;

typedef struct {
    uint8_t active;
    char msg[96];
} UI_EmergencyOverlay;

static UI_EmergencyOverlay g_emerg = {0};

static inline void ui_clear_title_latch(void){
    g_ui.title_win = SBW_INVALID_ID;
    g_ui.title_region = WH_NONE;
    g_ui.title_inside = 0;
}

static inline void ui_clear_drag(void){
    g_ui.drag_win = SBW_INVALID_ID;
    g_ui.drag_off_x = 0;
    g_ui.drag_off_y = 0;
}


/////// prototypes //////////////////

static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock);
static void normalize_zorder(void);



sbx_window_t* SBOS_getWindow(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return 0;
    return &gui_windows[id];
}



static inline int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


static inline void ui_clip_disable(void) { g_uiclip.enabled = 0; }

static inline void ui_clip_set(int16_t x, int16_t y, int16_t w, int16_t h){
    g_uiclip.enabled = 1;
    g_uiclip.x0 = x;
    g_uiclip.y0 = y;
    g_uiclip.x1 = (int16_t)(x + w);
    g_uiclip.y1 = (int16_t)(y + h);
}


static void enforce_screen_bounds(sbx_window_t *w){
    if (!w) return;
    if (!(w->flags & SBX_WF_SCREENBOUND)) return;

    // If window bigger than screen, just pin it to origin (or center it)
    int16_t max_x = (SCR_WIDTH > w->winrect.w) ? (int16_t)(SCR_WIDTH - w->winrect.w) : 0;
    int16_t max_y = (SCR_HEIGHT > w->winrect.h) ? (int16_t)(SCR_HEIGHT - w->winrect.h) : 0;

    w->winrect.x = clamp_i16(w->winrect.x, 0, max_x);
    w->winrect.y = clamp_i16(w->winrect.y, 0, max_y);
}



int16_t win_gutter_right(const sbx_window_t *w){
    if ((w->flags & SBX_WF_DOCKRIGHT)) return 0; // dock band owns this edge
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER))
        return (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);
    return 0;
}

int16_t win_gutter_bottom(const sbx_window_t *w){
    if ((w->flags & SBX_WF_DOCKBOTTOM)) return 0; // dock band owns this edge
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER))
        return (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);
    return 0;
}

GADGET_RECT_T win_inner_rect(const sbx_window_t *w){
    int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0;

    GADGET_RECT_T r;
    if (w->flags & SBX_WF_NOBORDER) {
        r.x = w->winrect.x;
        r.y = w->winrect.y;
        r.w = w->winrect.w;
        r.h = w->winrect.h;
    } else {
        r.x = (int16_t)(w->winrect.x + WIN_BORDER);
        r.y = (int16_t)(w->winrect.y + WIN_BORDER + title_h);
        r.w = (int16_t)(w->winrect.w - (WIN_BORDER * 2));
        r.h = (int16_t)(w->winrect.h - ((WIN_BORDER * 2) + title_h));
    }
    if (r.w < 0) r.w = 0;
    if (r.h < 0) r.h = 0;
    return r;
}


static void layoutWindow(sbx_window_t *w){
    if (!w) return;

    GADGET_RECT_T inner = win_inner_rect(w);

    // Start with client = inner
    w->clientrect.x = inner.x;
    w->clientrect.y = inner.y;
    w->clientrect.w = inner.w;
    w->clientrect.h = inner.h;

    // reserve resize gutter (keep gadgets out of the resize zone)
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        int16_t gb = win_gutter_bottom(w);
        if (w->clientrect.h >= gb) w->clientrect.h = (int16_t)(w->clientrect.h - gb);
        else w->clientrect.h = 0;
    }

    // reserve docked bands (these live in INNER, not in CLIENT)
    if (w->flags & SBX_WF_DOCKRIGHT) {
        if (w->clientrect.w >= SB_SCROLL_THICK)
            w->clientrect.w = (int16_t)(w->clientrect.w - SB_SCROLL_THICK);
        else
            w->clientrect.w = 0;
    }

    if (w->flags & SBX_WF_DOCKBOTTOM) {
        if (w->clientrect.h >= SB_SCROLL_THICK)
            w->clientrect.h = (int16_t)(w->clientrect.h - SB_SCROLL_THICK);
        else
            w->clientrect.h = 0;
    }
}

static inline void ui_end_interaction(void){
    // Hard reset of all active mouse/UI interaction state.
    // Safe to call from anywhere.
    g_ui.mouse_down  = 0;
    g_ui.down_win    = SBW_INVALID_ID;
    g_ui.down_region = WH_NONE;
    ui_clear_title_latch();
    ui_clear_drag();
    g_ui.resize_win  = SBW_INVALID_ID;
    callMouseMoveEvt = NULL;
    callMouseReleaseEvt = NULL;
    g_ui.capturing = 0;
    g_ui.capturedGadget = (CGGadgetHandle){0};
}



void SBOS_setFocus(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) {
        g_focusWin = SBW_INVALID_ID;
        return;
    }

    if (gui_windows[id].flags & SBX_WF_NOFOCUS) return;

    g_focusWin = id;
}

void SBOS_setWindowBackColour(SBXWindowId winId, uint8_t newcolor){
    sbx_window_t *w = &gui_windows[winId];
    if(!w) return;
    w->backColour = newcolor;
}


static int z_find(SBXWindowId id){
    for (int i = 0; i < (int)g_winZcount; i++) {
        if (g_winZorder[i] == id) return i;
    }
    return -1;
}
void SBOS_sendToBack(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return;
    if (gui_windows[id].flags & SBX_WF_ALWAYS_TO_FRONT) return;  // cannot sink 😄

    int pos = z_find(id);
    if (pos < 0) return;
    if (pos == 0) return;

    SBXWindowId temp = g_winZorder[pos];
    for (int i = pos; i > 0; i--) g_winZorder[i] = g_winZorder[i - 1];
    g_winZorder[0] = temp;

    normalize_zorder();
}



static CGWindowProcRes DefaultWindowProc(SBXWindowId win, const CGMessage_t *m)
{
    if (!m) return(CGPROC_DEFAULT);

    if (m->mtype != CGMSG_WINDOW) return(CGPROC_DEFAULT);

    switch (m->eventClass) {


    case CGEVT_WIN_CLOSE_REQUEST:
        SBOS_destroyWindow(win);
        break;

    case CGEVT_WIN_ZORDER:
        // Example: a==0 => send to back, a==1 => bring to front
        if (m->a == 0) SBOS_sendToBack(win);
        else          SBOS_bringToFront(win);
        break;

    case CGEVT_WIN_MINIMISE:
        // TODO: implement later
        // SBOS_minimiseWindow(win);
        break;

    case CGEVT_WIN_MAXRESTORED:
        // TODO: implement later
        // SBOS_toggleMaxRestore(win);
        break;

    case CGEVT_WIN_MOVED:
    case CGEVT_WIN_RESIZED:
        // Default: do nothing (apps may care; OS doesn't)
        break;

    default:
        break;
    }
    return(CGPROC_COMPLETE);// we got here so everything was good
}

CGWindowProcRes SBOS_DefaultWindowProc(SBXWindowId win, const CGMessage_t *m){
    return DefaultWindowProc(win, m);
}


static inline GADGET_BASE_T* UI_CapturedGadgetPtr(void) {
    if (!g_ui.capturing) return NULL;

    GADGET_BASE_T *g = SBOS_gadgetFromHandle(g_ui.capturedGadget);
    if (!g) {
        g_ui.capturing = 0;
        g_ui.capturedGadget = (CGGadgetHandle){0};

        // optional: prevent stale handler calls
        callMouseMoveEvt = NULL;
        callMouseReleaseEvt = NULL;
        return NULL;
    }
    return g;
}



SBXWindowId SBOS_createWindow(SBXWindowId *selfHandlePTR, int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags){
    for (SBXWindowId i = 0; i < MAX_WINDOWS; i++) {
        if (!gui_used[i]) {
            gui_used[i] = 0;// it should already be zero, but doesnt heard to really be sure ;)

            sbx_window_t *w = &gui_windows[i];
            w->winrect.x = x;
            w->winrect.y = y;
            w->winrect.w = (int16_t)width;
            w->winrect.h = (int16_t)height;
            w->flags = flags;
            w->backColour = PEN_WIN_BG;

            // resize limiter box (WORKS REAAAAAAAAAALLY well
            w->maxrect.x = 100;   // <-- these two points arent
            w->maxrect.y = 100;
            w->maxrect.w = 0x7FFF;  // pretty big
            w->maxrect.h = 0x7FFF;  // pretty big

            if (w->GADGETS) fastFree(w->GADGETS);   // clear out the old gadgets (for reusable windows)
            w->GADGETS = NULL;//
            w->gadCap = 0;

            //w->GADGETS = (GADGET_BASE_T**)fastAlloc((uint32_t)sizeof(GADGET_BASE_T*));
            //w->gadCap = 1;
            /*
            w->GADGETS = (GADGET_BASE_T**)fastAlloc((uint32_t)MAX_GADGETS_PER_WINDOW * sizeof(GADGET_BASE_T*));
            if (!w->GADGETS) {
                gui_used[i] = 0;
                SBOS_EmergencyError("Out of fastRam:\nResource: window gadget table");
                return SBW_INVALID_ID; // or your failure path
            }
            memset(w->GADGETS, 0, (uint32_t)MAX_GADGETS_PER_WINDOW * sizeof(GADGET_BASE_T*));
            w->gadCap = MAX_GADGETS_PER_WINDOW;
            */

            w->proc = DefaultWindowProc;

            if (title) {
                size_t n = 0;
                while (title[n] && n < sizeof(w->title) - 1) {
                    w->title[n] = title[n];
                    n++;
                }
                w->title[n] = '\0';
            } else {
                w->title[0] = '\0';
            }

            enforce_screen_bounds(w);
            layoutWindow(w);

            if (g_winZcount < MAX_WINDOWS) {
                g_winZorder[g_winZcount++] = i;
            } else {
                gui_used[i] = 0;
                if (w->GADGETS) {
                    fastFree(w->GADGETS);
                    w->GADGETS = NULL;
                }
                return SBW_INVALID_ID;
            }

            w->self = i;

            w->lptrRef = selfHandlePTR;
            if (w->lptrRef) *w->lptrRef = i;

            gui_used[i] = 1;    // everything checked out, slot is now used

            normalize_zorder();
            SBOS_paintAllWindows();

            return i;
        }
    }

    SBOS_EmergencyError("Out of Windows.\nPlease close some");
    return SBW_INVALID_ID;
}


void SBOS_setWindowProc(SBXWindowId win, MSGWndProc proc)
{
    sbx_window_t *w = SBOS_getWindow(win);
    if (w) w->proc = proc;
}

void SBOS_setWindowResizeLimits(SBXWindowId win, int16_t minw, int16_t minh, int16_t maxw, int16_t maxh){

    sbx_window_t *w = SBOS_getWindow(win);
    if (!w) return; // not a valid window!!
    if(minw < 100) minw = 100;
    if(w->flags && SBX_WF_RESIZABLE){
        if(minh < (WIN_TITLE_HEIGHT * 2)) minh = (WIN_TITLE_HEIGHT * 2);
    } else
        if(minh < WIN_TITLE_HEIGHT) minh = WIN_TITLE_HEIGHT;

    if(maxw > 0x7fff) maxw = 0x7fff;
    if(maxh > 0x7fff) maxh = 0x7fff;

    w->maxrect.x = minw;
    w->maxrect.y = minh;
    w->maxrect.w = maxw;
    w->maxrect.h = maxh;
}

void SBOS_getWindowSize(SBXWindowId win, int16_t *w, int16_t *h){
    sbx_window_t *winh = SBOS_getWindow(win);
    if (!winh) return; // not a valid window!!

    *w = winh->winrect.w;
    *h = winh->winrect.h;
}


void SBOS_paintWindow(SBXWindowId id){
    sbx_window_t *w = SBOS_getWindow(id);
    if (!w) return;

    layoutWindow(w);

    // --- geometry shortcuts ---
    const int16_t win_x = w->winrect.x;
    const int16_t win_y = w->winrect.y;
    const int16_t win_w = w->winrect.w;
    const int16_t win_h = w->winrect.h;

    const int16_t cli_x = w->clientrect.x;
    const int16_t cli_y = w->clientrect.y;
    const int16_t cli_w = w->clientrect.w;
    const int16_t cli_h = w->clientrect.h;

    const uint16_t borderPen = (id == g_focusWin) ? PEN_WIN_BORDER_ACTIVE : PEN_WIN_BORDER_INACTIVE;

    // --- outer frame outline (always) ---
    draw_rect_outline_thick(win_x, win_y, win_w, win_h, WIN_BORDER, borderPen);

    // --- client background ---
    // IMPORTANT: clientrect is the app-drawable area. Fill it.
    sbgfx_drawbox(cli_x, cli_y, cli_w, cli_h, w->backColour);




    // --- draw client gadgets (clip to clientrect) ---
    ui_clip_set(cli_x, cli_y, cli_w, cli_h);
    SBOS_drawControlsFiltered(w, 0);
    ui_clip_disable();

    // If borderless, we don't draw frame/title/gutter chrome.
    if (w->flags & SBX_WF_NOBORDER) {
        return;
    }

    // --- compute title bar height used by frame chrome ---
    //const int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? (WIN_TITLE_HEIGHT + 4) : 0;

    // --- draw resize gutter + glyph (chrome, not part of client) -----

    if(w->flags & SBX_WF_DOCKRIGHT){
        const int16_t gx = (int16_t)(win_x + win_w - WIN_RESIZE_GLYPH_SIZE);
        sbgfx_drawbox(gx, win_y, WIN_RESIZE_GLYPH_SIZE, win_h, borderPen);
    }

    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {

        // bottom-right glyph box (DO NOT subtract WIN_BORDER here)
        const int16_t gx = (int16_t)(win_x + win_w - WIN_RESIZE_GLYPH_SIZE);
        const int16_t gy = (int16_t)(win_y + win_h - WIN_RESIZE_GLYPH_SIZE);

        // bottom gutter strip spans inside the border area
        const int16_t inner_x = (int16_t)(win_x + WIN_BORDER);
        const int16_t inner_w = (int16_t)(win_w - (WIN_BORDER * 2));

        ui_clip_set(win_x, win_y, win_w, win_h);

        // bottom bar
        sbgfx_drawbox(inner_x, (int16_t)(gy + 1), inner_w, (int16_t)(WIN_RESIZE_GLYPH_SIZE - 2), borderPen);

        ui_clip_disable();

        sbgfx_drawbox(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, borderPen);
        draw_bevel(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 0);
        sbgfx_glyph(gx, gy, glyph_resize);

        // polish
        gfx_setcolour(PEN_WIN_BEVEL_L);
        ui_vline((int16_t)(gx - 1), gy, WIN_RESIZE_GLYPH_SIZE);

        if(w->flags & SBX_WF_DOCKRIGHT)
            //ui_hline((int16_t)(gx), (int16_t)(WIN_RESIZE_GLYPH_SIZE), WIN_BORDER);
            ui_hline((int16_t)(gx), (int16_t)(gy - 1), WIN_RESIZE_GLYPH_SIZE);
        else
            ui_hline((int16_t)(win_x + win_w - WIN_BORDER), (int16_t)(gy - 1), WIN_BORDER);
    }

    // --- docked controls (if you later put scrollbars in the non-client inner band) ---
    // For now, simplest: clip to the inner band (client + potential gutter regions)
    {
        //const int16_t inner_x = (int16_t)(win_x + WIN_BORDER);
        //const int16_t inner_y = (int16_t)(win_y + WIN_BORDER + ((w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0));
        //const int16_t inner_w = (int16_t)(win_w - (WIN_BORDER * 2));
        //const int16_t inner_h = (int16_t)(win_h - (WIN_BORDER * 2) - ((w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0));

        //ui_clip_set(inner_x, inner_y, inner_w, inner_h);
        ui_clip_set(win_x, win_y, win_w, win_h);   // <-- was inner rect
        SBOS_drawControlsFiltered(w, 1);
        ui_clip_disable();
    }

    // --- frame bevels ---
    draw_bevel(win_x, win_y, win_w, win_h, PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 0);

    // inner bevel around the client area
    draw_bevel((int16_t)(cli_x - 1), (int16_t)(cli_y - 1), (int16_t)(cli_w + 2), (int16_t)(cli_h + 2), PEN_WIN_BEVEL_H, PEN_WIN_BEVEL_L, 1);

    // --- title bar ---
    if (w->flags & SBX_WF_TITLE_BAR) {

        int16_t tb_x = win_x;
        int16_t tb_y = win_y;
        int16_t tb_w = win_w;
        int16_t tb_h = (WIN_TITLE_HEIGHT + 4);

        fill_rect_pen(tb_x, tb_y, tb_w, tb_h, borderPen);

        int16_t bx = (int16_t)(tb_x + tb_w); // right edge
        int16_t by = tb_y;
        int16_t twidth = win_w;

        // pressed visuals (latched)
        uint8_t downClose = (g_ui.mouse_down && g_ui.title_win == id && g_ui.title_inside && g_ui.title_region == WH_CLOSE);
        uint8_t downMin   = (g_ui.mouse_down && g_ui.title_win == id && g_ui.title_inside && g_ui.title_region == WH_MINIMISE);
        uint8_t downMax   = (g_ui.mouse_down && g_ui.title_win == id && g_ui.title_inside && g_ui.title_region == WH_MAXRESTORE);
        uint8_t downZo    = (g_ui.mouse_down && g_ui.title_win == id && g_ui.title_inside && g_ui.title_region == WH_ZORDER);

        // right-side gadgets
        if (w->flags & SBX_WF_ZORDER) {
            bx -= WIN_ZORDER_WIDTH; twidth -= WIN_ZORDER_WIDTH;
            draw_title_button(bx, by, WIN_ZORDER_WIDTH, tb_h, borderPen, downZo);
            glyph_zorder(bx, by, WIN_ZORDER_WIDTH, tb_h);
        }
        if (w->flags & SBX_WF_MAXRESTORE) {
            bx -= WIN_MAXRESTORE_WIDTH; twidth -= WIN_MAXRESTORE_WIDTH;
            draw_title_button(bx, by, WIN_MAXRESTORE_WIDTH, tb_h, borderPen, downMax);
            glyph_max_box(bx, by, WIN_MAXRESTORE_WIDTH, tb_h);
        }
        if (w->flags & SBX_WF_MINIMISE) {
            bx -= WIN_MINIMISE_WIDTH; twidth -= WIN_MINIMISE_WIDTH;
            draw_title_button(bx, by, WIN_MINIMISE_WIDTH, tb_h, borderPen, downMin);
            glyph_minimise(bx, by, WIN_MINIMISE_WIDTH, tb_h);
        }

        // left-side close
        int16_t win_tx = (int16_t)(win_x + WIN_BORDER + 4);
        int16_t win_ty = (int16_t)(win_y + WIN_BORDER);

        if (w->flags & SBX_WF_CLOSE) {
            int16_t cx = tb_x;
            draw_title_button(cx, by, WIN_CLOSE_WIDTH, tb_h, borderPen, downClose);
            glyph_close_x(cx, by, WIN_CLOSE_WIDTH, tb_h);
            win_tx = (int16_t)(cx + WIN_CLOSE_WIDTH + 4);
            tb_x   = (int16_t)(cx + WIN_CLOSE_WIDTH);
            twidth = (int16_t)(twidth - WIN_CLOSE_WIDTH);
        }

        // bevel around title text band
        draw_bevel_rect(tb_x, win_y, twidth, (WIN_TITLE_HEIGHT + WIN_BORDER));

        // title text (clipped by character count)
        //uint16_t titlePen = (id == g_focusWin) ? PEN_WIN_BORDER_ACTIVE : PEN_WIN_BORDER_INACTIVE;
        gfx_setcolour(PEN_WIN_TITLE);


        char tmpTitle[65];
        int16_t max_chars = (int16_t)((twidth - 8) / 8);
        if (max_chars < 0) max_chars = 0;
        if (max_chars > 64) max_chars = 64;

        uint8_t c = 0;
        for (; c < (uint8_t)max_chars && w->title[c]; c++) tmpTitle[c] = w->title[c];
        tmpTitle[c] = '\0';

        ui_draw_text816(win_tx, (int16_t)(win_ty - 2), (const unsigned char*)tmpTitle);
    }

    // --- focus dotted frame ---
    if (id == g_focusWin) {
        gfx_setcolour(PEN_WIN_BEVEL_L);
        ui_dotted_rect_thick(win_x-1, win_y-1, win_w+1, win_h+1, 3);
    }
}


static void normalize_zorder(void){
    SBXWindowId back[MAX_WINDOWS];
    SBXWindowId mid[MAX_WINDOWS];
    SBXWindowId front[MAX_WINDOWS];
    uint8_t nb = 0, nm = 0, nf = 0;

    for (uint8_t i = 0; i < g_winZcount; i++) {
        SBXWindowId id = g_winZorder[i];
        if (id >= MAX_WINDOWS || !gui_used[id]) continue;

        uint32_t f = gui_windows[id].flags;

        // If both are set, choose a rule (front wins here)
        if ((f & SBX_WF_ALWAYS_TO_FRONT) && (f & SBX_WF_ALWAYS_TO_BACK)) {
            f &= ~SBX_WF_ALWAYS_TO_BACK;
            gui_windows[id].flags = f;
        }

        if (f & SBX_WF_ALWAYS_TO_BACK)       back[nb++]  = id;
        else if (f & SBX_WF_ALWAYS_TO_FRONT) front[nf++] = id;
        else                                 mid[nm++]   = id;
    }

    // Rebuild g_zorder: back -> mid -> front
    uint8_t out = 0;
    for (uint8_t i = 0; i < nb; i++) g_winZorder[out++] = back[i];
    for (uint8_t i = 0; i < nm; i++) g_winZorder[out++] = mid[i];
    for (uint8_t i = 0; i < nf; i++) g_winZorder[out++] = front[i];

    g_winZcount = out;
}

static int z_front_barrier(void){
    for (int i = 0; i < (int)g_winZcount; i++) {
        SBXWindowId id = g_winZorder[i];
        if (id < MAX_WINDOWS && gui_used[id]) {
            if (gui_windows[id].flags & SBX_WF_ALWAYS_TO_FRONT)
                return i; // first always-front window
        }
    }
    return (int)g_winZcount;
}


void SBOS_bringToFront(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return;

    uint32_t f = gui_windows[id].flags;
    if (f & SBX_WF_ALWAYS_TO_BACK) return; // locked to back

    int pos = z_find(id);
    if (pos < 0) return;

    int barrier = z_front_barrier();         // index where always-front starts
    int target  = barrier - 1;               // last slot before always-front
    if (target < 0) target = 0;

    // If this window is itself always-front, true front is allowed
    if (f & SBX_WF_ALWAYS_TO_FRONT) target = (int)g_winZcount - 1;

    if (pos == target) return;

    SBXWindowId temp = g_winZorder[pos];

    if (pos < target) {
        for (int i = pos; i < target; i++) g_winZorder[i] = g_winZorder[i + 1];
    } else {
        for (int i = pos; i > target; i--) g_winZorder[i] = g_winZorder[i - 1];
    }
    g_winZorder[target] = temp;

    normalize_zorder();
}


static void z_remove(SBXWindowId id){
    int pos = z_find(id);
    if (pos < 0) return;

    for (int i = pos; i < (int)g_winZcount - 1; i++) {
        g_winZorder[i] = g_winZorder[i + 1];
    }
    g_winZcount--;
}


static SBXWindowId findTopFocusable(void){
    for (int zi = (int)g_winZcount - 1; zi >= 0; zi--) {
        SBXWindowId id = g_winZorder[zi];
        if (id >= MAX_WINDOWS || !gui_used[id]) continue;
        if (!(gui_windows[id].flags & SBX_WF_VISIBLE)) continue;
        if (gui_windows[id].flags & SBX_WF_NOFOCUS) continue;
        return id;
    }
    return SBW_INVALID_ID;
}

static void destroy_window_gadgets(sbx_window_t *w){
    //if (!w) return;
    if (!w || !w->GADGETS) return;  // guard against bad gadgets
    for (int i = 0; i < w->gadCap; i++){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g) continue;
        SBOS_destroyGadget(base_to_handle(g));
        w->GADGETS[i] = NULL;
    }
}

SBXWindowId SBOS_getWindowByGadget(const GADGET_BASE_T *b){
    if (!b || !b->gadget) return SBW_INVALID_ID;
    const GAD_HDR_T *h = (const GAD_HDR_T*)b->gadget; // header-first rule
    return h->winhnd;
}


void SBOS_ClearMessagesForWindow(SBXWindowId win)
{
    CGMessage_t tmp[CGMSG_QUEUE_CAP];
    uint16_t keep = 0;

    CGMessage_t m;
    while (SBOS_PopMessage(&m)) {
        if (m.winhnd != win) {
            tmp[keep++] = m;
        } else {
            // optionally track drops/purges
            // g_msg_dropped++;
        }
    }

    // Re-queue kept messages in original order
    for (uint16_t i = 0; i < keep; i++) {
        SBOS_PostMessage(&tmp[i]);
    }
}


void SBOS_destroyWindow(SBXWindowId id){
    if (id >= MAX_WINDOWS) return;
    if (!gui_used[id]) return;

    uint8_t involved = 0;

    sbx_window_t *w = &gui_windows[id];

    SBOS_ClearMessagesForWindow(id);

    if (g_ui.down_win   == id) involved = 1;
    if (g_ui.drag_win   == id) involved = 1;
    if (g_ui.resize_win == id) involved = 1;
    if (g_ui.title_win  == id) involved = 1;

    if (g_ui.capturing) {
        GADGET_BASE_T *cg = UI_CapturedGadgetPtr();
        if (cg && SBOS_getWindowByGadget(cg) == id) {
            involved = 1;
            g_ui.capturing = 0;
            g_ui.capturedGadget = (CGGadgetHandle){0};
        }
    }
    if (g_ui.drag_win == id) ui_clear_drag();
    if (g_ui.resize_win == id) g_ui.resize_win = SBW_INVALID_ID;
    if (g_ui.title_win == id) ui_clear_title_latch();
    if (g_ui.down_win == id) { g_ui.down_win = SBW_INVALID_ID; g_ui.down_region = WH_NONE; }

    destroy_window_gadgets(w);
    if (w->GADGETS) {
        fastFree(w->GADGETS);
        w->GADGETS = NULL;
    }


    w->flags = 0;
    w->title[0] = '\0';


    gui_used[id] = 0;
    z_remove(id);

    if (g_focusWin == id)
        g_focusWin = findTopFocusable();

    normalize_zorder();
    SBOS_paintAllWindows();

    if (involved)
        ui_end_interaction();

    if (w->lptrRef) {
        *w->lptrRef = 0;
        w->lptrRef = NULL;
    }
}




void SBOS_EmergencyError(const char *s){
    g_emerg.active = 1;
    strncpy(g_emerg.msg, s ? s : "UI error", sizeof(g_emerg.msg)-1);
    g_emerg.msg[sizeof(g_emerg.msg)-1] = 0;
}

void SBOS_CloseEmergencyError(){
    g_emerg.active = 0;
    // call all window redraw - when ready
}

static void draw_emergency_overlay(void){
    if (!g_emerg.active) return;

    int16_t w = 320, h = 140;           // or measure text width if you want
    int16_t x = (SCR_WIDTH  - w) / 2;
    int16_t y = (SCR_HEIGHT - h) / 2;

    fill_rect_pen(x, y, w, h, 5);
    draw_bevel_rect(x, y, w, h);

    draw_bevel_rect_inset(x+8, y+ 33, w-16, 78);

    gfx_setcolour(PEN_WIN_TITLE);

    ui_draw_text816(x + 8, y + 10, (const unsigned char*)"SIDBOX OS: Emergency Alert");
    ui_draw_text816(x + 14, y + 40, (const unsigned char*)g_emerg.msg);
    //ui_draw_text816(x + 14, y + 40, (const unsigned char*)"line 1\nline 2\nline 3###########1############2#####\nline 4");
    ui_draw_text816(x + 8, y + h - ( 22 ), (const unsigned char*)"Close: click / press top-left");
}



void SBOS_paintAllWindows(void){
    for (int zi = 0; zi < (int)g_winZcount; zi++) {
        SBXWindowId id = g_winZorder[zi];
        if (id >= MAX_WINDOWS) continue;
        if (!gui_used[id]) continue;

        if (gui_windows[id].flags & SBX_WF_VISIBLE) {
            SBOS_paintWindow(id);
        }
    }

    draw_emergency_overlay();
}

static inline uint8_t is_title_gadget_region(WHitRegion r){
    return (r == WH_CLOSE) || (r == WH_MINIMISE) || (r == WH_MAXRESTORE) || (r == WH_ZORDER);
}

static WHitResult hittest_window(SBXWindowId id, int16_t mx, int16_t my){
    WHitResult r = { SBW_INVALID_ID, WH_NONE, SBCTL_INVALID };

    sbx_window_t *w = SBOS_getWindow(id);
    if (!w) return r;
    if (!(w->flags & SBX_WF_VISIBLE)) return r;

    layoutWindow(w);

    // 1) overall window bounds (for quick reject)
    if (!mousept_in_rect(mx, my, w->winrect.x, w->winrect.y, w->winrect.w, w->winrect.h)) return r;

    r.id = id;

    // Borderless window: everything is client
    if (w->flags & SBX_WF_NOBORDER) {
        r.region = WH_CLIENT;
        return r;
    }

    // Title bar geometry
    uint8_t hasTitle = (w->flags & SBX_WF_TITLE_BAR) != 0;
    int16_t tb_h = hasTitle ? (WIN_TITLE_HEIGHT + 4) : 0;

    // 2) If inside title band, check buttons
    if (hasTitle && mousept_in_rect(mx, my, w->winrect.x, w->winrect.y, w->winrect.w, tb_h)) {

        int16_t bx = w->winrect.x + w->winrect.w;   // right edge
        int16_t by = w->winrect.y;          // top
        // IMPORTANT: draw_title_button makes height = WIN_TITLE_HEIGHT + WIN_BORDER

        int16_t bh = tb_h;

        // Right side: MAXRESTORE then ZORDER (same order paint)
        if (w->flags & SBX_WF_ZORDER) {
            bx -= WIN_ZORDER_WIDTH;
            if (mousept_in_rect(mx, my, bx, by, WIN_ZORDER_WIDTH, bh)) {
                r.region = WH_ZORDER;

                return r;
            }
        }

        if (w->flags & SBX_WF_MAXRESTORE) {
            bx -= WIN_MAXRESTORE_WIDTH;
            if (mousept_in_rect(mx, my, bx, by, WIN_MAXRESTORE_WIDTH, bh)) {
                r.region = WH_MAXRESTORE;

                return r;
            }
        }

        if (w->flags & SBX_WF_MINIMISE) {
            bx -= WIN_MINIMISE_WIDTH;
            if (mousept_in_rect(mx, my, bx, by, WIN_MINIMISE_WIDTH, bh)) {
                r.region = WH_MINIMISE;

                return r;
            }
        }

        // Left side: CLOSE
        if (w->flags & SBX_WF_CLOSE) {
            int16_t cx = w->winrect.x;
            if (mousept_in_rect(mx, my, cx, by, WIN_CLOSE_WIDTH, bh)) {
                r.region = WH_CLOSE;
                return r;
            }
        }

        /*
        if (w->flags & SBX_WF_RESIZABLE){
            int16_t cx = w->winrect.x;
            printf("RESIZINGS...");
            if (mousept_in_rect(mx, my, cx, by, WIN_RESIZE_GLYPH_SIZE, bh)) {
                r.region = WH_RESIZE;
                return r;
            }
            //resize_win
        }
        */

        // Otherwise it’s the title bar
        r.region = WH_TITLE;
        return r;
    }

    // Resize glyph hit (bottom-right)
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        int16_t gx = (int16_t)(w->winrect.x + w->winrect.w - WIN_RESIZE_GLYPH_SIZE);
        int16_t gy = (int16_t)(w->winrect.y + w->winrect.h - WIN_RESIZE_GLYPH_SIZE);
        if (mousept_in_rect(mx, my, gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE)) {
            r.region = WH_RESIZE;
            return r;
        }
    }

    // 3) Otherwise, check client area INCLUDING gutters (for docked controls)
    GADGET_RECT_T inner = win_inner_rect(w);

    // include the whole inner region so docked bands are clickable
    if (mousept_in_rect(mx, my, inner.x, inner.y, inner.w, inner.h)) {
        r.region = WH_CLIENT;
        return r;
    }

    // 4) Inside window but not title/client => border
    // add WH_BORDER laterwant resize hit zones.
    r.region = WH_CLIENT; // or WH_NONE
    return r;
}

static inline int16_t win_resize_overlap_inner(const sbx_window_t *w){
    if (!w) return 0;
    if (!(w->flags & SBX_WF_RESIZABLE)) return 0;
    if (w->flags & SBX_WF_NOBORDER) return 0;
    return (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);
}

// How much width to reserve on the RIGHT edge of INNER
int16_t win_inner_reserve_right(const sbx_window_t *w){
    // If you have a dock-right band, that band owns the right edge (not the resize overlap).
    //if (w->flags & SBX_WF_DOCKRIGHT) return 0;
    return win_resize_overlap_inner(w);
}

// How much height to reserve on the BOTTOM edge of INNER
int16_t win_inner_reserve_bottom(const sbx_window_t *w){
    // If you have a dock-bottom band, that band owns the bottom edge (not the resize overlap).
    //if (w->flags & SBX_WF_DOCKBOTTOM) return 0;
    return win_resize_overlap_inner(w) +3;
}




// ===================== HITTEST (VALUE-BASED) =====================


static inline uint8_t gadget_is_docked(const GAD_HDR_T *h){
    return (h->flags & (GAD_TOOL_DOCKED_RIGHT | GAD_TOOL_DOCKED_BOTTOM)) != 0;
}

static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock){

    if (!w || !w->GADGETS || w->gadCap == 0) return;
    for (int i = 0; i < w->gadCap; i++){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g || !g->gadget) continue;

        GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;

        // wantDock=0 => draw non-docked only
        // wantDock=1 => draw docked only
        if (wantDock) {
            if (!gadget_is_docked(h)) continue;
        } else {
            if (gadget_is_docked(h)) continue;
        }

        switch (g->gadgetType){
            case GAD_BITMAPVIEW: draw_bitmapview(w, g); break;
            case GAD_BUTTON:     draw_button(w, g);     break;
            case GAD_CANVAS:     draw_canvas(w, g);     break;
            case GAD_CHECKBOX:   draw_checkbox(w, g);   break;
            case GAD_GRIDSELECT: draw_gridselect(w, g); break;
            case GAD_LABEL:      draw_label(w, g);      break;
            case GAD_LISTBOX:    draw_listbox(w, g);    break;
            case GAD_PROGBAR:    draw_progbar(w, g);    break;
            case GAD_RADIO:      draw_radio(w, g);      break;
            case GAD_SCROLLBAR:  draw_scrollbar(w, g);  break;


            default: break;     // if we got here, then the GUI is BARFING UP randomness, so stop it here ;)
        }
    }
    return;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
////  WINDOW INTERFACE WITH MOUSE & EVENT HANDLING AND SUBMITTION  //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////



void SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my) {
    uint8_t repaint = 0;
    uint8_t cleanups = 0;
    switch(evt){
        ///////////////////////////////////// MOUSE DOWN ////////////////////////////////////////
        case MOUSE_DOWN:{
            g_ui.mouse_down = 1;
            g_ui.resize_win = SBW_INVALID_ID;
            callMouseMoveEvt = NULL;    // clear the call event (will move this to the mouse up tho later

            // hit-test once on press
            for (int zi = (int) g_winZcount - 1; zi >= 0; zi--) {
                SBXWindowId id = g_winZorder[zi];
                if (id >= MAX_WINDOWS || !gui_used[id])
                    continue;

                WHitResult hit = hittest_window(id, mx, my);
                if (hit.region == WH_NONE)
                    continue;
                sbx_window_t *w = SBOS_getWindow(hit.id);

                if (!(w->flags & SBX_WF_NOAUTOZORDER))
                    SBOS_bringToFront(hit.id);
                SBOS_setFocus(hit.id); // if(!(w->flags & SBX_WF_NOFOCUS))      checked inside setFocus
                repaint = 1;

                g_ui.down_region = hit.region;
                g_ui.down_win = hit.id;

                //// WINDOW FRAME LATCHES //////////////////////////////////
                // latch title gadget press
                if (is_title_gadget_region(hit.region)) {
                    g_ui.title_win = hit.id;
                    g_ui.title_region = hit.region;
                    g_ui.title_inside = 1; // currently inside by definition
                } else {
                    g_ui.title_win = SBW_INVALID_ID;
                    g_ui.title_region = WH_NONE;
                    g_ui.title_inside = 0;
                }

                if (hit.region == WH_RESIZE) {
                    if (w && (w->flags & SBX_WF_RESIZABLE)) {
                        g_ui.capturing = 0;
                        g_ui.capturedGadget = (CGGadgetHandle){0};
                        callMouseMoveEvt = NULL;
                        callMouseReleaseEvt = NULL;

                        g_ui.resize_win = hit.id;
                        g_ui.r_start_mx = mx;
                        g_ui.r_start_my = my;
                        g_ui.r_start_w = w->winrect.w;
                        g_ui.r_start_h = w->winrect.h;
                    }
                    break;
                }

                // start drag if title hit
                if (hit.region == WH_TITLE) {
                    if (w->flags & SBX_WF_MOVEABLE) {
                        g_ui.capturing = 0;
                        g_ui.capturedGadget = (CGGadgetHandle){0};
                        callMouseMoveEvt = NULL;
                        callMouseReleaseEvt = NULL;

                        g_ui.drag_win = hit.id;
                        g_ui.drag_off_x = mx - w->winrect.x;
                        g_ui.drag_off_y = my - w->winrect.y;
                    }
                }

                ////////// THEN do the hits inside the program window view port //////////////////
                /// Likely the gadgets /// will need to figure out the docked window frame gadgets later
                /// KEEP FOR NOW will ned them for later
                if (hit.region == WH_CLIENT && w) {
                    GADGET_BASE_T *g = hittest_gadget(w, mx, my);
                    if (g) {
                        ui_clear_title_latch();
                        ui_clear_drag();
                        g_ui.resize_win = SBW_INVALID_ID;
                        g_ui.down_win = hit.id;
                        g_ui.down_region = WH_CLIENT;
                        //g_ui.capturedGadget = g;
                        g_ui.capturing = 1;
                        g_ui.capturedGadget = base_to_handle(g);


                        GAD_HDR_T *h = (GAD_HDR_T*) g->gadget;
                        h->down = 1;

                        callMouseMoveEvt    = NULL;
                        callMouseReleaseEvt = NULL;

                        // keep this here for now, but eventually i'll change this to just be "onmouseDownCapture(a bunch of fields)" and let taht be the switch gate
                        switch(g->gadgetType){
                            case GAD_BITMAPVIEW:{
                                if(!onMouseDownCaptureBitmapview(g, &mx, &my)){   // capture success
                                    callMouseMoveEvt = onMouseMoveBitmapView;
                                    callMouseReleaseEvt = onMouseUpBitmapView;
                                }
                            } break;
                            case GAD_SCROLLBAR:{
                                if(!onMouseDownCaptureScrollBar(w, g, &mx, &my)){ // capture success
                                    callMouseMoveEvt = onMouseMoveScrollbar;
                                    callMouseReleaseEvt = onMouseReleaseScrollbar;
                                }
                            } break;

                            case GAD_LISTBOX: {
                                if (!onMouseDownCaptureListBox(w, g, &mx, &my)) {
                                    callMouseMoveEvt    = onMouseMoveListBox;   // optional
                                    callMouseReleaseEvt = onMouseReleaseListBox;                 // you can add onMouseUpListBox later
                                }
                            } break;

                            case GAD_GRIDSELECT: {
                                if (!onMouseDownCaptureGridSelect(w, g, &mx, &my)) {
                                    callMouseMoveEvt    = onMouseMoveGridSelect;      // optional (hover/drag)
                                    callMouseReleaseEvt = onMouseReleaseGridSelect;   // commit selection
                                }
                            } break;


                            default: break;
                        }
                    }
                }
                break;
            }
        } break;

        ///////////////////////////////////// MOUSE MOVE ////////////////////////////////////////
        case MOUSE_MOVE:{
            // ######################### RESIZE WINDOW #############################
            if (g_ui.mouse_down && g_ui.resize_win != SBW_INVALID_ID) {
                sbx_window_t *w = SBOS_getWindow(g_ui.resize_win);
                if (w) {
                    enforce_screen_bounds(w);
                    int16_t dx = (int16_t) (mx - g_ui.r_start_mx);
                    int16_t dy = (int16_t) (my - g_ui.r_start_my);

                    int16_t nw = (int16_t) (g_ui.r_start_w + dx);
                    int16_t nh = (int16_t) (g_ui.r_start_h + dy);

                    // Optional: keep window inside screen when growing
                    if (w->flags & SBX_WF_SCREENBOUND) {
                        if (w->winrect.x + nw > SCR_WIDTH)
                            nw = (int16_t) (SCR_WIDTH - w->winrect.x);
                        if (w->winrect.y + nh > SCR_HEIGHT)
                            nh = (int16_t) (SCR_HEIGHT - w->winrect.y);
                    }

                    // enforce minimum window size
                    //if (nw < 100)
                        //nw = 100;
                    //if (nh < 100)
                        //nh = 100;

                    if(nw < w->maxrect.x) nw = w->maxrect.x;
                    if(nh < w->maxrect.y) nh = w->maxrect.y;

                    if(nw > w->maxrect.w) nw = w->maxrect.w;
                    if(nh > w->maxrect.h) nh = w->maxrect.h;

                    w->winrect.w = nw;
                    w->winrect.h = nh;

                    layoutWindow(w);
                    repaint = 1;

                    //if(w)
                        CG_PostWindowMsg(g_ui.resize_win, CGEVT_WIN_RESIZED, w->winrect.w, w->winrect.h, 0, 0);

                }
                break;
            }

            // ######################### MOVE WINDOW #############################
            if (g_ui.mouse_down && g_ui.drag_win != SBW_INVALID_ID) {
                sbx_window_t *w = SBOS_getWindow(g_ui.drag_win);
                if (w) {
                    w->winrect.x = mx - g_ui.drag_off_x;
                    w->winrect.y = my - g_ui.drag_off_y;
                    // keep layout consistent
                    enforce_screen_bounds(w);

                    layoutWindow(w);
                    repaint = 1;
                    CG_PostWindowMsg(g_ui.drag_win, CGEVT_WIN_MOVE, w->winrect.x, w->winrect.y, 0, 0);
                }
                break;
            }

            // ######################### GADGETS IN WINDOW #########################
            if (g_ui.mouse_down && g_ui.capturing) {

                GADGET_BASE_T *g = UI_CapturedGadgetPtr();
                if (!g) break;  // capture dropped because gadget died

                SBXWindowId wid = SBOS_getWindowByGadget(g);
                sbx_window_t *gw = SBOS_getWindow(wid);
                if (gw) {
                    if (callMouseMoveEvt) {
                        uint32_t stop = callMouseMoveEvt(gw, g, &evt, &mx, &my);
                        if (stop) { repaint = 1; break; }
                    }

                    GAD_HDR_T *h = (GAD_HDR_T*) g->gadget;
                    uint8_t inside = gadget_mouse_inside(gw, g, mx, my);
                    uint8_t newDown = inside ? 1 : 0;
                    if (newDown != h->down) { h->down = newDown; repaint = 1; }
                }
                break;
            }


            // ######################### CANCEL MOTIONS ############################
            // when mouse is held down and a gadget is captured
            if (g_ui.mouse_down && g_ui.title_win != SBW_INVALID_ID
                && is_title_gadget_region(g_ui.title_region)) {
                WHitResult ht = hittest_window(g_ui.title_win, mx, my);
                uint8_t inside = (ht.region == g_ui.title_region);
                if (inside != g_ui.title_inside) {
                    g_ui.title_inside = inside;
                    repaint = 1;
                }
            }
        } break;

        ///////////////////////////////////// MOUSE UP //////////////////////////////////////////
        case MOUSE_UP:{
            if (g_emerg.active) {
                if (mx >= EMERG_CLOSE_X && mx < EMERG_CLOSE_X + EMERG_CLOSE_W &&
                    my >= EMERG_CLOSE_Y && my < EMERG_CLOSE_Y + EMERG_CLOSE_H) {

                    SBOS_CloseEmergencyError();
                }
            }

            if (g_ui.resize_win != SBW_INVALID_ID) {
                SBXWindowId win = g_ui.resize_win;
                sbx_window_t *w = SBOS_getWindow(win);
                g_ui.resize_win = SBW_INVALID_ID;

                if(w) CG_PostWindowMsg(win, CGEVT_WIN_RESIZED, w->winrect.w, w->winrect.h, 0, 0);

                repaint = 1;
                cleanups = 1;

                break;
            }

            // ---- MOVE RELEASE ----
            if (g_ui.drag_win != SBW_INVALID_ID) {
                SBXWindowId win = g_ui.drag_win;
                sbx_window_t *w = SBOS_getWindow(win);
                g_ui.drag_win = SBW_INVALID_ID;

                if(w) CG_PostWindowMsg(win, CGEVT_WIN_MOVED, w->winrect.x, w->winrect.y, 0, 0);

                repaint  = 1;
                cleanups = 1;

                break;
            }


            g_ui.resize_win = SBW_INVALID_ID;

            // 1) TITLE GADGET RELEASE (latched)
            if (g_ui.title_win != SBW_INVALID_ID && is_title_gadget_region(g_ui.title_region)) {
                SBXWindowId wclick = g_ui.title_win;
                WHitRegion rclick = g_ui.title_region;

                // pop visual first
                g_ui.title_win = SBW_INVALID_ID;
                g_ui.title_region = WH_NONE;
                g_ui.title_inside = 0;

                // click only if released inside same gadget
                WHitResult ht = hittest_window(wclick, mx, my);
                if (ht.region == rclick) {
                    switch(rclick){
                        case WH_CLOSE:{
                            CG_PostWindowMsg(wclick, CGEVT_WIN_CLOSE_REQUEST, 0, 0, 0, 0);
                        } break;

                        case WH_ZORDER: {
                            CG_PostWindowMsg(wclick, CGEVT_WIN_ZORDER, 0, 0, 0, 0);
                            SBOS_sendToBack(wclick);
                            //printf("GLYPH HIT - Zorder\r\n");
                        } break;

                        case WH_MINIMISE: {
                            //printf("GLYPH HIT - Minimised\r\n");
                            CG_PostWindowMsg(wclick, CGEVT_WIN_MINIMISE, 0, 0, 0, 0);
                        } break;

                        case WH_MAXRESTORE: {
                            printf("GLYPH HIT - MaxRestore\r\n");
                            CG_PostWindowMsg(wclick, CGEVT_WIN_MAXRESTORED, 0, 0, 0, 0);
                        } break;

                        default: break;
                    }
                }
                repaint = 1;
                cleanups = 1;
                break;
            }

            /// the gadget was released
            if (g_ui.capturing) {
                //GADGET_BASE_T *g = g_ui.capturedGadget;
                GADGET_BASE_T *g = UI_CapturedGadgetPtr();
                if (!g) { repaint = 1; cleanups = 1; break; }  // this was killed before we got here

                SBXWindowId wid = SBOS_getWindowByGadget(g);
                sbx_window_t *gw = SBOS_getWindow(wid);

                uint8_t inside = 0;
                if (gw)
                    inside = gadget_mouse_inside(gw, g, mx, my);

                // pop visual
                GAD_HDR_T *h = (GAD_HDR_T*) g->gadget;
                h->down = 0;

                uint32_t handled = 0;
                if(callMouseReleaseEvt && gw){
                    handled = callMouseReleaseEvt(g, &mx, &my);
                    //if(cres) return;
                }

                //g_ui.capturedGadget = NULL;
                g_ui.capturing = 0;
                g_ui.capturedGadget = (CGGadgetHandle){0};

                bool clickActivate =
                    (g->gadgetType == GAD_BUTTON)   ||
                    (g->gadgetType == GAD_CHECKBOX) ||
                    (g->gadgetType == GAD_RADIO)    ||
                    (g->gadgetType == GAD_LISTBOX)  ||
                    (g->gadgetType == GAD_GRIDSELECT);


                if (clickActivate && inside) {
                    commitGadgetRelease(gw, g);

                }

                repaint = 1;
                cleanups = 1;
                break;
            }
            // 4 OTHERWISE: just cleanup (end drag etc)
            repaint = 1;
            cleanups = 1;
        } break;

        default:
            break;
    }

    if(repaint)
        SBOS_paintAllWindows();

    if(cleanups){
        cleanups = 0;
        callMouseMoveEvt = NULL;    // clear the call event (will move this to the mouse up tho later
        callMouseReleaseEvt = NULL;
        ui_end_interaction();
    }
}

void initWb(void){
    SBOS_print_reserved_ui_memory();
    ui_clear_title_latch();
    ui_clear_drag();
    g_ui.mouse_down  = 0;
    g_ui.down_win    = SBW_INVALID_ID;
    g_ui.down_region = WH_NONE;
    g_ui.resize_win  = SBW_INVALID_ID;
    SBOS_gadgetsInit();
}

