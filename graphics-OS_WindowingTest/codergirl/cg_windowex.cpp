//// SBX_WINDOWEX.CPP //////


#include "stdio.h"

#include "stdint.h"

//#include "../fastram.h"
#include "cg_renderer.h"
#include "cg_input.h"
#include "cg_windowex.h"
#include "cg_gadgetrender.h"


#include "cg_glyphs.h"

#include "cg_gadgets.h"

// GADGET API's
#include "cg_gad_button.h"
#include "cg_gad_checkbox.h"
#include "cg_gad_radio.h"
#include "cg_gad_bitmapview.h"
#include "cg_gad_scrollbar.h"
#include "cg_gad_listbox.h"


sbx_window_t     gui_windows[MAX_WINDOWS];
uint8_t          gui_used[MAX_WINDOWS];

SBXWindowId      g_winZorder[MAX_WINDOWS];
uint8_t          g_winZcount = 0;


// globals ONLY for windows!! everything else needs to be in a contained
static SBXWindowId      g_focusWin   = SBW_INVALID_ID;



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

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// FUNCTIONALITY BELOW //////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

// semi macro functions //

static inline Rect16 r16(int16_t x, int16_t y, int16_t w, int16_t h){ Rect16 r = {x,y,w,h}; return r; }
static inline uint8_t r16_valid(const Rect16 *r){ return (r->w > 0) && (r->h > 0); }

static inline uint8_t pt_in_r16(int16_t px, int16_t py, const Rect16 *r){
    return (px >= r->x) && (py >= r->y) && (px < (int16_t)(r->x + r->w)) && (py < (int16_t)(r->y + r->h));
}


static inline int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


typedef struct {
    size_t basePool, btnPool, chkPool, radPool, sbPool, bvPool;
} SBOS_GadgetPoolBytes;

SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void);

#define BYTES_OF(x) ((size_t)sizeof(x))

#define SBOS_UI_BUDGET_BYTES (64u * 1024u)


