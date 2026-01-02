#include <stdio.h>
#include <stdint.h>

#include "cg_type.h"
//#include "cg_wintype.h"

//#include "cg_gadgets.h"

#include "cg_windowex.h"

#include "../fastram.h"
#include "cg_resources.h"

#include "cg_input.h"

#include "cg_gad_bitmapview.h"
#include "cg_gad_button.h"
#include "cg_gad_canvas.h"
#include "cg_gad_checkbox.h"
#include "cg_gad_radio.h"
#include "cg_gad_scrollbar.h"
#include "cg_gad_listbox.h"
#include "cg_gad_label.h"
#include "cg_gad_gridselect.h"

#include "lib_filerequest.h"
#include "lib_msgbox.h"



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
        gb.basePool + gb.btnPool + gb.chkPool + gb.radPool + gb.sbPool + gb.bvPool + gb.lbPool + gb.lblPool + gb.gsPool;

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
    printf("  lblPool           : %zu bytes\n", gb.lblPool);
    printf("  gsPool            : %zu bytes\n", gb.gsPool);
    printf("  Subtotal          : %zu bytes (%.1f KB)\n",
           bytes_gadgets_total, (double)bytes_gadgets_total / 1024.0);

    //printf("UI budget           : %u bytes (%.1f%% used)\n",
           //SBOS_UI_BUDGET_BYTES, 100.0 * (double)total / (double)SBOS_UI_BUDGET_BYTES);

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

    size_t r_gadgets = rb.basePool + rb.btnPool + rb.chkPool + rb.radPool + rb.sbPool + rb.bvPool + rb.lbPool + rb.lblPool + rb.gsPool;

    size_t r_total = r_ui + r_windows + r_used + r_zorder + r_zcount + r_gadgets;

    // Used bytes (counts * sizeof element)
    size_t u_windows = (size_t)uc.win_used * sizeof(gui_windows[0]);


    u_windows += (rb.frqPool + rb.msgPool);

    // base_used only if meaningful; else set to 0
    size_t u_base    = (size_t)uc.base_used * SBOS_GetBasePoolSize();

    size_t u_bv      = (size_t)uc.bv_used  * sizeof(GAD_BITMAPVIEW_T);
    size_t u_btn     = (size_t)uc.btn_used * sizeof(GAD_BUTTON_T);
    size_t u_chk     = (size_t)uc.chk_used * sizeof(GAD_CHECKBOX_T);
    size_t u_gs      = (size_t)uc.gs_used  * sizeof(GAD_GRIDSELECT_T);
    size_t u_rad     = (size_t)uc.rad_used * sizeof(GAD_RADIO_T);
    size_t u_lbl     = (size_t)uc.lbl_used * sizeof(GAD_LABEL_T);
    size_t u_lb      = (size_t)uc.lb_used  * sizeof(GAD_LISTBOX_T);
    size_t u_sb      = (size_t)uc.sb_used  * sizeof(GAD_SCROLLBAR_T);
    size_t u_cn      = (size_t)uc.cn_used  * sizeof(GAD_CANVAS_T);


    size_t u_gadgets = u_base +
                       u_bv + u_btn + u_chk + u_gs + u_rad + u_lbl + u_lb + u_sb;

    // Note: zorder tables are always reserved+used; same for gui_used[].
    size_t u_total = sizeof(g_ui) + u_windows + r_used + r_zorder + r_zcount + u_gadgets;

    size_t totalWindows_sizes;
    size_t WindowSize;

    totalWindows_sizes = sizeof(gui_windows);
    totalWindows_sizes += SBOS_msgbox_poolsize();
    totalWindows_sizes += SBOS_filerq_poolsize();

    WindowSize = sizeof(gui_windows[0]);
    WindowSize += SBOS_msgbox_poolsize1();
    WindowSize += SBOS_filerq_poolsize1();


    printf("\n[SBOS] UI memory usage (live)\n");
    printf("--------------------------------------------------\n");
    printf("Windows            : %u / %u\n", uc.win_used, (unsigned)( totalWindows_sizes / WindowSize ));
    printf("Buttons            : %u / %u\n", uc.btn_used, (unsigned)(rb.btnPool / sizeof(GAD_BUTTON_T)));
    printf("Checkboxes         : %u / %u\n", uc.chk_used, (unsigned)(rb.chkPool / sizeof(GAD_CHECKBOX_T)));
    printf("Radios             : %u / %u\n", uc.rad_used, (unsigned)(rb.radPool / sizeof(GAD_RADIO_T)));
    printf("Scrollbars         : %u / %u\n", uc.sb_used,  (unsigned)(rb.sbPool  / sizeof(GAD_SCROLLBAR_T)));
    printf("BitmapViews        : %u / %u\n", uc.bv_used,  (unsigned)(rb.bvPool  / sizeof(GAD_BITMAPVIEW_T)));
    printf("ListBoxs           : %u / %u\n", uc.lb_used,  (unsigned)(rb.lbPool  / sizeof(GAD_LISTBOX_T)));
    printf("Labels             : %u / %u\n", uc.lbl_used, (unsigned)(rb.lblPool / sizeof(GAD_LABEL_T)));
    printf("Gridselects        : %u / %u\n", uc.gs_used,  (unsigned)(rb.gsPool  / sizeof(GAD_GRIDSELECT_T)));
    printf("Canvases           : %u / %u\n", uc.cn_used,  (unsigned)(rb.cnPool  / sizeof(GAD_CANVAS_T)));
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

