//////////////////////////////////////////////
///  FILE REQUESTER V1.0  - library access ///
//////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "cg_wintype.h"

#include "cg_renderer.h"
#include "cg_glyphs.h"
#include "cg_windowex.h"     // SBXWindowId
#include "cg_gad_scrollbar.h"
#include "cg_gad_listbox.h"

#include "lib_filerequest.h"


typedef struct LIB_FILEREQUEST_PRIVATE {
    SBXWindowId parentWinId;     // owner window id (receives DONE)
    SBXWindowId selfWinId;       // this requester window id

    char     *out_file;          // caller-owned output buffer
    uint32_t  out_size;          // buffer capacity
    void     *user;              // cookie returned to owner

    char currdir[256];           // faux dir (later FS_FNLEN)

    CGGadgetHandle btnOk, btnCancel, btnParent, btnRoot;
    CGGadgetHandle bgBitmap;
    CGGadgetHandle fListBox;
    CGGadgetHandle scrollBar;

    ItemLists_t fileListData;    // listbox items backing store

    int16_t selected_idx;        // current selection

} LIB_FILEREQUEST_PRIVATE;

static struct LIB_FILEREQUEST_PRIVATE *g_filerq_state[MAX_WINDOWS];

#define MAX_FILERQS 4  // pick a sane limit

static LIB_FILEREQUEST_PRIVATE g_filerq_pool[MAX_FILERQS];
static uint8_t g_filerq_used[MAX_FILERQS];



uint16_t SBOS_filerq_used_count(void){
    uint16_t n = 0;
    for (int i = 0; i < MAX_FILERQS; i++)
        if (g_filerq_used[i]) n++;
    return n;
}

uint16_t SBOS_filerq_capacity (void){ return MAX_FILERQS;}
uint16_t SBOS_filerq_poolsize (void){ return sizeof(g_filerq_pool); }
uint16_t SBOS_filerq_poolsize1(void){ return sizeof(g_filerq_pool[0]); }

static LIB_FILEREQUEST_PRIVATE* filerq_alloc(void){
    for (int i = 0; i < MAX_FILERQS; i++){
        if (!g_filerq_used[i]){
            g_filerq_used[i] = 1;
            memset(&g_filerq_pool[i], 0, sizeof(LIB_FILEREQUEST_PRIVATE));
            return &g_filerq_pool[i];
        }
    }
    return NULL;
}

static void filerq_free(LIB_FILEREQUEST_PRIVATE *st){
    if (!st) return;
    int idx = (int)(st - g_filerq_pool);
    if (idx >= 0 && idx < MAX_FILERQS){
        g_filerq_used[idx] = 0;
    }
}







// prototypes at the top here
static CGWindowProcRes FileRqProc(SBXWindowId win, const CGMessage_t *m);
static void filerq_post_done(LIB_FILEREQUEST_PRIVATE *st, int ok);
static void filerq_destroy(SBXWindowId filerq_winhnd);

static inline LIB_FILEREQUEST_PRIVATE* filerq_get(SBXWindowId win){
    if (win >= MAX_WINDOWS) return NULL;
    return g_filerq_state[win];
}

static void filerq_post_done(LIB_FILEREQUEST_PRIVATE *st, int ok){
    if (!st) return;

    // Suggested DONE mapping:
    // a = ok(1)/cancel(0)
    // b = user cookie
    // c = out_path pointer (same pointer caller gave)
    // d = requester window id (so owner can match)
    CG_PostWindowMsg(st->parentWinId,
                     CGEVT_SYS_FILERQ_DONE,
                     ok ? 1 : 0,
                     MSG_PTR(st->user),
                     MSG_PTR(st->out_file),
                     (int32_t)st->selfWinId);
}

