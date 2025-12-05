#include "sms.h"
#include "internal.h"

void ClearPadRegisters(struct SMS_Core* sms){
    sms->port.a  = 0xff;
}
// [API]
void SMS_set_port_a(struct SMS_Core* sms, enum SMS_PortA pin)
{
    sms->port.a &= ~pin;
}

void SMS_set_port_b(struct SMS_Core *sms, enum SMS_PortB pin){
    sms->port.b &= ~pin;
    if (SMS_is_system_type_gg(sms)){
        sms->port.gg_regs[0x0] &= ~0x80;
    }
}


// captured joystick inputs
enum SMS_PortA joyPadBindA;     // set bits
enum SMS_PortB joyPadBindB;     // set bit
enum SMS_PortA joyPadBindAC;    // clear bits   -- if a bit is set here, it nulls out the set bit in joyPadBindA
enum SMS_PortB joyPadBindBC;    // clear bits   -- if a bit is set here, it nulls out the set bit in joyPadBindB


void sms_keydown(int keycode){
    //printf("KD: code: 0x%x[_]\n", keycode);
    if(!(keycode & 0xF000))
        joyPadBindA = (enum SMS_PortA)(joyPadBindA | ((enum SMS_PortA)(keycode & 0xFF)));
    else
        joyPadBindB = (enum SMS_PortB)(joyPadBindB | ((enum SMS_PortB)(keycode & 0xFF)));
}

void sms_keyup(int keycode){
    //printf("KU: code: 0x%x[-]\n", keycode);
    if(!(keycode & 0xF000))
        joyPadBindAC = (enum SMS_PortA)(joyPadBindAC | ((enum SMS_PortA)(keycode & 0xFF)));
    else
        joyPadBindBC = (enum SMS_PortB)(joyPadBindBC | ((enum SMS_PortB)(keycode & 0xFF)));
}