extern SBOS_GadgetPoolBytes SBOS_get_gadget_pool_bytes(void);
extern SBOS_UiUsageCounts   SBOS_get_ui_usage_counts(void);
extern uint32_t SBOS_GetBasePoolSize(void);

extern FastStats fastStats(void);

static size_t ui_reserved_bytes(void)
{
    SBOS_GadgetPoolBytes rb = SBOS_get_gadget_pool_bytes();

    size_t r_ui      = sizeof(g_ui);

    size_t r_windows = sizeof(gui_windows);
    size_t r_msgbox = SBOS_msgbox_used_count();
    size_t r_filerq = SBOS_filerq_used_count();

    r_windows += (r_msgbox + r_filerq);

    size_t r_used    = sizeof(gui_used);
    size_t r_zorder  = sizeof(g_winZorder);
    size_t r_zcount  = sizeof(g_winZcount);

    size_t r_gadgets =
        rb.basePool + rb.btnPool + rb.chkPool + rb.radPool +
        rb.sbPool + rb.bvPool + rb.lbPool + rb.lblPool +
        rb.gsPool;

    return r_ui + r_windows + r_used + r_zorder + r_zcount + r_gadgets;
}


static size_t ui_used_bytes(void)
{
    SBOS_GadgetPoolBytes rb = SBOS_get_gadget_pool_bytes();
    SBOS_UiUsageCounts   uc = SBOS_get_ui_usage_counts();

    size_t u_windows = (size_t)uc.win_used * sizeof(gui_windows[0]);

    size_t u_base = (size_t)uc.base_used * (size_t)SBOS_GetBasePoolSize();

    size_t u_bv   = (size_t)uc.bv_used   * sizeof(GAD_BITMAPVIEW_T);
    size_t u_btn  = (size_t)uc.btn_used  * sizeof(GAD_BUTTON_T);
    size_t u_chk  = (size_t)uc.chk_used  * sizeof(GAD_CHECKBOX_T);
    size_t u_gs   = (size_t)uc.gs_used   * sizeof(GAD_GRIDSELECT_T);
    size_t u_lbl  = (size_t)uc.lbl_used  * sizeof(GAD_LABEL_T);
    size_t u_lb   = (size_t)uc.lb_used   * sizeof(GAD_LISTBOX_T);
    size_t u_rad  = (size_t)uc.rad_used  * sizeof(GAD_RADIO_T);
    size_t u_sb   = (size_t)uc.sb_used   * sizeof(GAD_SCROLLBAR_T);

    size_t u_gadgets = u_base +
                       u_bv + u_btn + u_chk + u_gs + u_lbl + u_lb + u_rad + u_sb;


    size_t u_msb  = SBOS_msgbox_used_count();
    size_t u_frq  = SBOS_filerq_capacity();
    //c.filerq_used = SBOS_filerq_used_count();
    //c.msgbox_used = SBOS_msgbox_used_count();

    u_windows += (u_msb + u_frq);


    // Treat these as "always consumed" (they're fully reserved tables)
    size_t always = sizeof(g_ui) + sizeof(gui_used) + sizeof(g_winZorder) + sizeof(g_winZcount);

    return always + u_windows + u_gadgets;
}

void getMemAvailChipNFast(uint32_t *chip, uint32_t *fast)
{
    if (chip) {
        const size_t ui_used = ui_used_bytes();

        // "Available chip" here means "remaining UI budget"
        // (If you later want "remaining physical chip RAM", you'll need a system-wide chip allocator stat too.)
        size_t avail = 0;
        //if ((size_t)SBOS_UI_BUDGET_BYTES > ui_used) {
            //avail = (size_t)SBOS_UI_BUDGET_BYTES - ui_used;
        //}

        avail =  ui_reserved_bytes() - ui_used ;

        *chip = (uint32_t)avail;
    }

    if (fast) {
        uint32_t fastAll;
        fastAll = getMemAvail();

        // Pick ONE interpretation and stick to it:
        // 1) allocatable payload only (usually what you want)
        *fast = (uint32_t)fastAll;

        // 2) if you want "total bytes not currently allocated INCLUDING header space"
        // *fast = (uint32_t)(s.free_payload + s.overhead_bytes);
    }
}


// Writes comma-formatted unsigned integer into dst.
// dst must be large enough (max 10 digits + commas + NUL = 14 bytes)
char* fmt_commas_u32(char *dst, uint32_t value)
{
    char tmp[16];
    int ti = 0;
    int count = 0;

    // Special case: zero
    if (value == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return dst;
    }

    // Build reversed string with commas
    while (value > 0) {
        if (count == 3) {
            tmp[ti++] = ',';
            count = 0;
        }
        tmp[ti++] = '0' + (value % 10);
        value /= 10;
        count++;
    }

    // Reverse into dst
    int di = 0;
    while (ti > 0) {
        dst[di++] = tmp[--ti];
    }
    dst[di] = '\0';
    return dst;
}
