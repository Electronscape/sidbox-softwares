#ifndef PLAYERTF_H
#define PLAYERTF_H

#include <stdint.h>


#define PATHNAME_LENGTH     256


#define     BUF_SIZE        4096
#define     HALFBUFSIZE     (  (BUF_SIZE * 4) / 4)
#define     BUFSIZE         (( (BUF_SIZE * 2) * 4) / 4)
extern int32_t  tbuf[];


typedef struct Hdb Hdb;
typedef struct Cdb Cdb;


typedef union {
    uint32_t l;
    struct {uint16_t w1,w0;}          w;
    struct {uint8_t  b3,b2,b1,b0;}    b;
} UNI;

typedef struct {
    char        magic[10];
    char        pad[6];
    char        text[6][40];
    uint16_t    start[32], end[32], tempo[32];
    int16_t     mute[8];
    uint32_t    trackstart, pattstart, macrostart;
    char        pad2[36];
} Hdr;


typedef struct Hdb {
    uint32_t    pos;
    uint32_t    delta;
    uint16_t    slen, SampleLength;
    int8_t      *sbeg, *SampleStart;
    uint8_t     vol;
    uint8_t     mode;
    int32_t     (*loop)();
    int32_t     loopcnt;
    Cdb         *c;
} Hdb;

typedef struct  {
    uint16_t    Cue[4];
} Idb ;

typedef struct {
    char        PlayerEnable;
    char        EndFlag;
    char        CurrSong;
    uint16_t    SpeedCnt;
    uint16_t    CIASave;
    char        SongCont, SongNum;
    uint16_t    PlayPattFlag;
    char        MasterVol, FadeDest, FadeTime, FadeReset, FadeSlope;
    int16_t     TrackLoop;
    uint16_t    DMAon, DMAoff, DMAstate, DMAAllow;
} Mdb;

typedef struct {
    uint32_t    PAddr;
    uint8_t     PNum;
    int8_t      PXpose;
    uint16_t    PLoop, PStep;
    uint8_t     PWait;
    uint16_t    PRoAddr, PRoStep;
} Pdb;

typedef struct {
    uint16_t    FirstPos, LastPos, CurrPos;
    uint16_t    Prescale;
    Pdb         p[8];
} Pdblk;

typedef struct Cdb {
    int8_t      MacroRun, EfxRun;
    uint8_t     NewStyleMacro;
    uint8_t     PrevNote, CurrNote;
    uint8_t     Velocity, Finetune;
    uint8_t     KeyUp, ReallyWait;
    uint32_t    MacroPtr;
    uint16_t    MacroStep, MacroWait, MacroNum;
    int16_t     Loop;

    uint32_t    CurAddr, SaveAddr;
    uint16_t    CurrLength,SaveLen;

    uint16_t    WaitDMACount;
    uint16_t    DMABit;

    uint8_t     EnvReset, EnvTime, EnvRate;
    int8_t      EnvEndvol, CurVol;

    int16_t     VibOffset;
    int8_t      VibWidth;
    uint8_t     VibFlag, VibReset, VibTime;

    uint8_t     PortaReset, PortaTime;
    uint16_t    CurPeriod, DestPeriod, PortaPer;
    int16_t     PortaRate;

    uint8_t     AddBeginTime, AddBeginReset;
    uint16_t    ReturnPtr, ReturnStep;
    int32_t     AddBegin;

    uint8_t     SfxFlag, SfxPriority, SfxNum;
    int16_t     SfxLockTime;
    uint32_t    SfxCode;

    Hdb     *hw;
} Cdb;


typedef struct {

    // song data parts ----
    Hdr     hdr;
    Hdb     hdb[8];
    Cdb     cdb[16];
    Mdb     mdb;
    Pdblk   pdb;
    Idb     idb;


    //// -- globals -- ////
    uint8_t kim;

    // player
    uint32_t    eClocks;        // = 14318;
    //uint32_t    editbuf[16384]; // a buffer!
    uint32_t    *editbuf;
    char        act[8];         // = {1,1,1,1,1,1,1,1};

    // player
    int         over;
    int32_t     loops;
    int         songnum;
    int32_t     *patterns, *macros;

    int32_t     startPat,
                gemx,
                dangerFreakHack,
                oopsUpHack,
                monkeyHack;

    // audio manager
    uint8_t      *smplbuf;

    uint32_t    outRate;
    int32_t     multimode;
    int32_t     jiffies;
    //U32     stereo;

    volatile int bhead, btail;
    //int32_t bytes, bytes2;


} tfx_stream_t;

extern tfx_stream_t g_tfmx;


// TFMX expects these globals (they already exist in original codebase).
// Make sure they are defined EXACTLY ONCE across your build.
//extern char act[8];
#define TFMX_hdr        (g_tfmx.hdr)  // Hdr    hdr;
#define TFMX_hdb        (g_tfmx.hdb)  // Hdb    hdb[8];
#define TFMX_cdb        (g_tfmx.cdb)  // Cdb    cdb[16];
#define TFMX_mdb        (g_tfmx.mdb)  // Mdb    mdb;
#define TFMX_pdb        (g_tfmx.pdb)  // Pdblk  pdb;
#define TFMX_idb        (g_tfmx.idb)  // Idb    idb;

#define TFMX_smplbuf    (g_tfmx.smplbuf)    //extern U8   *smplbuf;
#define TFMX_patterns   (g_tfmx.patterns)   //extern int  *patterns;
#define TFMX_macros     (g_tfmx.macros)     //extern int  *macros;


void    NotePort(uint32_t i);
int     LoopOff(void /* struct Hdb *hw */);
int     LoopOn(Hdb *hw);
void    RunMacro(Cdb *c, uint32_t nChannel);
void    DoEffects(Cdb *c);
void    DoMacro(int cc);
void    DoAllMacros(void);
void    ChannelOff(int i);
void    DoFade(int sp,int dv);
void    GetTrackStep(void);
int     DoTrack(Pdb *p/* ,int pp  */);
void    DoTracks(void);
void    tfmxIrqIn(void);
void    AllOff(void);
void    TfmxInit(void);
void    StartSong(int song, int mode);

void    TfmxInit(void);
void    StartSong(int song, int mode);
void    tfmxIrqIn(void);
void    mixem(uint32_t nb, uint32_t bd);

int     load_tfmx(char *mfn, char *sfn);







void    mix_add_ov(Hdb *hw,int n,int32_t *b);
void    mix_add(Hdb *hw, int n, int32_t *b);
void    mixit(int n,int b);
void    mixem(uint32_t nb, uint32_t bd);
int     play_it(void);
void    tfmxIrqIn(void);

void    TFMX_SystemInit();






#endif