static void SBOS_print_reserved_ui_memory(void){
    SBOS_GadgetPoolBytes gb = SBOS_get_gadget_pool_bytes();

    // These must be visible in this translation unit (not static elsewhere).
    size_t bytes_ui        = BYTES_OF(g_ui);

    size_t bytes_windows   = BYTES_OF(gui_windows);
    size_t bytes_used      = BYTES_OF(gui_used);

    size_t bytes_zorder    = BYTES_OF(g_winZorder);
    size_t bytes_zcount    = BYTES_OF(g_winZcount);

    size_t bytes_gadgets_total =
        gb.basePool + gb.btnPool + gb.chkPool + gb.radPool + gb.sbPool + gb.bvPool;

    size_t bytes_windowing_total =
        bytes_windows + bytes_used + bytes_zorder + bytes_zcount;

    size_t total =
        bytes_ui +
        bytes_windowing_total +
        bytes_gadgets_total;

    printf("\n[SBOS] Reserved UI memory (static pools)\n");
    printf("--------------------------------------------------\n");

    printf("Core\n");
    printf("  g_ui              : %zu bytes\n", bytes_ui);

    printf("\nWindows + Z-order\n");
    printf("  gui_windows       : %zu bytes (%zu each x %zu)\n",
           bytes_windows, sizeof(gui_windows[0]),
           bytes_windows / sizeof(gui_windows[0]));
    printf("  gui_used          : %zu bytes\n", bytes_used);
    printf("  g_winZorder       : %zu bytes\n", bytes_zorder);
    printf("  g_winZcount       : %zu bytes\n", bytes_zcount);
    printf("  Subtotal          : %zu bytes (%.1f KB)\n",
           bytes_windowing_total, (double)bytes_windowing_total / 1024.0);

    printf("\nGadget pools\n");
    printf("  basePool          : %zu bytes\n", gb.basePool);
    printf("  btnPool           : %zu bytes\n", gb.btnPool);
    printf("  chkPool           : %zu bytes\n", gb.chkPool);
    printf("  radPool           : %zu bytes\n", gb.radPool);
    printf("  sbPool            : %zu bytes\n", gb.sbPool);
    printf("  bvPool            : %zu bytes\n", gb.bvPool);
    printf("  Subtotal          : %zu bytes (%.1f KB)\n",
           bytes_gadgets_total, (double)bytes_gadgets_total / 1024.0);

    printf("UI budget           : %u bytes (%.1f%% used)\n",
           SBOS_UI_BUDGET_BYTES, 100.0 * (double)total / (double)SBOS_UI_BUDGET_BYTES);

    printf("--------------------------------------------------\n");
    printf("TOTAL RESERVED UI   : %zu bytes (%.1f KB)\n",
           total, (double)total / 1024.0);
    printf("--------------------------------------------------\n\n");
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

uint32_t SBOS_GetBasePoolSize();

void SBOS_print_ui_usage(void){
    SBOS_GadgetPoolBytes rb = SBOS_get_gadget_pool_bytes();
    SBOS_UiUsageCounts   uc = SBOS_get_ui_usage_counts();

    // Reserved bytes (from real arrays)
    size_t r_windows = sizeof(gui_windows);
    size_t r_used    = sizeof(gui_used);
    size_t r_zorder  = sizeof(g_winZorder);
    size_t r_zcount  = sizeof(g_winZcount);
    size_t r_ui      = sizeof(g_ui);

    size_t r_gadgets = rb.basePool + rb.btnPool + rb.chkPool + rb.radPool + rb.sbPool + rb.bvPool;

    size_t r_total = r_ui + r_windows + r_used + r_zorder + r_zcount + r_gadgets;

    // Used bytes (counts * sizeof element)
    size_t u_windows = (size_t)uc.win_used * sizeof(gui_windows[0]);

    // base_used only if meaningful; else set to 0
    size_t u_base    = (size_t)uc.base_used * SBOS_GetBasePoolSize();

    size_t u_btn     = (size_t)uc.btn_used * sizeof(GAD_BUTTON_T);
    size_t u_chk     = (size_t)uc.chk_used * sizeof(GAD_CHECKBOX_T);
    size_t u_rad     = (size_t)uc.rad_used * sizeof(GAD_RADIO_T);
    size_t u_sb      = (size_t)uc.sb_used  * sizeof(GAD_SCROLLBAR_T);
    size_t u_bv      = (size_t)uc.bv_used  * sizeof(GAD_BITMAPVIEW_T);

    size_t u_gadgets = u_base + u_btn + u_chk + u_rad + u_sb + u_bv;

    // Note: zorder tables are always reserved+used; same for gui_used[].
    size_t u_total = sizeof(g_ui) + u_windows + r_used + r_zorder + r_zcount + u_gadgets;

    printf("\n[SBOS] UI memory usage (live)\n");
    printf("--------------------------------------------------\n");
    printf("Reserved total     : %zu bytes (%.1f KB)\n", r_total, (double)r_total/1024.0);
    printf("Used total         : %zu bytes (%.1f KB)\n", u_total, (double)u_total/1024.0);
    printf("Free in pools      : %zu bytes (%.1f KB)\n",
           (r_total > u_total) ? (r_total - u_total) : 0,
           (double)((r_total > u_total) ? (r_total - u_total) : 0)/1024.0);

    printf("\nWindows            : %u / %u (used/reserved)\n", uc.win_used, (unsigned)(sizeof(gui_windows)/sizeof(gui_windows[0])));
    printf("Buttons            : %u / %u\n", uc.btn_used, (unsigned)(rb.btnPool / sizeof(GAD_BUTTON_T)));
    printf("Checkboxes         : %u / %u\n", uc.chk_used, (unsigned)(rb.chkPool / sizeof(GAD_CHECKBOX_T)));
    printf("Radios             : %u / %u\n", uc.rad_used, (unsigned)(rb.radPool / sizeof(GAD_RADIO_T)));
    printf("Scrollbars         : %u / %u\n", uc.sb_used,  (unsigned)(rb.sbPool  / sizeof(GAD_SCROLLBAR_T)));
    printf("BitmapViews        : %u / %u\n", uc.bv_used,  (unsigned)(rb.bvPool  / sizeof(GAD_BITMAPVIEW_T)));
    printf("--------------------------------------------------\n\n");

    double usage_pct = 0.0;
    if (r_total > 0) {
        usage_pct = ((double)u_total / (double)r_total) * 100.0;
    }

    if (usage_pct > 100.0) usage_pct = 100.0;

    printf("\n[SBOS] UI memory usage (live)\n");
    printf("--------------------------------------------------\n");
    printf("Reserved total     : %zu bytes (%.1f KB)\n",
           r_total, (double)r_total / 1024.0);
    printf("Used total         : %zu bytes (%.1f KB)\n",
           u_total, (double)u_total / 1024.0);
    printf("Usage              : %zu / %zu bytes (%.2f%%)\n",
           u_total, r_total, usage_pct);
    printf("Free in pools      : %zu bytes (%.1f KB)\n",
           (r_total > u_total) ? (r_total - u_total) : 0,
           (double)((r_total > u_total) ? (r_total - u_total) : 0) / 1024.0);
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

Rect16 win_inner_rect(const sbx_window_t *w){
    int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0;

    Rect16 r;
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

    Rect16 inner = win_inner_rect(w);

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
    g_ui.mouse_down  = 0;
    g_ui.down_win    = SBW_INVALID_ID;
    g_ui.down_region = WH_NONE;

    ui_clear_title_latch();
    ui_clear_drag();

    g_ui.resize_win  = SBW_INVALID_ID;

    g_ui.capturedGadget = NULL;
    g_ui.capturedGadgetRect = NULL;
}

void SBOS_setFocus(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) {
        g_focusWin = SBW_INVALID_ID;
        return;
    }

    if (gui_windows[id].flags & SBX_WF_NOFOCUS) return;

    g_focusWin = id;
}

SBXWindowId SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags){
    for (SBXWindowId i = 0; i < MAX_WINDOWS; i++) {
        if (!gui_used[i]) {
            gui_used[i] = 1;

            sbx_window_t *w = &gui_windows[i];
            w->winrect.x = x;
            w->winrect.y = y;
            w->winrect.w = (int16_t)width;
            w->winrect.h = (int16_t)height;
            w->flags = flags;

            //w->ctrl_count = 0;
            //w->id = i;


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
                return SBW_INVALID_ID;
            }

            normalize_zorder();
            SBOS_paintAllWindows();

            w->self = i;
            for (int k = 0; k < MAX_GADGETS_PER_WINDOW; k++) w->GADGETS[k] = NULL;


            return i;
        }
    }

    return SBW_INVALID_ID;
}

