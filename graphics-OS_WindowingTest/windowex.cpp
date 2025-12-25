#include "windowex.h"
#include "sbapi_graphics.h"

#define     WIN_TITLE_HEIGHT    18      // title bar height
#define     WIN_BORDER          5       // border around the window frame

#define     WIN_BORDER_PEN      3
#define     WIN_BG_PEN          1
#define     WIN_TITLE_PEN       16

#define     WIN_BEVEL_H         2
#define     WIN_BEVEL_L         16      // actual black

// a basic crude window simply just draws the output for now
void createWindow(uint16_t width, uint16_t height, char *title){
    // dirty basic
    int win_x = 30;
    int win_y = 30;
    int win_w = width;
    int win_h = height;


    int win_cx  = win_x + WIN_BORDER;
    int win_cy  = win_y + WIN_BORDER + WIN_TITLE_HEIGHT;

    int win_cw  = win_w - ((WIN_BORDER * 2));
    int win_ch  = win_h - ((WIN_BORDER * 2) + WIN_TITLE_HEIGHT);

    int win_tx  = win_x + (WIN_BORDER + 4);     // where the title text should be
    int win_ty  = win_y + (WIN_BORDER);     // text title Y


    sbgfx_drawbox(win_x, win_y, win_w, win_h, WIN_BORDER_PEN);   // frame
    sbgfx_drawbox(win_cx, win_cy, win_cw, win_ch, WIN_BG_PEN);      // canvas area

    gfx_setcolour(WIN_TITLE_PEN);
    draw_text816(win_tx, win_ty, (const unsigned char *)title);

    // optional beval effects
    gfx_setcolour(WIN_BEVEL_H); // light side
    sbgfx_drawhline(win_x, win_y, win_w);
    sbgfx_drawvline(win_x, win_y, win_h);
    sbgfx_drawvline(win_cw + win_cx, win_cy, win_ch);
    sbgfx_drawhline(win_cx, win_cy + win_ch, win_cw+1);

    gfx_setcolour(WIN_BEVEL_L);     //
    sbgfx_drawhline(win_x, win_cy, win_w);
    sbgfx_drawhline(win_x, win_y + win_h - 1, win_w);
    sbgfx_drawvline(win_w + win_x - 1, win_y, win_h);

    sbgfx_drawvline(win_cx-1, win_cy, win_ch+1);


}
