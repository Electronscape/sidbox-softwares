#ifndef _SMSTYPES_
#define _SMSTYPES_


#include <stdio.h>
#include "types.h"




#ifdef __linux__        // LINUX blank outs
    #define dbug printf
    #define __ATTR_RAM_TC
    #define FORCE_INLINE
    extern unsigned char  duunmap_types[64][64];			// world data types
    extern unsigned char  duunmap_textureid[4][64][64];	// textures: floor+wall(side a)/ walls(side b)/ ceiling
    extern signed   short duunmap_lm[64][64];				// light levels
    // MASTERGEAR MOD
    #define SEGAHV_CONFIG_TONEMODE		0x01	// 0=standard SY chip, 1=pwm style ;)
#endif


extern volatile bool frameReady;

#define MAX_SPRITES 16


#if 0
    // runs at 3.58MHz
    #define CPU_CLOCK (3580000)
#else
    // this value was taken for sms power docs
    //#define CPU_CLOCK (4117000)// 15%
    //#define CPU_CLOCK (3899545)	// 8% faster
    //#define CPU_CLOCK (3580000)
    #define CPU_CLOCK (int)(3580000.0f * 1.08f)	// 20% faster
#endif

#define CYCLES_PER_FRAME (CPU_CLOCK / 60)

#define SMS_STATIC inline
#define SMS_INLINE inline
#define SMS_FORCE_INLINE __attribute__((always_inline))

#define LIKELY(c) 	((c))
#define UNLIKELY(c) ((c))

#define UNREACHABLE(ret) return ret
// used mainly in debugging when i want to quickly silence
// the compiler about unsed vars.
#define UNUSED(var) ((void)(var))

// ONLY use this for C-arrays, not pointers, not structs
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

//#define SMS_log dbug
//#define SMS_log_err dbug
//#define SMS_log_fatal dbug

// returns 1 OR 0
//#define IS_BIT_SET(v, bit) (!!(v & (1 << bit)))
#define IS_BIT_SET(v, bit) (((v) >> (bit)) & 1U)


#ifdef __linux__
    extern uint32_t PROJ_CRAM[256];
    #define clut PROJ_CRAM
#else
    #define clut CLUT
#endif


#define BIT_TST0(a)  (((a) >> 0)  & 1U)
#define BIT_TST1(a)  (((a) >> 1)  & 1U)
#define BIT_TST2(a)  (((a) >> 2)  & 1U)
#define BIT_TST3(a)  (((a) >> 3)  & 1U)
#define BIT_TST4(a)  (((a) >> 4)  & 1U)
#define BIT_TST5(a)  (((a) >> 5)  & 1U)
#define BIT_TST6(a)  (((a) >> 6)  & 1U)
#define BIT_TST7(a)  (((a) >> 7)  & 1U)

#define BIT_TST8(a)  (((a) >> 8)  & 1U)
#define BIT_TST9(a)  (((a) >> 9)  & 1U)
#define BIT_TST10(a) (((a) >> 10) & 1U)
#define BIT_TST11(a) (((a) >> 11) & 1U)
#define BIT_TST12(a) (((a) >> 12) & 1U)
#define BIT_TST13(a) (((a) >> 13) & 1U)
#define BIT_TST14(a) (((a) >> 14) & 1U)
#define BIT_TST15(a) (((a) >> 15) & 1U)



// SMS MEMORY INTERACTION DECLARATIONS
// [BUS]
uint8_t SMS_read8(struct SMS_Core* sms, uint16_t addr);
void SMS_write8(struct SMS_Core* sms, uint16_t addr, uint8_t value);
uint16_t SMS_read16(struct SMS_Core* sms, uint16_t addr);
void SMS_write16(struct SMS_Core* sms, uint16_t addr, uint16_t value);
void SMS_write_io(struct SMS_Core* sms, unsigned char addr, unsigned char value);
unsigned char SMS_read_io(struct SMS_Core* sms, unsigned char addr);
void sega_mapper_setup(struct SMS_Core* sms);

// SMS System DECLATIONS
bool SMS_is_system_type_gg(const struct SMS_Core* sms);


// CPU DELCATATIONS
void z80_init(struct SMS_Core *sms);
void z80_run(struct SMS_Core *sms);
void z80_nmi(struct SMS_Core* sms);
void z80_irq(struct SMS_Core* sms);
void z80_build_cyclesDB();
void z80_init(struct SMS_Core *sms);


// VDP DECLARATIONS
void vdp_init(struct SMS_Core* sms);
uint8_t vdp_status_flag_read(struct SMS_Core* sms);
void vdp_io_write(struct SMS_Core *sms, uint8_t addr, uint8_t value);
void vdp_run(struct SMS_Core* sms, uint8_t cycles);

uint8_t vdp_status_flag_read(struct SMS_Core *sms);
void vdp_io_write(struct SMS_Core *sms, uint8_t addr, uint8_t value);
bool vdp_has_interrupt(const struct SMS_Core* sms);

// AUP DELCARATIONS
void SN76489_init(struct SMS_Core* sms);
void SN76489_run(struct SMS_Core* sms, uint8_t cycles);
void SN76489_rend(struct SMS_Core *sms, int8_t *tonesL, int8_t *tonesR);
void SN76489_reg_write(struct SMS_Core *sms, uint8_t value);
// extern
void addToAudio(int16_t l, int16_t r);
void playFrameAudio(int16_t *snd, unsigned long SPF);


// [MISC]
SMS_FORCE_INLINE bool SMS_parity(unsigned int value);
bool SMS_parity8(uint8_t value);
bool SMS_parity16(uint16_t value);
// [hacks]
bool SMS_is_spiderman_int_hack_enabled(const struct SMS_Core* sms);

// PERIPHERALS
void SMS_set_port_a(struct SMS_Core *sms, enum SMS_PortA pin);
void SMS_set_port_b(struct SMS_Core *sms, enum SMS_PortB pin);

// PROJECT_SPECIFIC TO graphics-test::
void setSMSPlotter(uint8_t mode);
void SMSPlot(int32_t x, int32_t y, uint8_t c);


// MAIN SMS_EmuProgram
void core_on_vblank();
bool SMS_init(struct SMS_Core* sms);
bool SMS_loadrom(struct SMS_Core* sms, const uint8_t* rom, size_t size);
void SMS_run_frame(struct SMS_Core *sms);
void testRead();


// dialog.cpp
void updateSMSScreen();
void sms_update_screen();

#endif
