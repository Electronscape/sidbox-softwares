#include "playsid.h"
#include "bus.h"
#include "cpu6502.h"
#include "vic.h"
#include "sid8579.h"
#include "cia.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static word load_addr, init_addr, play_addr;
static byte subsongs, startsong, speed;
static long ticks = 0;
static unsigned long nRefreshCIA = 20000;
static int mixing_frequency = AUDIO_MIX_FREQ;

extern uint8_t bSidPlay2SIDmode;
uint16_t g_sid2_base = 0;   // 0 = no SID2

// Whether user-supplied ROMs were loaded (BASIC/KERNAL/CHARGEN)
static int rsid_have_roms = 0;

////////////////////// RSID //////////////////////////////////////////////////
// playsid.c (RSID-only helper)

#define RSID_TRAMP_ADDR  0xF000u

// Installs an IRQ handler at RSID_TRAMP_ADDR that:
// - reads $0314/$0315
// - self-modifies a JSR target
// - calls it
// - ACKs VIC $D019
// - restores regs and RTI
static void rsid_install_vic_irq_trampoline(void){
    uint16_t a = RSID_TRAMP_ADDR;

    // 6502 bytes, assembled for address RSID_TRAMP_ADDR, no branches needed.
    // IRQ handler:
    //   PHA
    //   TXA / PHA
    //   TYA / PHA
    //   LDA $0314 ; STA jsr+1
    //   LDA $0315 ; STA jsr+2
    // jsr: JSR $FFFF
    //   LDA #$01  ; STA $D019   (ACK raster IRQ)
    //   PLA / TAY
    //   PLA / TAX
    //   PLA
    //   RTI
    static const uint8_t tramp[] = {
        0x48,                   // PHA  (save A)

        // tick counter in $02
        0xE6, 0x02,             // INC $02
        0xA5, 0x02,             // LDA $02
        0xC9, 0x32,             // CMP #$32 (50)
        0xD0, 0x0C,             // BNE notyet

        0xA9, 0x00,             // LDA #$00
        0x85, 0x02,             // STA $02

        0xA9, '\0',              // LDA #'.'
        0x8D, 0xF0, 0xD7,       // STA $D7F0
        0xA9, '\0',             // LDA #'\n'
        0x8D, 0xF0, 0xD7,       // STA $D7F0

        // notyet:
        0xA9, 0x01,             // LDA #$01
        0x8D, 0x19, 0xD0,       // STA $D019   (ACK raster IRQ)

        0x68,                   // PLA (restore A)

        // if ($0314|$0315)==0 => RTI
        0xAD, 0x14, 0x03,       // LDA $0314
        0x0D, 0x15, 0x03,       // ORA $0315
        0xF0, 0x03,             // BEQ do_rti

        0x6C, 0x14, 0x03,       // JMP ($0314)  (chain to real handler; it will RTI)

        // do_rti:
        0x40                    // RTI
    };

    for (size_t i = 0; i < sizeof(tramp); i++){
        bus_write8(a++, tramp[i]);
    }


    // Reset counter
    bus_write8(0x0002, 0x00);


    // Set IRQ/BRK vector to our trampoline
    bus_write16(0xFFFE, RSID_TRAMP_ADDR);

    // Optional: also mirror the KERNAL IRQ vector area so code that reads it sees something sane
    //bus_write16(0x0314, RSID_TRAMP_ADDR); // not required, but harmless in this ROMless sandbox
}

// Enable one raster IRQ per frame (≈50Hz PAL-ish in your VIC model).
static void rsid_enable_vic_50hz_raster_irq(uint8_t raster_line){
    bus_write8(0xD012, raster_line);    // set compare line
    uint8_t d011 = bus_read8(0xD011);   // clear high-bit latch in $D011 compare (bit7)
    d011 &= 0x7F;
    bus_write8(0xD011, d011);
    bus_write8(0xD019, 0x01);           // clear pending raster IRQ
    bus_write8(0xD01A, 0x01);           // enable raster IRQ
    bus_write8(0xD019, 0x01);           // clear again right before we let CPU run (reduces "start burst")
}


#pragma pack(push,1)
typedef struct {
    char     magic[4];      // "PSID" or "RSID"
    uint16_t version;       // big-endian
    uint16_t dataOffset;    // big-endian
    uint16_t loadAddress;   // big-endian (0 = read from first 2 data bytes)
    uint16_t initAddress;   // big-endian (RSID often 0)
    uint16_t playAddress;   // big-endian (RSID often 0)
    uint16_t songs;         // big-endian
    uint16_t startSong;     // big-endian
    uint32_t speed;         // big-endian bitfield // 19-22
    char     name[32];      // 22
    char     author[32];    // 54
    char     released[32];  // 86
    // v2+ has more fields, but we can ignore for “load & pray”
} SIDHeader;
#pragma pack(pop)

