#ifndef SBX_RENDER_H
#define SBX_RENDER_H

// standard includes
#include "stdint.h"

// hardware access
#include "../sbapi_graphics.h"



// typedefs
typedef struct {
    int16_t x0, y0, x1, y1; // [x0,x1), [y0,y1)
    uint8_t enabled;
} UIClipRect;


/// this is used for clipping drawing, also used for maybe dirty region redraws
extern UIClipRect g_uiclip;


// protoypes
void ui_draw_glyph(int16_t x, int16_t y, uint8_t *glyph);
void ui_ppixel(int16_t x, int16_t y);
void ui_draw_text816(int x, int y, const unsigned char* textptr);

void fill_rect_pen(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pen);
void ui_fill_dots(int16_t x, int16_t y, int16_t w, int16_t h, int16_t step);

void ui_hlinedotted(int16_t x, int16_t y, int16_t w);
void ui_vlinedotted(int16_t x, int16_t y, int16_t w);
void ui_hline(int16_t x, int16_t y, int16_t h);
void ui_vline(int16_t x, int16_t y, int16_t h);
void ui_draw_line(int16_t x, int16_t y, int16_t x2, int16_t y2);

void draw_title_button      (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t fill_pen, uint8_t pressed);
void draw_rect_outline_thick(int16_t x, int16_t y, int16_t w, int16_t h, int16_t t, uint16_t pen);
void draw_bevel_rect        (int16_t x, int16_t y, int16_t w, int16_t h);
void draw_bevel             (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t pen_hi, uint16_t pen_lo, uint8_t pressed);
void ui_dotted_rect_thick   (int16_t x, int16_t y, int16_t w, int16_t h, int16_t t);


void glyph_zorder(int16_t x, int16_t y, int16_t w, int16_t h);
void glyph_minimise(int16_t x, int16_t y, int16_t w, int16_t h);
void glyph_max_box(int16_t x, int16_t y, int16_t w, int16_t h);
void glyph_close_x(int16_t x, int16_t y, int16_t w, int16_t h);
void glyph_resize_grip(int16_t x, int16_t y, int16_t w, int16_t h);

void draw_disabled_dots(int16_t x, int16_t y, int16_t w, int16_t h);





#endif // SBX_RENDER_H
