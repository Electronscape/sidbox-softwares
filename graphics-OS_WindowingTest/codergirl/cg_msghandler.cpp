#include <stdio.h>
#include <stdint.h>

#include "cg_wintype.h"
#include "cg_windowex.h"

#include "cg_msghandler.h"


// INTERNALS ------------------------------------------------------------------------------------------------------


static CGMessage_t  g_msgs[CGMSG_QUEUE_CAP];

static volatile uint16_t g_msg_hx = 0;
static volatile uint16_t g_msg_tx = 0;

static uint32_t g_msg_dropped = 0;
static uint32_t g_msg_posted  = 0;
static uint32_t g_msg_sent    = 0;

static inline uint16_t msgq_n(uint16_t v) { // next message
    return (uint16_t)((v + 1u) % CGMSG_QUEUE_CAP);
}

static inline uint8_t msgq_f(void) {    // messages full check
    return (msgq_n(g_msg_hx) == g_msg_tx);
}

static inline uint8_t msgq_e(void) {        // message end check
    return (g_msg_hx == g_msg_tx);
}




// API INTERFACES -------------------------------------------------------------------------------------------------
uint8_t SBOS_PostMessage(const CGMessage_t *m)
{
    if (!m) return 0;

    if (msgq_f()) {
        g_msg_dropped++;
        return 0;
    }

    g_msgs[g_msg_hx] = *m;
    g_msg_hx = msgq_n(g_msg_hx);

    g_msg_posted++;
    return 1;
}


uint8_t SBOS_PopMessage(CGMessage_t *out)
{
    if (!out) return 0;
    if (msgq_e()) return 0;

    *out = g_msgs[g_msg_tx];
    g_msg_tx = msgq_n(g_msg_tx);

    return 1;
}



void cg_os_messagehandler(uint8_t msgticks){
    // message ticks are How many messages can be done PER CALL to this
    // this should ONLY be put into a Hardware timer, OR in the main OS loop!
    // NEVER ANY PROGRAMS and should ONLY be internal, NO API access to this at all
    CGMessage_t m;
    while (msgticks-- > 0) {
        if (!SBOS_PopMessage(&m)) break;
        sbx_window_t *w = SBOS_getWindow(m.winhnd);

        if (w && w->proc) {
            printf("MSG: winID: %lu, type: %u\n", m.winhnd, m.gadget);
            w->proc(m.winhnd, &m);  // programmer runs here (may freeze themselves 😄)
            g_msg_sent++;
        }
        printf(".");    // message handler heart beat;
    }

}