static inline uint8_t clamp_u8(int v, int lo, int hi){
    if (v < lo) return (uint8_t)lo;
    if (v > hi) return (uint8_t)hi;
    return (uint8_t)v;
}

static uint16_t be16(uint16_t v){ return (uint16_t)((v >> 8) | (v << 8)); }
static uint32_t be32(uint32_t v){ return ((v>>24)&0x000000FF) | ((v>>8)&0x0000FF00) | ((v<<8)&0x00FF0000) | ((v<<24)&0xFF000000); }

static inline uint16_t rd_u16be(const uint8_t *p){ return (uint16_t)(p[0] << 8) | p[1]; }


static int LoadSIDFromFile(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) return 0;

    uint8_t hdr[124];
    if(fread(hdr, 1, 124, f) != 124){
        fclose(f);
        return 0;
    }

    // PSID header:
    // 0..3 "PSID"/"RSID"
    // 6..7 dataOffset (big endian)
    // 8..9 loadAddr
    // 10..11 initAddr
    // 12..13 playAddr
    // 14..15 songs
    // 16..17 startSong
    // 18..21 speed (v2 uses flags; we keep your 0x15 usage style loosely)
    uint16_t dataOffset = rd_u16be(&hdr[6]);
    load_addr  = rd_u16be(&hdr[8]);
    init_addr  = rd_u16be(&hdr[10]);
    play_addr  = rd_u16be(&hdr[12]);
    subsongs   = (byte)(rd_u16be(&hdr[14]) - 1);
    startsong  = (byte)(rd_u16be(&hdr[16]) - 1);

    // your old code used pData[0x15] (which is in first 128 bytes region)
    // Here we approximate speed using the first byte of the speed word.
    speed = hdr[0x15];

    // if load_addr==0 -> first two bytes of data are load address (little endian)
    // seek to dataOffset
    fseek(f, dataOffset, SEEK_SET);

    if(load_addr == 0){
        uint8_t lo = (uint8_t)fgetc(f);
        uint8_t hi = (uint8_t)fgetc(f);
        load_addr = (word)(lo | (hi << 8));
    }

    // load the rest into C64 RAM
    uint32_t addr = load_addr;
    int c;
    while((c = fgetc(f)) != EOF){
        bus_write8((uint16_t)addr, (uint8_t)c);
        addr = (addr + 1) & 0xFFFF;
    }

    fclose(f);

    // if play_addr == 0, call init and read IRQ vector ($0314/5) like your original
    if(play_addr == 0){
        cpu_call_jsr_resetting(init_addr, 0);
        play_addr = (word)((bus_read8(0x0315) << 8) | bus_read8(0x0314));
    }
    return 1;
}


static void c64Init(void){
    clear64KRam();
    restartSidChipModes();
    synth_init((uint32_t)mixing_frequency);
    cpuReset();
    vic_reset();
    cia_reset_all();
    // volume poke like your code (both chips)
    bus_write8(0xD418, 15);
    bus_write8(0xD438, 15);
}


int CheckSIDType(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f) return 0;
    uint8_t hdr[124];
    memset(hdr, 0, sizeof(hdr));
    // Need at least 4 bytes for magic
    if(fread(hdr, 1, 4, f) != 4){
        fclose(f);
        return 0;
    }
    bSidPlay2SIDmode = 0;
    g_sid2_base = 0;
    // --- PSID ---
    if(hdr[0]=='P' && hdr[1]=='S' && hdr[2]=='I' && hdr[3]=='D'){
        // Read rest of header so we can see version + sid2 fields
        if(fread(hdr + 4, 1, 124 - 4, f) != (124 - 4)){
            fclose(f);
            return 0;
        }
        uint16_t ver = rd_u16be(&hdr[4]);   // PSID version (big-endian)
        // Only PSID v3+ has secondSIDAddress (offset 0x7A)
        if(ver >= 3){
            uint8_t sid2 = hdr[0x7A];       // secondSIDAddress (encoded as $Dxx0 => xx=sid2)
            // Valid values are even; 0 means "no SID2"
            if(sid2 != 0 && ((sid2 & 1) == 0)){
                uint16_t base = (uint16_t)(0xD000u | ((uint16_t)sid2 << 4)); // $Dxx0
                // Keep it sane: only accept bases where 2SID commonly lives
                if(base >= 0xD420 && base <= 0xDFE0){
                    g_sid2_base = base;
                    bSidPlay2SIDmode = 1;
                }
            }
        }
        fclose(f);
        return SIDPLAY_PLAYMODE_PSID;
    }

    // --- RSID ---
    if(hdr[0]=='R' && hdr[1]=='S' && hdr[2]=='I' && hdr[3]=='D'){
        if(fread(hdr + 4, 1, 124 - 4, f) != (124 - 4)){
            fclose(f);
            return 0;
        }

        uint16_t ver = rd_u16be(&hdr[4]);   // RSID version (big-endian)

        // RSID v2+ also has secondSIDAddress at 0x7A (same header layout)
        if(ver >= 2){
            uint8_t sid2 = hdr[0x7A];
            if(sid2 != 0 && ((sid2 & 1) == 0)){
                uint16_t base = (uint16_t)(0xD000u | ((uint16_t)sid2 << 4)); // $Dxx0
                if(base >= 0xD420 && base <= 0xDFE0){
                    g_sid2_base = base;
                    bSidPlay2SIDmode = 1;
                }
            }
        }

        fclose(f);
        return SIDPLAY_PLAYMODE_RSID;
    }

    fclose(f);
    return 0;
}