sbx_window_t* SBOS_getWindow(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return 0;
    return &gui_windows[id];
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

    const uint16_t borderPen = (id == g_focusWin) ? WIN_BORDER_ACTIVE_PEN : WIN_BORDER_INACTIVE_PEN;

    // --- outer frame outline (always) ---
    draw_rect_outline_thick(win_x, win_y, win_w, win_h, WIN_BORDER, borderPen);

    // --- client background ---
    // IMPORTANT: clientrect is the app-drawable area. Fill it.
    sbgfx_drawbox(cli_x, cli_y, cli_w, cli_h, WIN_BG_PEN);



    // --- draw client gadgets (clip to clientrect) ---
    ui_clip_set(cli_x, cli_y, cli_w, cli_h);
    SBOS_drawControlsFiltered(w, 0);
    ui_clip_disable();

    // If borderless, we don't draw frame/title/gutter chrome.
    if (w->flags & SBX_WF_NOBORDER) {
        return;
    }

    // --- compute title bar height used by frame chrome ---
    const int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? (WIN_TITLE_HEIGHT + 4) : 0;

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
        draw_bevel(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, WIN_BEVEL_H, WIN_BEVEL_L, 0);
        sbgfx_glyph(gx, gy, glyph_resize);

        // polish
        gfx_setcolour(WIN_BEVEL_L);
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
    draw_bevel(win_x, win_y, win_w, win_h, WIN_BEVEL_H, WIN_BEVEL_L, 0);

    // inner bevel around the client area
    draw_bevel((int16_t)(cli_x - 1), (int16_t)(cli_y - 1), (int16_t)(cli_w + 2), (int16_t)(cli_h + 2), WIN_BEVEL_H, WIN_BEVEL_L, 1);

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
        uint16_t titlePen = (id == g_focusWin) ? WIN_TITLE_PEN_ACTIVE : WIN_TITLE_PEN_INACTIVE;
        gfx_setcolour(titlePen);

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
        gfx_setcolour(WIN_BEVEL_L);
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

static int z_find(SBXWindowId id){
    for (int i = 0; i < (int)g_winZcount; i++) {
        if (g_winZorder[i] == id) return i;
    }
    return -1;
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
    if (!w) return;
    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g) continue;
        SBOS_destroyGadget(base_to_handle(g));
        w->GADGETS[i] = NULL;
    }
}


