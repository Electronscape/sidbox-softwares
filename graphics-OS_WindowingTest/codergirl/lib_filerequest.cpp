//////////////////////////////////////////////
///  FILE REQUESTER V1.0  - library access ///
//////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "cg_wintype.h"
#include "lib_filerequest.h"
#include "cg_gad_listbox.h"



typedef struct LIB_FILEREQUEST_PRIVATE {
    SBXWindowId parentWinId;     // owner window id (receives DONE)
    SBXWindowId selfWinId;       // this requester window id

    char     *out_file;          // caller-owned output buffer
    uint32_t  out_size;          // buffer capacity
    void     *user;              // cookie returned to owner

    char currdir[256];           // faux dir (later FS_FNLEN)

    CGGadgetHandle btnOk, btnCancel;
    CGGadgetHandle fListBox;

    ItemLists_t fileListData;    // listbox items backing store

    int16_t selected_idx;        // current selection

} LIB_FILEREQUEST_PRIVATE;

static struct LIB_FILEREQUEST_PRIVATE *g_filerq_state[MAX_WINDOWS];

// prototypes at the top here
static void FileRqProc(SBXWindowId win, const CGMessage_t *m);
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

    printf("SYS:FILEREQ:%s\n", st->out_file);
}

static void filerq_destroy(SBXWindowId filerq_winhnd){
    if (filerq_winhnd >= MAX_WINDOWS) return;
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
    printf("BuildOutPath: ");
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


static void FileRqProc(SBXWindowId win, const CGMessage_t *m){
    if (!m || m->mtype != CGMSG_GADGET && m->mtype != CGMSG_WINDOW) {
        // Depending on your enum values, you can just accept all and switch on eventClass.
    }

    LIB_FILEREQUEST_PRIVATE *st = filerq_get(win);
    if (!st) return;

    switch(m->eventClass){

    // window chrome close
    case CGEVT_WIN_CLOSE_REQUEST:
        filerq_post_done(st, 0);
        filerq_destroy(win);
        break;

    // --- gadgets ---
    case CGEVT_GAD_LISTBOX_CHANGED:
        // a == selected index in your emitter
        st->selected_idx = (int16_t)m->a;
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

    default:
        break;
    }
}

SBXWindowId SBOS_OpenFileRequester(SBXWindowId owner_winhnd, const CGFileRqParams *p){
    if (!p) return SBW_INVALID_ID;
    if (!p->out_path || p->out_cap <= 1) return SBW_INVALID_ID;

    // Allocate state (choose your allocator; using malloc as placeholder)
    // LIB_FILEREQUEST_PRIVATE *st = (LIB_FILEREQUEST_PRIVATE*)malloc(sizeof(*st));
    // if (!st) return SBW_INVALID_ID;

    static LIB_FILEREQUEST_PRIVATE st_store; // TEMP: single instance (replace with pool/malloc)
    LIB_FILEREQUEST_PRIVATE *st = &st_store;

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
    SBXWindowId win = SBOS_createWindow(NULL, 40, 40, 290, 206, title,
                                        SBX_WF_VISIBLE | SBX_WF_TITLE_BAR | SBX_WF_CLOSE | SBX_WF_MOVEABLE | SBX_WF_ZORDER | SBX_WF_SCREENBOUND);
    if (win == SBW_INVALID_ID){
        // free(st);
        return SBW_INVALID_ID;
    }

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



    // Create gadgets
    st->fListBox = SBOS_CreateListBox(win, 8, 8, 264, 140, &st->fileListData, GAD_TOOL_DEFAULT);
    st->btnOk    = SBOS_CreateButton (win, 140, 154, 60, 18, "OK",     GAD_TOOL_DEFAULT);
    st->btnCancel= SBOS_CreateButton (win, 210, 154, 60, 18, "Cancel", GAD_TOOL_DEFAULT);

    SBOS_paintAllWindows();
    return win;
}

void SBOS_CloseFileRequester(SBXWindowId filerq_winhnd){
    LIB_FILEREQUEST_PRIVATE *st = filerq_get(filerq_winhnd);
    if (!st) return;

    filerq_post_done(st, 0);
    filerq_destroy(filerq_winhnd);
}