static int LoadRSIDHeaderOnly(const char *filename, uint16_t *dataOffsetOut){
    FILE *f = fopen(filename, "rb");
    if(!f) return 0;

    uint8_t hdr[124];
    if(fread(hdr, 1, 124, f) != 124){ fclose(f); return 0; }

    if(!(hdr[0]=='R' && hdr[1]=='S' && hdr[2]=='I' && hdr[3]=='D')){
        fclose(f);
        return 0;
    }

    *dataOffsetOut = rd_u16be(&hdr[6]);
    load_addr  = rd_u16be(&hdr[8]);
    init_addr  = rd_u16be(&hdr[10]);
    play_addr  = rd_u16be(&hdr[12]);
    subsongs   = (byte)(rd_u16be(&hdr[14]));
    startsong  = (byte)(rd_u16be(&hdr[16]));
    speed      = hdr[0x15];

    fclose(f);
    return 1;
}

static int LoadRSIDDataOnly(const char *filename, uint16_t dataOffset){
    FILE *f = fopen(filename, "rb");
    if(!f) return 0;

    // seek to data
    fseek(f, dataOffset, SEEK_SET);

    // load address may be 0 => first 2 bytes of data
    if(load_addr == 0){
        uint8_t lo = (uint8_t)fgetc(f);
        uint8_t hi = (uint8_t)fgetc(f);
        load_addr = (word)(lo | (hi << 8));
    }

    uint32_t addr = load_addr;
    int c;
    while((c = fgetc(f)) != EOF){
        bus_write8((uint16_t)addr, (uint8_t)c);
        addr = (addr + 1) & 0xFFFF;
    }

    fclose(f);

    // minimal I/O config (keep yours)
    bus_write8(0x0000, 0x2F);
    bus_write8(0x0001, 0x37);

    synth_prep_per_step();
    return 1;
}



// songs_count = header.songs (NOT minus 1)
// startSong_1based = header.startSong (1..songs)
static inline uint8_t rsid_pick_song0(int startSong_1based, int songs_count, int user_song0){
    int max0 = (songs_count > 0) ? (songs_count - 1) : 0;
    int s0;
    if (user_song0 >= 0) {
        s0 = user_song0;
    } else {
        // header is 1-based, convert to 0-based
        s0 = (startSong_1based > 0) ? (startSong_1based - 1) : 0;
    }
    return clamp_u8(s0, 0, max0);
}

static inline void c64_tick_1cycle(void);
static void c64_run_cycles(uint32_t cycles){
    for (uint32_t i = 0; i < cycles; i++) {
        c64_tick_1cycle();   // your working 1-cycle master tick
    }
}

static void rsid_boot_kernel_window(void){
    // After cpuReset(), let KERNAL do its init work.
    // Roughly: run ~ 1–3 frames worth of CPU cycles.
    // PAL: ~985248 cycles/sec, ~19656 cycles/frame at 50Hz.
    c64_run_cycles(19656 * 2);  // ~2 frames
}