void SBOS_destroyWindow(SBXWindowId id){
    if (id >= MAX_WINDOWS) return;
    if (!gui_used[id]) return;

    sbx_window_t *w = &gui_windows[id];

    if (g_ui.capturedGadget && g_ui.capturedGadget->winhnd == id) {
        g_ui.capturedGadget = NULL;
    }
    if (g_ui.drag_win == id) ui_clear_drag();
    if (g_ui.resize_win == id) g_ui.resize_win = SBW_INVALID_ID;
    if (g_ui.title_win == id) ui_clear_title_latch();
    if (g_ui.down_win == id) { g_ui.down_win = SBW_INVALID_ID; g_ui.down_region = WH_NONE; }

    destroy_window_gadgets(w);

    w->flags = 0;
    w->title[0] = '\0';

    gui_used[id] = 0;
    z_remove(id);

    if (g_focusWin == id)
        g_focusWin = findTopFocusable();

    normalize_zorder();
    SBOS_paintAllWindows();
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

void SBOS_paintAllWindows(void){
    for (int zi = 0; zi < (int)g_winZcount; zi++) {
        SBXWindowId id = g_winZorder[zi];
        if (id >= MAX_WINDOWS) continue;
        if (!gui_used[id]) continue;

        if (gui_windows[id].flags & SBX_WF_VISIBLE) {
            SBOS_paintWindow(id);
        }
    }
}

static inline uint8_t is_title_gadget_region(WHitRegion r){
    return (r == WH_CLOSE) || (r == WH_MINIMISE) || (r == WH_MAXRESTORE) || (r == WH_ZORDER);
}

static inline uint8_t mousept_in_rect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h){
    Rect16 r = r16(x,y,w,h);
    return pt_in_r16(px,py,&r);
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
    int16_t gr = win_gutter_right(w);
    int16_t gb = win_gutter_bottom(w);
    Rect16 inner = win_inner_rect(w);

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

// Map thumb position (0..travel) -> VALUE (min..max)
static inline int16_t sb_value_from_thumb_pos(int16_t pos, int16_t min, int16_t max, int16_t travel){
    if (travel <= 0) return min;
    if (max <= min) return min;

    if (pos < 0) pos = 0;
    if (pos > travel) pos = travel;

    int32_t range = (int32_t)max - (int32_t)min;
    int32_t v = ((int32_t)pos * range + travel/2) / travel;

    return (int16_t)(min + (int16_t)v);
}


// ===================== HITTEST (VALUE-BASED) =====================

static SBPart hittest_scrollbar_part(const sbx_window_t *w, const GAD_SCROLLBAR_T *s,
                                     int16_t mx, int16_t my,
                                     int16_t *out_thumb_axis_start,
                                     int16_t *out_thumb_len,
                                     int16_t *out_track_axis_start)
{
    // Compute abs rect same way draw does
    int16_t ax, ay, aw, ah;

    Rect16 inner = win_inner_rect(w);
    int16_t reserveR = win_inner_reserve_right(w);
    int16_t reserveB = win_inner_reserve_bottom(w);

    // If docked, adjust the position accordingly
    if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT) && (s->orient == SB_ORIENT_VERT)) {
        ax = (int16_t)(inner.x + inner.w - SB_SCROLL_THICK + SB_RDOCK_OFFSET_X);
        ay = inner.y + SB_RDOCK_OFFSET_Y;
        aw = SB_SCROLL_THICK;
        ah = (int16_t)(inner.h - reserveB);
    } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM) && (s->orient == SB_ORIENT_HORZ)) {
        ax = inner.x + SB_BDOCK_OFFSET_X;
        ay = (int16_t)(inner.y + inner.h - SB_SCROLL_THICK + SB_BDOCK_OFFSET_Y);
        aw = (int16_t)(inner.w - reserveR);
        ah = SB_SCROLL_THICK;
    } else {
        ax = (int16_t)(w->clientrect.x + s->h.rect.x);
        ay = (int16_t)(w->clientrect.y + s->h.rect.y);
        aw = s->h.rect.w;
        ah = s->h.rect.h;
    }

    // Check if the mouse is within the scrollbar area
    if (!mousept_in_rect(mx, my, ax, ay, aw, ah)) return SB_PART_NONE;

    // Shrink the track area if arrows are enabled

    if (s->show_arrows) {
        if (s->orient == SB_ORIENT_VERT) {
            ah -= SB_ARROW_SHRINK * 2;  // Shrink the height for the top and bottom arrows
            ay += SB_ARROW_SHRINK;      // Move the track down to fit arrows
        } else {
            aw -= SB_ARROW_SHRINK * 2;  // Shrink the width for the left and right arrows
            ax += SB_ARROW_SHRINK;      // Move the track right to fit arrows
        }
    }


    // Calculate track length (reduced by arrows)
    int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;

    // Ensure the track length doesn't go negative
    if (track_len < 0) track_len = 0;

    // Calculate the thumb length
    int16_t thumb_len = sb_thumb_len_from_step(track_len, s->min, s->max, s->step);

    // Calculate the available space (travel distance) for the thumb
    int16_t travel = (int16_t)(track_len - thumb_len);
    if (travel < 0) travel = 0;

    // Calculate the thumb position based on the value
    int16_t tpos = sb_thumb_pos_from_value(s->value, s->min, s->max, travel);

    // Start position of the track
    int16_t track_axis_start = (s->orient == SB_ORIENT_VERT) ? ay : ax;
    int16_t thumb_axis_start = (int16_t)(track_axis_start + tpos);

    // Output the thumb's start position and length
    if (out_thumb_axis_start) *out_thumb_axis_start = thumb_axis_start;
    if (out_thumb_len) *out_thumb_len = thumb_len;
    if (out_track_axis_start) *out_track_axis_start = track_axis_start;

    // Handle arrow clicks
    if (s->show_arrows) {
        if (s->orient == SB_ORIENT_VERT) {
            // Vertical scrollbar arrows

            // FIX: Subtract SB_ARROW_SHRINK because 'ay' is currently the track start
            if (mousept_in_rect(mx, my, ax, ay - SB_ARROW_SHRINK, aw, SB_ARROW_SHRINK)) {
                printf("SCROLL-BAR-BUTTON!! -- {UP}\n");
                return SB_PART_ARROW_UP;
            }

            // Bottom arrow (this actually works fine with current ax/ay/ah values)
            if (mousept_in_rect(mx, my, ax, ay + ah, aw, SB_ARROW_SHRINK)) {
                printf("SCROLL-BAR-BUTTON!! -- {DOWN}\n");
                return SB_PART_ARROW_DOWN;
            }
        } else {
            // Horizontal scrollbar arrows

            // FIX: Subtract SB_ARROW_SHRINK because 'ax' is currently the track start
            if (mousept_in_rect(mx, my, ax - SB_ARROW_SHRINK, ay, SB_ARROW_SHRINK, ah)) {
                printf("SCROLL-BAR-BUTTON!! -- {LEFT}\n");
                return SB_PART_ARROW_LEFT;
            }

            // Right arrow
            if (mousept_in_rect(mx, my, ax + aw, ay, SB_ARROW_SHRINK, ah)) {
                printf("SCROLL-BAR-BUTTON!! -- {RIGHT}\n");
                return SB_PART_ARROW_RIGHT;
            }
        }
    }

    // Check if the mouse is over the thumb
    if (s->orient == SB_ORIENT_VERT) {
        if (mousept_in_rect(mx, my, ax, thumb_axis_start, aw, thumb_len)) return SB_PART_THUMB;
    } else {
        if (mousept_in_rect(mx, my, thumb_axis_start, ay, thumb_len, ah)) return SB_PART_THUMB;
    }

    return SB_PART_TRACK;
}

