#include "windowex.h"
#include "sbui_controls.h"
#include "sbapi_graphics.h"


static SBWindow_t   gui_windows[MAX_WINDOWS];
static uint8_t      gui_used[MAX_WINDOWS];

static SBWindowId   g_zorder[MAX_WINDOWS];
static uint8_t      g_zcount = 0;


static uint8_t      g_mouseDown = 0;
static SBWindowId   g_dragWin   = SBW_INVALID_ID;
static int16_t      g_dragOffX  = 0;
static int16_t      g_dragOffY  = 0;
static WHitRegion   g_downRegion = WH_NONE;

static SBWindowId g_focusWin = SBW_INVALID_ID;

#define     WIN_TITLE_PEN_ACTIVE        16
#define     WIN_TITLE_PEN_INACTIVE      16

#define     WIN_BORDER_ACTIVE_PEN       3
#define     WIN_BORDER_INACTIVE_PEN     6

/////// prototypes //////////////////
static void SBOS_drawControls(SBWindow_t *w);

static UIClip g_uiclip = {0,0,0,0,0};

static void normalize_zorder(void);

static inline void ui_clip_disable(void) { g_uiclip.enabled = 0; }

static inline void ui_clip_set(int16_t x, int16_t y, int16_t w, int16_t h){
    g_uiclip.enabled = 1;
    g_uiclip.x0 = x;
    g_uiclip.y0 = y;
    g_uiclip.x1 = (int16_t)(x + w);
    g_uiclip.y1 = (int16_t)(y + h);
}


static inline int16_t clampi16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void enforce_screen_bounds(SBWindow_t *w, int16_t screen_w, int16_t screen_h){
    if (!w) return;
    if (!(w->flags & SBW_SCREENBOUND)) return;

    // If window bigger than screen, just pin it to origin (or you can center it)
    int16_t max_x = (screen_w > w->w) ? (int16_t)(screen_w - w->w) : 0;
    int16_t max_y = (screen_h > w->h) ? (int16_t)(screen_h - w->h) : 0;

    w->x = clampi16(w->x, 0, max_x);
    w->y = clampi16(w->y, 0, max_y);
}


static inline void ui_ppixel(int16_t x, int16_t y){
    if ((unsigned)x >= SCR_WIDTH || (unsigned)y >= SCR_HEIGHT) return;

    if (g_uiclip.enabled) {
        if (x < g_uiclip.x0 || x >= g_uiclip.x1 || y < g_uiclip.y0 || y >= g_uiclip.y1)
            return;
    }

    sbgfx_ppixel(x, y);
}

static void ui_hline(int16_t x, int16_t y, int16_t w){
    for (int16_t i = 0; i < w; i++)
        ui_ppixel((int16_t)(x + i), y);
}

static void ui_hlinedotted(int16_t x, int16_t y, int16_t w){
    for (int16_t i = 0; i < w; i+=2)
        ui_ppixel((int16_t)(x + i), y);
}

static void ui_vline(int16_t x, int16_t y, int16_t h){
    for (int16_t i = 0; i < h; i++)
        ui_ppixel(x, (int16_t)(y + i));
}
static void ui_vlinedotted(int16_t x, int16_t y, int16_t h){
    for (int16_t i = 0; i < h; i+=2)
        ui_ppixel(x, (int16_t)(y + i));
}

static void ui_fillrect(int16_t x, int16_t y, int16_t w, int16_t h){
    for (int16_t yy = 0; yy < h; yy++)
        ui_hline(x, (int16_t)(y + yy), w);
}



static void layoutWindow(SBWindow_t *w){
    int16_t title_h = (w->flags & SBW_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0;

    if (w->flags & SBW_NOBORDER) {
        w->cx = w->x;
        w->cy = w->y;
        w->cw = w->w;
        w->ch = w->h;
    } else {
        w->cx = w->x + WIN_BORDER;
        w->cy = w->y + WIN_BORDER + title_h;
        w->cw = w->w - (WIN_BORDER * 2);
        w->ch = w->h - ((WIN_BORDER * 2) + title_h);
    }
}

void SBOS_setFocus(SBWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) {
        g_focusWin = SBW_INVALID_ID;
        return;
    }

    // optional: if a window says "don't focus me" (toolbars/overlays)
    if (gui_windows[id].flags & SBW_NOFOCUS) return;

    g_focusWin = id;
}


