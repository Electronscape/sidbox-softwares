#ifndef SMS_H
#define SMS_H



#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif



//extern const uint8_t BIOS_ROM[];
extern uint8_t *BIOS_ROM;
extern const uint8_t BIOS_ROM_ZEXALL[];
extern const uint8_t BIOS_ROM_ALEXKID[];

// alex kid 128k
// zexell 64k

#define BIOS_ROM_SIZE	128

extern volatile uint8_t vram[16384];        // 16KB   // VIDEO MEMORY
extern volatile uint8_t system_ram[8192];   // 8kb system RAM
extern volatile uint8_t *ROM;               // a ROM location
extern volatile uint8_t *ram[3];// not all values are listed here because the other

extern struct SMS_Core sms;


#ifdef __cplusplus
}
#endif

#endif // SMS_H