static inline uint8_t gadget_is_docked(const GAD_HDR_T *h){
    return (h->flags & (GAD_TOOL_DOCKED_RIGHT | GAD_TOOL_DOCKED_BOTTOM)) != 0;
}

static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock){
    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
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
            case GAD_BUTTON:     draw_button(w, g);     break;
            case GAD_CHECKBOX:   draw_checkbox(w, g);   break;
            case GAD_SCROLLBAR:  draw_scrollbar(w, g);  break;
            case GAD_RADIO:      draw_radio(w, g);      break;
            case GAD_BITMAPVIEW: draw_bitmapview(w, g); break;
            case GAD_LISTBOX:    draw_listbox(w, g);    break;

            default: break;     // if we got here, then the GUI is BARFING UP randomness, so stop it here ;)
        }
    }
    return;
}

static GADGET_BASE_T* hittest_gadget(sbx_window_t *w, int16_t mx, int16_t my){
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

        Rect16 r = r16(h->rect.x, h->rect.y, h->rect.w, h->rect.h);
        if (pt_in_r16(lx, ly, &r)) return g;
    }
    return NULL;
}

static uint8_t gadget_mouse_inside(const sbx_window_t *w, const GADGET_BASE_T *g, int16_t mx, int16_t my){
    if (!w || !g || !g->gadget) return 0;

    int16_t lx = (int16_t)(mx - w->clientrect.x);
    int16_t ly = (int16_t)(my - w->clientrect.y);

    GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
    Rect16 r = r16(h->rect.x, h->rect.y, h->rect.w, h->rect.h);
    return pt_in_r16(lx, ly, &r);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////  WINDOW INTERFACE WITH MOUSE  //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//// introducing a very function pointer way of doing things, instead of a branch of IF ELSE Types,


static uint32_t (*callMouseMoveEvt)   (sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my) = NULL;
static uint32_t (*callMouseReleaseEvt)(GADGET_BASE_T *g, int16_t *mx, int16_t *my) = NULL;

// function calls for MouseMoves ____
// __BITMAPVIEW BOX__

uint32_t onMouseMoveBitmapView(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my){
    // the bitmap view system is more or less simples
    (void)win; (void)evt;

    GAD_BITMAPVIEW_T *bv = (GAD_BITMAPVIEW_T*) g->gadget;
    if (bv && bv->panning && (bv->bv_flags & BVF_PAN)) {
        int16_t dx = (int16_t) (*mx - bv->pan_start_mx);
        int16_t dy = (int16_t) (*my - bv->pan_start_my);
        bv->scroll_x = (int16_t) (bv->pan_start_x - dx);
        bv->scroll_y = (int16_t) (bv->pan_start_y - dy);
        SBOS_paintAllWindows();
        return(1);
    }
    return 0;
}

uint32_t onMouseMoveScrollbar(sbx_window_t *win, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my){
    (void)win; (void)evt;
    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*) g->gadget;
    if (s->dragging) {
        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;
        int16_t new_thumb_pos = (int16_t) (m_axis - g_ui.sb_track_start - g_ui.sb_drag_off);
        int16_t new_val = sb_value_from_thumb_pos(new_thumb_pos, s->min, s->max, g_ui.sb_travel);

        if (new_val != s->value) {
            s->value = new_val;
            printf("SCROLLING: %d\n", s->value);
            SBOS_paintAllWindows();
        }
    }
    if (!s || !s->dragging) return 1;

    return 0;   // this is happening all the time
}