static void draw_bevel_rect(int16_t x, int16_t y, int16_t w, int16_t h){
    // light (top + left)
    gfx_setcolour(WIN_BEVEL_H);
    ui_hline(x, y, w);
    ui_vline(x, y, h);

    // dark (bottom + right)
    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(x, y + h - 1, w);
    ui_vline(x + w - 1, y, h);
}

static void draw_bevel_rect2(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen_hi, uint16_t pen_lo, uint8_t pressed)
{
    if (w <= 0 || h <= 0) return;

    uint16_t tl = pressed ? pen_lo : pen_hi;
    uint16_t br = pressed ? pen_hi : pen_lo;

    // top + left
    gfx_setcolour(tl);
    ui_hline(x, y, w);
    ui_vline(x, y, h);

    // bottom + right
    gfx_setcolour(br);
    ui_hline(x, (int16_t)(y + h - 1), w);
    ui_vline((int16_t)(x + w - 1), y, h);
}


static void fill_rect_pen(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen){
    sbgfx_drawbox(x, y, w, h, pen);
}

static void glyph_close_x(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t ix = x + 4, iy = y + 4;
    int16_t iw = w - 8, ih = h - 8;
    if (iw <= 0 || ih <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    for (int16_t i = 0; i < iw && i < ih; i++) {
        ui_hline(ix + i, iy + i, 1);
        ui_hline(ix + (iw - 1 - i), iy + i, 1);
    }
}

static void glyph_min_line(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t lx = x + 4;
    int16_t ly = y + h - 6;
    int16_t lw = w - 8;
    if (lw <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(lx, ly, lw);
}

static void glyph_max_box(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t bx = x + 4;
    int16_t by = y + 4;
    int16_t bw = w - 8;
    int16_t bh = h - 8;
    if (bw <= 0 || bh <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(bx, by, bw);
    ui_hline(bx, by + bh - 1, bw);
    ui_vline(bx, by, bh);
    ui_vline(bx + bw - 1, by, bh);
}

static void glyph_zorder(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t ix = x + 4;
    int16_t iy = y + 5;
    int16_t iw = w - 8;
    if (iw <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(ix, iy, iw);
    ui_hline(ix + 2, iy + 3, iw - 2);
    ui_hline(ix + 4, iy + 6, iw - 4);
}

static void draw_title_button(int16_t x, int16_t y, int16_t size, uint16_t fill_pen){
    fill_rect_pen(x, y, size, WIN_TITLE_HEIGHT   + (WIN_BORDER), fill_pen);
    draw_bevel_rect(x, y, size, WIN_TITLE_HEIGHT + (WIN_BORDER));
}

SBWindowId SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags){
    for (SBWindowId i = 0; i < MAX_WINDOWS; i++) {
        if (!gui_used[i]) {
            gui_used[i] = 1;

            SBWindow_t *w = &gui_windows[i];
            w->x = x;
            w->y = y;
            w->w = (int16_t)width;
            w->h = (int16_t)height;
            w->flags = flags;

            SBCtrl  ctrls[MAX_CONTROLS];
            uint8_t ctrl_count;

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

            enforce_screen_bounds(w, SCR_WIDTH, SCR_HEIGHT);
            layoutWindow(w);

            if (g_zcount < MAX_WINDOWS) {
                g_zorder[g_zcount++] = i;
            } else {
                gui_used[i] = 0;
                return SBW_INVALID_ID;
            }

            normalize_zorder();
            SBOS_paintAllWindows();

            return i;
        }
    }

    return SBW_INVALID_ID;
}

SBWindow_t* SBOS_getWindow(SBWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return 0;
    return &gui_windows[id];
}

static void gfx_fillrect(int16_t x, int16_t y, int16_t w, int16_t h){
    if (w <= 0 || h <= 0) return;
    for (int16_t yy = 0; yy < h; yy++) {
        ui_hline(x, y + yy, w);
    }
}

static void draw_rect_outline_thick(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, uint16_t pen){
    gfx_setcolour(pen);
    for (int16_t i = 0; i < t; i++) {
        int16_t xx = x + i, yy = y + i;
        int16_t ww = w - 2*i, hh = h - 2*i;
        if (ww <= 0 || hh <= 0) break;

        ui_hline(xx, yy, ww);
        ui_hline(xx, yy + hh - 1, ww);
        ui_vline(xx, yy, hh);
        ui_vline(xx + ww - 1, yy, hh);
    }
}

void SBOS_paintWindow(SBWindowId id){
    SBWindow_t *w = SBOS_getWindow(id);
    if (!w) return;

    layoutWindow(w);

    int16_t win_x  = w->x;
    int16_t win_y  = w->y;
    int16_t win_w  = w->w;
    int16_t win_h  = w->h;

    int16_t win_cx = w->cx;
    int16_t win_cy = w->cy;
    int16_t win_cw = w->cw;
    int16_t win_ch = w->ch;

    int16_t win_tx = win_x + (WIN_BORDER + 4);
    int16_t win_ty = win_y + (WIN_BORDER);

    uint16_t borderPen = (id == g_focusWin) ? WIN_BORDER_ACTIVE_PEN: WIN_BORDER_INACTIVE_PEN;

    // outer frame (outline only)
    draw_rect_outline_thick(win_x, win_y, win_w, win_h, WIN_BORDER, borderPen);

    // client canvas (fill)
    sbgfx_drawbox(win_cx, win_cy, win_cw, win_ch, WIN_BG_PEN);

    ui_clip_set(win_cx, win_cy, win_cw, win_ch);
    SBOS_drawControls(w);
    ui_clip_disable();

    if (w->flags & SBW_NOBORDER) return;
    if (w->flags & SBW_TITLE_BAR) {
        int16_t tb_x = win_x;
        int16_t tb_y = win_y;
        int16_t tb_w = win_w;
        int16_t tb_h = WIN_TITLE_HEIGHT + 4;
        fill_rect_pen(tb_x, tb_y, tb_w, tb_h, borderPen);

        gfx_setcolour(WIN_BEVEL_L);
        ui_hline(win_x, win_cy - 1, win_w);

        int16_t bx = tb_x + tb_w;   // right edge inside border
        int16_t by = tb_y;          // top of title bar
        int16_t twidth = win_w;

        // MAXIMIZE/ RESORE
        if (w->flags & SBW_MAXRESTORE) {
            bx -= WIN_MINMAX_WIDTH;
            twidth -= (WIN_MINMAX_WIDTH);
            draw_title_button(bx, by, WIN_MINMAX_WIDTH, borderPen);
            glyph_max_box(bx, by, WIN_MINMAX_WIDTH, tb_h);
        }

        // ZORDER button (before min/max)
        if (w->flags & SBW_ZORDER) {
            bx -= WIN_ZORDER_WIDTH;
            twidth -= (WIN_ZORDER_WIDTH);
            draw_title_button(bx, by, WIN_ZORDER_WIDTH, borderPen);
            glyph_zorder(bx, by, WIN_ZORDER_WIDTH, tb_h);
        }

        // CLOSE button (top-left)
        if (w->flags & SBW_CLOSE) {
            int16_t cx = tb_x; // left inside border
            int16_t cy = by;
            draw_title_button(cx, cy, WIN_CLOSE_WIDTH, borderPen);
            glyph_close_x(cx, cy, WIN_CLOSE_WIDTH, tb_h);

            win_tx = cx + WIN_CLOSE_WIDTH + 4;
            tb_x = cx + WIN_CLOSE_WIDTH;
            twidth -= (WIN_CLOSE_WIDTH);
        }

        draw_bevel_rect(tb_x, win_y, twidth, WIN_TITLE_HEIGHT + (WIN_BORDER));

        // Title text
        uint16_t titlePen = (id == g_focusWin) ? WIN_TITLE_PEN_ACTIVE : WIN_TITLE_PEN_INACTIVE;

        gfx_setcolour(titlePen);
        draw_text816(win_tx, win_ty-2, (const unsigned char *)w->title);

    } else {
        gfx_setcolour(WIN_BEVEL_L);
        ui_hline(win_cx - 1, win_cy - 1, win_cw + 2);
    }

    // ---- Bevel outer frame accents ----
    gfx_setcolour(WIN_BEVEL_H);
    ui_hline(win_x, win_y, win_w);
    ui_vline(win_x, win_y, win_h);
    ui_vline(win_cw + win_cx, win_cy, win_ch);
    ui_hline(win_cx, win_cy + win_ch, win_cw + 1);

    // focus
    if (id == g_focusWin) {
        gfx_setcolour(WIN_BEVEL_L); // or whatever pen you want for focus dots
        ui_vlinedotted(win_x-1, win_y, win_h);
        ui_hlinedotted(win_x,   win_y-1, win_w);
        ui_hlinedotted(win_x,   win_y + win_h, win_w);
        ui_vlinedotted(win_w + win_x, win_y, win_h);
    }



    gfx_setcolour(WIN_BEVEL_L);
    ui_hline(win_x, win_y + win_h - 1, win_w);
    ui_vline(win_w + win_x - 1, win_y, win_h);  // outter
    ui_vline(win_cx - 1, win_cy, win_ch + 1);   // inner

}

static void normalize_zorder(void){
    SBWindowId back[MAX_WINDOWS];
    SBWindowId mid[MAX_WINDOWS];
    SBWindowId front[MAX_WINDOWS];
    uint8_t nb = 0, nm = 0, nf = 0;

    for (uint8_t i = 0; i < g_zcount; i++) {
        SBWindowId id = g_zorder[i];
        if (id >= MAX_WINDOWS || !gui_used[id]) continue;

        uint32_t f = gui_windows[id].flags;

        // If both are set, choose a rule (front wins here)
        if ((f & SBW_ALWAYS_TO_FRONT) && (f & SBW_ALWAYS_TO_BACK)) {
            f &= ~SBW_ALWAYS_TO_BACK;
            gui_windows[id].flags = f;
        }

        if (f & SBW_ALWAYS_TO_BACK)       back[nb++]  = id;
        else if (f & SBW_ALWAYS_TO_FRONT) front[nf++] = id;
        else                              mid[nm++]   = id;
    }

    // Rebuild g_zorder: back -> mid -> front
    uint8_t out = 0;
    for (uint8_t i = 0; i < nb; i++) g_zorder[out++] = back[i];
    for (uint8_t i = 0; i < nm; i++) g_zorder[out++] = mid[i];
    for (uint8_t i = 0; i < nf; i++) g_zorder[out++] = front[i];

    g_zcount = out;
}

static int z_front_barrier(void){
    for (int i = 0; i < (int)g_zcount; i++) {
        SBWindowId id = g_zorder[i];
        if (id < MAX_WINDOWS && gui_used[id]) {
            if (gui_windows[id].flags & SBW_ALWAYS_TO_FRONT)
                return i; // first always-front window
        }
    }
    return (int)g_zcount;
}

static int z_find(SBWindowId id){
    for (int i = 0; i < (int)g_zcount; i++) {
        if (g_zorder[i] == id) return i;
    }
    return -1;
}

void SBOS_bringToFront(SBWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return;

    uint32_t f = gui_windows[id].flags;
    if (f & SBW_ALWAYS_TO_BACK) return; // locked to back

    int pos = z_find(id);
    if (pos < 0) return;

    int barrier = z_front_barrier();         // index where always-front starts
    int target  = barrier - 1;               // last slot before always-front
    if (target < 0) target = 0;

    // If this window is itself always-front, true front is allowed
    if (f & SBW_ALWAYS_TO_FRONT) target = (int)g_zcount - 1;

    if (pos == target) return;

    SBWindowId temp = g_zorder[pos];

    if (pos < target) {
        for (int i = pos; i < target; i++) g_zorder[i] = g_zorder[i + 1];
    } else {
        for (int i = pos; i > target; i--) g_zorder[i] = g_zorder[i - 1];
    }
    g_zorder[target] = temp;

    normalize_zorder();
}


static void z_remove(SBWindowId id){
    int pos = z_find(id);
    if (pos < 0) return;

    for (int i = pos; i < (int)g_zcount - 1; i++) {
        g_zorder[i] = g_zorder[i + 1];
    }
    g_zcount--;
}


static SBWindowId findTopFocusable(void){
    for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
        SBWindowId id = g_zorder[zi];
        if (id >= MAX_WINDOWS || !gui_used[id]) continue;
        if (!(gui_windows[id].flags & SBW_VISIBLE)) continue;
        if (gui_windows[id].flags & SBW_NOFOCUS) continue;
        return id;
    }
    return SBW_INVALID_ID;
}

void SBOS_destroyWindow(SBWindowId id){
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



void SBOS_sendToBack(SBWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) return;
    if (gui_windows[id].flags & SBW_ALWAYS_TO_FRONT) return;  // cannot sink 😄

    int pos = z_find(id);
    if (pos < 0) return;
    if (pos == 0) return;

    SBWindowId temp = g_zorder[pos];
    for (int i = pos; i > 0; i--) g_zorder[i] = g_zorder[i - 1];
    g_zorder[0] = temp;

    normalize_zorder();
}

void SBOS_paintAllWindows(void){
    for (int zi = 0; zi < (int)g_zcount; zi++) {
        SBWindowId id = g_zorder[zi];
        if (id >= MAX_WINDOWS) continue;
        if (!gui_used[id]) continue;

        if (gui_windows[id].flags & SBW_VISIBLE) {
            SBOS_paintWindow(id);
        }
    }
}


static inline uint8_t mousept_in_rect(int16_t px, int16_t py, int16_t x, int16_t y, int16_t w, int16_t h){
    return (px >= x) && (py >= y) && (px < (int16_t)(x + w)) && (py < (int16_t)(y + h));
}

static WHitResult hittest_window(SBWindowId id, int16_t mx, int16_t my){
    WHitResult r = { SBW_INVALID_ID, WH_NONE };

    SBWindow_t *w = SBOS_getWindow(id);
    if (!w) return r;
    if (!(w->flags & SBW_VISIBLE)) return r;

    layoutWindow(w);

    // 1) overall window bounds (for quick reject)
    if (!mousept_in_rect(mx, my, w->x, w->y, w->w, w->h)) return r;

    r.id = id;

    // Borderless window: everything is client
    if (w->flags & SBW_NOBORDER) {
        r.region = WH_CLIENT;
        return r;
    }

    // Title bar geometry (matches your paintWindow)
    uint8_t hasTitle = (w->flags & SBW_TITLE_BAR) != 0;
    int16_t tb_h = hasTitle ? (WIN_TITLE_HEIGHT + 4) : 0;

    // 2) If inside title band, check buttons
    if (hasTitle && mousept_in_rect(mx, my, w->x, w->y, w->w, tb_h)) {

        int16_t bx = w->x + w->w;   // right edge
        int16_t by = w->y;          // top
        // IMPORTANT: draw_title_button makes height = WIN_TITLE_HEIGHT + WIN_BORDER
        // but in paint you use tb_h = WIN_TITLE_HEIGHT + 4. Use tb_h consistently here.
        int16_t bh = tb_h;

        // Right side: MAXRESTORE then ZORDER (same order you paint)
        if (w->flags & SBW_MAXRESTORE) {
            bx -= WIN_MINMAX_WIDTH;
            if (mousept_in_rect(mx, my, bx, by, WIN_MINMAX_WIDTH, bh)) {
                r.region = WH_MAXRESTORE;
                return r;
            }
        }

        if (w->flags & SBW_ZORDER) {
            bx -= WIN_ZORDER_WIDTH;
            if (mousept_in_rect(mx, my, bx, by, WIN_ZORDER_WIDTH, bh)) {
                r.region = WH_ZORDER;
                return r;
            }
        }

        // Left side: CLOSE
        if (w->flags & SBW_CLOSE) {
            int16_t cx = w->x;
            if (mousept_in_rect(mx, my, cx, by, WIN_CLOSE_WIDTH, bh)) {
                r.region = WH_CLOSE;
                return r;
            }
        }

        // Otherwise it’s the title bar
        r.region = WH_TITLE;
        return r;
    }

    // 3) Otherwise, check client area
    if (mousept_in_rect(mx, my, w->cx, w->cy, w->cw, w->ch)) {
        r.region = WH_CLIENT;
        return r;
    }

    // 4) Inside window but not title/client => border
    // add WH_BORDER later if you want resize hit zones.
    r.region = WH_CLIENT; // or WH_NONE
    return r;
}

void windowHittest(int16_t mx, int16_t my){
    // scan front->back so topmost window wins
    for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
        SBWindowId id = g_zorder[zi];
        if (id >= MAX_WINDOWS) continue;
        if (!gui_used[id]) continue;

        WHitResult hit = hittest_window(id, mx, my);
        if (hit.region == WH_NONE) continue;

        // topmost hit window found: act on it
        switch (hit.region) {
        case WH_CLOSE:
            SBOS_destroyWindow(hit.id);
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
            break;

        default:
            break;
        }

        return; // IMPORTANT: stop after first (topmost) hit
    }
}


void SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my){
    if (evt == MOUSE_DOWN) {
        g_mouseDown = 1;

        // hit-test once on press
        for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
            SBWindowId id = g_zorder[zi];
            if (id >= MAX_WINDOWS || !gui_used[id]) continue;

            WHitResult hit = hittest_window(id, mx, my);
            if (hit.region == WH_NONE) continue;

            SBOS_bringToFront(hit.id);
            g_downRegion = hit.region;

            // start drag if title hit
            if (hit.region == WH_TITLE) {
                SBWindow_t *w = SBOS_getWindow(hit.id);
                g_dragWin = hit.id;
                g_dragOffX = mx - w->x;
                g_dragOffY = my - w->y;
            }

            if (hit.region == WH_TITLE || hit.region == WH_CLIENT) {
                SBOS_bringToFront(hit.id);
                SBOS_setFocus(hit.id);
            }
            break;
        }

        SBOS_paintAllWindows();
        return;
    }
    if (evt == MOUSE_MOVE) {
        if (g_mouseDown && g_dragWin != SBW_INVALID_ID) {
            SBWindow_t *w = SBOS_getWindow(g_dragWin);
            if (w) {
                w->x = mx - g_dragOffX;
                w->y = my - g_dragOffY;
                // keep layout consistent
                enforce_screen_bounds(w, SCR_WIDTH, SCR_HEIGHT);

                layoutWindow(w);
                SBOS_paintAllWindows();
            }
        }
        return;
    }

    if (evt == MOUSE_UP) {

        for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
            SBWindowId id = g_zorder[zi];
            if (id >= MAX_WINDOWS || !gui_used[id]) continue;

            WHitResult hit = hittest_window(id, mx, my);
            if (hit.region == WH_NONE) continue;

            // buttons: act on press (or store and act on release if you prefer)
            if (hit.region == WH_CLOSE) SBOS_destroyWindow(hit.id);
            if (hit.region == WH_ZORDER) SBOS_sendToBack(hit.id);
            if (hit.region == WH_MAXRESTORE) { /* TODO toggle */ }

            break;
        }

        SBOS_paintAllWindows();
        g_mouseDown = 0;
        g_dragWin = SBW_INVALID_ID;
        g_downRegion = WH_NONE;
        return;
    }
}




