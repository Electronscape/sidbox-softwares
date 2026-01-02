//////////////////////////////////////////////
///  FILE REQUESTER V1.0  - library access ///
//////////////////////////////////////////////

//#include <stdio.h>
#include <string.h>

#include "sys_font.h"
#include "../sbapi_graphics.h"

#include "cg_glyphs.h"

#include "cg_wintype.h"
#include "cg_windowex.h"     // SBXWindowId

#include "lib_msgbox.h"



// a dirt simple msgbox dialog, that can double up as a confirmation box too
// FUTURE SELF: make this disable the parent window so it cant focus bring to front or something, UNDECIDED RIGHT NOW

typedef struct LIB_MSGBOX_PRIVATE {
    SBXWindowId parentWinId;        // owner window id (receives DONE)
    SBXWindowId selfWinId;          // this requester window id
    void     *user;                 // cookie returned to owner

    char txtMsg[DEF_GADGET_TEXT_SIZE];               // faux dir (later FS_FNLEN)

    CGGadgetHandle btnOk, btnCancel, btnYes, btnNo;
    CGGadgetHandle lblBg,   // background area
        lblMsg,
        bgShape;            // bitmap view for the background fill



    uint8_t flags;

} LIB_MSGBOX_PRIVATE;


static struct LIB_MSGBOX_PRIVATE *g_msgbox_state[MAX_WINDOWS];


#define MAX_MSGBOXES 4   // choose your limit

static LIB_MSGBOX_PRIVATE g_msgbox_pool[MAX_MSGBOXES];
static uint8_t            g_msgbox_used[MAX_MSGBOXES];

uint16_t SBOS_msgbox_used_count(void){
    uint16_t n = 0;
    for (int i = 0; i < MAX_MSGBOXES; i++)
        if (g_msgbox_used[i]) n++;
    return n;
}

uint16_t SBOS_msgbox_capacity (void){ return MAX_MSGBOXES; }
uint16_t SBOS_msgbox_poolsize (void){ return sizeof(g_msgbox_used); }
uint16_t SBOS_msgbox_poolsize1(void){ return sizeof(g_msgbox_used[0]); }

static LIB_MSGBOX_PRIVATE* msgbox_alloc(void){
    for (int i = 0; i < MAX_MSGBOXES; i++){
        if (!g_msgbox_used[i]){
            g_msgbox_used[i] = 1;
            memset(&g_msgbox_pool[i], 0, sizeof(g_msgbox_pool[i]));
            return &g_msgbox_pool[i];
        }
    }
    return NULL;
}

static void msgbox_free(LIB_MSGBOX_PRIVATE *mb){
    if (!mb) return;
    int idx = (int)(mb - g_msgbox_pool);
    if (idx >= 0 && idx < MAX_MSGBOXES){
        g_msgbox_used[idx] = 0;
    }
}



// prototypes at the top here
static CGWindowProcRes MsgBoxProc(SBXWindowId win, const CGMessage_t *m);
static void msgbox_post_done(LIB_MSGBOX_PRIVATE *msgbox, int ok);
static void msgbox_destroy(SBXWindowId msgbox_winhnd);


static inline LIB_MSGBOX_PRIVATE* msgbox_get(SBXWindowId win){
    if (win >= MAX_WINDOWS) return NULL;
    return g_msgbox_state[win];
}


static void msgbox_post_done(LIB_MSGBOX_PRIVATE *mb, int choice){
    if (!mb) return;

    // Suggested DONE mapping:
    // a = ok(1)/cancel(0)
    // b = user cookie
    // c = out_path pointer (same pointer caller gave)
    // d = requester window id (so owner can match)

    CG_PostWindowMsg(mb->parentWinId,
                     CGEVT_SYS_MSGBOX_DONE,
                     choice,
                     MSG_PTR(mb->user),
                     0,
                     (int32_t)mb->selfWinId);

}



