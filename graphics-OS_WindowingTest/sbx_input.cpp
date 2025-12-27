//// SBX_INPUT.CPP //////


//#include "sbx_gadgets.h"
#include "sbx_input.h"
//#include "sbx_windowex.h"



UIInputState g_ui = {
    .mouse_down   = 0,
    .down_win     = SBW_INVALID_ID,
    .down_region  = WH_NONE,

    .title_win    = SBW_INVALID_ID,
    .title_region = WH_NONE,
    .title_inside = 0,

    .drag_win     = SBW_INVALID_ID,
    .resize_win   = SBW_INVALID_ID,
};