static void filerq_destroy(SBXWindowId filerq_winhnd){
    if (filerq_winhnd >= MAX_WINDOWS) return;
    //LIB_FILEREQUEST_PRIVATE *st = g_filerq_state[filerq_winhnd];
    //g_filerq_state[filerq_winhnd] = NULL;

    LIB_FILEREQUEST_PRIVATE *st = g_filerq_state[filerq_winhnd];
    g_filerq_state[filerq_winhnd] = NULL;

    if (st){
        // IMPORTANT: prevent listbox from double-freeing the same list
        // (only needed if lb_free() also frees items)
        // Option 1: detach listbox's pointer before destroying window/gadgets
        // Option 2: decide listbox never owns items
        //  --- Option 1 below. ---

        // If you can get the listbox gadget:
        // GADGET_BASE_T *gb = SBOS_gadgetFromHandle(st->fListBox);
        // if (gb && gb->gadgetType == GAD_LISTBOX) {
        //     GAD_LISTBOX_T *lb = (GAD_LISTBOX_T*)gb->gadget;
        //     lb->items = NULL;
        // }
        //listitem_free(&st->fileListData);   // this handles the list free item
        listitem_deinit(&st->fileListData);
        filerq_free(st);
    }

    SBOS_destroyWindow(filerq_winhnd);
}