static void msgbox_destroy(SBXWindowId msgbox_winhnd){
    if (msgbox_winhnd >= MAX_WINDOWS) return;

    LIB_MSGBOX_PRIVATE *mb = g_msgbox_state[msgbox_winhnd];
    g_msgbox_state[msgbox_winhnd] = NULL;

    if (mb){
        msgbox_free(mb);
    }

    SBOS_destroyWindow(msgbox_winhnd);
}




static CGWindowProcRes MsgBoxProc(SBXWindowId win, const CGMessage_t *m){
    if (!m) return CGPROC_DEFAULT;

    LIB_MSGBOX_PRIVATE *mb = msgbox_get(win);
    if (!mb) return CGPROC_DEFAULT;

    switch(m->eventClass){
    case CGEVT_WIN_CLOSE_REQUEST:
        msgbox_post_done(mb, MSGBOX_CANCEL);
        msgbox_destroy(win);
        return CGPROC_COMPLETE;

    case CGEVT_GAD_BUTTON_HIT:
        if (m->gadget == mb->btnCancel){
            msgbox_post_done(mb, MSGBOX_CANCEL);
            msgbox_destroy(win);
            return CGPROC_COMPLETE;
        }
        if (m->gadget == mb->btnOk){
            msgbox_post_done(mb, MSGBOX_OK);
            msgbox_destroy(win);
            return CGPROC_COMPLETE;
        }
        if (m->gadget == mb->btnYes){
            msgbox_post_done(mb, MSGBOX_YES);
            msgbox_destroy(win);
            return CGPROC_COMPLETE;
        }
        if (m->gadget == mb->btnNo){
            msgbox_post_done(mb, MSGBOX_NO);
            msgbox_destroy(win);
            return CGPROC_COMPLETE;
        }
        return CGPROC_DEFAULT;

    default:
        return CGPROC_DEFAULT;
    }
    return CGPROC_DEFAULT;
}

#define MSG_WIN_WIDTH           290
#define MSG_WIN_HEIGHT          100
//#define MSG_BTN_HEIGHT          24
#define MSG_BTN_MARGIN_BOTTOM  (12 + WIN_TITLE_HEIGHT)
#define MGS_BTN_WIDTH           80