int PlaySID_InitRSID(const char *filename, uint8_t subsong){
    clear64KRam();
    restartSidChipModes();
    synth_init((uint32_t)mixing_frequency);
    vic_reset();
    cia_reset_all();

    rsid_have_roms = bus_load_roms("../../basic.rom", "../../kernal.rom", "../../chargen.rom");

    uint16_t dataOffset = 0;
    if(!LoadRSIDHeaderOnly(filename, &dataOffset)){
        fprintf(stderr, "PlaySID_InitRSID: failed reading header %s\n", filename);
        return 0;
    }

    if (!rsid_have_roms) {
        rsid_enable_vic_50hz_raster_irq(50);
        rsid_install_vic_irq_trampoline();
        // In ROM-less mode you probably want to load immediately (no KERNAL anyway)
        if(!LoadRSIDDataOnly(filename, dataOffset)) return 0;
        cpu_force_cli();
        return 1;
    }

    // ROMs present: do the KERNAL reset/init first
    cpuReset();
    rsid_boot_kernel_window();

    // STOP interrupts while we load/patch/start the tune
    cpu_force_sei();

    // Clear pending IRQ latches ONLY (do NOT disable enables/masks)
    bus_write8(0xD019, 0x0F);               // VIC: clear pending sources
    (void)cia_read(CIA_CHIP_1, CIA_ICR);    // CIA: clear pending latch+flags
    (void)cia_read(CIA_CHIP_2, CIA_ICR);

    // NOW load the RSID program after KERNAL has done its scribbling
    if(!LoadRSIDDataOnly(filename, dataOffset)){
        fprintf(stderr, "PlaySID_InitRSID: failed loading data %s\n", filename);
        return 0;
    }

    // Your $0110 trampoline (CLI; JMP $0110)
    bus_write8(0x0110, 0x58);
    bus_write8(0x0111, 0x4C);
    bus_write8(0x0112, 0x10);
    bus_write8(0x0113, 0x01);

    // Return-to-$0110 setup
    cpu_set_sp(0xFF);
    cpu_push_byte(0x01);
    cpu_push_byte(0x0F);

    // pick song (0-based)
    const int user_song0 = subsong;
    const uint8_t song0 = rsid_pick_song0(startsong, subsongs, user_song0);

    cpu_set_a(song0);
    cpu_set_x(0);
    cpu_set_y(0);

    // IMPORTANT: re-enable interrupts before starting init in case init never RTS's
    //cpu_force_cli();

    // jump into init/load address
    const uint16_t start_addr = init_addr ? init_addr : load_addr;
    cpuSetPC(start_addr);

    return 1;
}


















int PlaySID_Init(const char *filename, int subsong){
    c64Init();
    if(!LoadSIDFromFile(filename)){
        fprintf(stderr, "PlaySID_Init: failed loading %s\n", filename);
        return 0;
    }
    if(subsong < 0) subsong = startsong;
    ticks = 0;
    cpu_call_jsr_resetting(init_addr, (byte)subsong);
    synth_prep_per_step(); // prime osc/filter based on regs after init
    return 1;
}

////////////////// P_SID ROUTER /////////////////////////////////////////////////////
static inline void refresh_ticks_from_cia(void){
    int gm1 = bus_read8(0xDC04);
    int gm2 = bus_read8(0xDC05);
    unsigned long cia = (unsigned long)(gm1 | (256L * gm2));
    nRefreshCIA = (unsigned long)(20000UL * cia / 0x4C00);

    if(nRefreshCIA == 0 || speed == 0) nRefreshCIA = 20000UL;
    ticks = (long)((mixing_frequency * nRefreshCIA) / 1000000UL);
    if(ticks <= 0) ticks = (mixing_frequency / 50);
}

static inline void call_psid_play(void){
    // Most tunes expect this each call:
    cpu_set_regs(0, 0, 0);
    cpu_call_jsr(play_addr);
}

uint32_t doPlaySidStep(int16_t *out_interleaved, uint32_t frames){
    if(!out_interleaved || frames == 0) return 0;
    for(uint32_t i=0; i<frames; i++){
        if(ticks <= 0){
            // simple PSID player
            call_psid_play();
            refresh_ticks_from_cia();
            synth_prep_per_step();
        }
        int16_t L, R;
        sid_render_sample(&L, &R);

        out_interleaved[i*2 + 0] = L;
        out_interleaved[i*2 + 1] = R;
        ticks--;
    }
    return frames;
}





static uint8_t g_pending_nmi = 0;
static uint8_t g_pending_irq = 0;

static int      g_cpu_debt = 0;        // cycles remaining in current instruction
static uint8_t  g_prev_nmi_pin = 1;    // edge detect for CIA2->NMI

