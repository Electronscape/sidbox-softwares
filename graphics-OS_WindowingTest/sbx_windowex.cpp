//// SBX_WINDOWEX.CPP //////


#include "stdio.h"

#include "stdint.h"


#include "sbx_render.h"
#include "sbx_input.h"
#include "sbx_windowex.h"
#include "sbx_gadgets.h"

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







// graphics glyphs up here out the way
uint8_t glyph_resize[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02
};



/////// prototypes //////////////////

static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock);
static void normalize_zorder(void);

//////////////////////////////////////////////////////////////////////////////////////////////////////
/// FUNCTIONALITY BELOW //////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

// semi macro functions //
static inline int16_t win_client_x(const sbx_window_t *w, int16_t mx) { return (int16_t)(mx - w->clientrect.x); }
static inline int16_t win_client_y(const sbx_window_t *w, int16_t my) { return (int16_t)(my - w->clientrect.y); }

static inline int16_t i16_min(int16_t a, int16_t b){ return (a < b) ? a : b; }
static inline int16_t i16_max(int16_t a, int16_t b){ return (a > b) ? a : b; }


typedef struct { int16_t x, y, w, h; } Rect16;
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


void initWb(void){
    ui_clear_title_latch();
    ui_clear_drag();
    g_ui.mouse_down  = 0;
    g_ui.down_win    = SBW_INVALID_ID;
    g_ui.down_region = WH_NONE;
    g_ui.resize_win  = SBW_INVALID_ID;
    SBOS_gadgetsInit();

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


static inline int16_t win_gutter_right(const sbx_window_t *w){
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER))
        return (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);  // the strip already draw
    return 0;
}

static inline int16_t win_gutter_bottom(const sbx_window_t *w){
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER))
        return (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);
    return 0;
}



static void layoutDockedControls(sbx_window_t *w){
    if (!w) return;

    // aw/ah are "content/app" size in *pixels*, client-local
    int16_t aw = w->clientrect.w;
    int16_t ah = w->clientrect.h;

    w->clientrect.w = aw ;
    w->clientrect.h = ah;


    // shrink client area if docked scrollbars exist
    if (w->hasDockedGadget & GAD_TOOL_DOCKED_RIGHT) {
        if (w->clientrect.w >= SB_SCROLL_THICK)
            w->clientrect.w = (int16_t)(w->clientrect.w - SB_SCROLL_THICK);
        else
            w->clientrect.w = 0;
    }

    if (w->hasDockedGadget & GAD_TOOL_DOCKED_BOTTOM) {
        if (w->clientrect.h >= SB_SCROLL_THICK)
            w->clientrect.h = (int16_t)(w->clientrect.h - SB_SCROLL_THICK);
        else
            w->clientrect.h = 0;
    }



}


