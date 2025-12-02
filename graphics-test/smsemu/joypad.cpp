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
    if (SMS_is_system_type_gg(sms))
        sms->port.gg_regs[0x0] &= ~0x80;
}

void SMS_set_port_b_old(struct SMS_Core* sms, enum SMS_PortB pin, uint8_t down)
{
    if (pin == PAUSE_BUTTON)
    {
        if (SMS_is_system_type_gg(sms))
        {
            sms->port.gg_regs[0x0] &= ~0x80;

            if (!down)
            {
                sms->port.gg_regs[0x0] |= 0x80;
            }
        }
        else
        {
            if (down)
            {
                z80_nmi(sms);
            }
        }
    }
    else
    {
        if (down)
        {
            sms->port.b &= ~pin;
        }
        else
        {
            sms->port.b |= pin;
        }

    }
}