static inline void c64_tick_1cycle(void)
{
    // 1) always advance hardware time
    vic_step(1);
    cia_step_all(1);
    //sid_clock_cycles(1);

    // CALL ONCE (important: vic_cpu_can_run_this_cycle() consumes stall)
    const int cpu_can_run = vic_cpu_can_run_this_cycle();

    // 2) CPU only runs if VIC allows it
    if (g_cpu_debt <= 0) {
        if (cpu_can_run) {
            int inst = cpuStep();          // returns cycles cost
            if (inst <= 0) inst = 1;
            g_cpu_debt = inst - 1;         // we already spent 1 cycle right now
        } else {
            // VIC stole this cycle, CPU does nothing
            g_cpu_debt = 0;
        }
    } else {
        // mid-instruction: still only burn debt if VIC allows CPU to progress
        if (cpu_can_run) {
            g_cpu_debt--;
        }
    }

    // 3) interrupt sampling (same as before)
    uint8_t cia2_asserted = (CIA2_IRQ_LINE != 0);
    uint8_t nmi_pin = cia2_asserted ? 0 : 1;

    if (g_prev_nmi_pin == 1 && nmi_pin == 0) {
        cpu_nmi();

        // PAY the interrupt entry time instead of "free" entry
        // (7 cycles total; we are already inside 1 master tick -> 6 left)
        g_cpu_debt = 7 - 1;
        g_prev_nmi_pin = nmi_pin;
        return;
    }
    g_prev_nmi_pin = nmi_pin;

    if (!getIFlagStatus()) {
        if (VIC_IRQ_LINE || CIA1_IRQ_LINE) {
            cpu_irq();
            g_cpu_debt = 7 - 1;
            return;
        }
    }
}






uint32_t doRSIDStep(int16_t *out_interleaved, uint32_t frames, uint32_t sample_rate) {
    if(!out_interleaved || frames == 0 || sample_rate == 0) return 0;

    const double cycles_per_sample = (double)C64_CPU_HZ_PAL / (double)sample_rate;
    static double cyc_accumulator = 0;
    static uint8_t prev_cia2_nmi = 0;
    static uint32_t sampletimer = 0;

    for(uint32_t i = 0; i < frames; i++) {
        cyc_accumulator += cycles_per_sample;

        // We run until we have exhausted the cycles allocated for THIS sample


        while (cyc_accumulator >= 1.0) {
            c64_tick_1cycle();
            cyc_accumulator -= 1.0;
        }

        // render: DO NOT advance SID here
        int16_t L, R;
        synth_prep_per_step(); // [PLEASE DO NOT REMOVE THIS!!! EVER!! ] now it sees the SID writes that just happened
        //sid_render_sample_noadvance(&L, &R);
        sid_render_sample(&L, &R);  // im using this as internally the timers are on tick perfectly

        out_interleaved[i*2 + 0] = L;
        out_interleaved[i*2 + 1] = R;
    }

    return frames;
}

void playsid_switch_subsong_rsid_soft(uint8_t song0)
{
    // If your init_addr is 0 in RSID header, you’re usually not meant to do this.
    //if (!init_addr) return 0;

    // 1) mask CPU IRQs
    cpu_force_sei();

    // 2) clear VIC pending IRQ flags (don’t necessarily change mask)
    bus_write8(0xD019, 0x0F);

    // 3) clear CIA pending latches + stop timers
    cia_write(CIA_CHIP_1, 0x0D /*ICR*/, 0x7F); (void)cia_read(CIA_CHIP_1, 0x0D);
    cia_write(CIA_CHIP_2, 0x0D /*ICR*/, 0x7F); (void)cia_read(CIA_CHIP_2, 0x0D);

    cia_write(CIA_CHIP_1, 0x0E /*CRA*/, 0x00);
    cia_write(CIA_CHIP_1, 0x0F /*CRB*/, 0x00);
    cia_write(CIA_CHIP_2, 0x0E /*CRA*/, 0x00);
    cia_write(CIA_CHIP_2, 0x0F /*CRB*/, 0x00);

    // 4) Silence SID1 regs quickly (key ones)
    for (uint16_t r = 0; r <= 0x18; r++) bus_write8(0xD400 + r, 0x00);
    bus_write8(0xD418, 0x00);

    // If you have 2SID active at D420 (your current setup):
    if (bSidPlay2SIDmode) {
        for (uint16_t r = 0; r <= 0x18; r++) bus_write8(0xD420 + r, 0x00);
        bus_write8(0xD438, 0x00);
    }

    // Call init like a “driver”: A = subsong, X/Y often 0
    cpu_set_regs(song0, 0, 0);
    //cpu_call_jsr_resetting(init_addr, 0);
    cpu_call_jsr(init_addr);// if that sets PC correctly

    // Let tune re-enable interrupts if it wants; otherwise you can
    // enable here (depends on your environment):
    cpu_force_cli();

    // Prime synth after writes
    synth_prep_per_step();


}