static void layoutWindow(sbx_window_t *w){
    if (!w) return;

    int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0;

    // Start with "clientrect = usable app area"
    if (w->flags & SBX_WF_NOBORDER) {
        // Borderless: entire window is client
        w->clientrect.x = w->winrect.x;
        w->clientrect.y = w->winrect.y;
        w->clientrect.w = w->winrect.w;
        w->clientrect.h = w->winrect.h;
    } else {
        w->clientrect.x = (int16_t)(w->winrect.x + WIN_BORDER);
        w->clientrect.y = (int16_t)(w->winrect.y + WIN_BORDER + title_h);
        w->clientrect.w = (int16_t)(w->winrect.w - (WIN_BORDER * 2));
        w->clientrect.h = (int16_t)(w->winrect.h - ((WIN_BORDER * 2) + title_h));
    }

    // Safety clamp (avoid negative sizes)
    if (w->clientrect.w < 0) w->clientrect.w = 0;
    if (w->clientrect.h < 0) w->clientrect.h = 0;

    // If resizable, reserve the bottom/right gutter area (so gadgets don't draw under it)
    int16_t gr = win_gutter_right(w);
    int16_t gb = win_gutter_bottom(w);

    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        w->clientrect.h = (int16_t)(w->clientrect.h - gb);
    }
    if(w->hasDockedGadget & GAD_TOOL_DOCKED_RIGHT){
        w->clientrect.w = (int16_t)(w->clientrect.w - gr); // <-- this should only happen if there is a docked slider - for now turn it off
    }
    if (w->clientrect.w < 0) w->clientrect.w = 0;
    if (w->clientrect.h < 0) w->clientrect.h = 0;

    // Later: docked controls (scrollbars etc) should shrink clientrect further.
    // layoutDockedControls(w);  // keep, but update it to use clientrect now
    layoutDockedControls(w);
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
/*
static void ctrl_set_text(sbx_control_t *c, const char *text){
    if (!c) return;
    if (!text) { c->text[0] = '\0'; return; }

    int i = 0;
    for (; text[i] && i < (int)sizeof(c->text) - 1; i++)
        c->text[i] = text[i];
    c->text[i] = '\0';
}
*/



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
            w->hasDockedGadget = 0;

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

    // --- draw resize gutter + glyph (chrome, not part of client) ---
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {

        // bottom-right glyph box (DO NOT subtract WIN_BORDER here)
        const int16_t gx = (int16_t)(win_x + win_w - WIN_RESIZE_GLYPH_SIZE);
        const int16_t gy = (int16_t)(win_y + win_h - WIN_RESIZE_GLYPH_SIZE);

        // bottom gutter strip spans inside the border area
        const int16_t inner_x = (int16_t)(win_x + WIN_BORDER);
        const int16_t inner_w = (int16_t)(win_w - (WIN_BORDER * 2));

        ui_clip_set(win_x, win_y, win_w, win_h);



        if(w->hasDockedGadget & GAD_TOOL_DOCKED_RIGHT)
            sbgfx_drawbox(gx, win_y, WIN_RESIZE_GLYPH_SIZE, win_h, borderPen);

        // bottom bar
        sbgfx_drawbox(inner_x, (int16_t)(gy + 1), inner_w, (int16_t)(WIN_RESIZE_GLYPH_SIZE - 2), borderPen);

        ui_clip_disable();

        sbgfx_drawbox(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, borderPen);
        draw_bevel(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, WIN_BEVEL_H, WIN_BEVEL_L, 0);
        sbgfx_glyph(gx, gy, glyph_resize);

        // polish
        gfx_setcolour(WIN_BEVEL_L);
        ui_vline((int16_t)(gx - 1), gy, WIN_RESIZE_GLYPH_SIZE);

        if(w->hasDockedGadget & GAD_TOOL_DOCKED_RIGHT)
            //ui_hline((int16_t)(gx), (int16_t)(WIN_RESIZE_GLYPH_SIZE), WIN_BORDER);
            ui_hline((int16_t)(gx), (int16_t)(gy - 1), WIN_RESIZE_GLYPH_SIZE);
        else
            ui_hline((int16_t)(win_x + win_w - WIN_BORDER), (int16_t)(gy - 1), WIN_BORDER);
    }

    // --- docked controls (if you later put scrollbars in the non-client inner band) ---
    // For now, simplest: clip to the inner band (client + potential gutter regions)
    {
        const int16_t inner_x = (int16_t)(win_x + WIN_BORDER);
        const int16_t inner_y = (int16_t)(win_y + WIN_BORDER + ((w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0));
        const int16_t inner_w = (int16_t)(win_w - (WIN_BORDER * 2));
        const int16_t inner_h = (int16_t)(win_h - (WIN_BORDER * 2) - ((w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0));

        ui_clip_set(inner_x, inner_y, inner_w, inner_h);
        //SBOS_drawControlsFiltered(w, 1);
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
        ui_vlinedotted((int16_t)(win_x - 1), win_y, win_h);
        ui_hlinedotted(win_x, (int16_t)(win_y - 1), win_w);
        ui_hlinedotted(win_x, (int16_t)(win_y + win_h), win_w);
        ui_vlinedotted((int16_t)(win_x + win_w), win_y, win_h);
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

void SBOS_destroyWindow(SBXWindowId id){
    if (id >= MAX_WINDOWS) return;
    if (!gui_used[id]) return;

    gui_windows[id].flags = 0;
    gui_windows[id].title[0] = '\0';

    gui_used[id] = 0;
    z_remove(id);

    if (g_focusWin == id) g_focusWin = findTopFocusable();

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

    if (mousept_in_rect(mx, my, w->clientrect.x, w->clientrect.y, (int16_t)(w->clientrect.w + gr), (int16_t)(w->clientrect.h + gb))) {
        r.region = WH_CLIENT;
        return r;
    }


    // 4) Inside window but not title/client => border
    // add WH_BORDER laterwant resize hit zones.
    r.region = WH_CLIENT; // or WH_NONE
    return r;
}

void windowHittest(int16_t mx, int16_t my){
    // scan front->back so topmost window wins
    for (int zi = (int)g_winZcount - 1; zi >= 0; zi--) {
        SBXWindowId id = g_winZorder[zi];
        if (id >= MAX_WINDOWS) continue;
        if (!gui_used[id]) continue;

        WHitResult hit = hittest_window(id, mx, my);
        if (hit.region == WH_NONE) continue;

        // topmost hit window found: act on it
        switch (hit.region) {
        case WH_CLOSE:
            // dont kill it yet, we'll probably want to send this message our selves in the software,
            // realistically this should send a message that this windows close glyph was hit
            //SBOS_destroyWindow(hit.id);
            break;

        case WH_ZORDER:
            // send to back (or toggle behaviour)
            SBOS_sendToBack(hit.id);
            break;

        case WH_MAXRESTORE:
            // TODO: implement maximize toggle
            // for now just bring to front so it feels responsive
            break;

        case WH_TITLE:

            // TODO: start dragging (store drag offset)
            break;

        case WH_CLIENT:
            // TODO: route click to window/app
            // unless we have a flag that says "dont take focus"
            //SBOS_sendToFront(hit.id);
            break;

        default:
            break;
        }

        return; // IMPORTANT: stop after first (topmost) hit
    }
}




/////////////////////////////////////////////////////////////////////////////////////////////////////
////  SCROLLBAR HELPER FEATURES  ////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int16_t sb_thumb_len_from_step(int16_t track_len, int16_t min, int16_t max, int16_t step){
    const int16_t MIN_THUMB = 8;
    int32_t range = (int32_t)max - (int32_t)min;

    if (track_len <= 0) return 0;
    if (range <= 0) return track_len;

    if (step <= 0) step = 1;
    if ((int32_t)step > range) step = (int16_t)range;

    int32_t len = ((int32_t)step * (int32_t)track_len + range/2) / range;
    if (len < MIN_THUMB) len = MIN_THUMB;
    if (len > track_len) len = track_len;
    return (int16_t)len;
}

static inline int16_t sb_thumb_pos_from_pct(int16_t pct, int16_t travel){
    pct = clamp_i16(pct, 0, 100);
    if (travel <= 0) return 0;
    return (int16_t)(((int32_t)pct * (int32_t)travel + 50) / 100); // rounded
}

static inline int16_t sb_pct_from_thumb_pos(int16_t pos, int16_t travel){
    if (travel <= 0) return 0;
    if (pos < 0) pos = 0;
    if (pos > travel) pos = travel;
    return (int16_t)(((int32_t)pos * 100 + travel/2) / travel);
}

// step in percent derived from step in value units
static inline int16_t sb_step_pct(int16_t min, int16_t max, int16_t step){
    int32_t range = (int32_t)max - (int32_t)min;
    if (range <= 0) return 1;
    if (step <= 0) step = 1;
    int32_t sp = ((int32_t)step * 100 + range/2) / range;
    if (sp < 1) sp = 1;
    if (sp > 100) sp = 100;
    return (int16_t)sp;
}


static SBPart hittest_scrollbar_part(const sbx_window_t *w, const GAD_SCROLLBAR_T *s, int16_t mx, int16_t my,
                                     int16_t *out_thumb_axis_start, int16_t *out_thumb_len,
                                     int16_t *out_track_axis_start)
{
    // Compute abs rect same way draw does
    int16_t ax, ay, aw, ah;

    if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT) && (s->orient == SB_ORIENT_VERT)) {
        ax = (int16_t)(w->clientrect.x + w->clientrect.w);
        ay = w->clientrect.y;
        aw = SB_SCROLL_THICK;
        ah = w->clientrect.h;
    } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM) && (s->orient == SB_ORIENT_HORZ)) {
        ax = w->clientrect.x;
        ay = (int16_t)(w->clientrect.y + w->clientrect.h);
        aw = w->clientrect.w;
        ah = SB_SCROLL_THICK;
    } else {
        ax = (int16_t)(w->clientrect.x + s->h.rect.x);
        ay = (int16_t)(w->clientrect.y + s->h.rect.y);
        aw = s->h.rect.w;
        ah = s->h.rect.h;
    }

    if (!mousept_in_rect(mx, my, ax, ay, aw, ah)) return SB_PART_NONE;

    int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;
    int16_t thumb_len = sb_thumb_len_from_step(track_len, s->min, s->max, s->step);
    int16_t travel = (int16_t)(track_len - thumb_len);
    int16_t tpos = sb_thumb_pos_from_pct(s->pct, travel);

    int16_t thumb_axis_start = (int16_t)(((s->orient == SB_ORIENT_VERT) ? ay : ax) + tpos);

    // output for drag math
    if (out_thumb_axis_start) *out_thumb_axis_start = thumb_axis_start;
    if (out_thumb_len) *out_thumb_len = thumb_len;
    if (out_track_axis_start) *out_track_axis_start = (s->orient == SB_ORIENT_VERT) ? ay : ax;

    // Thumb rect test
    if (s->orient == SB_ORIENT_VERT) {
        if (mousept_in_rect(mx, my, ax, thumb_axis_start, aw, thumb_len)) return SB_PART_THUMB;
    } else {
        if (mousept_in_rect(mx, my, thumb_axis_start, ay, thumb_len, ah)) return SB_PART_THUMB;
    }

    return SB_PART_TRACK;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