static int16_t clamp16(int16_t v, int16_t lo, int16_t hi){
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

SBXWindowId SBOS_MessageBox(SBXWindowId owner_winhnd, const CGFMsgBoxParams *p){
    if (!p) return SBW_INVALID_ID;

    LIB_MSGBOX_PRIVATE *mb = msgbox_alloc();
    if (!mb) return SBW_INVALID_ID;

    memset(mb, 0, sizeof(*mb));
    mb->parentWinId = owner_winhnd;
    mb->user = p->user;
    mb->flags = p->flags;

    if (p->txtMsg && p->txtMsg[0])
        strncpy(mb->txtMsg, p->txtMsg, sizeof(mb->txtMsg)-1);
    else
        strncpy(mb->txtMsg, "- NO MESSAGE! -", sizeof(mb->txtMsg)-1);

    const char *title = (p->title && p->title[0]) ? p->title : "SIDBOX OS: Message";

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
    int16_t prepWidth  = clamp16((int16_t)(maxChars * FONT_WIDTH + 32), MSG_WIN_WIDTH, SCR_WIDTH);

    // --- window pos ---
    int16_t msgx = (int16_t)((SCR_WIDTH  / 2) - (prepWidth  / 2));
    int16_t msgy = (int16_t)((SCR_HEIGHT / 3) - (prepHeight / 3));

    // --- buttons count ---
    int8_t nButtons = 1;
    if (mb->flags == MSGBOXF_OK)          nButtons = 1;
    if (mb->flags == MSGBOXF_OKCANCEL)    nButtons = 2;
    if (mb->flags == MSGBOXF_YESNO)       nButtons = 2;
    if (mb->flags == MSGBOXF_YESNOCANCEL) nButtons = 3;

    // --- button layout ---
    const int16_t gap = 8;

    int16_t btnY = (int16_t)(prepHeight - MSG_BTN_MARGIN_BOTTOM - DEF_DIALOG_BUTTON_HEIGHT);

    // keep your “content region” fidelity (close button margins)
    int16_t contentX = WIN_CLOSE_WIDTH;
    int16_t contentW = (int16_t)(prepWidth - 2 * WIN_CLOSE_WIDTH);

    int16_t totalW = (int16_t)(nButtons * MGS_BTN_WIDTH + (nButtons - 1) * gap);
    int16_t startX = (int16_t)(contentX + (contentW - totalW + 1) / 2);

    int16_t btnX[3];
    for (int i = 0; i < nButtons; i++)
        btnX[i] = (int16_t)(startX + i * (MGS_BTN_WIDTH + gap));

    // --- create window ---
    SBXWindowId win = SBOS_createWindow(NULL, msgx, msgy, prepWidth, prepHeight, title,
                                        SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_CLOSE | SBX_WF_MOVEABLE | SBX_WF_ZORDER | SBX_WF_SCREENBOUND);

    if (win == SBW_INVALID_ID) return SBW_INVALID_ID;

    mb->selfWinId = win;
    g_msgbox_state[win] = mb;
    SBOS_setWindowProc(win, MsgBoxProc);

    SBOS_setFocus(win);
    SBOS_bringToFront(win);

    // --- gadgets ---
    mb->bgShape = SBOS_CreateBitmapView(win, 0, 0, prepWidth, prepHeight, baseGridLight, 32, 32, 32,
                                        BVF_WRAP | BVF_SRC_ROWMAJOR, GAD_TOOL_NOBORDER);

    mb->lblBg  = SBOS_CreateLabel(win, 10, 10, prepWidth - 30, (int16_t)(btnY - 15), "", GAD_TOOL_INSET);
    mb->lblMsg = SBOS_CreateLabel(win, 16, 16, prepWidth - 42, (int16_t)(btnY - 28), mb->txtMsg, GAD_TOOL_DEFAULT);

    if (mb->flags == MSGBOXF_OK) {
        mb->btnOk = SBOS_CreateButton(win, btnX[0], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "OK", GAD_TOOL_DEFAULT);
    } else if (mb->flags == MSGBOXF_OKCANCEL) {
        mb->btnOk     = SBOS_CreateButton(win, btnX[0], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "OK",     GAD_TOOL_DEFAULT);
        mb->btnCancel = SBOS_CreateButton(win, btnX[1], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "Cancel", GAD_TOOL_DEFAULT);
    } else if (mb->flags == MSGBOXF_YESNO) {
        mb->btnYes = SBOS_CreateButton(win, btnX[0], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "Yes", GAD_TOOL_DEFAULT);
        mb->btnNo  = SBOS_CreateButton(win, btnX[1], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "No",  GAD_TOOL_DEFAULT);
    } else { // MSGBOXF_YESNOCANCEL
        mb->btnYes    = SBOS_CreateButton(win, btnX[0], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "Yes",    GAD_TOOL_DEFAULT);
        mb->btnNo     = SBOS_CreateButton(win, btnX[1], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "No",     GAD_TOOL_DEFAULT);
        mb->btnCancel = SBOS_CreateButton(win, btnX[2], btnY, MGS_BTN_WIDTH, DEF_DIALOG_BUTTON_HEIGHT, "Cancel", GAD_TOOL_DEFAULT);
    }

    SBOS_paintAllWindows();
    return win;
}



void SBOS_CloseMessageBox(SBXWindowId msgbox_winhnd){
    LIB_MSGBOX_PRIVATE *mb = msgbox_get(msgbox_winhnd);
    if (!mb) return;

    msgbox_post_done(mb, 0);
    msgbox_destroy(msgbox_winhnd);
}