static void filerq_build_outpath(LIB_FILEREQUEST_PRIVATE *st){
    if (!st || !st->out_file || st->out_size <= 1) return;
    st->out_file[0] = '\0';
    if (st->selected_idx < 0) return;

    // You need a getter for ItemLists_t by index.
    // Replace this call with your real accessor.
    const char *item = listitem_get(&st->fileListData, st->selected_idx);
    if (!item || !item[0]) return;

    // Build: currdir + (maybe slash) + item
    // Ensure currdir ends with '/' or add one.
    char tmp[512]; // temp join buffer
    tmp[0] = '\0';

    strncpy(tmp, st->currdir, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    size_t n = strlen(tmp);
    if (n && tmp[n - 1] != '/'){
        if (n + 1 < sizeof(tmp)){
            tmp[n] = '/';
            tmp[n + 1] = '\0';
        }
    }

    strncat(tmp, item, sizeof(tmp) - 1 - strlen(tmp));

    // Copy into caller buffer (bounded)
    strncpy(st->out_file, tmp, st->out_size - 1);
    st->out_file[st->out_size - 1] = '\0';
}


static CGWindowProcRes FileRqProc(SBXWindowId win, const CGMessage_t *m){
    if (!m || m->mtype != CGMSG_GADGET && m->mtype != CGMSG_WINDOW) {
        // Depending on your enum values, you can just accept all and switch on eventClass.
    }

    LIB_FILEREQUEST_PRIVATE *st = filerq_get(win);
    if (!st) return(CGPROC_DEFAULT);

    switch(m->eventClass){

    case CGEVT_GAD_SCROLL_CHANGED: {
            // only one scroller so we know where this came from?
            if(m->gadget == st->scrollBar){
                GADGET_BASE_T *lb = SBOS_gadgetFromHandle(st->fListBox);
                if (!lb || !lb->gadget) return CGPROC_DEFAULT;
                SBOS_setListbox_top(lb, (int16_t)m->a);
            }
        }
        break;

    // window chrome close
    case CGEVT_WIN_CLOSE_REQUEST:
        filerq_post_done(st, 0);
        filerq_destroy(win);
        break;

    // --- gadgets ---
    case CGEVT_GAD_LISTBOX_CHANGED:
        // a == selected index in your emitter
        if(m->gadget == st->fListBox){
            st->selected_idx = (int16_t)m->a;
            SBOS_setScrollBarValue(st->scrollBar, m->b);
        }

        // Optional: update a label with filename preview later
        break;

    case CGEVT_GAD_LISTBOX_DBLHIT:
        // treat as OK (and build path)
        // TODO: build selection -> out_file (padded safe)
        filerq_post_done(st, 1);
        filerq_destroy(win);
        break;

    case CGEVT_GAD_BUTTON_HIT:
        if (m->gadget == st->btnCancel){
            filerq_post_done(st, 0);
            filerq_destroy(win);
        } else if (m->gadget == st->btnOk){
            // Build st->out_file = currdir + selected filename
            int16_t sel = SBOS_ListBoxGetSelectedIndex(st->fListBox);
            st->selected_idx = sel;

            filerq_build_outpath(st);

            // If nothing selected, treat as cancel (or just ignore)
            if (!st->out_file || st->out_file[0] == '\0'){
                filerq_post_done(st, 0);   // or: return; (to force user to select)
                filerq_destroy(win);
                break;
            }

            filerq_post_done(st, 1);
            filerq_destroy(win);
        }
        break;

    case CGEVT_WIN_RESIZED:
        {
            int16_t Req_height, Req_width;
            SBOS_getWindowSize(win, &Req_width, &Req_height);

            SBOS_resizeGadget(st->bgBitmap, Req_width, Req_height);

            Req_height -=80;
            //Req_width = FILEREQUEST_DEF_WIDTH - 40 - (WIN_BORDER * 2);
            Req_width -= 48 - (WIN_BORDER * 2);
            //SBOS_ListBoxResize(st->fListBox, Req_width, Req_height);
            SBOS_resizeGadget(st->fListBox, Req_width-8, Req_height);
            SBOS_resizeGadget(st->scrollBar, DEF_DIALOG_SCROLL_WIDTH, Req_height);

            SBOS_moveGadget(st->scrollBar, Req_width, 8);
            int16_t visCount = SBOS_getListBoxVisableCount(st->fListBox);

            int16_t fileCount = listitem_count(&st->fileListData) - (visCount); // 8 is what is visable items
            if(fileCount <0) fileCount = 0;

            //fileCount, 4, (fileCount/4)
            SBOS_setScrollBarMinMax(st->scrollBar, 0, fileCount, 4, (fileCount/4) );

            Req_height += 12;

            int16_t buttox = 8;
            SBOS_moveGadget(st->btnOk, buttox, Req_height);
            buttox += 5 + 40;
            SBOS_moveGadget(st->btnCancel, buttox, Req_height);
            buttox += 5 + 60;
            SBOS_moveGadget(st->btnParent, buttox, Req_height);
            buttox += 5 + 90;
            SBOS_moveGadget(st->btnRoot, buttox, Req_height);

        } break;

    default:
        break;
    }
    return(CGPROC_COMPLETE);
}

SBXWindowId SBOS_OpenFileRequester(SBXWindowId owner_winhnd, const CGFileRqParams *p){
    if (!p) return SBW_INVALID_ID;
    if (!p->out_path || p->out_cap <= 1) return SBW_INVALID_ID;

    // Allocate state (choose your allocator; using malloc as placeholder)
    // LIB_FILEREQUEST_PRIVATE *st = (LIB_FILEREQUEST_PRIVATE*)malloc(sizeof(*st));
    // if (!st) return SBW_INVALID_ID;

    LIB_FILEREQUEST_PRIVATE *st = filerq_alloc();
    if (!st) return SBW_INVALID_ID;

    int16_t Req_width = FILEREQUEST_DEF_WIDTH;
    int16_t Req_height = FILEREQUEST_DEF_HEIGHT;

    memset(st, 0, sizeof(*st));

    st->parentWinId = owner_winhnd;
    st->out_file = p->out_path;
    st->out_size = (uint32_t)p->out_cap;
    st->user = p->user;
    st->selected_idx = -1;

    // initial dir
    if (p->initial_dir && p->initial_dir[0]){
        strncpy(st->currdir, p->initial_dir, sizeof(st->currdir)-1);
    } else {
        strncpy(st->currdir, "/", sizeof(st->currdir)-1);
    }

    const char *title = (p->title && p->title[0]) ? p->title : "Open File";

    // create requester window
    SBXWindowId win = SBOS_createWindow(NULL, 40, 40, Req_width, Req_height, title,
                                        SBX_WF_RESIZABLE | SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_CLOSE | SBX_WF_MOVEABLE | SBX_WF_ZORDER | SBX_WF_SCREENBOUND);
    if (win == SBW_INVALID_ID){
        // free(st);
        return SBW_INVALID_ID;
    }

    SBOS_setWindowResizeLimits(win, Req_width, Req_height, SCR_WIDTH, 0x7fff); // set resize constraints

    st->bgBitmap = SBOS_CreateBitmapView(win, 0, 0, Req_width, Req_height, baseGrid, 32, 32, 32,
                                BVF_WRAP | BVF_SRC_ROWMAJOR, GAD_TOOL_NOBORDER);



    st->selfWinId = win;
    g_filerq_state[win] = st;

    SBOS_setWindowProc(win, FileRqProc);

    // Build file list (FAUX for now)
    //st->fileListData = SBOS_createItemList(); // whatever your API is
    listitem_init(&st->fileListData);   //
    listitem_add(&st->fileListData, "test1.txt");
    listitem_add(&st->fileListData, "test2.bmp");
    listitem_add(&st->fileListData, "docs/");
    listitem_add(&st->fileListData, "monty1.sid");
    listitem_add(&st->fileListData, "1_67YT-Turrican_III_Remix.sid");
    listitem_add(&st->fileListData, "Bionic_Commando(+).sid");
    listitem_add(&st->fileListData, "Co-Axis_Remix(+).sid");
    listitem_add(&st->fileListData, "Mega_Apocalypse_remix.sid");
    listitem_add(&st->fileListData, "Wiklund_-_Cheese.sid");
    listitem_add(&st->fileListData, "1943.YM");
    listitem_add(&st->fileListData, "Union Demo - Alloy Run.ym");
    listitem_add(&st->fileListData, "sonic 2.vgm");
    listitem_add(&st->fileListData, "sonic 3 - stage 2.vgm");
    listitem_add(&st->fileListData, "turrican 2 - title.tfx");
    listitem_add(&st->fileListData, "sweet dreams.mod");
    listitem_add(&st->fileListData, "unreal.mod");
    listitem_add(&st->fileListData, "matkamis.mod");
    listitem_add(&st->fileListData, "egyption-knights.mod");
    listitem_add(&st->fileListData, "mcappin-dance.mod");

    listitem_add(&st->fileListData, "test1.txt");
    listitem_add(&st->fileListData, "test2.bmp");
    listitem_add(&st->fileListData, "docs/");
    listitem_add(&st->fileListData, "monty1.sid");
    listitem_add(&st->fileListData, "1_67YT-Turrican_III_Remix.sid");
    listitem_add(&st->fileListData, "Bionic_Commando(+).sid");
    listitem_add(&st->fileListData, "Co-Axis_Remix(+).sid");
    listitem_add(&st->fileListData, "Mega_Apocalypse_remix.sid");
    listitem_add(&st->fileListData, "Wiklund_-_Cheese.sid");
    listitem_add(&st->fileListData, "1943.YM");
    listitem_add(&st->fileListData, "Union Demo - Alloy Run.ym");
    listitem_add(&st->fileListData, "sonic 2.vgm");
    listitem_add(&st->fileListData, "sonic 3 - stage 2.vgm");
    listitem_add(&st->fileListData, "turrican 2 - title.tfx");
    listitem_add(&st->fileListData, "sweet dreams.mod");
    listitem_add(&st->fileListData, "unreal.mod");
    listitem_add(&st->fileListData, "matkamis.mod");
    listitem_add(&st->fileListData, "egyption-knights.mod");
    listitem_add(&st->fileListData, "mcappin-dance.mod");

    listitem_add(&st->fileListData, "test1.txt");
    listitem_add(&st->fileListData, "test2.bmp");
    listitem_add(&st->fileListData, "docs/");
    listitem_add(&st->fileListData, "monty1.sid");
    listitem_add(&st->fileListData, "1_67YT-Turrican_III_Remix.sid");
    listitem_add(&st->fileListData, "Bionic_Commando(+).sid");
    listitem_add(&st->fileListData, "Co-Axis_Remix(+).sid");
    listitem_add(&st->fileListData, "Mega_Apocalypse_remix.sid");
    listitem_add(&st->fileListData, "Wiklund_-_Cheese.sid");
    listitem_add(&st->fileListData, "1943.YM");
    listitem_add(&st->fileListData, "Union Demo - Alloy Run.ym");
    listitem_add(&st->fileListData, "sonic 2.vgm");
    listitem_add(&st->fileListData, "sonic 3 - stage 2.vgm");
    listitem_add(&st->fileListData, "turrican 2 - title.tfx");
    listitem_add(&st->fileListData, "sweet dreams.mod");
    listitem_add(&st->fileListData, "unreal.mod");
    listitem_add(&st->fileListData, "matkamis.mod");
    listitem_add(&st->fileListData, "egyption-knights.mod");
    listitem_add(&st->fileListData, "mcappin-dance.mod");


    listitem_add(&st->fileListData, "test1.txt");
    listitem_add(&st->fileListData, "test2.bmp");
    listitem_add(&st->fileListData, "docs/");
    listitem_add(&st->fileListData, "monty1.sid");
    listitem_add(&st->fileListData, "1_67YT-Turrican_III_Remix.sid");
    listitem_add(&st->fileListData, "Bionic_Commando(+).sid");
    listitem_add(&st->fileListData, "Co-Axis_Remix(+).sid");
    listitem_add(&st->fileListData, "Mega_Apocalypse_remix.sid");
    listitem_add(&st->fileListData, "Wiklund_-_Cheese.sid");
    listitem_add(&st->fileListData, "1943.YM");
    listitem_add(&st->fileListData, "Union Demo - Alloy Run.ym");
    listitem_add(&st->fileListData, "sonic 2.vgm");
    listitem_add(&st->fileListData, "sonic 3 - stage 2.vgm");
    listitem_add(&st->fileListData, "turrican 2 - title.tfx");
    listitem_add(&st->fileListData, "sweet dreams.mod");
    listitem_add(&st->fileListData, "unreal.mod");
    listitem_add(&st->fileListData, "matkamis.mod");
    listitem_add(&st->fileListData, "egyption-knights.mod");
    listitem_add(&st->fileListData, "mcappin-dance.mod");


    listitem_add(&st->fileListData, "test1.txt");
    listitem_add(&st->fileListData, "test2.bmp");
    listitem_add(&st->fileListData, "docs/");
    listitem_add(&st->fileListData, "monty1.sid");
    listitem_add(&st->fileListData, "1_67YT-Turrican_III_Remix.sid");
    listitem_add(&st->fileListData, "Bionic_Commando(+).sid");
    listitem_add(&st->fileListData, "Co-Axis_Remix(+).sid");
    listitem_add(&st->fileListData, "Mega_Apocalypse_remix.sid");
    listitem_add(&st->fileListData, "Wiklund_-_Cheese.sid");
    listitem_add(&st->fileListData, "1943.YM");
    listitem_add(&st->fileListData, "Union Demo - Alloy Run.ym");
    listitem_add(&st->fileListData, "sonic 2.vgm");
    listitem_add(&st->fileListData, "sonic 3 - stage 2.vgm");
    listitem_add(&st->fileListData, "turrican 2 - title.tfx");
    listitem_add(&st->fileListData, "sweet dreams.mod");
    listitem_add(&st->fileListData, "unreal.mod");
    listitem_add(&st->fileListData, "matkamis.mod");
    listitem_add(&st->fileListData, "egyption-knights.mod");
    listitem_add(&st->fileListData, "mcappin-dance.mod");


    int16_t fileCount = listitem_count(&st->fileListData) - 8; // 8 is what is visable items

    //CGGadgetHandle SBOS_CreateScrollbar(SBXWindowId win, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t orient, int16_t min, int16_t max, int16_t step_small, int16_t step_large, uint32_t flags, uint8_t BLONDE)
    Req_height -= 80;
    // Create gadgets
    st->scrollBar = SBOS_CreateScrollbar(win,
                                         Req_width - 40 - (WIN_BORDER * 2) + 8, 8, DEF_DIALOG_SCROLL_WIDTH, Req_height, SB_ORIENT_VERT, 0, fileCount, 4, (fileCount/4), GAD_TOOL_SCROLLARROWS | GAD_TOOL_DEFAULT);
    st->fListBox  = SBOS_CreateListBox(win, 8, 8, Req_width - 40 - (WIN_BORDER * 2), Req_height, &st->fileListData, GAD_TOOL_DEFAULT);

    int16_t buttox = 8;

    Req_height += 12;
    st->btnOk     = SBOS_CreateButton (win, buttox, Req_height, 40, DEF_DIALOG_BUTTON_HEIGHT, "OK",     GAD_TOOL_DEFAULT);
    buttox += 5 + 40;
    st->btnCancel = SBOS_CreateButton (win, buttox, Req_height, 60, DEF_DIALOG_BUTTON_HEIGHT, "Cancel", GAD_TOOL_DEFAULT);
    buttox += 5 + 60;
    st->btnParent = SBOS_CreateButton (win, buttox, Req_height, 90, DEF_DIALOG_BUTTON_HEIGHT, "Parent...", GAD_TOOL_DEFAULT);
    buttox += 5 + 90;
    st->btnRoot   = SBOS_CreateButton (win, buttox, Req_height, 50, DEF_DIALOG_BUTTON_HEIGHT, "Root", GAD_TOOL_DEFAULT);

    SBOS_bringToFront(win);
    SBOS_setFocus(win);

    SBOS_paintAllWindows();
    return win;
}

void SBOS_CloseFileRequester(SBXWindowId filerq_winhnd){
    LIB_FILEREQUEST_PRIVATE *st = filerq_get(filerq_winhnd);
    if (!st) return;

    filerq_post_done(st, 0);
    filerq_destroy(filerq_winhnd);
}