/// MOUSE CAPTURE FEATURES -----------------


uint32_t onMouseDownCaptureScrollBar(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my){
    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*) g->gadget;

    int16_t thumb_start = 0, thumb_len = 0, track_start = 0;
    SBPart part = hittest_scrollbar_part(w, s, *mx, *my, &thumb_start, &thumb_len, &track_start);

    s->dragging = 0;

    if (part == SB_PART_ARROW_UP) {
        s->value = clamp_i16(s->value - s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_DOWN) {
        s->value = clamp_i16(s->value + s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_LEFT) {
        s->value = clamp_i16(s->value - s->step, s->min, s->max);
    } else if (part == SB_PART_ARROW_RIGHT) {
        s->value = clamp_i16(s->value + s->step, s->min, s->max);

    } else if (part == SB_PART_THUMB) {
        s->dragging = 1;

        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;

        // Cache drag offset (mouse position relative to thumb start)
        g_ui.sb_drag_off = (int16_t) (m_axis - thumb_start);
        s->drag_off = g_ui.sb_drag_off; // optional

        // Cache track start (axis start of the track in screen coords)
        g_ui.sb_track_start = track_start;

        // Compute track_len using the same geometry logic

        Rect16 inner = win_inner_rect(w);
        int16_t reserveR = win_inner_reserve_right(w);
        int16_t reserveB = win_inner_reserve_bottom(w);

        int16_t ax, ay, aw, ah;

        if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT)
            && (s->orient == SB_ORIENT_VERT)) {
            ax = (int16_t) (inner.x + inner.w
                            - SB_SCROLL_THICK);
            ay = inner.y;
            aw = SB_SCROLL_THICK;
            ah = (int16_t) (inner.h - reserveB);
        } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM)
                   && (s->orient == SB_ORIENT_HORZ)) {
            ax = inner.x;
            ay = (int16_t) (inner.y + inner.h
                            - SB_SCROLL_THICK);
            aw = (int16_t) (inner.w - reserveR);
            ah = SB_SCROLL_THICK;
        } else {
            ax = (int16_t) (w->clientrect.x + s->h.rect.x);
            ay = (int16_t) (w->clientrect.y + s->h.rect.y);
            aw = s->h.rect.w;
            ah = s->h.rect.h;
        }

        if (s->show_arrows) {
            if (s->orient == SB_ORIENT_VERT) {
                ah -= SB_ARROW_SHRINK * 2;
                ay += SB_ARROW_SHRINK; // Not strictly needed for length calc, but good for consistency
            } else {
                aw -= SB_ARROW_SHRINK * 2;
                ax += SB_ARROW_SHRINK;
            }
        }

        int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;
        int16_t travel = (int16_t) (track_len - thumb_len);
        if (travel < 0)
            travel = 0;
        g_ui.sb_travel = travel;

    } else if (part == SB_PART_TRACK) {
        // Track step click (VALUE-BASED)
        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? *my : *mx;

        // ensure sane step
        int16_t step = (s->step <= 0) ? 1 : s->step;

        if (m_axis < thumb_start)
            s->value = clamp_i16( (int16_t) (s->value - step), s->min, s->max);
        else
            s->value = clamp_i16(
                (int16_t) (s->value + step), s->min,
                s->max);

        // IMPORTANT: not dragging
        s->dragging = 0;
        s->drag_off = 0;
        g_ui.sb_track_start = 0;
        g_ui.sb_travel = 0;
        g_ui.sb_drag_off = 0;
        //SBOS_paintAllWindows();
    }

    //if (!s->dragging) return 1; // not dragging,
    return 0x00;    // always ok FOR NOW
}




