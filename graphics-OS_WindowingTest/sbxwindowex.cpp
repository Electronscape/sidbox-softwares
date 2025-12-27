
#include "stdio.h"

#include "font.h"
#include "sbxwindowex.h"
#include "sbxctrlex.h"


#include "sbapi_graphics.h"


static sbx_window_t gui_windows[MAX_WINDOWS];
static uint8_t      gui_used[MAX_WINDOWS];

static SBXWindowId  g_zorder[MAX_WINDOWS];
static uint8_t      g_zcount = 0;

static SBXWindowId  g_titleDownWin    = SBW_INVALID_ID;
static WHitRegion   g_titleDownRegion = WH_NONE;
static uint8_t      g_titleDownInside = 0;


static uint8_t      g_mouseDown  = 0;
static SBXWindowId  g_dragWin    = SBW_INVALID_ID;
static int16_t      g_dragOffX   = 0;
static int16_t      g_dragOffY   = 0;
static WHitRegion   g_downRegion = WH_NONE;




static SBXWindowId  g_sbDownWin   = SBW_INVALID_ID;
//static int16_t      g_sbDownIx    = -1;
static SBControlHandle g_sbDownH  = SBCTL_INVALID;
static uint8_t      g_sbDragging  = 0;
static int16_t      g_sbDragGrab  = 0;   // offset inside thumb (px)



static SBXWindowId  g_focusWin   = SBW_INVALID_ID;
static SBXWindowId  g_btnDownWin = SBW_INVALID_ID;
//static int16_t      g_btnDownIx  = -1;   // index into w->ctrls[]
static SBControlHandle g_btnDownH = SBCTL_INVALID;

static SBXWindowId  g_resizeWin  = SBW_INVALID_ID;
static int16_t      g_rStartMX   = 0;
static int16_t      g_rStartMY   = 0;
static int16_t      g_rStartW    = 0;
static int16_t      g_rStartH    = 0;

#define     WIN_TITLE_PEN_ACTIVE        16
#define     WIN_TITLE_PEN_INACTIVE      16

#define     WIN_BORDER_ACTIVE_PEN       3
#define     WIN_BORDER_INACTIVE_PEN     6




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

///////////////////////////////////////////////////////////////////////
/// COMPACTION ////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////


typedef struct { int16_t x, y, w, h; } R16;

static inline R16 r16(int16_t x, int16_t y, int16_t w, int16_t h){ R16 r = {x,y,w,h}; return r; }
static inline uint8_t r16_valid(const R16 *r){ return (r->w > 0) && (r->h > 0); }

static inline uint8_t pt_in_r16(int16_t px, int16_t py, const R16 *r){
    return (px >= r->x) && (py >= r->y) && (px < (int16_t)(r->x + r->w)) && (py < (int16_t)(r->y + r->h));
}

static inline int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

///////////////////////////////////////////////////////////////////////
/// END: COMPACTION ///////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////




static inline sbx_control_t* ctrl_from_handle(sbx_window_t *w, SBControlHandle h){
    if (!w || h == SBCTL_INVALID) return 0;

    uint8_t ix  = SBCTL_IDX(h);
    uint8_t gen = SBCTL_GEN(h);

    if (ix >= w->ctrl_count) return 0;

    sbx_control_t *c = &w->ctrls[ix];
    if (!c->used) return 0;
    if (c->gen != gen) return 0;

    return c;
}

static inline SBControlHandle ctrl_handle_of(const sbx_control_t *c, uint8_t ix){
    return SBCTL_MAKE(c->gen, ix);
}


void SBOS_DestroyControl(sbx_window_t *w, SBControlHandle h){
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return;
    c->used = 0;
    c->down = 0;
    c->thumb_down = 0;
    c->gen = (uint8_t)(c->gen + 1);
    if (c->gen == 0) c->gen = 1;
}


static void ctrl_set_text(sbx_control_t *c, const char *text);      // <<------------------ prototype

void SBOS_CtrlSetText(sbx_window_t *w, SBControlHandle h, const char *text){
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return;
    ctrl_set_text(c, text);
}

void SBOS_CtrlSetVisible(sbx_window_t *w, SBControlHandle h, uint8_t visible){
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return;
    c->visible = visible ? 1 : 0;
}






//********************************************************************//
//********************************************************************//
//********************************************************************//



/////// prototypes //////////////////
static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock);

// radio buttons
static SBControlHandle find_radio_at(sbx_window_t *w, int16_t cx, int16_t cy);
static void radio_select(sbx_window_t *w, SBControlHandle h);
static void draw_radio(sbx_window_t *w, const sbx_control_t *c);

// check boxes
static SBControlHandle find_checkbox_at(sbx_window_t *w, int16_t cx, int16_t cy);
static void draw_checkbox(sbx_window_t *w, const sbx_control_t *c);


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


static void enforce_screen_bounds(sbx_window_t *w, int16_t screen_w, int16_t screen_h){
    if (!w) return;
    if (!(w->flags & SBX_WF_SCREENBOUND)) return;

    // If window bigger than screen, just pin it to origin (or center it)
    int16_t max_x = (screen_w > w->w) ? (int16_t)(screen_w - w->w) : 0;
    int16_t max_y = (screen_h > w->h) ? (int16_t)(screen_h - w->h) : 0;

    w->x = clamp_i16(w->x, 0, max_x);
    w->y = clamp_i16(w->y, 0, max_y);
}


static inline void ui_ppixel(int16_t x, int16_t y){
    if ((unsigned)x >= SCR_WIDTH || (unsigned)y >= SCR_HEIGHT) return;

    if (g_uiclip.enabled) {
        if (x < g_uiclip.x0 || x >= g_uiclip.x1 || y < g_uiclip.y0 || y >= g_uiclip.y1)
            return;
    }
    sbgfx_ppixel(x, y);
}

static void ui_fill_dots(int16_t x, int16_t y, int16_t w, int16_t h, int16_t step){
    for (int16_t yy = 0; yy < h; yy++){
        for (int16_t xx = ((yy & 1) ? 1 : 0); xx < w; xx += step){
            ui_ppixel((int16_t)(x + xx), (int16_t)(y + yy));
        }
    }
}