////  DECLARATION OF DRAW FUNCTIONS  ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static void draw_button(const sbx_window_t *w, const GADGET_BASE_T *g);
static void draw_checkbox(const sbx_window_t *w, const GADGET_BASE_T *g);
static void draw_radio(const sbx_window_t *w, const GADGET_BASE_T *g);
static void draw_scrollbar(const sbx_window_t *w, const GADGET_BASE_T *g);




/////////////////////////////////////////////////////////////////////////////////////////////////////


static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock){
    (void)wantDock; // ignore for now

    for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g) continue;

        switch (g->gadgetType){
            case GAD_BUTTON:    draw_button(w, g);      break;
            case GAD_CHECKBOX:  draw_checkbox(w, g);    break;
            case GAD_RADIO:     draw_radio(w, g);       break;
            case GAD_SCROLLBAR: draw_scrollbar(w, g);   break;
            default: break;
        }
    }
    return;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
////  RENDERING THE GUI  ////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////


static void draw_button(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_BUTTON_T *b = (GAD_BUTTON_T*)g->gadget;
    if (!b->h.visible) return;

    int16_t ax = (int16_t)(w->clientrect.x + b->h.rect.x);
    int16_t ay = (int16_t)(w->clientrect.y + b->h.rect.y);

    // face
    fill_rect_pen(ax, ay, b->h.rect.w, b->h.rect.h, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(ax, ay, b->h.rect.w, b->h.rect.h, WIN_BEVEL_H, WIN_BEVEL_L, b->h.down);

    // centered text
    const int16_t char_w = 8, char_h = 16;
    int16_t len = 0;
    while (b->text[len] && len < (DEF_GADGET_TEXT_SIZE - 1)) len++;

    int16_t text_w = (int16_t)(len * char_w);
    int16_t tx = (int16_t)(ax + (b->h.rect.w - text_w) / 2);
    int16_t ty = (int16_t)(ay + (b->h.rect.h - char_h) / 2);

    if (b->h.down) { tx++; ty++; }

    gfx_setcolour(WIN_TITLE_PEN);
    ui_draw_text816(tx, ty, (const unsigned char*)b->text);
}

static void draw_checkbox(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_CHECKBOX_T *c = (GAD_CHECKBOX_T*)g->gadget;
    if (!c->h.visible) return;

    const int16_t ax = (int16_t)(w->clientrect.x + c->h.rect.x);
    const int16_t ay = (int16_t)(w->clientrect.y + c->h.rect.y);

    // background for the whole control rect (optional, but consistent look)
    // If you want it transparent, remove this.
    //fill_rect_pen(ax, ay, c->h.rect.w, c->h.rect.h, WIN_BG_PEN);

    // checkbox square size: 16x16 centered vertically in control height
    const int16_t box = 16;
    int16_t box_x = ax;
    int16_t box_y = (int16_t)(ay + (c->h.rect.h - box) / 2);

    // face + bevel
    fill_rect_pen(box_x, box_y, box, box, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(box_x, box_y, box, box, WIN_BEVEL_H, WIN_BEVEL_L, c->h.down);

    // check mark
    if (c->checked) {
        gfx_setcolour(WIN_BEVEL_L);

        // simple “tick” using pixels/lines (cheap + readable)
        // adjust offsets for your font/pixel vibe
        int16_t cx = (int16_t)(box_x + 4);
        int16_t cy = (int16_t)(box_y + 8);

        // down-left to center
        ui_ppixel(cx,     cy);
        ui_ppixel((int16_t)(cx+1), (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+2), (int16_t)(cy+2));

        // center to up-right
        ui_ppixel((int16_t)(cx+3), (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+4), cy);
        ui_ppixel((int16_t)(cx+5), (int16_t)(cy-1));
        ui_ppixel((int16_t)(cx+6), (int16_t)(cy-2));
    }

    // label text (optional)
    if (c->text[0]) {
        gfx_setcolour(WIN_TITLE_PEN);

        // text baseline centered-ish
        int16_t tx = (int16_t)(box_x + box + 6);
        int16_t ty = (int16_t)(ay + (c->h.rect.h - 16) / 2);

        if (c->h.down) { tx++; ty++; }

        ui_draw_text816(tx, ty, (const unsigned char*)c->text);
    }
}

static void draw_radio(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_RADIO_T *r = (GAD_RADIO_T*)g->gadget;
    if (!r->h.visible) return;

    const int16_t ax = (int16_t)(w->clientrect.x + r->h.rect.x);
    const int16_t ay = (int16_t)(w->clientrect.y + r->h.rect.y);

    // circle-ish box (16x16 like checkbox)
    const int16_t box = 16;
    int16_t box_x = ax;
    int16_t box_y = (int16_t)(ay + (r->h.rect.h - box) / 2);

    fill_rect_pen(box_x, box_y, box, box, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(box_x, box_y, box, box, WIN_BEVEL_H, WIN_BEVEL_L, r->h.down);

    // “radio dot” when checked
    if (r->checked) {
        gfx_setcolour(WIN_BEVEL_L);

        // cheap filled center blob (looks fine at 8x16 font scale)
        int16_t cx = (int16_t)(box_x + 6);
        int16_t cy = (int16_t)(box_y + 6);

        ui_ppixel(cx, cy);
        ui_ppixel((int16_t)(cx+1), cy);
        ui_ppixel(cx, (int16_t)(cy+1));
        ui_ppixel((int16_t)(cx+1), (int16_t)(cy+1));

        ui_ppixel((int16_t)(cx-1), cy);
        ui_ppixel((int16_t)(cx+2), cy);
        ui_ppixel(cx, (int16_t)(cy-1));
        ui_ppixel(cx, (int16_t)(cy+2));
    }

    // label
    if (r->text[0]) {
        gfx_setcolour(WIN_TITLE_PEN);
        int16_t tx = (int16_t)(box_x + box + 6);
        int16_t ty = (int16_t)(ay + (r->h.rect.h - 16) / 2);
        if (r->h.down) { tx++; ty++; }
        ui_draw_text816(tx, ty, (const unsigned char*)r->text);
    }
}

static void draw_scrollbar(const sbx_window_t *w, const GADGET_BASE_T *g){
    if (!w || !g || !g->gadget) return;

    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g->gadget;
    if (!s->h.visible) return;

    // Determine absolute rect.
    // If docked, override position to window edge.
    int16_t ax, ay, aw, ah;

    if ((s->h.flags & GAD_TOOL_DOCKED_RIGHT) && (s->orient == SB_ORIENT_VERT)) {
        ax = (int16_t)(w->clientrect.x + w->clientrect.w);
        ay = w->clientrect.y;
        aw = SB_SCROLL_THICK;
        ah = w->clientrect.h;
    } else if ((s->h.flags & GAD_TOOL_DOCKED_BOTTOM) && (s->orient == SB_ORIENT_HORZ)) {
        ax = w->clientrect.x;
        ay = (int16_t)(w->clientrect.y + w->clientrect.h);
        aw = w->clientrect.w;
        ah = SB_SCROLL_THICK;
    } else {
        ax = (int16_t)(w->clientrect.x + s->h.rect.x);
        ay = (int16_t)(w->clientrect.y + s->h.rect.y);
        aw = s->h.rect.w;
        ah = s->h.rect.h;
    }

    // Track fill
    fill_rect_pen(ax, ay, aw, ah, WIN_BORDER_INACTIVE_PEN);
    draw_bevel(ax, ay, aw, ah, WIN_BEVEL_H, WIN_BEVEL_L, 0);

    // Track axis length
    int16_t track_len = (s->orient == SB_ORIENT_VERT) ? ah : aw;
    int16_t thumb_len = sb_thumb_len_from_step(track_len, s->min, s->max, s->step);
    int16_t travel = (int16_t)(track_len - thumb_len);
    int16_t tpos = sb_thumb_pos_from_pct(s->pct, travel);

    // Thumb rect
    int16_t tx = ax, ty = ay, tw = aw, th = ah;
    if (s->orient == SB_ORIENT_VERT) { ty = (int16_t)(ay + tpos); th = thumb_len; }
    else                            { tx = (int16_t)(ax + tpos); tw = thumb_len; }

    // Thumb face + bevel (pressed if dragging or down)
    uint8_t pressed = (s->dragging || s->h.down) ? 1 : 0;
    fill_rect_pen(tx, ty, tw, th, WIN_BORDER_ACTIVE_PEN);
    draw_bevel(tx, ty, tw, th, WIN_BEVEL_H, WIN_BEVEL_L, pressed);
}



//////////////////////////////////

static GADGET_BASE_T* hittest_gadget(sbx_window_t *w, int16_t mx, int16_t my){
    if (!w) return NULL;

    int16_t lx = (int16_t)(mx - w->clientrect.x);
    int16_t ly = (int16_t)(my - w->clientrect.y);

    for (int i = MAX_GADGETS_PER_WINDOW - 1; i >= 0; i--){
        GADGET_BASE_T *g = w->GADGETS[i];
        if (!g || !g->gadget) continue;

        GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
        if (!h->visible || !h->enabled) continue;

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


void SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my){
    if (evt == MOUSE_DOWN) {
        g_ui.mouse_down = 1;
        g_ui.resize_win = SBW_INVALID_ID;

        // hit-test once on press
        for (int zi = (int)g_winZcount - 1; zi >= 0; zi--) {
            SBXWindowId id = g_winZorder[zi];
            if (id >= MAX_WINDOWS || !gui_used[id]) continue;

            WHitResult hit = hittest_window(id, mx, my);
            if (hit.region == WH_NONE) continue;
            sbx_window_t *w = SBOS_getWindow(hit.id);

            if(!(w->flags & SBX_WF_NOAUTOZORDER)) SBOS_bringToFront(hit.id);
            SBOS_setFocus(hit.id);  // if(!(w->flags & SBX_WF_NOFOCUS))      checked inside setFocus

            g_ui.down_region = hit.region;
            g_ui.down_win = hit.id;

            //// WINDOW FRAME LATCHES //////////////////////////////////
            // latch title gadget press
            if (is_title_gadget_region(hit.region)) {
                g_ui.title_win    = hit.id;
                g_ui.title_region = hit.region;
                g_ui.title_inside = 1; // currently inside by definition
            } else {
                g_ui.title_win    = SBW_INVALID_ID;
                g_ui.title_region = WH_NONE;
                g_ui.title_inside = 0;
            }

            if (hit.region == WH_RESIZE) {
                if (w && (w->flags & SBX_WF_RESIZABLE)) {
                    g_ui.resize_win = hit.id;
                    g_ui.r_start_mx  = mx;
                    g_ui.r_start_my  = my;
                    g_ui.r_start_w   = w->winrect.w;
                    g_ui.r_start_h   = w->winrect.h;
                }
                break;
            }

            // start drag if title hit
            if (hit.region == WH_TITLE) {
                if(w->flags & SBX_WF_MOVEABLE){
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

                    g_ui.down_win    = hit.id;
                    g_ui.down_region = WH_CLIENT;

                    g_ui.capturedGadget = g;

                    // press it
                    GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
                    h->down = 1;

                    // scrollbar interaction check

                    if (g->gadgetType == GAD_SCROLLBAR) {
                        GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g->gadget;

                        int16_t thumb_start=0, thumb_len=0, track_start=0;
                        SBPart part = hittest_scrollbar_part(w, s, mx, my, &thumb_start, &thumb_len, &track_start);

                        if (part == SB_PART_THUMB) {
                            s->dragging = 1;

                            int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? my : mx;

                            // Cache drag offset (mouse position relative to thumb start)
                            g_ui.sb_drag_off = (int16_t)(m_axis - thumb_start);
                            s->drag_off      = g_ui.sb_drag_off;

                            // Cache track start (axis start of the track in screen coords)
                            g_ui.sb_track_start = track_start;

                            // Compute track_len using the same geometry logic
                            int16_t track_len;
                            if (s->orient == SB_ORIENT_VERT) {
                                track_len = (s->h.flags & GAD_TOOL_DOCKED_RIGHT) ? w->clientrect.h : s->h.rect.h;
                            } else {
                                track_len = (s->h.flags & GAD_TOOL_DOCKED_BOTTOM) ? w->clientrect.w : s->h.rect.w;
                            }

                            int16_t travel = (int16_t)(track_len - thumb_len);
                            if (travel < 0) travel = 0;
                            g_ui.sb_travel = travel;

                        } else if (part == SB_PART_TRACK) {
                            // track step click
                            int16_t sp = sb_step_pct(s->min, s->max, s->step);
                            int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? my : mx;

                            if (m_axis < thumb_start) s->pct = clamp_i16((int16_t)(s->pct - sp), 0, 100);
                            else                     s->pct = clamp_i16((int16_t)(s->pct + sp), 0, 100);

                            // IMPORTANT: not dragging
                            s->dragging = 0;
                            s->drag_off = 0;
                            g_ui.sb_track_start = 0;
                            g_ui.sb_travel = 0;
                            g_ui.sb_drag_off = 0;
                        }

                    }


                    SBOS_paintAllWindows();
                    return;
                }

            }

            break;
        }

        SBOS_paintAllWindows();
        return;
    }
    if (evt == MOUSE_MOVE) {

        // resize window -------- resize window gadget
        if (g_ui.mouse_down && g_ui.resize_win != SBW_INVALID_ID) {
            sbx_window_t *w = SBOS_getWindow(g_ui.resize_win);
            if (w) {

                enforce_screen_bounds(w);

                int16_t dx = (int16_t)(mx - g_ui.r_start_mx);
                int16_t dy = (int16_t)(my - g_ui.r_start_my);

                int16_t nw = (int16_t)(g_ui.r_start_w + dx);
                int16_t nh = (int16_t)(g_ui.r_start_h + dy);

                // Optional: keep window inside screen when growing
                if (w->flags & SBX_WF_SCREENBOUND) {
                    if (w->winrect.x + nw > SCR_WIDTH)  nw = (int16_t)(SCR_WIDTH - w->winrect.x);
                    if (w->winrect.y + nh > SCR_HEIGHT) nh = (int16_t)(SCR_HEIGHT - w->winrect.y);
                }

                // enforce minimum window size
                if (nw < 100) nw = 100;
                if (nh < 100) nh = 100;

                w->winrect.w = nw;
                w->winrect.h = nh;

                layoutWindow(w);
                SBOS_paintAllWindows();
            }
            return;
        }


        // dragging the window about the place
        if (g_ui.mouse_down && g_ui.drag_win != SBW_INVALID_ID) {
            sbx_window_t *w = SBOS_getWindow(g_ui.drag_win);
            if (w) {
                w->winrect.x = mx - g_ui.drag_off_x;
                w->winrect.y = my - g_ui.drag_off_y;
                // keep layout consistent
                enforce_screen_bounds(w);

                layoutWindow(w);
                SBOS_paintAllWindows();
            }
            return;
        }

        /// GADGET HANDLING
        if (g_ui.mouse_down && g_ui.capturedGadget) {
            sbx_window_t *gw = SBOS_getWindow(g_ui.capturedGadget->winhnd);
            if (gw) {
                GAD_HDR_T *h = (GAD_HDR_T*)g_ui.capturedGadget->gadget;


                if (g_ui.capturedGadget->gadgetType == GAD_SCROLLBAR) {
                    GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g_ui.capturedGadget->gadget;

                    if (s->dragging) {
                        int16_t m_axis = (s->orient == SB_ORIENT_VERT) ? my : mx;

                        int16_t new_thumb_pos = (int16_t)(m_axis - g_ui.sb_track_start - g_ui.sb_drag_off);
                        int16_t new_pct       = sb_pct_from_thumb_pos(new_thumb_pos, g_ui.sb_travel);

                        if (new_pct != s->pct) {
                            s->pct = new_pct;
                            SBOS_paintAllWindows();
                        }
                    }

                    return;
                }


                // non-scrollbar default "down if inside" behaviour
                uint8_t inside = gadget_mouse_inside(gw, g_ui.capturedGadget, mx, my);
                uint8_t newDown = inside ? 1 : 0;
                if (newDown != h->down) {
                    h->down = newDown;
                    SBOS_paintAllWindows();
                }
            }
            return;
        }




        // when mouse is held down and a gadget is captured
        if (g_ui.mouse_down && g_ui.title_win != SBW_INVALID_ID && is_title_gadget_region(g_ui.title_region)) {
            WHitResult ht = hittest_window(g_ui.title_win, mx, my);
            uint8_t inside = (ht.region == g_ui.title_region);
            if (inside != g_ui.title_inside) {
                g_ui.title_inside = inside;
                SBOS_paintAllWindows();
            }
        }

        return;
    }

    if (evt == MOUSE_UP) {

        // stop resize no matter what
        g_ui.resize_win = SBW_INVALID_ID;

        // 1) TITLE GADGET RELEASE (latched)
        if (g_ui.title_win != SBW_INVALID_ID && is_title_gadget_region(g_ui.title_region)) {
            SBXWindowId wclick = g_ui.title_win;
            WHitRegion  rclick = g_ui.title_region;

            // pop visual first
            g_ui.title_win    = SBW_INVALID_ID;
            g_ui.title_region = WH_NONE;
            g_ui.title_inside = 0;

            // click only if released inside same gadget
            WHitResult ht = hittest_window(wclick, mx, my);
            if (ht.region == rclick) {
                if (rclick == WH_CLOSE){
                    //SBOS_destroyWindow(wclick);
                    printf("GLYPH HIT - Close Window\r\n");
                }
                else if (rclick == WH_ZORDER){
                    SBOS_sendToBack(wclick);
                    printf("GLYPH HIT - Zorder\r\n");
                }
                else if (rclick == WH_MINIMISE) {
                    printf("GLYPH HIT - Minimised\r\n");
                    /* TODO */
                }
                else if (rclick == WH_MAXRESTORE) {
                    printf("GLYPH HIT - MaxRestore\r\n");
                    /* TODO */
                }
            }
            SBOS_paintAllWindows();

            // common cleanup
            ui_end_interaction();
            return;
        }

        /// the gadget was released
        if (g_ui.capturedGadget) {
        GADGET_BASE_T *g = g_ui.capturedGadget;
        sbx_window_t *gw = SBOS_getWindow(g->winhnd);

        uint8_t inside = 0;
        if (gw) inside = gadget_mouse_inside(gw, g, mx, my);

        // pop visual
        GAD_HDR_T *h = (GAD_HDR_T*)g->gadget;
        h->down = 0;

        // scroller importance
        // If scrollbar, ALWAYS stop dragging (regardless of inside)
        if (g->gadgetType == GAD_SCROLLBAR) {
            GAD_SCROLLBAR_T *s = (GAD_SCROLLBAR_T*)g->gadget;
            s->dragging = 0;
            s->drag_off = 0;
            g_ui.sb_track_start = 0;
            g_ui.sb_travel = 0;
            g_ui.sb_drag_off = 0;

            if(gw){
                printf("SCROLLBAR USED: %d\r\n", s->pct);
            }

        }

        g_ui.capturedGadget = NULL;

        if (inside) {
            switch(g->gadgetType){
                case GAD_BUTTON:{
                    GAD_BUTTON_T *b = (GAD_BUTTON_T*)g->gadget;
                    printf("BUTTON CLICK: %s\r\n", b->text);
                } break;
                case GAD_CHECKBOX:{
                    GAD_CHECKBOX_T *c = (GAD_CHECKBOX_T*)g->gadget;
                    c->checked ^= 1;
                    printf("CHECKBOX TOGGLE: %s => %d\r\n", c->text, c->checked);
                } break;
                case GAD_RADIO:{
                    GAD_RADIO_T *r = (GAD_RADIO_T*)g->gadget;
                    if (gw) {
                        // clear all radios in same group in this window
                        for (int i = 0; i < MAX_GADGETS_PER_WINDOW; i++){
                            GADGET_BASE_T *og = gw->GADGETS[i];
                            if (!og || og->gadgetType != GAD_RADIO || !og->gadget) continue;

                            GAD_RADIO_T *ort = (GAD_RADIO_T*)og->gadget;
                            if (ort->group == r->group) ort->checked = 0;
                        }
                        r->checked = 1;
                    }
                    printf("RADIO SELECT: %s (grp %d)\r\n", r->text, r->group);
                } break;

                default: break;
            }
        }

        SBOS_paintAllWindows();
        ui_end_interaction();
        return;
    }



        // 4 OTHERWISE: just cleanup (end drag etc)
        SBOS_paintAllWindows();
        ui_end_interaction();
        return;
    }

}