static int16_t listbox_index_from_mouse(const sbx_window_t *w, const GAD_LISTBOX_T *lb,
                                        int16_t mx, int16_t my,
                                        int16_t *out_rows_visible)
{
    if (!w || !lb || !lb->items) return -1;

    ItemLists_t *list = lb->items;

    // Gadget absolute rect (screen coords)
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    int16_t aw = lb->h.rect.w;
    int16_t ah = lb->h.rect.h;
    if (aw <= 0 || ah <= 0) return -1;

    // Inner matches draw_listbox inset
    int16_t ix = (int16_t)(ax + 2);
    int16_t iy = (int16_t)(ay + 2);
    int16_t iw = (int16_t)(aw - 4);
    int16_t ih = (int16_t)(ah - 4);
    if (iw <= 0 || ih <= 0) return -1;

    // Defaults
    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    // Visible rows (match your draw fix: 2px less sensitive)
    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return -1;

    int16_t rows = (int16_t)(usable_h / row_h);
    if (rows <= 0) return -1;

    if (out_rows_visible) *out_rows_visible = rows;

    int16_t count = (int16_t)list->count;
    if (count <= 0) return -1;

    // Mouse -> local y inside the list rows band
    int16_t ly = (int16_t)(my - iy - pad_y);

    // Clamp row when mouse is above/below the band (this is the key change)
    int16_t row;
    if (ly < 0) {
        row = 0;
    } else {
        int16_t band_h = (int16_t)(rows * row_h);
        if (ly >= band_h) row = (int16_t)(rows - 1);
        else              row = (int16_t)(ly / row_h);
    }

    // Convert to model index and clamp
    int16_t idx = (int16_t)(list->top + row);
    if (idx < 0) idx = 0;
    if (idx >= count) idx = (int16_t)(count - 1);

    return idx;
}

uint32_t onMouseMoveListBox(sbx_window_t *w, GADGET_BASE_T *g, MouseEvt *evt, int16_t *mx, int16_t *my)
{
    (void)evt;
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    ItemLists_t *list = lb->items;

    int16_t rows = 0;
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);
    if (idx < 0) return 0;

    // Recompute the same inner band to detect “outside above/below”
    int16_t ax = (int16_t)(w->clientrect.x + lb->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + lb->h.rect.y);
    int16_t aw = lb->h.rect.w;
    int16_t ah = lb->h.rect.h;

    int16_t ix = (int16_t)(ax + 2);
    int16_t iy = (int16_t)(ay + 2);
    int16_t ih = (int16_t)(ah - 4);

    int16_t row_h = (lb->row_h > 0) ? lb->row_h : 16;
    int16_t pad_y = (lb->padding_y > 0) ? lb->padding_y : 2;

    // Same band height used by listbox_index_from_mouse()
    int16_t usable_h = (int16_t)(ih - pad_y * 2 + 2);
    if (usable_h <= 0) return 0;

    int16_t band_h = (int16_t)(rows * row_h);

    // Mouse position relative to first row
    int16_t ly = (int16_t)(*my - iy - pad_y);

    // Auto-scroll when dragging outside above/below
    int16_t count = (int16_t)list->count;
    int16_t maxTop = (int16_t)(count - rows);
    if (maxTop < 0) maxTop = 0;

    uint8_t changed = 0;

    if (ly < 0) {
        if (list->top > 0) { list->top--; changed = 1; }
    } else if (ly >= band_h) {
        if (list->top < maxTop) { list->top++; changed = 1; }
    }

    // Update selection (now keeps working even outside)
    if (list->sel != idx) {
        list->sel = idx;
        changed = 1;
    }

    // Keep selection visible (nice behavior even when not outside)
    if (list->sel < list->top) { list->top = list->sel; changed = 1; }
    if (list->sel >= list->top + rows) { list->top = (int16_t)(list->sel - rows + 1); changed = 1; }

    // Clamp top
    if (list->top < 0) list->top = 0;
    if (list->top > maxTop) list->top = maxTop;

    if (changed) SBOS_paintAllWindows();
    return 0;
}


