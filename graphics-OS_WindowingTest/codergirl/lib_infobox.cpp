#include <string.h>

#include "../resources/lang/lang.h"

#include "cg_wintype.h"
#include "cg_windowex.h"     // SBXWindowId
#include "sys_font.h"
#include "../sbapi_graphics.h"

#include "cg_glyphs.h"

#include "lib_infobox.h"



typedef struct LIB_INFBOX_PRIVATE {
    SBXWindowId parentWinId;        // owner window id (receives DONE)
    SBXWindowId selfWinId;          // this requester window id
    void     *user;                 // cookie returned to owner

    char txtMsg[DEF_GADGET_TEXT_SIZE];               // faux dir (later FS_FNLEN)

    CGGadgetHandle btnOk;
    CGGadgetHandle lblBg,   // background area
        lblMsg,
        bgShape;            // bitmap view for the background fill
} LIB_INFBOX_PRIVATE;


static struct LIB_INFBOX_PRIVATE *g_infbox_state[MAX_WINDOWS];

#define MAX_INFBOXES    4


static LIB_INFBOX_PRIVATE g_infbox_pool[MAX_INFBOXES];
static uint8_t            g_infbox_used[MAX_INFBOXES];


uint16_t SBOS_infbox_used_count(void){
    uint16_t n = 0;
    for (int i = 0; i < MAX_INFBOXES; i++)
        if (g_infbox_used[i]) n++;
    return n;
}

uint16_t SBOS_infbox_capacity (void){ return MAX_INFBOXES; }
uint16_t SBOS_infbox_poolsize (void){ return sizeof(g_infbox_used); }
uint16_t SBOS_infbox_poolsize1(void){ return sizeof(g_infbox_used[0]); }




static LIB_INFBOX_PRIVATE* infbox_alloc(void){
    for (int i = 0; i < MAX_INFBOXES; i++){
        if (!g_infbox_used[i]){
            g_infbox_used[i] = 1;
            memset(&g_infbox_pool[i], 0, sizeof(g_infbox_pool[i]));
            return &g_infbox_pool[i];
        }
    }
    return NULL;
}

static void infbox_free(LIB_INFBOX_PRIVATE *mb){
    if (!mb) return;
    int idx = (int)(mb - g_infbox_pool);
    if (idx >= 0 && idx < MAX_INFBOXES){
        g_infbox_used[idx] = 0;
    }
}



// prototypes at the top here
static CGWindowProcRes InfoBoxProc(SBXWindowId win, const CGMessage_t *m);
static void infbox_post_done(LIB_INFBOX_PRIVATE *infbox, int ok);
static void infbox_destroy(SBXWindowId infbox_winhnd);


static inline LIB_INFBOX_PRIVATE* infbox_get(SBXWindowId win){
    if (win >= MAX_WINDOWS) return NULL;
    return g_infbox_state[win];
}


static void infbox_post_done(LIB_INFBOX_PRIVATE *mb, int choice){
    if (!mb) return;

    // Suggested DONE mapping:
    // a = ok(1)/cancel(0)
    // b = user cookie
    // c = out_path pointer (same pointer caller gave)
    // d = requester window id (so owner can match)

    CG_PostWindowMsg(mb->parentWinId,
                     CGEVT_SYS_INFOBOX_DONE,
                     0,
                     MSG_PTR(mb->user),
                     0,
                     (int32_t)mb->selfWinId);

}



static void infbox_destroy(SBXWindowId infbox_winhnd){
    if (infbox_winhnd >= MAX_WINDOWS) return;

    LIB_INFBOX_PRIVATE *mb = g_infbox_state[infbox_winhnd];
    g_infbox_state[infbox_winhnd] = NULL;

    if (mb){
        infbox_free(mb);
    }

    SBOS_destroyWindow(infbox_winhnd);
}


static CGWindowProcRes InfBoxProc(SBXWindowId win, const CGMessage_t *m){
    if (!m) return CGPROC_DEFAULT;

    LIB_INFBOX_PRIVATE *mb = infbox_get(win);
    if (!mb) return CGPROC_DEFAULT;

    switch(m->eventClass){

    case CGEVT_GAD_BUTTON_HIT:
        if (m->gadget == mb->btnOk){
            infbox_post_done(mb, INFOBOX_OK);
            infbox_destroy(win);
            return CGPROC_COMPLETE;
        }
        return CGPROC_DEFAULT;

    default:
        return CGPROC_DEFAULT;
    }
    return CGPROC_DEFAULT;
}

#define INF_WIN_WIDTH           290
#define INF_WIN_HEIGHT          100
//#define MSG_BTN_HEIGHT          24
#define INF_BTN_MARGIN_BOTTOM  (12 + WIN_TITLE_HEIGHT)
#define INF_BTN_WIDTH           80

