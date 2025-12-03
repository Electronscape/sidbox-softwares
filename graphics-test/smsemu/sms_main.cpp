#ifdef __linux__
#include <cstdio>
#include <cstring>

#else
#include <stdio.h>
#include <string.h>
#endif
#include <unistd.h>

#include <stdint.h>


#include "../dialog.h"
#include "sms.h"
#include "types.h"
#include "internal.h"
#include "../hardware/stm32ram.h"

#include <QTimer>


// system RAMS
volatile uint8_t *ROM;
volatile uint8_t *ram[3];// not all values are listed here because the other

volatile uint8_t *system_ram;

volatile uint8_t *vram;

struct SMS_Core sms = {0};


FILE *cartfile;


void VDPUpdate1(){
#ifndef __linux__
    SCB_CleanDCache();
    LCDUpdateDD();
#else
    //TODO: Render Graphics Port only.
    sms_update_screen();
#endif

}
void (*VPDUpdate)();
void core_on_vblank(){
    VPDUpdate();
}


#define smsRAMLocation 	(uint8_t *)duunmap_lm[0]	// 8KB


void sms_build_hardware(struct SMS_Core* sms){
    // we need this to assign locations to the various memory locations, as they'e all pointers now, NOT actual declared sizes

    //uint8_t *system_ram; // 8Kb
    //uint8_t *vram;// 1024 * 16
    //uint8_t *ram[2]; //[1024 * 16];

    // the CPU needs Loading


    system_ram = (uint8_t*)smsRAMLocation; // 8kb
    vram =   (uint8_t*)smsVRAMLocation;  // 16kb
    ROM =    (uint8_t*)(SYSRAM);//(RAM_OFFSETS_0MEG + SDRAM_BANK_ADDR);

    ram[0] = (uint8_t*)smsCARTRam0;	  // 16kb
    ram[1] = (uint8_t*)smsCARTRam1;   // 16kb
    ram[2] = (uint8_t*)smsSysRamBuff; // 8kb

    //volatile uint8_t *vram;
    //volatile uint8_t *system_ram;
    //volatile uint8_t *ROM;
    //volatile uint8_t *ram[2];// not all values are listed here because the other

    //clearTCache();
    //memset(spriteTextures[0], 0x00, 0x8000);

    int l;
    for(l=0; l<0x2000; l++)// clear system ram
        *(system_ram+l) = 0x00;

    for(l=0; l<0x4000; l++)// clear system ram
        *(vram+l) = 0x00;

    for(l=0; l<0x4000; l++) {// clear system ram
        *(ram[0]+l) = 0x00;
        *(ram[1]+l) = 0x00;
    }


    dbug("RAM LOCATIONS ---\n");
    dbug("cart ram 0 = %p\n", (void *)ram[0]);
    dbug("cart ram 1 = %p\n", (void *)ram[1]);
    dbug("sys ram = %p\n", (void *)system_ram);
    dbug("vram = %p\n", (void *)vram);
    dbug("location of textures %p\n", (void *)textures);
    dbug("location of spriteTextures %p\n", (void *)spriteTextures);
    dbug("-----------------------------------------------\n");
}

/*
void testRead(){
    uint32_t uCount;
    ROM =    (uint8_t*)(SYSRAM);//(RAM_OFFSETS_0MEG + SDRAM_BANK_ADDR);
    cartfile = fopen("/home/kbox/test.txt", "rb");
    if(cartfile){
        uCount = std::fread((uint8_t *)ROM, 1, SMS_ROM_SIZE_MAX, cartfile);
        std::fclose(cartfile);

        printf("FROM 'test.txt' = %s\n", ROM);
    } else printf("Didnt open the file\n");
}
*/

