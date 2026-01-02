#ifndef CG_GADGETRENDER_H
#define CG_GADGETRENDER_H

#include "cg_windowex.h"



// prototype access (internal stuffs)
void draw_bitmapview (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_button     (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_canvas     (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_checkbox   (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_gridselect (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_label      (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_listbox    (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_radio      (const sbx_window_t *w, const GADGET_BASE_T *g);
void draw_scrollbar  (const sbx_window_t *w, const GADGET_BASE_T *g);



#endif // CG_GADGETRENDER_H
