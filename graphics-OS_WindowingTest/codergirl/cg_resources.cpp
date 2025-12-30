#include <stdio.h>
#include <stdint.h>

#include "cg_type.h"
//#include "cg_wintype.h"

//#include "cg_gadgets.h"

#include "cg_windowex.h"

#include "cg_resources.h"

#include "cg_input.h"

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




uint32_t SBOS_GetBasePoolSize();



SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void);


void SBOS_print_reserved_ui_memory(void){
    SBOS_GadgetPoolBytes gb = SBOS_get_gadget_pool_bytes();

    // These must be visible in this translation unit (not static elsewhere).
    size_t bytes_ui        = BYTES_OF(g_ui);

    size_t bytes_windows   = BYTES_OF(gui_windows);
    size_t bytes_used      = BYTES_OF(gui_used);

    size_t bytes_zorder    = BYTES_OF(g_winZorder);
    size_t bytes_zcount    = BYTES_OF(g_winZcount);

    size_t bytes_gadgets_total =
        gb.basePool + gb.btnPool + gb.chkPool + gb.radPool + gb.sbPool + gb.bvPool + gb.lbPool;

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
    printf("  lbPool            : %zu bytes\n", gb.lbPool);
    printf("  Subtotal          : %zu bytes (%.1f KB)\n",
           bytes_gadgets_total, (double)bytes_gadgets_total / 1024.0);

    printf("UI budget           : %u bytes (%.1f%% used)\n",
           SBOS_UI_BUDGET_BYTES, 100.0 * (double)total / (double)SBOS_UI_BUDGET_BYTES);

    printf("--------------------------------------------------\n");
    printf("TOTAL RESERVED UI   : %zu bytes (%.1f KB)\n",
           total, (double)total / 1024.0);
    printf("--------------------------------------------------\n\n");
}


void SBOS_print_ui_usage(void){
    SBOS_GadgetPoolBytes rb = SBOS_get_gadget_pool_bytes();
    SBOS_UiUsageCounts   uc = SBOS_get_ui_usage_counts();

    // Reserved bytes (from real arrays)
    size_t r_windows = sizeof(gui_windows);
    size_t r_used    = sizeof(gui_used);
    size_t r_zorder  = sizeof(g_winZorder);
    size_t r_zcount  = sizeof(g_winZcount);
    size_t r_ui      = sizeof(g_ui);

    size_t r_gadgets = rb.basePool + rb.btnPool + rb.chkPool + rb.radPool + rb.sbPool + rb.bvPool + rb.lbPool;

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
    size_t u_lb      = (size_t)uc.lb_used  * sizeof(GAD_BITMAPVIEW_T);

    size_t u_gadgets = u_base + u_btn + u_chk + u_rad + u_sb + u_bv + u_lb;

    // Note: zorder tables are always reserved+used; same for gui_used[].
    size_t u_total = sizeof(g_ui) + u_windows + r_used + r_zorder + r_zcount + u_gadgets;

    printf("\n[SBOS] UI memory usage (live)\n");
    printf("--------------------------------------------------\n");
    printf("Windows            : %u / %u\n", uc.win_used, (unsigned)(sizeof(gui_windows)/sizeof(gui_windows[0])));
    printf("Buttons            : %u / %u\n", uc.btn_used, (unsigned)(rb.btnPool / sizeof(GAD_BUTTON_T)));
    printf("Checkboxes         : %u / %u\n", uc.chk_used, (unsigned)(rb.chkPool / sizeof(GAD_CHECKBOX_T)));
    printf("Radios             : %u / %u\n", uc.rad_used, (unsigned)(rb.radPool / sizeof(GAD_RADIO_T)));
    printf("Scrollbars         : %u / %u\n", uc.sb_used,  (unsigned)(rb.sbPool  / sizeof(GAD_SCROLLBAR_T)));
    printf("BitmapViews        : %u / %u\n", uc.bv_used,  (unsigned)(rb.bvPool  / sizeof(GAD_BITMAPVIEW_T)));
    printf("ListBoxs           : %u / %u\n", uc.lb_used,  (unsigned)(rb.lbPool  / sizeof(GAD_BITMAPVIEW_T)));
    printf("--------------------------------------------------\n");

    double usage_pct = 0.0;
    if (r_total > 0) {
        usage_pct = ((double)u_total / (double)r_total) * 100.0;
    }

    if (usage_pct > 100.0) usage_pct = 100.0;

    printf("Reserved total     : %zu bytes (%.1f KB)\n", r_total, (double)r_total / 1024.0);
    printf("Used total         : %zu bytes (%.1f KB)\n", u_total, (double)u_total / 1024.0);
    printf("Usage              : %zu / %zu bytes (%.2f%%)\n", u_total, r_total, usage_pct);
    printf("Free in pools      : %zu bytes (%.1f KB)\n", (r_total > u_total) ? (r_total - u_total) : 0, (double)((r_total > u_total) ? (r_total - u_total) : 0) / 1024.0);
}