static int16_t clamp16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

SBXWindowId SBOS_InfoBox(SBXWindowId owner_winhnd, const CGFInfoBoxParams *p){
    if (!p) return SBW_INVALID_ID;

    LIB_INFBOX_PRIVATE *mb = infbox_alloc();
    if (!mb) return SBW_INVALID_ID;

    memset(mb, 0, sizeof(*mb));
    mb->parentWinId = owner_winhnd;
    mb->user = p->user;

    if (p->txtMsg && p->txtMsg[0])
        strncpy(mb->txtMsg, p->txtMsg, sizeof(mb->txtMsg)-1);
    else
        strncpy(mb->txtMsg, "- NO INFO! -", sizeof(mb->txtMsg)-1);

    const char *title = (p->title && p->title[0]) ? p->title : "SIDBOX OS: Info box";

    // --- measure lines + widest line (in chars) ---
    int16_t maxChars = 0, curChars = 0, lines = 0;

    for (const char *s = mb->txtMsg; ; s++) {
        if (*s == '\n' || *s == '\0') {

            if (curChars > 0 || lines == 0) {
                if ((curChars + 1) > maxChars)
                    maxChars = (curChars + 1);
                lines++;
            }

            curChars = 0;

            if (*s == '\0')
                break;
        } else {
            curChars++;
        }
    }
    //lines--;

    // --- window size ---
    int16_t prepHeight = (int16_t)(76 + lines * FONT_HEIGHT);
    int16_t prepWidth  = clamp16((int16_t)(maxChars * FONT_WIDTH + 32), INF_WIN_WIDTH, SCR_WIDTH);

    // --- window pos ---
    int16_t msgx = (int16_t)((SCR_WIDTH  / 2) - (prepWidth  / 2));
    int16_t msgy = (int16_t)((SCR_HEIGHT / 3) - (prepHeight / 3));

    // --- buttons count ---
    int8_t nButtons = 1;

    // --- button layout ---
    const int16_t gap = 8;

    int16_t btnY = (int16_t)(prepHeight - INF_BTN_MARGIN_BOTTOM - DEF_DIALOG_BUTTON_HEIGHT);

    // keep your “content region” fidelity (close button margins)
    int16_t contentX = WIN_CLOSE_WIDTH;
    int16_t contentW = (int16_t)(prepWidth - 2 * WIN_CLOSE_WIDTH);

    int16_t totalW = (int16_t)(nButtons * INF_BTN_WIDTH + (nButtons - 1) * gap);
    int16_t startX = (int16_t)(contentX + (contentW - totalW + 1) / 2);

    int16_t btnX[3];
    for (int i = 0; i < nButtons; i++)
        btnX[i] = (int16_t)(startX + i * (INF_BTN_WIDTH + gap));

    // --- create window ---
    SBXWindowId win = SBOS_createWindow(NULL, msgx, msgy, prepWidth, prepHeight, title,
                                        SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_CLOSE | SBX_WF_MOVEABLE | SBX_WF_ZORDER | SBX_WF_SCREENBOUND);

    if (win == SBW_INVALID_ID) return SBW_INVALID_ID;

    mb->selfWinId = win;
    g_infbox_state[win] = mb;
    SBOS_setWindowProc(win, InfBoxProc);

    SBOS_setFocus(win);
    SBOS_bringToFront(win);

    // --- gadgets ---
    mb->bgShape = SBOS_CreateBitmapView(win, 0, 0, prepWidth, prepHeight, baseGridLight, 32, 32, 32,
                                        BVF_WRAP | BVF_SRC_ROWMAJOR, GAD_TOOL_NOBORDER);

    mb->lblBg  = SBOS_CreateLabel(win, 10, 10, prepWidth - 30, (int16_t)(btnY - 15), "", GAD_TOOL_INSET);
    mb->lblMsg = SBOS_CreateLabel(win, 16, 16, prepWidth - 42, (int16_t)(btnY - 28), mb->txtMsg, GAD_TOOL_DEFAULT);

    mb->btnOk = SBOS_CreateButton(win, btnX[0], btnY, INF_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, lang_get(STR_COMMDLG_BTN_OK), GAD_TOOL_DEFAULT);

    SBOS_paintAllWindows();
    return win;
}



void SBOS_CloseInfoBox(SBXWindowId infbox_winhnd){
    LIB_INFBOX_PRIVATE *mb = infbox_get(infbox_winhnd);
    if (!mb) return;

    infbox_post_done(mb, 0);
    infbox_destroy(infbox_winhnd);
}





























