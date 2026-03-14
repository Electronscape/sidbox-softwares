#ifndef SMS_H
#define SMS_H


#include "types.h"

//extern const uint8_t BIOS_ROM[];
extern const uint8_t *BIOS_ROM;

// alex kid 128k
// zexell 64k

#define BIOS_ROM_SIZE	128

extern volatile uint8_t *vram;
extern volatile uint8_t *system_ram;
extern volatile uint8_t *ROM;
extern volatile uint8_t *ram[3];// not all values are listed here because the other

extern struct SMS_Core sms;



#endif // SMS_H
