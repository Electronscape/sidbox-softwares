#ifndef LIB_FILEREQUEST_H
#define LIB_FILEREQUEST_H

#include <stdint.h>
#include "cg_windowex.h"     // SBXWindowId
#include "cg_msghandler.h"   // MSG_PTR, MSG_AS_PTR


// Result event posted back to owner window.
// eventClass = CGEVT_SYS_FILERQ_DONE
// a = 1 ok, 0 cancel
// b = MSG_PTR(user_cookie)
// c = MSG_PTR(out_path)   (same pointer the caller gave)
// d = (int32_t)filerq_winhnd   (so owner can match which requester finished)


typedef struct CGFileRqParams {
    const char *title;        // window title text (optional)
    const char *initial_dir;  // initial directory string (optional)
    const char *filter;       // optional filter string, your format (e.g. "*.bmp|*.txt|*.*")
    char       *out_path;     // caller-owned output buffer
    uint32_t    out_cap;      // size of out_path buffer in bytes
    void       *user;  // returned to caller via message
} CGFileRqParams;

// Opens a non-modal file requester window.
// Returns the requester window id, or SBW_INVALID_ID on failure.
// When finished, it posts CGEVT_SYS_FILERQ_DONE to owner_winhnd.
SBXWindowId SBOS_OpenFileRequester(SBXWindowId owner_winhnd, const CGFileRqParams *p);

// Optional: programmatically close (cancel) an existing requester.
void SBOS_CloseFileRequester(SBXWindowId filerq_winhnd);


#endif // LIB_FILEREQUEST_H