static void draw_label(SBWindow_t *w, const SBCtrl *c)
{
    if (!w || !c) return;
    if (!c->visible) return;

    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    gfx_setcolour(WIN_TITLE_PEN);   // or a dedicated label pen later
    draw_text816(ax, ay, (const unsigned char*)c->text);
}



static void draw_button(SBWindow_t *w, const SBCtrl *c){
    if (!w || !c) return;
    if (!c->visible) return;

    // absolute coords on screen
    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    // button face
    sbgfx_drawbox(ax, ay, c->w, c->h, WIN_BORDER_INACTIVE_PEN /* or UI face pen */);

    // bevel (pressed makes it look “in”)
    draw_bevel_rect2(ax, ay, c->w, c->h, WIN_BEVEL_H, WIN_BEVEL_L, c->down);

    // text
    gfx_setcolour(WIN_TITLE_PEN); // or another readable pen

    const char *t = c->text;

    // fixed 8x16 font assumptions
    int16_t text_len = 0;
    while (t[text_len]) text_len++;

    int16_t text_w = (int16_t)(text_len * 8);
    int16_t text_h = 16;

    // default: top-left fallback
    int16_t tx = (int16_t)(ax + 4);
    int16_t ty = (int16_t)(ay + 2);

    // if the button is big enough, center the text
    if (c->w >= text_w + 8 && c->h >= text_h + 4) {
        tx = (int16_t)(ax + (c->w - text_w) / 2);
        ty = (int16_t)(ay + (c->h - text_h) / 2);
    }

    // pressed effect: nudge text down/right
    if (c->down) {
        tx++;
        ty++;
    }

    draw_text816(tx, ty, (const unsigned char*)t);

}