uint32_t onMouseDownCaptureListBox(sbx_window_t *w, GADGET_BASE_T *g, int16_t *mx, int16_t *my)
{
    if (!w || !g || !g->gadget) return 1;

    GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)g->gadget;
    if (!lb->items) return 1;

    int16_t rows = 0;
    int16_t idx = listbox_index_from_mouse(w, lb, *mx, *my, &rows);
    if (idx < 0) return 0; // clicked inside gadget but not on a row: ignore

    ItemLists_t *list = lb->items;

    if (list->sel != idx) {
        list->sel = idx;

        // keep selection visible (nice UX)
        if (list->sel < list->top) list->top = list->sel;
        if (list->sel >= list->top + rows) list->top = (int16_t)(list->sel - rows + 1);

        SBOS_paintAllWindows();
    }

    return 0;
}





uint32_t doCancellables(sbx_window_t *gw, GADGET_BASE_T *g){
    switch (g->gadgetType) {
        case GAD_BUTTON: {
            GAD_BUTTON_T *b = (GAD_BUTTON_T*) g->gadget;

            b->current_option++;
            if (b->current_option > b->max_options - 1)
                b->current_option = 0;

            printf("BUTTON CLICK: %s (cycle %d)\r\n", b->text,
                   b->current_option);
        }
        break;
        case GAD_CHECKBOX: {
            GAD_CHECKBOX_T *c = (GAD_CHECKBOX_T*) g->gadget;
            c->checked ^= 1;
            printf("CHECKBOX TOGGLE: %s => %d\r\n", c->text, c->checked);
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
            printf("RADIO SELECT: %s (grp %d)\r\n", r->text, r->group);
        }
        break;

        case GAD_LISTBOX: {
            // this should be handled on the onMouseReleaseListBox
        }
        break;


        default:
            break;
    }
    return (0x00);/// this doesnt really need to worry about anything
}


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
                        g_ui.capturedGadget = g;

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
                                    callMouseReleaseEvt = NULL;                 // you can add onMouseUpListBox later
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
                    if (nw < 100)
                        nw = 100;
                    if (nh < 100)
                        nh = 100;

                    w->winrect.w = nw;
                    w->winrect.h = nh;

                    layoutWindow(w);
                    repaint = 1;
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
                }
                break;
            }

            // ######################### GADGETS IN WINDOW #########################
            if (g_ui.mouse_down && g_ui.capturedGadget) {
                sbx_window_t *gw = SBOS_getWindow(g_ui.capturedGadget->winhnd);
                if (gw) {
                    // this handles the last gadget that was click mouse down'd (this is NOT a multi-point OS so thank GOD!)
                    if(callMouseMoveEvt) {
                        uint32_t stop = callMouseMoveEvt(gw, g_ui.capturedGadget, &evt, &mx, &my);
                        if (stop) { repaint = 1; break; }  // skip default inside/down behavior
                    }

                    // non-scrollbar default "down if inside" behaviour
                    // this is default when something can be cancelled just by moving out the gadget.
                    GAD_HDR_T *h = (GAD_HDR_T*) g_ui.capturedGadget->gadget;
                    uint8_t inside = gadget_mouse_inside(gw, g_ui.capturedGadget, mx, my);
                    uint8_t newDown = inside ? 1 : 0;
                    if (newDown != h->down) {
                        h->down = newDown;
                        repaint = 1;
                    }
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
                    if (rclick == WH_CLOSE) {
                        SBOS_destroyWindow(wclick);
                        printf("GLYPH HIT - Close Window\r\n");
                    } else if (rclick == WH_ZORDER) {
                        SBOS_sendToBack(wclick);
                        printf("GLYPH HIT - Zorder\r\n");
                    } else if (rclick == WH_MINIMISE) {
                        printf("GLYPH HIT - Minimised\r\n");
                        /* TODO */
                    } else if (rclick == WH_MAXRESTORE) {
                        printf("GLYPH HIT - MaxRestore\r\n");
                        /* TODO */
                    }
                }
                repaint = 1;
                cleanups = 1;
                break;
            }

            /// the gadget was released
            if (g_ui.capturedGadget) {
                GADGET_BASE_T *g = g_ui.capturedGadget;
                sbx_window_t *gw = SBOS_getWindow(g->winhnd);

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

                g_ui.capturedGadget = NULL;

                bool clickActivate =
                    (g->gadgetType == GAD_BUTTON)   ||
                    (g->gadgetType == GAD_CHECKBOX) ||
                    (g->gadgetType == GAD_RADIO)    ||
                    (g->gadgetType == GAD_LISTBOX);


                if (clickActivate && inside) {
                    doCancellables(gw, g);

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

