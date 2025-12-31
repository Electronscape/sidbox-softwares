#include <stdio.h>
#include <stdint.h>


void cg_os_messagehandler(uint8_t msgticks){
    // message ticks are How many messages can be done PER CALL to this
    // this should ONLY be put into a Hardware timer, OR in the main OS loop!
    // NEVER ANY PROGRAMS and should ONLY be internal, NO API access to this at all
    printf(".");    // message handler heart beat;
}