static void SBOS_drawControls(SBWindow_t *w)
{
    if (!w) return;

    for (uint8_t i = 0; i < w->ctrl_count; i++) {
        const SBCtrl *c = &w->ctrls[i];
        if (!c->visible) continue;

        switch (c->type) {
        case CTL_BUTTON:
            draw_button(w, c);
            break;

        case CTL_LABEL:
            draw_label(w, c);
            break;


        default:
            break;
        }
    }
}









int SBOS_addButton(SBWindow_t *w,  uint16_t id,
                   int16_t x, int16_t y,
                   int16_t bw, int16_t bh,
                   const char *text){
    if (!w) return -1;

    // no space left
    if (w->ctrl_count >= MAX_CONTROLS)
        return -1;

    // grab next free slot
    SBCtrl *c = &w->ctrls[w->ctrl_count];

    // fill it
    c->type    = CTL_BUTTON;
    c->id      = id;
    c->x       = x;
    c->y       = y;
    c->w       = bw;
    c->h       = bh;
    c->visible = 1;
    c->down    = 0;

    // copy text safely
    if (text) {
        int i = 0;
        for (; text[i] && i < (int)sizeof(c->text) - 1; i++)
            c->text[i] = text[i];
        c->text[i] = '\0';
    } else {
        c->text[0] = '\0';
    }

    // claim the slot
    w->ctrl_count++;

    // return index of control (useful later)
    return (int)(w->ctrl_count - 1);
}


int SBOS_addLabel(SBWindow_t *w, uint16_t id, int16_t x, int16_t y, const char *text)
{
    if (!w) return -1;
    if (w->ctrl_count >= MAX_CONTROLS) return -1;

    SBCtrl *c = &w->ctrls[w->ctrl_count];

    c->type    = CTL_LABEL;
    c->id      = id;
    c->x       = x;
    c->y       = y;
    c->w       = 0;
    c->h       = 0;
    c->visible = 1;
    c->down    = 0;

    if (text) {
        int i = 0;
        for (; text[i] && i < (int)sizeof(c->text) - 1; i++)
            c->text[i] = text[i];
        c->text[i] = '\0';
    } else {
        c->text[0] = '\0';
    }

    w->ctrl_count++;
    return (int)(w->ctrl_count - 1);
}