void sbx_draw_text816(int x, int y, const unsigned char* textptr){
    if (!textptr) return;

    int start_x = x;

    for (int i = 0; textptr[i] != '\0'; ++i) {
        unsigned char ch = textptr[i];

        if (ch == '\n') {
            x = start_x;
            y += 16;
            continue;
        }

        const uint8_t* pixeldata = DEFAULT_SYSFONT[ch];

        // 8 columns per glyph
        for (int j = 0; j < 8; ++j) {
            int xc = x + j;
            uint8_t pixdat = pixeldata[j];

            // Each bit represents a 2-pixel-tall segment
            if (pixdat & 0x01) { ui_ppixel((int16_t)xc, (int16_t)(y + 0));  ui_ppixel((int16_t)xc, (int16_t)(y + 1));  }
            if (pixdat & 0x02) { ui_ppixel((int16_t)xc, (int16_t)(y + 2));  ui_ppixel((int16_t)xc, (int16_t)(y + 3));  }
            if (pixdat & 0x04) { ui_ppixel((int16_t)xc, (int16_t)(y + 4));  ui_ppixel((int16_t)xc, (int16_t)(y + 5));  }
            if (pixdat & 0x08) { ui_ppixel((int16_t)xc, (int16_t)(y + 6));  ui_ppixel((int16_t)xc, (int16_t)(y + 7));  }
            if (pixdat & 0x10) { ui_ppixel((int16_t)xc, (int16_t)(y + 8));  ui_ppixel((int16_t)xc, (int16_t)(y + 9));  }
            if (pixdat & 0x20) { ui_ppixel((int16_t)xc, (int16_t)(y + 10)); ui_ppixel((int16_t)xc, (int16_t)(y + 11)); }
            if (pixdat & 0x40) { ui_ppixel((int16_t)xc, (int16_t)(y + 12)); ui_ppixel((int16_t)xc, (int16_t)(y + 13)); }
            if (pixdat & 0x80) { ui_ppixel((int16_t)xc, (int16_t)(y + 14)); ui_ppixel((int16_t)xc, (int16_t)(y + 15)); }
        }

        // Advance to next character (8px wide)
        x += 8;
    }
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
    int16_t aw = w->aw;
    int16_t ah = w->ah;

    int16_t gr = win_gutter_right(w);
    int16_t gb = win_gutter_bottom(w);

    for (uint8_t i = 0; i < w->ctrl_count; i++){
        sbx_control_t *c = &w->ctrls[i];
        if (!c->used) continue;
        if (!c->visible) continue;
        if (c->type != CTL_SCROLLBAR) continue;

        int16_t thick = (c->orient == SBX_SB_VERT) ? c->w : c->h;
        if (thick <= 0) thick = WIN_RESIZE_GLYPH_SIZE;

        switch (c->dock){

        case SBX_DOCK_RIGHT: {
            int16_t sbw = (gr > 0) ? gr : thick;

            // shrink content first
            aw = (int16_t)(aw - sbw);
            if (aw < 0) aw = 0;

            // scrollbar sits immediately to the right of content
            c->x = aw;
            c->y = 0;
            c->w = sbw + 4;
            c->h = (gb > 0) ? (int16_t)(ah - gb) : ah;
            if (c->h < 0) c->h = 0;
            break;
        }

        case SBX_DOCK_BOTTOM: {
            int16_t sbh = (gb > 0) ? gb : thick;

            // shrink content first
            ah = (int16_t)(ah - sbh);
            if (ah < 0) ah = 0;

            c->x = 0;
            c->y = ah;
            c->w = (gr > 0) ? (int16_t)(aw) : w->cw;   // usually aw is what want here
            c->h = sbh + 4;
            break;
        }

        default:
            break;
        }
    }

    w->aw = aw ;
    w->ah = ah;
}