static bool read_file(const char *path, uint32_t *out_size) {
    uint32_t uCount;

    std::memset((uint8_t *)ROM, 0x00, SMS_ROM_SIZE_MAX);

    cartfile = fopen(path, "rb");
    if(cartfile){
        uCount = std::fread((uint8_t *)ROM, 1, SMS_ROM_SIZE_MAX, cartfile);
        std::fclose(cartfile);
        *out_size = (uint32_t) uCount;
        return true;
    } else return false;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
void processSMSAudio() {
    SN76489_run(&sms, 82);
    SN76489_rend(&sms, &SMPin1, &SMPin2);

    int16_t s1 = (int16_t) SMPin1 - 0x80;
    int16_t s2 = (int16_t) SMPin2 - 0x80;

    DAC1->DHR12R1 = (uint16_t) (s1 << 4);  // Left

    if (SMS_is_system_type_gg(&sms)) {
        DAC1->DHR12R2 = (uint16_t) (s2 << 4);  // Right (stereo)
    } else {
        DAC1->DHR12R2 = (uint16_t) (s1 << 4);  // Right (echo)
    }
}
*/

char romfilename[256];	// store the filename but will need to add. SAV - SAVE game state buffer name.


extern volatile unsigned char *noxgf_current_buffer;
int main_sms(char *filename)
{
    uint32_t rom_size = 0;
    uint8_t *CROM;

    if (!strncmp(filename, "_bios_", 6)) {
        sprintf(romfilename, "sms_bios.sav");
        //ROM = BIOS_ROM;
        CROM = (uint8_t *)ROM;
        // copy rom into RAM seems to be some how faster :/
        for(rom_size=0; rom_size<(BIOS_ROM_SIZE * 1024); rom_size++){
            *CROM++ = (BIOS_ROM[rom_size]);
        }

        rom_size = (BIOS_ROM_SIZE * 1024);

    } else {
        if (!read_file(filename, &rom_size)) {
            dbug("failed to read file %s\n", filename);
            return -1;
        }
        strcpy(romfilename, filename);
        strcat(romfilename, ".sav");
    }

    z80_build_cyclesDB();	// need this first to build the CYCLES database for RAM
    if (!SMS_init(&sms))
    {
        return -1;
    }

#ifndef __linux__
    WriteAUD=1023;
    ReadAUD=0;

    DMAAudioBufferR = dma_audio_out_r + lBufferOffset;
    DMAAudioBufferL = dma_audio_out_l + lBufferOffset;

    lSoundBufferLenDMA = 512;
#endif
    //open_audio_device();

    //SMS_set_apu_callback(&sms, core_on_apu, NULL, 44100+ 4096);
    //SMS_set_vblank_callback(&sms, core_on_vblank, NULL);
    //SMS_set_pixels(&sms, noxgf_current_buffer, SMS_SCREEN_WIDTH, 1);
    //SMS_set_pixels(&sms, NULL, SMS_SCREEN_WIDTH, 8);

    if (!SMS_loadrom(&sms, (unsigned char *)ROM, rom_size))
    {
        dbug("failed to load rom %s\r\n", filename);
        return -1;
    }

    dbug("File Size : %lu\r\n", (long)rom_size);
    dbug("Rom opened... doing something now though...\r\n");

    long dF;
    dF=0;
    for(long f=0; f<0x80000; f++){
        dF += ROM[f];
    }
    dbug("READ from location 0=%p CRCF_VALUE %lu\r\n", &ROM[0], (unsigned long)dF);
    dbug("[INFO]: Original source from: https://github.com/ITotalJustice/TotalSMS/\n");
    return 0;

}

uint16_t tech=0;

//extern __ATTR_RAM_TC uint8_t CMDLINE[CMDLINE_LEN];

//uint8_t cmdTmr=0;
//uint8_t cmdMd=0;

extern char audioMode1;

extern unsigned char bSEGAConfigs;
extern float bSEGAPWMer;
extern void (*audioIRQ)();


uint8_t textRes[128];
void BEGIN_SMSEMU(char *filename){
    uint32_t cyclePisser=0;

    //audioIRQ = &processSMSAudio;

    //bClearInit=6;
    VPDUpdate = &VDPUpdate1;

    // TODO: Clear the screen here

    //clearPlayground();
    //clearBackground(0);

    //setColourPaletteHEX(200,0);
    //LCDUpdateDD();

    sms_build_hardware(&sms);

    main_sms(filename);

    /*
    if(SMS_set_system_type(sms, SMS_System_GG)){
        for(char i=0; i<200; i++){
            LCDSetBuffer(0);
            LCDClearBuffer(200);
            LCDUpdateDD();

            LCDSetBuffer(1);
            LCDClearBuffer(200);
            LCDUpdateDD();
        }
    }
    */
    sms.apu.better_sid = (!!(bSEGAConfigs & SEGAHV_CONFIG_TONEMODE));



    //for(;;)
    {
        /*
        if(JOYSTICK_A_INPUT_DOWN && JOYSTICK_A_INPUT_FIRE && JOYSTICK_A_INPUT_FIRE2){
            SMS_set_port_b(&sms, PAUSE_BUTTON, 1);
        }

        if(JOYSTICK_A_INPUT_UP && JOYSTICK_A_INPUT_FIRE && JOYSTICK_A_INPUT_FIRE2){
            SMS_set_port_b(&sms, RESET_BUTTON, 1);
        }
        */



    };
}

static int8_t SMPRes, SMPin1, SMPin2;
#define VDPFPS  60
#define AUDIOSAMPLERATE 44100

#define SAMPLES_PER_FRAME (AUDIOSAMPLERATE / VDPFPS)
int16_t frameAudBuffer[SAMPLES_PER_FRAME * 2];// stereo output
#define AMP 120

void doSMSFrames(){
    SMS_run_frame(&sms);


    for(int t=0; t<SAMPLES_PER_FRAME; t++){


        SN76489_run(&sms, 82);
        SN76489_rend(&sms, &SMPin1, &SMPin2);

        int16_t s1 = (int16_t) SMPin1 - 0x80;
        int16_t s2 = (int16_t) SMPin2 - 0x80;

        //DAC1->DHR12R1 = (uint16_t) (s1 << 4);  // Left

        if (SMS_is_system_type_gg(&sms)) {
            //DAC1->DHR12R2 = (uint16_t) (s2 << 4);  // Right (stereo)
        } else {
            //DAC1->DHR12R2 = (uint16_t) (s1 << 4);  // Right (echo)
        }


        //addToAudio(s1 * 120, s2 * 120);
        frameAudBuffer[t * 2 + 0] = s1 * AMP;
        frameAudBuffer[t * 2 + 1] = s2 * AMP;
    }


    playFrameAudio(frameAudBuffer, (unsigned long)SAMPLES_PER_FRAME);






    //processAudio();
    //sms_update_screen();
}

enum { STATE_MAGIC = 0x5E6A };
enum { STATE_VERSION = 2 };