static void layoutWindow(sbx_window_t *w){
    int16_t title_h = (w->flags & SBX_WF_TITLE_BAR) ? WIN_TITLE_HEIGHT : 0;

    if (w->flags & SBX_WF_NOBORDER) {
        //client demensions (basically the whole window
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

    // start app area = full client
    w->ax = w->cx;
    w->ay = w->cy;
    w->aw = w->cw;
    w->ah = w->ch;

    layoutDockedControls(w);
}


void SBOS_setFocus(SBXWindowId id){
    if (id >= MAX_WINDOWS || !gui_used[id]) {
        g_focusWin = SBW_INVALID_ID;
        return;
    }

    // optional: if a window says "don't focus me" (toolbars/overlays)
    if (gui_windows[id].flags & SBX_WF_NOFOCUS) return;

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
    ui_hline(x+1, (int16_t)(y + h - 1), w-1);
    ui_vline((int16_t)(x + w - 1), y, h);
}


static void fill_rect_pen(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen){
    //sbgfx_drawbox(x, y, w, h, pen);
    gfx_setcolour(pen);
    for(int dx = x; dx < x + w; dx++){
        for(int dy = y; dy < y + h; dy++){
            ui_ppixel(dx, dy);
        }
    }
}

static void glyph_close_x(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t bx = x + 6;
    int16_t by = y + 6;
    int16_t bw = w - 12;
    int16_t bh = h - 12;
    if (bw <= 0 || bh <= 0) return;

    gfx_setcolour(WIN_BEVEL_L);
    ui_vline(bx, by, bh);
    ui_hline(bx, by, bw);


    gfx_setcolour(WIN_BEVEL_H);
    ui_hline(bx+1, by + bh - 1, bw);
    ui_vline(bx + bw, by, bh);
}


static void glyph_resize_grip(int16_t x, int16_t y, int16_t w, int16_t h){
    // Draw 3 diagonal dotted lines (classic gripper vibe)
    gfx_setcolour(WIN_BEVEL_L);

    // bottom-right diagonal-ish marks
    ui_hline((int16_t)(x + w - 4), (int16_t)(y + h - 2), 3);
    ui_hline((int16_t)(x + w - 7), (int16_t)(y + h - 5), 4);
    ui_hline((int16_t)(x + w - 10), (int16_t)(y + h - 8), 5);

    gfx_setcolour(WIN_BEVEL_H);
    ui_hline((int16_t)(x + w - 3), (int16_t)(y + h - 3), 2);
    ui_hline((int16_t)(x + w - 6), (int16_t)(y + h - 6), 3);
    ui_hline((int16_t)(x + w - 9), (int16_t)(y + h - 9), 4);
}



static void glyph_minimise(int16_t x, int16_t y, int16_t w, int16_t h){
    int16_t lx = x + 6;
    int16_t ly = y + h - 5;
    int16_t lw = w - 12;
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

static void draw_title_button(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fill_pen, uint8_t pressed){
    fill_rect_pen(x, y, w, h, fill_pen);

    // pressed => invert bevel
    draw_bevel_rect2(x, y, w, h, WIN_BEVEL_H, WIN_BEVEL_L, pressed);
}


__attribute__((weak)) void SBOS_onButtonClick(SBXWindowId win, SBControlHandle h){
    (void)win; (void)h;
    // default: do nothing
    printf("output click: window: %d = button handle: 0x%04X\n", win, (unsigned)h);
}


static inline uint8_t pt_in_ctrl(const sbx_control_t *c, int16_t cx, int16_t cy){
    R16 rc = r16(c->x, c->y, c->w, c->h);
    return pt_in_r16(cx, cy, &rc);
}

static SBControlHandle find_button_at(sbx_window_t *w, int16_t cx, int16_t cy){
    if (!w) return SBCTL_INVALID;

    for (int16_t i = (int16_t)w->ctrl_count - 1; i >= 0; i--){
        sbx_control_t *c = &w->ctrls[i];
        if (!c->used || !c->visible) continue;
        if (c->type != CTL_BUTTON) continue;
        if (pt_in_ctrl(c, cx, cy)) return ctrl_handle_of(c, (uint8_t)i);
    }
    return SBCTL_INVALID;
}

static inline int16_t win_client_x(const sbx_window_t *w, int16_t mx) { return (int16_t)(mx - w->cx); }
static inline int16_t win_client_y(const sbx_window_t *w, int16_t my) { return (int16_t)(my - w->cy); }



static void ctrl_set_text(sbx_control_t *c, const char *text){
    if (!c) return;
    if (!text) { c->text[0] = '\0'; return; }

    int i = 0;
    for (; text[i] && i < (int)sizeof(c->text) - 1; i++)
        c->text[i] = text[i];
    c->text[i] = '\0';
}

static SBControlHandle ctrl_alloc(sbx_window_t *w, CtlType type, sbx_control_t **out){
    if (out) *out = 0;
    if (!w) return SBCTL_INVALID;

    uint8_t ix = 0xFF;

    // reuse free slot
    for (uint8_t i = 0; i < w->ctrl_count; i++){
        if (!w->ctrls[i].used) { ix = i; break; }
    }

    // append new slot
    if (ix == 0xFF) {
        if (w->ctrl_count >= MAX_CONTROLS) return SBCTL_INVALID;
        ix = w->ctrl_count++;
    }

    sbx_control_t *c = &w->ctrls[ix];

    // bump generation when (re)allocating this slot
    c->gen = (uint8_t)(c->gen + 1);
    if (c->gen == 0) c->gen = 1;

    c->used = 1;
    c->type = type;

    c->x = c->y = 0;
    c->w = c->h = 0;

    c->visible = 1;
    c->down = 0;

    c->orient = 0;
    c->dock = 0;
    c->thumb_down = 0;

    c->text[0] = '\0';

    // scroll bar
    c->sb.min = 0;
    c->sb.max = 0;
    c->sb.value = 0;
    c->sb.step = 1;

    // radio button
    c->radio_group   = 0;
    c->radio_checked = 0;

    // check box
    c->cb_checked = 0;



    if (out) *out = c;

    return SBCTL_MAKE(c->gen, ix);
}

void initWb(){
    //SBCtrl  ctrls[MAX_CONTROLS];
    //uint8_t ctrl_count;
    //ctrl_count = 0;
}

static inline int16_t i16_min(int16_t a, int16_t b){ return (a < b) ? a : b; }
static inline int16_t i16_max(int16_t a, int16_t b){ return (a > b) ? a : b; }


typedef struct {
    int16_t minv, maxv, range;
    int16_t trackLen;
    int16_t thumbLen;
    int16_t travel;
} SBMetrics;


static uint8_t sb_metrics(const sbx_control_t *c, int16_t trackLen, SBMetrics *m){
    if (!c || !m) return 0;

    int16_t minv = c->sb.min;
    int16_t maxv = c->sb.max;
    if (maxv < minv) { int16_t t=minv; minv=maxv; maxv=t; }

    int16_t range = (int16_t)(maxv - minv);
    if (range < 0) range = 0;

    int16_t amt = c->sb.step;
    if (amt <= 0) amt = 1;

    int16_t minThumb = 6;

    int32_t denom = (int32_t)range + (int32_t)amt;
    int16_t thumbLen = (denom > 0) ? (int16_t)(((int32_t)trackLen * (int32_t)amt) / denom) : trackLen;
    thumbLen = clamp_i16(thumbLen, minThumb, trackLen);

    int16_t travel = (int16_t)(trackLen - thumbLen);
    if (travel < 0) travel = 0;

    m->minv=minv; m->maxv=maxv; m->range=range;
    m->trackLen=trackLen; m->thumbLen=thumbLen; m->travel=travel;
    return 1;
}

// Returns CONTROL-LOCAL rects (like your original sb_calc_thumb_rect assumed)
static uint8_t sb_get_track_well_local(const sbx_control_t *c, R16 *outWell){
    if (!c || !outWell) return 0;

    // your original: ix/iy = 2, inner = w-4/h-4, well inset by 1
    int16_t iw = (int16_t)(c->w - 4);
    int16_t ih = (int16_t)(c->h - 4);
    if (iw <= 0 || ih <= 0) return 0;

    R16 track = r16(2, 2, iw, ih);
    R16 well  = r16((int16_t)(track.x + 1), (int16_t)(track.y + 1),
                   (int16_t)(track.w - 2), (int16_t)(track.h - 2));
    if (!r16_valid(&well)) return 0;

    *outWell = well;
    return 1;
}

static uint8_t sb_calc_thumb_rect2(const sbx_control_t *c, R16 *outThumb){
    if (!c || !outThumb) return 0;

    R16 well;
    if (!sb_get_track_well_local(c, &well)) return 0;

    int16_t minv  = c->sb.min;
    int16_t maxv  = c->sb.max;
    if (maxv < minv) { int16_t t=minv; minv=maxv; maxv=t; }

    int16_t value = clamp_i16(c->sb.value, minv, maxv);

    int16_t trackLen = (c->orient == SBX_SB_VERT) ? well.h : well.w;
    SBMetrics m;
    if (!sb_metrics(c, trackLen, &m)) return 0;

    int16_t pos = 0;
    if (m.travel > 0 && m.range > 0) {
        int32_t num = (int32_t)(value - m.minv) * (int32_t)m.travel;
        // your rounding style (nice!)
        pos = (int16_t)((num + (m.range/2)) / m.range);
        pos = clamp_i16(pos, 0, m.travel);
    }

    if (c->orient == SBX_SB_VERT) {
        *outThumb = r16(well.x, (int16_t)(well.y + pos), well.w, m.thumbLen);
    } else {
        *outThumb = r16((int16_t)(well.x + pos), well.y, m.thumbLen, well.h);
    }

    return 1;
}


static uint8_t sb_calc_thumb_rect(const sbx_control_t *c,
                                  int16_t *outx, int16_t *outy, int16_t *outw, int16_t *outh)
{
    R16 t;
    if (!sb_calc_thumb_rect2(c, &t)) return 0;
    *outx = t.x; *outy = t.y; *outw = t.w; *outh = t.h;
    return 1;
}

static void draw_scrollbar(sbx_window_t *w, const sbx_control_t *c){
    if (!w || !c || !c->visible) return;

    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    uint16_t borderPen = (w->id == g_focusWin) ? WIN_BORDER_ACTIVE_PEN : WIN_BORDER_INACTIVE_PEN;

    // background
    if (!c->dock) draw_bevel_rect2(ax, ay, c->w, c->h, WIN_BEVEL_H, WIN_BEVEL_L, 0);
    else          fill_rect_pen(ax, ay, c->w, c->h, borderPen);

    // Track inner (screen coords)
    R16 track = r16((int16_t)(ax + 2), (int16_t)(ay + 2), (int16_t)(c->w - 4), (int16_t)(c->h - 4));
    if (!r16_valid(&track)) return;

    if (c->dock) fill_rect_pen(track.x, track.y, track.w, track.h, borderPen);

    // Well inside bevel
    R16 well = r16((int16_t)(track.x + 1), (int16_t)(track.y + 1), (int16_t)(track.w - 2), (int16_t)(track.h - 2));
    if (!r16_valid(&well)) return;

    // dots (your vibe 😄)
    gfx_setcolour(WIN_BEVEL_L);
    ui_fill_dots(well.x, well.y, well.w, well.h, 2);

    // pressed-in track bevel
    draw_bevel_rect2(track.x, track.y, track.w, track.h, WIN_BEVEL_H, WIN_BEVEL_L, 1);

    // Thumb (use compact calc)
    R16 tLocal;
    if (sb_calc_thumb_rect2(c, &tLocal)) {
        // control-local -> screen
        int16_t sx = (int16_t)(ax + tLocal.x);
        int16_t sy = (int16_t)(ay + tLocal.y);

        fill_rect_pen(sx, sy, tLocal.w, tLocal.h, WIN_SCROLLER_PROP_PEN);
        draw_bevel_rect2(sx, sy, tLocal.w, tLocal.h, WIN_BEVEL_H, WIN_BEVEL_L, c->thumb_down);
    }
}





SBXWindowId SBOS_createWindow(int16_t x, int16_t y, uint16_t width, uint16_t height, const char *title, uint32_t flags){
    for (SBXWindowId i = 0; i < MAX_WINDOWS; i++) {
        if (!gui_used[i]) {
            gui_used[i] = 1;

            sbx_window_t *w = &gui_windows[i];
            w->x = x;
            w->y = y;
            w->w = (int16_t)width;
            w->h = (int16_t)height;
            w->flags = flags;

            w->ctrl_count = 0;
            w->id = i;


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

sbx_window_t* SBOS_getWindow(SBXWindowId id){
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



void SBOS_paintWindow(SBXWindowId id){
    sbx_window_t *w = SBOS_getWindow(id);
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
    int16_t tb_h = 0;

    uint16_t borderPen = (id == g_focusWin) ? WIN_BORDER_ACTIVE_PEN: WIN_BORDER_INACTIVE_PEN;

    // outer frame (outline only)
    draw_rect_outline_thick(win_x, win_y, win_w, win_h, WIN_BORDER, borderPen);

    uint16_t shadew = WIN_BORDER;

    // client canvas (fill)
    if ((w->flags & SBX_WF_RESIZABLE)){
        // this should only happen if there is a scroll bar at the bottom - for now perma do this
        win_cw = w->aw;
        win_ch -= (WIN_RESIZE_GLYPH_SIZE - WIN_BORDER);
    }
    // client background
    sbgfx_drawbox(w->ax, w->ay, w->aw, w->ah, WIN_BG_PEN);


    // client only gadgets
    // draw ONLY gadgets that are NOT docked
    ui_clip_set(w->cx, w->cy, w->cw, w->ch);
    SBOS_drawControlsFiltered(w, 0);
    ui_clip_disable();

    // draw the blue gutter bar, we draw this, that paints over the client controls if not clipped properly
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        int16_t gx = (int16_t)(w->x + w->w - WIN_RESIZE_GLYPH_SIZE);
        int16_t gy = (int16_t)(w->y + w->h - WIN_RESIZE_GLYPH_SIZE);
        ui_clip_set(win_x, win_y, win_w, win_h);

        // right side --------------
        sbgfx_drawbox(win_x, gy+1, (int16_t)win_cw + WIN_BORDER, (int16_t)(WIN_RESIZE_GLYPH_SIZE - 2), borderPen);
        ui_clip_disable();

        // the resize glyph
        sbgfx_drawbox(gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE, borderPen);
        draw_bevel_rect2(gx, gy,
                         WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE,
                         WIN_BEVEL_H, WIN_BEVEL_L,
                         0);

        sbgfx_glyph(gx, gy, glyph_resize);
    }

    int16_t gr = win_gutter_right(w);
    int16_t gb = win_gutter_bottom(w);

    // draw the docked controls AFTER the gutter rendering
    ui_clip_set(w->cx, w->cy, (int16_t)(w->cw + gr), (int16_t)(w->ch + gb));
    SBOS_drawControlsFiltered(w, 1);
    ui_clip_disable();

    gfx_setcolour(WIN_BEVEL_L);
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        int16_t gx = (int16_t)(w->x + w->w - WIN_RESIZE_GLYPH_SIZE);
        int16_t gy = (int16_t)(w->y + w->h - WIN_RESIZE_GLYPH_SIZE);
    }


    if (w->flags & SBX_WF_NOBORDER) return;
    // frame
    draw_bevel_rect2(win_x, win_y, win_w, win_h, WIN_BEVEL_H, WIN_BEVEL_L, 0);
    draw_bevel_rect2(win_cx-1, win_cy-1, win_cw+2, win_ch+2, WIN_BEVEL_H, WIN_BEVEL_L, 1);
    //draw_bevel_rect2(win_cx-1, win_cy-1, win_cw, win_ch+2, WIN_BEVEL_H, WIN_BEVEL_L, 1);


    if (w->flags & SBX_WF_TITLE_BAR) {
        int16_t tb_x = win_x;
        int16_t tb_y = win_y;
        int16_t tb_w = win_w;
        tb_h = WIN_TITLE_HEIGHT + 4;
        fill_rect_pen(tb_x, tb_y, tb_w, tb_h, borderPen);

        gfx_setcolour(WIN_BEVEL_L);
        //ui_hline(win_x, win_cy - 1, win_w);

        int16_t bx = tb_x + tb_w;   // right edge inside border
        int16_t by = tb_y;          // top of title bar
        int16_t twidth = win_w;


        uint8_t downClose = (g_mouseDown && g_titleDownWin == id && g_titleDownInside && g_titleDownRegion == WH_CLOSE);
        uint8_t downMin   = (g_mouseDown && g_titleDownWin == id && g_titleDownInside && g_titleDownRegion == WH_MINIMISE);
        uint8_t downMax   = (g_mouseDown && g_titleDownWin == id && g_titleDownInside && g_titleDownRegion == WH_MAXRESTORE);
        uint8_t downZo    = (g_mouseDown && g_titleDownWin == id && g_titleDownInside && g_titleDownRegion == WH_ZORDER);

        // ZORDER button (before min/max)
        if (w->flags & SBX_WF_ZORDER) {
            bx -= WIN_ZORDER_WIDTH;
            twidth -= (WIN_ZORDER_WIDTH);

            draw_title_button(bx, by, WIN_ZORDER_WIDTH, tb_h, borderPen, downZo);
            glyph_zorder(bx, by, WIN_ZORDER_WIDTH, tb_h);

        }

        // MAXIMIZE/ RESORE
        if (w->flags & SBX_WF_MAXRESTORE) {
            bx -= WIN_MAXRESTORE_WIDTH;
            twidth -= (WIN_MAXRESTORE_WIDTH);
            draw_title_button(bx, by, WIN_MAXRESTORE_WIDTH, tb_h, borderPen, downMax);
            glyph_max_box(bx, by, WIN_MAXRESTORE_WIDTH, tb_h);

        }

        // MINIMISE
        if (w->flags & SBX_WF_MINIMISE) {
            bx -= WIN_MINIMISE_WIDTH;
            twidth -= (WIN_MINIMISE_WIDTH);
            draw_title_button(bx, by, WIN_MINIMISE_WIDTH, tb_h, borderPen, downMin);
            glyph_minimise(bx, by, WIN_MINIMISE_WIDTH, tb_h);

        }

        // CLOSE button (top-left)
        if (w->flags & SBX_WF_CLOSE) {
            int16_t cx = tb_x; // left inside border
            int16_t cy = by;
            draw_title_button(cx, cy, WIN_CLOSE_WIDTH, tb_h, borderPen, downClose);
            glyph_close_x(cx, cy, WIN_CLOSE_WIDTH, tb_h);


            win_tx = cx + WIN_CLOSE_WIDTH + 4;
            tb_x = cx + WIN_CLOSE_WIDTH;
            twidth -= (WIN_CLOSE_WIDTH);
        }

        draw_bevel_rect(tb_x, win_y, twidth, WIN_TITLE_HEIGHT + (WIN_BORDER));

        // Title text
        uint16_t titlePen = (id == g_focusWin) ? WIN_TITLE_PEN_ACTIVE : WIN_TITLE_PEN_INACTIVE;

        gfx_setcolour(titlePen);

        char tmpTitle[65];
        int16_t titlewidthclip = twidth;

        // how many characters fit?
        int16_t max_chars = (titlewidthclip - 8) / 8;
        if (max_chars < 0) max_chars = 0;
        if (max_chars > 64) max_chars = 64;

        // copy only what fits
        uint8_t c = 0;
        for (; c < max_chars && w->title[c]; c++) {
            tmpTitle[c] = w->title[c];
        }
        tmpTitle[c] = '\0';

        sbx_draw_text816(win_tx, win_ty-2, (const unsigned char *)tmpTitle);
    }

    // focus
    if (id == g_focusWin) {
        gfx_setcolour(WIN_BEVEL_L);
        ui_vlinedotted(win_x-1, win_y, win_h);
        ui_hlinedotted(win_x,   win_y-1, win_w);
        ui_hlinedotted(win_x,   win_y + win_h, win_w);
        ui_vlinedotted(win_w + win_x, win_y, win_h);
    }
}

static void normalize_zorder(void){
    SBXWindowId back[MAX_WINDOWS];
    SBXWindowId mid[MAX_WINDOWS];
    SBXWindowId front[MAX_WINDOWS];
    uint8_t nb = 0, nm = 0, nf = 0;

    for (uint8_t i = 0; i < g_zcount; i++) {
        SBXWindowId id = g_zorder[i];
        if (id >= MAX_WINDOWS || !gui_used[id]) continue;

        uint32_t f = gui_windows[id].flags;

        // If both are set, choose a rule (front wins here)
        if ((f & SBX_WF_ALWAYS_TO_FRONT) && (f & SBX_WF_ALWAYS_TO_BACK)) {
            f &= ~SBX_WF_ALWAYS_TO_BACK;
            gui_windows[id].flags = f;
        }

        if (f & SBX_WF_ALWAYS_TO_BACK)       back[nb++]  = id;
        else if (f & SBX_WF_ALWAYS_TO_FRONT) front[nf++] = id;
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
        SBXWindowId id = g_zorder[i];
        if (id < MAX_WINDOWS && gui_used[id]) {
            if (gui_windows[id].flags & SBX_WF_ALWAYS_TO_FRONT)
                return i; // first always-front window
        }
    }
    return (int)g_zcount;
}

static int z_find(SBXWindowId id){
    for (int i = 0; i < (int)g_zcount; i++) {
        if (g_zorder[i] == id) return i;
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
    if (f & SBX_WF_ALWAYS_TO_FRONT) target = (int)g_zcount - 1;

    if (pos == target) return;

    SBXWindowId temp = g_zorder[pos];

    if (pos < target) {
        for (int i = pos; i < target; i++) g_zorder[i] = g_zorder[i + 1];
    } else {
        for (int i = pos; i > target; i--) g_zorder[i] = g_zorder[i - 1];
    }
    g_zorder[target] = temp;

    normalize_zorder();
}


static void z_remove(SBXWindowId id){
    int pos = z_find(id);
    if (pos < 0) return;

    for (int i = pos; i < (int)g_zcount - 1; i++) {
        g_zorder[i] = g_zorder[i + 1];
    }
    g_zcount--;
}


static SBXWindowId findTopFocusable(void){
    for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
        SBXWindowId id = g_zorder[zi];
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

    SBXWindowId temp = g_zorder[pos];
    for (int i = pos; i > 0; i--) g_zorder[i] = g_zorder[i - 1];
    g_zorder[0] = temp;

    normalize_zorder();
}

void SBOS_paintAllWindows(void){
    for (int zi = 0; zi < (int)g_zcount; zi++) {
        SBXWindowId id = g_zorder[zi];
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
    R16 r = r16(x,y,w,h);
    return pt_in_r16(px,py,&r);
}


static SBControlHandle find_scrollbar_at(sbx_window_t *w, int16_t cx, int16_t cy){
    if (!w) return SBCTL_INVALID;

    uint8_t inDockZone = (cx >= w->aw) || (cy >= w->ah);

    for (int16_t pass = 0; pass < 2; pass++){
        for (int16_t i = (int16_t)w->ctrl_count - 1; i >= 0; i--){
            sbx_control_t *c = &w->ctrls[i];
            if (!c->used || !c->visible) continue;
            if (c->type != CTL_SCROLLBAR) continue;

            uint8_t wantDock = inDockZone ? 1 : 0;
            if (pass == 0) {
                if (!!c->dock != wantDock) continue;
            } else {
                if (!!c->dock == wantDock) continue;
            }

            if (pt_in_ctrl(c, cx, cy)) return ctrl_handle_of(c, (uint8_t)i);
        }
    }

    return SBCTL_INVALID;
}



static void sb_set_value_from_mouse(sbx_control_t *c, int16_t mouseAlong, int16_t grabOffset)
{
    if (!c) return;

    int16_t minv = c->sb.min;
    int16_t maxv = c->sb.max;
    if (maxv < minv) { int16_t t=minv; minv=maxv; maxv=t; }

    int16_t range = (int16_t)(maxv - minv);
    if (range < 0) range = 0;

    R16 well;
    if (!sb_get_track_well_local(c, &well)) return;

    int16_t trackLen = (c->orient == SBX_SB_VERT) ? well.h : well.w;

    SBMetrics m;
    if (!sb_metrics(c, trackLen, &m)) return;

    if (m.travel <= 0 || m.range <= 0) {
        c->sb.value = m.minv;
        return;
    }

    // mouseAlong is already "inside well" space in your caller
    int16_t desired = (int16_t)(mouseAlong - grabOffset);
    desired = clamp_i16(desired, 0, m.travel);

    // rounded mapping (same feel as thumb calc)
    c->sb.value = (int16_t)(m.minv + (((int32_t)desired * m.range) + (m.travel/2)) / m.travel);
    c->sb.value = clamp_i16(c->sb.value, m.minv, m.maxv);
}




static WHitResult hittest_window(SBXWindowId id, int16_t mx, int16_t my){
    WHitResult r = { SBW_INVALID_ID, WH_NONE };

    sbx_window_t *w = SBOS_getWindow(id);
    if (!w) return r;
    if (!(w->flags & SBX_WF_VISIBLE)) return r;

    layoutWindow(w);

    // 1) overall window bounds (for quick reject)
    if (!mousept_in_rect(mx, my, w->x, w->y, w->w, w->h)) return r;

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
    if (hasTitle && mousept_in_rect(mx, my, w->x, w->y, w->w, tb_h)) {

        int16_t bx = w->x + w->w;   // right edge
        int16_t by = w->y;          // top
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


    // Resize glyph hit (bottom-right)
    if ((w->flags & SBX_WF_RESIZABLE) && !(w->flags & SBX_WF_NOBORDER)) {
        int16_t gx = (int16_t)(w->x + w->w - WIN_RESIZE_GLYPH_SIZE);
        int16_t gy = (int16_t)(w->y + w->h - WIN_RESIZE_GLYPH_SIZE);
        if (mousept_in_rect(mx, my, gx, gy, WIN_RESIZE_GLYPH_SIZE, WIN_RESIZE_GLYPH_SIZE)) {
            r.region = WH_RESIZE;
            return r;
        }
    }

    // 3) Otherwise, check client area INCLUDING gutters (for docked controls)
    int16_t gr = win_gutter_right(w);
    int16_t gb = win_gutter_bottom(w);

    if (mousept_in_rect(mx, my, w->cx, w->cy, (int16_t)(w->cw + gr), (int16_t)(w->ch + gb))) {
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
    for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
        SBXWindowId id = g_zorder[zi];
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


void SBOS_MouseInterface(MouseEvt evt, int16_t mx, int16_t my){
    if (evt == MOUSE_DOWN) {
        g_mouseDown = 1;
        g_btnDownWin = SBW_INVALID_ID;
        g_resizeWin = SBW_INVALID_ID;


        // hit-test once on press
        for (int zi = (int)g_zcount - 1; zi >= 0; zi--) {
            SBXWindowId id = g_zorder[zi];
            if (id >= MAX_WINDOWS || !gui_used[id]) continue;

            WHitResult hit = hittest_window(id, mx, my);
            if (hit.region == WH_NONE) continue;
            SBOS_bringToFront(hit.id);
            SBOS_setFocus(hit.id);

            // latch title gadget press
            if (is_title_gadget_region(hit.region)) {
                g_titleDownWin    = hit.id;
                g_titleDownRegion = hit.region;
                g_titleDownInside = 1; // currently inside by definition
            } else {
                g_titleDownWin    = SBW_INVALID_ID;
                g_titleDownRegion = WH_NONE;
                g_titleDownInside = 0;
            }


            g_downRegion = hit.region;

            if (hit.region == WH_RESIZE) {
                sbx_window_t *w = SBOS_getWindow(hit.id);
                if (w && (w->flags & SBX_WF_RESIZABLE)) {
                    //SBOS_bringToFront(hit.id);
                    //SBOS_setFocus(hit.id);

                    g_resizeWin = hit.id;
                    g_rStartMX  = mx;
                    g_rStartMY  = my;
                    g_rStartW   = w->w;
                    g_rStartH   = w->h;
                }
                break;
            }


            if (hit.region == WH_CLIENT) {
                sbx_window_t *w = SBOS_getWindow(hit.id);
                if (w) {
                    int16_t cx = win_client_x(w, mx);
                    int16_t cy = win_client_y(w, my);


                    // 1 scrollbars
                    SBControlHandle sh = find_scrollbar_at(w, cx, cy);
                    if (sh != SBCTL_INVALID) {
                        sbx_control_t *sc = ctrl_from_handle(w, sh);
                        if (sc) {

                            // mouse position in scrollbar-local coords
                            int16_t lx = (int16_t)(cx - sc->x);
                            int16_t ly = (int16_t)(cy - sc->y);

                            int16_t tx, ty, tw, th;
                            uint8_t hasThumb = sb_calc_thumb_rect(sc, &tx, &ty, &tw, &th);

                            g_sbDownWin  = hit.id;
                            g_sbDragging = 0;
                            g_sbDragGrab = 0;

                            g_sbDownWin = hit.id;
                            g_sbDownH   = sh;

                            if (hasThumb && mousept_in_rect(lx, ly, tx, ty, tw, th)) {
                                // start dragging thumb
                                g_sbDragging = 1;
                                sc->thumb_down = 1;
                                g_sbDragGrab = (sc->orient == SBX_SB_VERT) ? (int16_t)(ly - ty) : (int16_t)(lx - tx);
                            } else {
                                // track click => page up/down
                                int16_t clickAlong = (sc->orient == SBX_SB_VERT) ? ly : lx;
                                int16_t thumbAlong = (sc->orient == SBX_SB_VERT) ? ty : tx;

                                if (clickAlong < thumbAlong) sc->sb.value -= sc->sb.step;
                                else                         sc->sb.value += sc->sb.step;

                                // clamp like draw does
                                int16_t minv = sc->sb.min;
                                int16_t maxv = sc->sb.max;

                                sc->sb.value = clamp_i16(sc->sb.value, minv, maxv);

                                if (maxv < minv) { int16_t t=minv; minv=maxv; maxv=t; }
                                int16_t maxPos = (int16_t)(maxv);
                                if (maxPos < minv) maxPos = minv;
                                sc->sb.value = clamp_i16(sc->sb.value, minv, maxPos);
                            }

                            //SBOS_bringToFront(hit.id);
                            //SBOS_setFocus(hit.id);

                            SBOS_paintAllWindows();
                            return; // IMPORTANT: swallow click (don’t also click buttons)
                        }
                    }

                    // 2 radio buttons
                    SBControlHandle rh = find_radio_at(w, cx, cy);
                    if (rh != SBCTL_INVALID) {
                        sbx_control_t *rc = ctrl_from_handle(w, rh);
                        if (rc) rc->down = 1;
                        g_btnDownWin = hit.id;     // reuse the same latch vars
                        g_btnDownH   = rh;         // yes reuse, it’s just “pressed control handle”
                        SBOS_paintAllWindows();
                        return;
                    }

                    // 3) Checkboxes
                    SBControlHandle ch = find_checkbox_at(w, cx, cy);
                    if (ch != SBCTL_INVALID) {
                        sbx_control_t *cc = ctrl_from_handle(w, ch);
                        if (cc) cc->down = 1;
                        g_btnDownWin = hit.id;
                        g_btnDownH   = ch;
                        SBOS_paintAllWindows();
                        return;
                    }

                    // 4 Buttons
                    SBControlHandle bh = find_button_at(w, cx, cy);
                    if (bh != SBCTL_INVALID) {
                        sbx_control_t *bc = ctrl_from_handle(w, bh);
                        if (bc) bc->down = 1;
                        g_btnDownWin = hit.id;
                        g_btnDownH   = bh;
                    } else {
                        g_btnDownWin = SBW_INVALID_ID;
                        g_btnDownH   = SBCTL_INVALID;
                    }

                }
            }


            // start drag if title hit
            if (hit.region == WH_TITLE) {
                sbx_window_t *w = SBOS_getWindow(hit.id);
                if(w->flags & SBX_WF_MOVEABLE){
                    g_dragWin = hit.id;
                    g_dragOffX = mx - w->x;
                    g_dragOffY = my - w->y;
                }
            }

            if (hit.region == WH_TITLE || hit.region == WH_CLIENT) {
                //SBOS_bringToFront(hit.id);
                //SBOS_setFocus(hit.id);
            }

            if (hit.region == WH_RESIZE) {

            }

            break;
        }

        SBOS_paintAllWindows();
        return;
    }
    if (evt == MOUSE_MOVE) {
        if (g_mouseDown && g_resizeWin != SBW_INVALID_ID) {
            sbx_window_t *w = SBOS_getWindow(g_resizeWin);
            if (w) {
                int16_t dx = (int16_t)(mx - g_rStartMX);
                int16_t dy = (int16_t)(my - g_rStartMY);

                int16_t nw = (int16_t)(g_rStartW + dx);
                int16_t nh = (int16_t)(g_rStartH + dy);

                if (nw < SBX_MIN_WIN_W) nw = SBX_MIN_WIN_W;
                if (nh < SBX_MIN_WIN_H) nh = SBX_MIN_WIN_H;

                // Optional: keep window inside screen when growing
                if (w->flags & SBX_WF_SCREENBOUND) {
                    if (w->x + nw > SCR_WIDTH)  nw = (int16_t)(SCR_WIDTH - w->x);
                    if (w->y + nh > SCR_HEIGHT) nh = (int16_t)(SCR_HEIGHT - w->y);
                    if (nw < SBX_MIN_WIN_W) nw = SBX_MIN_WIN_W;
                    if (nh < SBX_MIN_WIN_H) nh = SBX_MIN_WIN_H;
                }

                w->w = nw;
                w->h = nh;

                layoutWindow(w);
                SBOS_paintAllWindows();
            }
            return;
        }

        // scrollbar interfasing
        if (g_mouseDown && g_sbDownWin != SBW_INVALID_ID && g_sbDownH != SBCTL_INVALID && g_sbDragging) {
            sbx_window_t *w = SBOS_getWindow(g_sbDownWin);
            sbx_control_t *sc = ctrl_from_handle(w, g_sbDownH);
            if (w && sc) {
                int16_t cx = win_client_x(w, mx);
                int16_t cy = win_client_y(w, my);

                // local along axis, inside scrollbar control
                int16_t lx = (int16_t)(cx - sc->x);
                int16_t ly = (int16_t)(cy - sc->y);

                int16_t along = (sc->orient == SBX_SB_VERT) ? (int16_t)(ly - (2+1)) : (int16_t)(lx - (2+1));
                sb_set_value_from_mouse(sc, along, g_sbDragGrab);


                //int16_t valx, valy;
                //valx = SBOS_getScrollX(w, g_sbDownIx);
                //valy = SBOS_getScrollY(w, g_sbDownIx);
                //printf("SCROLL VALUE: X=%d, Y=%d\n", valx, valy);

                SBOS_paintAllWindows();
            }
            return;
        }



        if (g_mouseDown && g_dragWin != SBW_INVALID_ID) {
            sbx_window_t *w = SBOS_getWindow(g_dragWin);
            if (w) {
                w->x = mx - g_dragOffX;
                w->y = my - g_dragOffY;
                // keep layout consistent
                enforce_screen_bounds(w, SCR_WIDTH, SCR_HEIGHT);

                layoutWindow(w);
                SBOS_paintAllWindows();
            }
        }

        if (g_mouseDown && g_titleDownWin != SBW_INVALID_ID && is_title_gadget_region(g_titleDownRegion)) {
            WHitResult ht = hittest_window(g_titleDownWin, mx, my);
            uint8_t inside = (ht.region == g_titleDownRegion);
            if (inside != g_titleDownInside) {
                g_titleDownInside = inside;
                SBOS_paintAllWindows();
            }
        }

        // If a button is currently held, update "down" depending on hover
        if (g_mouseDown && g_btnDownWin != SBW_INVALID_ID && g_btnDownH != SBCTL_INVALID) {
            sbx_window_t *w = SBOS_getWindow(g_btnDownWin);
            sbx_control_t *bc = ctrl_from_handle(w, g_btnDownH);
            if (w && bc) {
                int16_t cx = win_client_x(w, mx);
                int16_t cy = win_client_y(w, my);

                uint8_t inside = pt_in_ctrl(bc, cx, cy);
                uint8_t newDown = inside ? 1 : 0;

                if (bc->down != newDown) {
                    bc->down = newDown;
                    SBOS_paintAllWindows();
                }
            }

        }

        return;
    }

    if (evt == MOUSE_UP) {

        // stop resize no matter what
        g_resizeWin = SBW_INVALID_ID;

        // 1) TITLE GADGET RELEASE (latched)
        if (g_titleDownWin != SBW_INVALID_ID && is_title_gadget_region(g_titleDownRegion)) {

            SBXWindowId wclick = g_titleDownWin;
            WHitRegion  rclick = g_titleDownRegion;

            // pop visual first
            g_titleDownWin    = SBW_INVALID_ID;
            g_titleDownRegion = WH_NONE;
            g_titleDownInside = 0;

            //printf("GLYPH HIT - Zorder\r\n");
            //printf("GLYPH HIT - MAXIMIZE RESTORE\r\n");
            //printf("GLYPH HIT - MINIMISE\r\n");

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
            g_mouseDown  = 0;
            g_dragWin    = SBW_INVALID_ID;
            g_downRegion = WH_NONE;
            return;
        }

        // 2) CLIENT CONTROL RELEASE
        if (g_btnDownWin != SBW_INVALID_ID && g_btnDownH != SBCTL_INVALID) {
            sbx_window_t *w = SBOS_getWindow(g_btnDownWin);
            sbx_control_t *bc = ctrl_from_handle(w, g_btnDownH);

            uint8_t doClick = 0;
            SBControlHandle clickH = g_btnDownH;

            if (w && bc) {
                int16_t cx = win_client_x(w, mx);
                int16_t cy = win_client_y(w, my);
                doClick = pt_in_ctrl(bc, cx, cy);
                bc->down = 0;
            }

            SBXWindowId clickWin = g_btnDownWin;
            g_btnDownWin = SBW_INVALID_ID;
            g_btnDownH   = SBCTL_INVALID;

            SBOS_paintAllWindows();

            if (doClick) {
                sbx_control_t *c = ctrl_from_handle(w, clickH);
                if (c) {
                    if (c->type == CTL_CHECKBOX) {
                        c->cb_checked ^= 1;   // toggle
                        SBOS_paintAllWindows();
                        // optional future callback: SBOS_onCheckboxChange(clickWin, clickH, c->cb_checked);
                    } else if (c->type == CTL_RADIO) {
                        radio_select(w, clickH);
                        SBOS_paintAllWindows();
                        // optional callback later
                    } else {
                        SBOS_onButtonClick(clickWin, clickH);
                    }
                }
            }

            g_mouseDown  = 0;
            g_dragWin    = SBW_INVALID_ID;
            g_downRegion = WH_NONE;
            return;
        }


        // 3 scroll bar release
        if (g_sbDownWin != SBW_INVALID_ID && g_sbDownH != SBCTL_INVALID) {
            sbx_window_t *w = SBOS_getWindow(g_sbDownWin);
            sbx_control_t *sc = ctrl_from_handle(w, g_sbDownH);
            if (sc) sc->thumb_down = 0;

            g_sbDownWin  = SBW_INVALID_ID;
            g_sbDownH    = SBCTL_INVALID;
            g_sbDragging = 0;
            g_sbDragGrab = 0;

            SBOS_paintAllWindows();

            g_mouseDown  = 0;
            g_dragWin    = SBW_INVALID_ID;
            g_downRegion = WH_NONE;
            return;
        }


        // 4 OTHERWISE: just cleanup (end drag etc)
        SBOS_paintAllWindows();
        g_mouseDown  = 0;
        g_dragWin    = SBW_INVALID_ID;
        g_downRegion = WH_NONE;
        return;
    }

}


static void draw_label(sbx_window_t *w, const sbx_control_t *c){
    if (!w || !c) return;
    if (!c->visible) return;

    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    gfx_setcolour(WIN_TITLE_PEN);   // or a dedicated label pen later
    sbx_draw_text816(ax, ay, (const unsigned char*)c->text);
}

static void draw_button(sbx_window_t *w, const sbx_control_t *c){
    if (!w || !c) return;
    if (!c->visible) return;

    // absolute coords on screen
    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    // button face
    //sbgfx_drawbox(ax, ay, c->w, c->h, WIN_BORDER_INACTIVE_PEN /* or UI face pen */);


    fill_rect_pen(ax, ay, c->w, c->h, WIN_BORDER_INACTIVE_PEN);

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
    sbx_draw_text816(tx, ty, (const unsigned char*)t);
}



static void SBOS_drawControlsFiltered(sbx_window_t *w, uint8_t wantDock){
    if (!w) return;

    for (uint8_t i = 0; i < w->ctrl_count; i++) {
        const sbx_control_t *c = &w->ctrls[i];
        if (!c->used) continue;
        if (!c->visible) continue;
        if (!!c->dock != !!wantDock) continue;

        switch (c->type) {
        case CTL_BUTTON:    draw_button(w, c);      break;
        case CTL_LABEL:     draw_label(w, c);       break;
        case CTL_SCROLLBAR: draw_scrollbar(w, c);   break;
        case CTL_RADIO:     draw_radio(w, c);       break;
        case CTL_CHECKBOX:  draw_checkbox(w, c);    break;


        default: break;
        }
    }
}


SBControlHandle SBOS_CreateButton(sbx_window_t *w, int16_t x, int16_t y, int16_t bw, int16_t bh, const char *text){
    sbx_control_t *c = 0;
    SBControlHandle h = ctrl_alloc(w, CTL_BUTTON, &c);
    if (h == SBCTL_INVALID) return SBCTL_INVALID;

    c->x = x; c->y = y; c->w = bw; c->h = bh;
    ctrl_set_text(c, text);
    return h;
}



SBControlHandle SBOS_CreateLabel(sbx_window_t *w, int16_t x, int16_t y, const char *text){
    sbx_control_t *c = 0;
    SBControlHandle h = ctrl_alloc(w, CTL_LABEL, &c);
    if (h == SBCTL_INVALID) return SBCTL_INVALID;

    c->x = x; c->y = y;
    c->w = 0; c->h = 0;
    ctrl_set_text(c, text);
    return h;
}



void SBOS_MoveScrollbar(sbx_window_t *w, SBControlHandle h,
                         int16_t x, int16_t y, int16_t wpx, int16_t hpx,
                         uint8_t orient)
{
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return;
    if (c->type != CTL_SCROLLBAR) return;

    c->x = x; c->y = y; c->w = wpx; c->h = hpx;
    c->orient = orient;
}


SBControlHandle SBOS_CreateScrollbar(sbx_window_t *w, uint8_t orient, uint8_t dock, int16_t thickness, int16_t min, int16_t max, int16_t value, int16_t step)
{
    sbx_control_t *c = 0;
    SBControlHandle h = ctrl_alloc(w, CTL_SCROLLBAR, &c);
    if (h == SBCTL_INVALID) return SBCTL_INVALID;

    c->orient = orient;
    c->dock   = dock;

    c->sb.min   = min;
    c->sb.max   = max;
    c->sb.value = value;
    c->sb.step  = (step <= 0) ? 1 : step;

    if (orient == SBX_SB_VERT) { c->w = thickness; c->h = 0; }
    else                      { c->h = thickness; c->w = 0; }

    return h;
}



int16_t SBOS_getScrollXH(sbx_window_t *w, SBControlHandle h)
{
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return 0;
    if (c->type != CTL_SCROLLBAR) return 0;
    if (c->orient != SBX_SB_HORZ) return 0;
    return c->sb.value;
}

int16_t SBOS_getScrollYH(sbx_window_t *w, SBControlHandle h)
{
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return 0;
    if (c->type != CTL_SCROLLBAR) return 0;
    if (c->orient != SBX_SB_VERT) return 0;
    return c->sb.value;
}


/// radio button
SBControlHandle SBOS_CreateRadioButton(sbx_window_t *w, int16_t x, int16_t y, uint8_t groupid, const char *text, uint8_t checked){
    sbx_control_t *c = 0;
    SBControlHandle h = ctrl_alloc(w, CTL_RADIO, &c);
    if (h == SBCTL_INVALID) return SBCTL_INVALID;

    c->x = x;
    c->y = y;

    // size: circle + gap + text width (8px per char), height ~16
    int16_t len = 0;
    if (text) while (text[len]) len++;

    c->w = (int16_t)(RADIO_DIAM + RADIO_GAP + (len * 8));
    c->h = RADIO_H;

    c->radio_group = groupid;
    c->radio_checked = checked;

    ctrl_set_text(c, text);
    return h;
}

static void radio_select(sbx_window_t *w, SBControlHandle h){
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return;
    if (c->type != CTL_RADIO) return;

    uint8_t g = c->radio_group;

    // uncheck everyone else in group
    for (uint8_t i = 0; i < w->ctrl_count; i++){
        sbx_control_t *o = &w->ctrls[i];
        if (!o->used || !o->visible) continue;
        if (o->type != CTL_RADIO) continue;
        if (o->radio_group != g) continue;
        o->radio_checked = 0;
    }

    c->radio_checked = 1;
}

static SBControlHandle find_radio_at(sbx_window_t *w, int16_t cx, int16_t cy){
    if (!w) return SBCTL_INVALID;

    for (int16_t i = (int16_t)w->ctrl_count - 1; i >= 0; i--){
        sbx_control_t *c = &w->ctrls[i];
        if (!c->used || !c->visible) continue;
        if (c->type != CTL_RADIO) continue;
        if (pt_in_ctrl(c, cx, cy)) return ctrl_handle_of(c, (uint8_t)i);
    }
    return SBCTL_INVALID;
}

static void draw_radio(sbx_window_t *w, const sbx_control_t *c){
    if (!w || !c || !c->visible) return;

    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    // circle box area
    int16_t cx = ax;
    int16_t cy = (int16_t)(ay + (c->h - RADIO_DIAM)/2);

    // outer “circle” (fake it with bevel box – looks retro and matches your UI)
    fill_rect_pen(cx, cy, RADIO_DIAM, RADIO_DIAM, WIN_BORDER_INACTIVE_PEN);
    draw_bevel_rect2(cx, cy, RADIO_DIAM, RADIO_DIAM, WIN_BEVEL_H, WIN_BEVEL_L, c->down);

    // inner dot if selected
    if (c->radio_checked) {
        int16_t dx = cx + 4;
        int16_t dy = cy + 4;
        fill_rect_pen(dx, dy, RADIO_DIAM - 8, RADIO_DIAM - 8, WIN_TITLE_PEN);
    }

    // label
    gfx_setcolour(WIN_TITLE_PEN);
    sbx_draw_text816(cx + RADIO_DIAM + RADIO_GAP, ay, (const unsigned char*)c->text);
}


void SBOS_RadioSetChecked(sbx_window_t *w, SBControlHandle h, uint8_t checked)
{
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!w || !c) return;
    if (c->type != CTL_RADIO) return;

    if (checked) radio_select(w, h);
    else c->radio_checked = 0;
}

uint8_t SBOS_RadioGetChecked(sbx_window_t *w, SBControlHandle h)
{
    sbx_control_t *c = ctrl_from_handle(w, h);
    if (!c) return 0;
    if (c->type != CTL_RADIO) return 0;
    return c->radio_checked ? 1 : 0;
}




/// check boxes
SBControlHandle SBOS_CreateCheckbox(sbx_window_t *w, int16_t x, int16_t y, const char *text, uint8_t checked){
    sbx_control_t *c = 0;
    SBControlHandle h = ctrl_alloc(w, CTL_CHECKBOX, &c);
    if (h == SBCTL_INVALID) return SBCTL_INVALID;

    c->x = x;
    c->y = y;

    int16_t len = 0;
    if (text) while (text[len]) len++;

    c->w = (int16_t)(CB_BOX + CB_GAP + (len * 8));
    c->h = CB_H;

    c->cb_checked = checked ? 1 : 0;
    ctrl_set_text(c, text);
    return h;
}

static SBControlHandle find_checkbox_at(sbx_window_t *w, int16_t cx, int16_t cy){
    if (!w) return SBCTL_INVALID;

    for (int16_t i = (int16_t)w->ctrl_count - 1; i >= 0; i--){
        sbx_control_t *c = &w->ctrls[i];
        if (!c->used || !c->visible) continue;
        if (c->type != CTL_CHECKBOX) continue;
        if (pt_in_ctrl(c, cx, cy)) return ctrl_handle_of(c, (uint8_t)i);
    }
    return SBCTL_INVALID;
}


static void draw_checkbox(sbx_window_t *w, const sbx_control_t *c){
    if (!w || !c || !c->visible) return;

    int16_t ax = (int16_t)(w->cx + c->x);
    int16_t ay = (int16_t)(w->cy + c->y);

    // box aligned vertically
    int16_t bx = ax;
    int16_t by = (int16_t)(ay + (c->h - CB_BOX)/2);

    // box face
    fill_rect_pen(bx, by, CB_BOX, CB_BOX, WIN_BORDER_INACTIVE_PEN);
    draw_bevel_rect2(bx, by, CB_BOX, CB_BOX, WIN_BEVEL_H, WIN_BEVEL_L, c->down);

    // check mark (simple filled inner square)
    if (c->cb_checked) {
        fill_rect_pen((int16_t)(bx + 3), (int16_t)(by + 3), (int16_t)(CB_BOX - 6), (int16_t)(CB_BOX - 6), WIN_TITLE_PEN);
        //gfx_setcolour(WIN_TITLE_PEN);
        //sbx_draw_text816((int16_t)(bx + 2), (int16_t)(by - 2), (const unsigned char*)"X");
    }

    // label
    gfx_setcolour(WIN_TITLE_PEN);
    sbx_draw_text816((int16_t)(bx + CB_BOX + CB_GAP), ay,
                     (const unsigned char*)c->text);
}

