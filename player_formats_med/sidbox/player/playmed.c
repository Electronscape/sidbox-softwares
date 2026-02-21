// playmed.c - minimal MMD0/MMD1 MED player core (no external libs)

#include "playmed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define MED_FILE_MAX (1u * 1024u * 1024u)   // adjust as needed
#define MED_SAMPLE_RAM_MAX (1u * 1024u * 1024u)   // decode pool for packed samples



static uint8_t  g_med_filebuf[MED_FILE_MAX];


static int8_t   g_med_sampram[MED_SAMPLE_RAM_MAX];


static uint32_t g_med_filebuf_len;
static uint32_t g_med_sampram_used;

static uint8_t g_dbg_needMixHit = 0;

static uint16_t g_rowPlaying = 0;

// When pattern delay repeats a row, do NOT run process_row() again on tick0.
static uint8_t  g_skipRowProcess = 0;

// After finishing a row (tick wrap), should we advance normally?
static uint8_t  g_rowAdvancePending = 0;




// sample stepping FP (keep)
#define FP_SHIFT          14
#define FP_ONE            (1u << FP_SHIFT)

// tick accumulator FP (smaller to keep 32-bit math safe)
#define TICK_FP_SHIFT     14
#define TICK_FP_ONE       (1u << TICK_FP_SHIFT)







// PAL / NTSC clocks
#define AMIGA_CLOCK_PAL    7093790u
#define AMIGA_ECLOCK_PAL    709378u

#define MED_MAX_SAMPLES   63
#define MED_MAX_TRACKS    64
#define MED_MAX_ROWS      256


typedef struct {
    uint8_t note;   // 0 = empty, else 1..?
    uint8_t inst;   // 0 = empty, else 1..63
    uint8_t cmd;
    uint8_t param;
} MedEvent;

typedef struct {
    int8_t  *data;          // signed 8-bit mono
    uint32_t length;        // bytes
    uint32_t loopStart;     // bytes
    uint32_t loopLen;       // bytes
    uint8_t  volume;        // 0..64
    int8_t   transpose;     // semitones
    int8_t   finetune;      // -8..+7 (ProTracker-style 1/8 semitone)
    uint8_t  hold;          // OctaMED hold
    uint8_t  decay;         // OctaMED decay
    uint8_t  valid;
} MedSample;

typedef struct {
    uint8_t  active;
    uint8_t  inst;          // 0..62 (sample index)
    uint8_t  vol;           // 0..64

    uint16_t period;        // current period
    uint16_t targetPeriod;  // tone porta target
    uint16_t portaSpeed;    // tone porta speed

    uint8_t  vibSpeed;
    uint8_t  vibDepth;
    uint8_t  vibPos;

    uint8_t  arpX, arpY;

    uint8_t  noteDelay;     // EDx
    uint8_t  noteCut;       // ECx

    uint8_t  envHold;      // ticks remaining to hold
    uint8_t  envDecay;     // decay speed/rate copied from instrument
    uint8_t  envActive;    // 1 if envelope running

    uint8_t  lastInst1;     // last instrument in 1..63
    uint8_t  lastNote;      // last note value

    uint32_t stepFP;
    uint32_t posFP;

    uint8_t  loopCount;     // E6x
    uint16_t loopRow;       // E60

    // ---- MED/PT state ----
    uint8_t  pan;           // 0..255 (0=left, 255=right)

    // command memory (param==0 reuse)
    uint8_t  mem_1;         // porta up
    uint8_t  mem_2;         // porta down
    uint8_t  mem_3;         // tone porta speed
    uint8_t  mem_4;         // vibrato (speed/depth packed: hi=speed lo=depth)
    uint8_t  mem_A;         // volslide
    uint8_t  mem_1A;
    uint8_t  mem_D;         // D command (either volslide or patternbreak, heuristic)
} MedChan;


typedef struct {
    uint16_t rep;
    uint16_t replen;
    uint8_t  midich;
    uint8_t  midipreset;
    uint8_t  svol;
    int8_t   strans;
} SampleHDR;

typedef struct {
    uint8_t  *file;
    uint32_t  fileLen;
    char      lastErr[256];

    uint8_t   version;          // 0 or 1
    uint32_t  songOffset;
    uint32_t  blockArrOffset;
    uint32_t  sampleArrOffset;
    uint32_t  expDataOffset;

    // song header fields
    uint16_t  numBlocks;
    uint16_t  songLen;
    uint8_t   playSeq[256];

    uint16_t  defTempo;
    int8_t    playTransp;
    uint8_t   flags;
    uint8_t   flags2;
    uint8_t   tempo2;           // ticks-per-line (speed)
    uint8_t   trkVol[16];
    uint8_t   masterVol;
    uint8_t   numSamples;

    SampleHDR smphdr[MED_MAX_SAMPLES];


    MedSample samples[MED_MAX_SAMPLES];

    // derived
    uint8_t  rowsPerBeat;       // LPB (flags2 low bits + 1)
    uint8_t  ticksPerLine;      // speed

    // playback
    uint32_t outRate;

    uint16_t curOrder;
    uint16_t curRow;
    uint8_t  curTick;

    uint32_t samplesPerTickFP;
    uint32_t tickAccFP;

    uint32_t paulaClock;
    uint32_t eclock;

    uint8_t  patDelayCnt;

    // ---- pending flow control (apply at end of row) ----
    uint8_t  flowPending;
    uint16_t flowOrder;
    uint16_t flowRow;

    // pattern cache
    uint8_t  patLoaded;
    uint16_t patIndex;
    uint16_t patRows;
    uint16_t patTracks;
    uint8_t  numTracks;
    MedEvent pat[MED_MAX_ROWS][MED_MAX_TRACKS];

    MedChan  ch[MED_MAX_TRACKS];
} MedState;

static MedState g_med;


static uint16_t rd_be16(const uint8_t *p){
    return (uint16_t)((p[0] << 8) | p[1]);
}
static uint32_t rd_be32(const uint8_t *p){
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}



// ---------------- basic utils ----------------
static void med_err(const char *msg){
    snprintf(g_med.lastErr, sizeof(g_med.lastErr), "%s", msg ? msg : "error");
}
const char *playMED_LastError(void){
    return g_med.lastErr[0] ? g_med.lastErr : "";
}


static int file_can(uint32_t off, uint32_t need){
    return (g_med.file && off <= g_med.fileLen && need <= g_med.fileLen && (off + need) <= g_med.fileLen);
}
static inline int16_t clamp16(int32_t v){
    if(v < -32768) return -32768;
    if(v >  32767) return  32767;
    return (int16_t)v;
}
static inline uint8_t clamp_u8_64(int v){
    if(v < 0) return 0;
    if(v > 64) return 64;
    return (uint8_t)v;
}

// ---------------- period table ----------------
// ProTracker base periods C-1..B-4
static const uint16_t basePeriods[48] = {
    1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960, 906,
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113
};

static const uint32_t kFinetunePeriodMulQ16[16] = {
    69433, 68933, 68437, 67945, 67456, 66971, 66490, 66012,
    65536, 65064, 64595, 64129, 63666, 63206, 62749, 62295
};

static const char *kNoteNames[12] = {
    "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
};

// MED note: 1=C-0, 2=C#0 ... 12=B-0, 13=C-1 ...
static void format_med_note(uint8_t note, char out[4])
{
    if(note == 0 || note >= 0x80){
        out[0]='-'; out[1]='-'; out[2]='-'; out[3]=0;
        return;
    }

    int n = (int)note - 1;
    int name = n % 12;
    int oct  = n / 12;

    // "C-0" / "C#3" etc (3 chars)
    out[0] = kNoteNames[name][0];
    out[1] = kNoteNames[name][1];
    out[2] = (char)('0' + (oct % 10)); // oct usually 0..7; keep simple
    out[3] = 0;
}


static inline uint16_t apply_finetune_to_period(uint16_t period, int8_t finetune){
    if(period == 0) return 0;

    int ft = (int)finetune;
    if(ft < -8) ft = -8;
    if(ft >  7) ft =  7;

    uint32_t mul = kFinetunePeriodMulQ16[(uint32_t)(ft + 8)];
    uint32_t p = (uint32_t)period;
    // (p * mul + 0.5) >> 16
    uint32_t out = (p * mul + 32768u) >> 16;
    if(out < 1u) out = 1u;
    if(out > 65535u) out = 65535u;
    return (uint16_t)out;
}

// Read finetune from ExpData->InstrExt (MMD0Exp/MMDInstrExt in OpenMPT)
// MMD0Exp layout: instrExtOffset @ +4, instrExtEntries @ +8, instrExtEntrySize @ +10
// MMDInstrExt layout: finetune is 4th byte (hold,decay,suppress,finetune) => +3
static void load_instrext_envelope(uint32_t count)
{
    for(uint32_t i = 0; i < count; i++){
        g_med.samples[i].finetune = 0;
        g_med.samples[i].hold     = 0;
        g_med.samples[i].decay    = 0;
    }

    if(g_med.expDataOffset == 0) return;
    if(!file_can(g_med.expDataOffset, 80u)) return;

    uint32_t ex = g_med.expDataOffset;

    uint32_t instrExtOffset   = rd_be32(&g_med.file[ex + 4u]);
    uint16_t instrExtEntries  = rd_be16(&g_med.file[ex + 8u]);
    uint16_t instrExtEntSize  = rd_be16(&g_med.file[ex + 10u]);

    if(instrExtOffset == 0 || instrExtEntries == 0) return;
    if(instrExtEntSize < 4u) return;

    uint32_t n = count;
    if(n > (uint32_t)instrExtEntries) n = (uint32_t)instrExtEntries;

    uint32_t need = (uint32_t)instrExtEntSize * n;
    if(!file_can(instrExtOffset, need)) return;

    for(uint32_t i = 0; i < n; i++)
    {
        uint32_t off = instrExtOffset + (uint32_t)instrExtEntSize * i;

        g_med.samples[i].hold     = g_med.file[off + 0u];
        g_med.samples[i].decay    = g_med.file[off + 1u];
        // suppress at off+2 (ignored for now)
        g_med.samples[i].finetune = (int8_t)g_med.file[off + 3u];
    }
}

static void schedule_flow(uint16_t order, uint16_t row){
    g_med.flowPending = 1;
    g_med.flowOrder = order;
    g_med.flowRow   = row;

    // Any pending flow change means "do not normal-advance" at row end.
    g_rowAdvancePending = 0;

    // Pattern delay should not keep repeating into a jump/break.
    g_med.patDelayCnt = 0;
    g_skipRowProcess  = 0;
}

static void advance_row_normal(void){
    g_med.curRow++;
    if(g_med.curRow >= g_med.patRows) {
        g_med.curRow = 0;
        g_med.curOrder++;
        if(g_med.curOrder >= g_med.songLen) g_med.curOrder = 0;
    }
}


static uint16_t note_to_period(int noteVal /*1..*/){
    if(noteVal <= 0) return 0;
    if(noteVal >= 0x80) return 0;

    // MED note numbers are typically 1=C-0, but ProTracker base table here is C-1..B-4.
    // So shift notes up by one octave to match the PT period table.
    int idx = (noteVal - 1) + 12;   // <-- critical fix (+12 semitones)

    int oct = idx / 12;
    int n   = idx % 12;

    int baseOct = oct;
    if(baseOct > 3) baseOct = 3;
    int baseIdx = baseOct * 12 + n;
    if(baseIdx < 0) baseIdx = 0;
    if(baseIdx > 47) baseIdx = 47;

    uint32_t p = basePeriods[baseIdx];

    int diff = oct - baseOct;
    while(diff > 0){ p >>= 1; diff--; }
    while(diff < 0){ p <<= 1; diff++; }

    if(p < 1) p = 1;
    if(p > 65535u) p = 65535u;
    return (uint16_t)p;
}

static uint32_t period_to_stepFP(uint16_t period){
    if(period == 0) return 0;

    // Paula rate = paulaClock / (period * 2)
    // stepFP = rate * FP_ONE / outRate
    //
    // Do division early to keep 32-bit safe:
    // stepFP = (paulaClock * FP_ONE) / (period*2*outRate)

    uint32_t den = (uint32_t)period * 2u;

    // avoid overflow: (paulaClock / den) first
    uint32_t rate = g_med.paulaClock / den;
    if(rate == 0) rate = 1;

    // now step = rate * FP_ONE / outRate (rate up to ~ 32k-ish)
    uint32_t step = (rate * FP_ONE) / g_med.outRate;
    if(step == 0) step = 1;

    return step;
}

// ---------------- timing (Audacious/libopenmpt-like behavior without the libs) ----------------
// PAL Amiga E-Clock (CIA timer clock). This is NOT Paula (7.09MHz).
// This is what old MED non-BPM tempo values are effectively based on.
// ---------------- timing (OpenMPT / libopenmpt style, integer) ----------------
// We compute BPM in milli-BPM (x1000) to avoid floats but still support 111.5 / 157.86 quirks.

static uint32_t u32_min(uint32_t a, uint32_t b){ return (a < b) ? a : b; }

 uint32_t mmd_tempo_to_milli_bpm(uint32_t tempo,
                                       uint8_t is8Ch,
                                       uint8_t softwareMixing,
                                       uint8_t bpmMode,
                                       uint8_t rowsPerBeat)
{
    static const uint16_t tempos8ch[10] = {179,164,152,141,131,123,116,110,104,99};

    if(tempo == 0) tempo = 125;
    if(rowsPerBeat == 0) rowsPerBeat = 4;

    if(bpmMode && !is8Ch)    {
        if(tempo < 7) return 111500u; // 111.5 BPM

        // milliBPM = tempo * rowsPerBeat * 1000 / 4
        // all 32-bit safe: tempo<=65535, rowsPerBeat<=32 => product <= ~2 million, *1000 <= ~2 billion
        uint32_t num = tempo * (uint32_t)rowsPerBeat * 1000u;
        return (num + 2u) / 4u;
    }

    if(is8Ch && tempo > 0)    {
        if(tempo > 10) tempo = 10;
        return (uint32_t)tempos8ch[tempo - 1] * 1000u;
    }

    if(!softwareMixing && tempo > 0 && tempo <= 10)    {
        // milliBPM = round( (6*1773447*1000) / (14500*tempo) )
        // constants are big -> do staged division
        // K = 6*1773447*1000 = 10,640,682,000 (too big for 32-bit)
        // So: (6*1773447) = 10,640,682 then *1000 later with remainder handling.

        uint32_t a = 6u * 1773447u;                 // 10,640,682 fits 32-bit
        uint32_t den = 14500u * tempo;              // <=145,000 fits 32-bit

        // compute (a*1000 + den/2)/den without a*1000 overflowing:
        // a*1000 = a* (8*125) => still overflow risk if done directly, so split:
        // (a/den)*1000 + (a%den)*1000/den
        uint32_t q = a / den;
        uint32_t r = a - q * den;

        uint32_t part = (r * 1000u + den/2u) / den; // r<den so r*1000 <= 145,000,000 safe
        return q * 1000u + part;
    }

    if(softwareMixing && tempo < 8)
    {
        return 157860u; // 157.86 BPM
    }

    // milliBPM = tempo * 1000000 / 264
    // tempo<=65535 => tempo*1,000,000 overflow 32-bit, so split:
    // (tempo/264)*1,000,000 + (tempo%264)*1,000,000/264
    {
        const uint32_t DEN = 264u;

        uint32_t q = tempo / DEN;
        uint32_t r = tempo - q * DEN;

        uint32_t part = (r * 1000000u + (DEN/2u)) / DEN; // rounding
        return q * 1000000u + part;
    }
}

static void recompute_timing(void)
{
    // ticks per row / line (speed)
    uint32_t speed = g_med.ticksPerLine;
    if(speed == 0) speed = 6;
    if(speed > 0x20) speed = 0x20;
    g_med.ticksPerLine = (uint8_t)speed;

    // LPB = (flags2 & 0x1F) + 1
    uint32_t lpb = (uint32_t)((g_med.flags2 & 0x1F) + 1u);
    if(lpb == 0) lpb = 4;
    g_med.rowsPerBeat = (uint8_t)lpb;

    // OpenMPT-ish mode flags (as you had)
    uint8_t is8Ch = (g_med.flags & 0x40) ? 1 : 0;
    uint8_t softwareMixing = (g_med.flags2 & 0x80) ? 1 : 0;
    uint8_t bpmMode = (g_med.flags2 & 0x20) ? 1 : 0;

    uint32_t tempo = g_med.defTempo;
    if(tempo == 0) tempo = 125;

    uint32_t bpm_x1000 = mmd_tempo_to_milli_bpm(tempo, is8Ch, softwareMixing, bpmMode, (uint8_t)lpb);
    /*
    if(!bpmMode) {
        // bpm_x1000 = bpm_x1000 * lpb / 4 (rounded)
        uint32_t num = bpm_x1000 * (uint32_t)lpb;
        bpm_x1000 = (num + 2u) / 5u;
    }
*/
    if(bpm_x1000 < 1000u) bpm_x1000 = 1000u;

    // ---- 32-bit safe tick length ----
    // samplesPerTick = outRate * 2500 / bpm_x1000
    // store in TICK_FP fixed-point:
    // samplesPerTickFP = samplesPerTick * TICK_FP_ONE
    //
    // Do division early with remainder to preserve fractional without 64-bit.

    // outRate*2500 fits in 32-bit for normal audio rates (<=192k)
    uint32_t base = g_med.outRate * 2500u;     // up to 192000*2500=480,000,000 < 2^32

    uint32_t q = base / bpm_x1000;             // integer part (samples per tick)
    uint32_t r = base - q * bpm_x1000;         // remainder

    // fractional part: (r / bpm_x1000) * TICK_FP_ONE
    // r < bpm_x1000, so r*TICK_FP_ONE <= bpm_x1000*TICK_FP_ONE
    // With TICK_FP_SHIFT=10 => max about 4,194,304,000 if bpm_x1000 hits 4,096,000
    // (still within uint32_t). Real-world bpm_x1000 is way lower.
    uint32_t frac = (r * TICK_FP_ONE) / bpm_x1000;

    // combine
    g_med.samplesPerTickFP = (q * TICK_FP_ONE) + frac;
    if(g_med.samplesPerTickFP == 0) g_med.samplesPerTickFP = 1;
}

// ---------------- file load ----------------
static int load_entire_file(const char *filename){
    FILE *f = fopen(filename, "rb");
    if(!f){ med_err("fopen failed"); return 0; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(sz <= 0){ fclose(f); med_err("empty file"); return 0; }
    if((uint32_t)sz > (uint32_t)MED_FILE_MAX){ fclose(f); med_err("file too big"); return 0; }

    size_t got = fread(g_med_filebuf, 1, (size_t)sz, f);
    fclose(f);

    if(got != (size_t)sz){ med_err("fread failed"); return 0; }

    g_med.file    = g_med_filebuf;
    g_med.fileLen = (uint32_t)sz;

    g_med_filebuf_len = (uint32_t)sz;
    return 1;
}

// ---------------- parse headers ----------------
static int parse_headers(void){
    if(!file_can(0, 52)){ med_err("file too small"); return 0; }

    if(!(g_med.file[0]=='M' && g_med.file[1]=='M' && g_med.file[2]=='D')){
        med_err("not MMD");
        return 0;
    }
    char verch = (char)g_med.file[3];
    if(verch < '0' || verch > '3'){
        med_err("only MMD0/MMD1 supported");
        return 0;
    }
    g_med.version = (uint8_t)(verch - '0');

    g_med.songOffset      = rd_be32(&g_med.file[8]);
    g_med.blockArrOffset  = rd_be32(&g_med.file[16]);
    g_med.sampleArrOffset = rd_be32(&g_med.file[24]);
    g_med.expDataOffset   = rd_be32(&g_med.file[32]);

    if(g_med.songOffset == 0 || !file_can(g_med.songOffset, 788u)){
        med_err("bad songOffset");
        return 0;
    }
    if(g_med.blockArrOffset == 0 || !file_can(g_med.blockArrOffset, 4u)){
        med_err("bad blockArrOffset");
        return 0;
    }

    uint32_t so = g_med.songOffset;

    for(uint32_t i=0;i<63;i++){
        const uint8_t *p = &g_med.file[so + i*8u];
        g_med.smphdr[i].rep        = rd_be16(p+0);
        g_med.smphdr[i].replen     = rd_be16(p+2);
        g_med.smphdr[i].midich     = p[4];
        g_med.smphdr[i].midipreset = p[5];
        g_med.smphdr[i].svol       = p[6];
        g_med.smphdr[i].strans     = (int8_t)p[7];
    }

    g_med.numBlocks  = rd_be16(&g_med.file[so + 504]);
    g_med.songLen    = rd_be16(&g_med.file[so + 506]);
    memcpy(g_med.playSeq, &g_med.file[so + 508], 256);

    g_med.defTempo   = rd_be16(&g_med.file[so + 764]);
    g_med.playTransp = (int8_t)g_med.file[so + 766];
    g_med.flags      = g_med.file[so + 767];
    g_med.flags2     = g_med.file[so + 768];
    g_med.tempo2     = g_med.file[so + 769];
    memcpy(g_med.trkVol, &g_med.file[so + 770], 16);
    g_med.masterVol  = g_med.file[so + 786];
    g_med.numSamples = g_med.file[so + 787];

    if(g_med.numSamples == 0 && g_med.sampleArrOffset != 0) {
        // If the table is present, treat it as up to 63 instruments.
        if(file_can(g_med.sampleArrOffset, 63u * 4u)) {
            g_med.numSamples = 63;
        }
    }


    if(g_med.numSamples > 63){ med_err("numsamples > 63"); return 0; }
    if(g_med.numBlocks == 0){ med_err("no patterns"); return 0; }
    if(g_med.songLen == 0 || g_med.songLen > 256){ med_err("bad song length"); return 0; }

    g_med.ticksPerLine = g_med.tempo2 ? g_med.tempo2 : 6;

    recompute_timing();
    return 1;
}

static int load_samples(void)
{
    for(uint32_t i=0; i<MED_MAX_SAMPLES; i++){
        g_med.samples[i].data      = NULL;
        g_med.samples[i].length    = 0;
        g_med.samples[i].loopStart = 0;
        g_med.samples[i].loopLen   = 0;
        g_med.samples[i].volume    = 0;
        g_med.samples[i].transpose = 0;   // Ensure this is initialized
        g_med.samples[i].finetune  = 0;
        g_med.samples[i].hold      = 0;
        g_med.samples[i].decay     = 0;
        g_med.samples[i].valid     = 0;
    }

    g_med_sampram_used = 0;

    uint32_t count = g_med.numSamples ? g_med.numSamples : 63u;
    if(count > 63u) count = 63u;

    load_instrext_envelope(count);

    if(g_med.sampleArrOffset == 0) return 1;
    if(!file_can(g_med.sampleArrOffset, count * 4u)){
        med_err("sample offset table out of range");
        return 0;
    }

    static const int8_t codeToDelta[16] = {
        -34,-21,-13,-8,-5,-3,-2,-1, 0, 1, 2, 3, 5, 8,13,21
    };

    for(uint32_t i=0; i<count; i++)
    {
        g_med.samples[i].volume    = clamp_u8_64(g_med.smphdr[i].svol);
        g_med.samples[i].transpose = g_med.smphdr[i].strans;   // Here it is!

        uint32_t instOff = rd_be32(&g_med.file[g_med.sampleArrOffset + i*4u]);
        if(instOff == 0) continue;
        if(!file_can(instOff, 6)) continue;

        uint32_t len  = rd_be32(&g_med.file[instOff + 0]);
        int16_t  type = (int16_t)rd_be16(&g_med.file[instOff + 4]);
        if(len == 0) continue;

        if(type < 0) continue; // synth/external

        uint32_t dataOff = instOff + 6;
        if(dataOff >= g_med.fileLen) continue;

        uint32_t avail = g_med.fileLen - dataOff;
        if(len > avail) len = avail;
        if(len == 0) continue;

        // ---- loops (ONLY HERE, ONCE) ----
        // ---- loops (rep/replen may be in bytes OR words depending on writer) ----
        uint32_t rep    = (uint32_t)g_med.smphdr[i].rep;
        uint32_t replen = (uint32_t)g_med.smphdr[i].replen;

        uint32_t loopStart = 0;
        uint32_t loopLen2  = 0;

        if(replen > 1u)
        {
            uint32_t b_start = rep;
            uint32_t b_len   = replen;

            uint32_t w_start = rep * 2u;
            uint32_t w_len   = replen * 2u;

            uint8_t b_ok = (b_len >= 2u) && (b_start < len) && (b_start + b_len <= len);
            uint8_t w_ok = (w_len >= 2u) && (w_start < len) && (w_start + w_len <= len);

            if(b_ok && !w_ok)
            {
                loopStart = b_start;
                loopLen2  = b_len;
            }
            else if(w_ok && !b_ok)
            {
                loopStart = w_start;
                loopLen2  = w_len;
            }
            else if(b_ok && w_ok)
            {
                // tie-break: pick the one that looks more "loop-ish"
                // 1) prefer the one whose loop end is closer to sample end
                uint32_t b_end = b_start + b_len;
                uint32_t w_end = w_start + w_len;

                uint32_t b_tail = len - b_end;
                uint32_t w_tail = len - w_end;

                // 2) penalize absurdly small loops (very often wrong-unit interpretation)
                // keep it gentle: only bias, don't forbid
                uint32_t b_pen = (b_len < 32u) ? 1u : 0u;
                uint32_t w_pen = (w_len < 32u) ? 1u : 0u;

                // score: smaller is better
                uint32_t b_score = b_tail + (b_pen ? (len / 2u) : 0u);
                uint32_t w_score = w_tail + (w_pen ? (len / 2u) : 0u);

                if(w_score < b_score){
                    loopStart = w_start;
                    loopLen2  = w_len;
                } else {
                    loopStart = b_start;
                    loopLen2  = b_len;
                }
            }
        }

        // ---- flags ----
        uint16_t tt = (uint16_t)type;
        uint8_t tf0 = (uint8_t)(tt & 0xFF);
        uint8_t tf1 = (uint8_t)((tt >> 8) & 0xFF);

        uint8_t isDelta  = ((tf0 & 0x40) || (tf1 & 0x40)) ? 1 : 0;
        uint8_t isPack   = ((tf0 & 0x80) || (tf1 & 0x80)) ? 1 : 0;
        uint8_t is16     = ((tf0 & 0x10) || (tf1 & 0x10)) ? 1 : 0;
        uint8_t isStereo = ((tf0 & 0x20) || (tf1 & 0x20)) ? 1 : 0;

        // ---- PACKED ----
        if(isPack)
        {
            if(len < 2) continue;

            uint32_t outLen = 2u * (len - 2u);
            if(outLen == 0) continue;
            if(g_med_sampram_used + outLen > MED_SAMPLE_RAM_MAX) continue;

            int8_t *dstBase = &g_med_sampram[g_med_sampram_used];
            int8_t *dst     = dstBase;
            g_med_sampram_used += outLen;

            const uint8_t *src = &g_med.file[dataOff];
            int32_t x = (int8_t)src[1];

            const uint8_t *p = src + 2;
            uint32_t n = len - 2u;

            for(uint32_t k=0; k<n; k++){
                uint8_t b = p[k];

                x += (int32_t)codeToDelta[(b >> 4) & 0x0F];
                if(x < -128) x = -128;
                if(x >  127) x =  127;
                *dst++ = (int8_t)x;

                x += (int32_t)codeToDelta[b & 0x0F];
                if(x < -128) x = -128;
                if(x >  127) x =  127;
                *dst++ = (int8_t)x;
            }

            g_med.samples[i].data      = dstBase;
            g_med.samples[i].length    = outLen;
            g_med.samples[i].loopStart = (loopStart < outLen) ? loopStart : 0;
            g_med.samples[i].loopLen   = ((loopLen2 >= 2u) && (g_med.samples[i].loopStart + loopLen2 <= outLen)) ? loopLen2 : 0;
            g_med.samples[i].valid     = 1;
            continue;
        }

        // ---- DELTA ----
        if(isDelta)
        {
            if(g_med_sampram_used + len > MED_SAMPLE_RAM_MAX) continue;

            int8_t *dst = &g_med_sampram[g_med_sampram_used];
            g_med_sampram_used += len;

            const int8_t *src = (const int8_t *)&g_med.file[dataOff];

            int32_t x = 0;
            for(uint32_t k=0; k<len; k++){
                x += (int32_t)src[k];
                if(x < -128) x = -128;
                if(x >  127) x =  127;
                dst[k] = (int8_t)x;
            }

            g_med.samples[i].data      = dst;
            g_med.samples[i].length    = len;
            g_med.samples[i].loopStart = (loopStart < len) ? loopStart : 0;
            g_med.samples[i].loopLen   = ((loopLen2 >= 2u) && (g_med.samples[i].loopStart + loopLen2 <= len)) ? loopLen2 : 0;
            g_med.samples[i].valid     = 1;
            continue;
        }

        // ---- RAW (8/16-bit, mono/stereo) -> convert to signed 8-bit mono ----
        {
            uint64_t frames = (uint64_t)len;
            uint64_t bytesPerFrame = 1ull;
            if(is16)     bytesPerFrame *= 2ull;
            if(isStereo) bytesPerFrame *= 2ull;

            uint64_t needBytes = frames * bytesPerFrame;
            if(needBytes > (uint64_t)avail) needBytes = (uint64_t)avail;
            if(needBytes == 0) continue;

            uint32_t outFrames = (uint32_t)(needBytes / bytesPerFrame);
            if(outFrames == 0) continue;

            if(g_med_sampram_used + outFrames > MED_SAMPLE_RAM_MAX) continue;

            int8_t *dst = &g_med_sampram[g_med_sampram_used];
            g_med_sampram_used += outFrames;

            const uint8_t *src = &g_med.file[dataOff];

            if(!is16 && !isStereo)
            {
                for(uint32_t k=0; k<outFrames; k++){
                    dst[k] = (int8_t)src[k];
                }
            }
            else if(is16 && !isStereo)
            {
                for(uint32_t k=0; k<outFrames; k++){
                    uint32_t o = k * 2u;
                    dst[k] = (int8_t)src[o + 1]; // high byte (LE 16-bit)
                }
            }
            else if(!is16 && isStereo)
            {
                for(uint32_t k=0; k<outFrames; k++){
                    uint32_t o = k * 2u;
                    int16_t L = (int8_t)src[o + 0];
                    int16_t R = (int8_t)src[o + 1];
                    dst[k] = (int8_t)((L + R) / 2);
                }
            }
            else // is16 && isStereo
            {
                for(uint32_t k=0; k<outFrames; k++){
                    uint32_t o = k * 4u;
                    int16_t L = (int8_t)src[o + 1];
                    int16_t R = (int8_t)src[o + 3];
                    dst[k] = (int8_t)((L + R) / 2);
                }
            }

            g_med.samples[i].data      = dst;
            g_med.samples[i].length    = outFrames;
            g_med.samples[i].loopStart = (loopStart < outFrames) ? loopStart : 0;
            g_med.samples[i].loopLen   = ((loopLen2 >= 2u) && (g_med.samples[i].loopStart + loopLen2 <= outFrames)) ? loopLen2 : 0;
            g_med.samples[i].valid     = 1;
            continue;
        }
    }

    return 1;
}




// ---------------- pattern load ----------------
static int load_pattern(uint16_t pat)
{
    if(pat >= g_med.numBlocks) return 0;

    if(!file_can(g_med.blockArrOffset + pat*4u, 4u)) return 0;
    uint32_t blkOff = rd_be32(&g_med.file[g_med.blockArrOffset + pat*4u]);
    if(blkOff == 0) return 0;

    if(g_med.version == 0){
        if(!file_can(blkOff, 2u)) return 0;
    } else {
        if(!file_can(blkOff, 8u)) return 0;
    }

    memset(g_med.pat, 0, sizeof(g_med.pat));
    g_med.patIndex = pat;

    uint16_t tracks = 0;
    uint16_t rows   = 0;
    uint32_t dataOff = 0;

    if(g_med.version == 0){
        tracks = g_med.file[blkOff + 0];
        rows   = (uint16_t)g_med.file[blkOff + 1] + 1u;
        dataOff= blkOff + 2u;
    } else {
        tracks = rd_be16(&g_med.file[blkOff + 0]);
        rows   = (uint16_t)rd_be16(&g_med.file[blkOff + 2]) + 1u;
        dataOff= blkOff + 8u;
    }

    if(tracks == 0) tracks = 1;
    if(rows == 0) rows = 1;
    if(rows > MED_MAX_ROWS) rows = MED_MAX_ROWS;

    g_med.patRows   = rows;
    g_med.patTracks = tracks;
    g_med.numTracks = (uint8_t)((tracks > MED_MAX_TRACKS) ? MED_MAX_TRACKS : tracks);

    const uint32_t bytesPer = (g_med.version == 0) ? 3u : 4u;

    // ---- 32-bit safe size check ----
    if(dataOff > g_med.fileLen) return 0;

    // rt = rows * tracks (guard overflow)
    uint32_t rt = (uint32_t)rows * (uint32_t)tracks;
    if(tracks != 0 && (rt / (uint32_t)tracks) != (uint32_t)rows) return 0;

    // need = rt * bytesPer (guard overflow)
    uint32_t need = rt * bytesPer;
    if(bytesPer != 0 && (need / bytesPer) != rt) return 0;

    // dataOff + need <= fileLen (written overflow-safe)
    if(need > (g_med.fileLen - dataOff)) return 0;

    const uint8_t *p = &g_med.file[dataOff];

    for(uint16_t r=0; r<rows; r++){
        for(uint16_t t=0; t<tracks; t++){
            if(g_med.version == 0){
                uint8_t b0 = *p++;
                uint8_t b1 = *p++;
                uint8_t b2 = *p++;

                uint8_t note = (uint8_t)(b0 & 0x3F);
                uint8_t inst = (uint8_t)(((b0 & 0xC0) >> 2) | ((b1 & 0xF0) >> 4));
                uint8_t cmd  = (uint8_t)(b1 & 0x0F);
                uint8_t par  = b2;

                if(t < MED_MAX_TRACKS){
                    g_med.pat[r][t].note  = note;
                    g_med.pat[r][t].inst  = inst;
                    g_med.pat[r][t].cmd   = cmd;
                    g_med.pat[r][t].param = par;
                }
            } else {
                uint8_t note = *p++;
                uint8_t inst = *p++;
                uint8_t cmd  = *p++;
                uint8_t par  = *p++;

                if(t < MED_MAX_TRACKS){
                    g_med.pat[r][t].note  = note;
                    g_med.pat[r][t].inst  = inst;
                    g_med.pat[r][t].cmd   = cmd;
                    g_med.pat[r][t].param = par;
                }
            }
        }
    }

    g_med.patLoaded = 1;
    return 1;
}

// ---------------- reset player ----------------
static void reset_player(void){
    g_med.curOrder = 0;
    g_med.curRow   = 0;
    g_med.curTick  = 0;
    g_med.flowPending = 0;
    g_med.flowOrder   = 0;
    g_med.flowRow     = 0;

    g_med.tickAccFP = 0;
    g_med.patDelayCnt = 0;

    g_med.patLoaded = 0;
    g_med.patIndex  = 0xFFFF;

    for(uint32_t i=0;i<MED_MAX_TRACKS;i++){
        memset(&g_med.ch[i], 0, sizeof(g_med.ch[i]));

        // mild classic-ish stereo defaults (can be overridden by cmd 8 / E8)
        g_med.ch[i].pan = (uint8_t)((i & 1) ? 200 : 56);
    }
}

// ---------------- effect helpers ----------------
static int8_t sine64(int idx){
    static const int8_t s[64] = {
        0, 12, 25, 37, 49, 60, 71, 81,
        90, 98,105,111,116,120,123,125,
        127,125,123,120,116,111,105, 98,
        90, 81, 71, 60, 49, 37, 25, 12,
        0,-12,-25,-37,-49,-60,-71,-81,
        -90,-98,-105,-111,-116,-120,-123,-125,
        -127,-125,-123,-120,-116,-111,-105, -98,
        -90, -81, -71, -60, -49, -37, -25, -12
    };
    return s[idx & 63];
}

static void fx_arpeggio(MedChan *c, uint8_t tick)
{
    // Arp uses lastNote, but must include song transpose + instrument transpose
    if(c->lastNote == 0) return;

    uint8_t add = 0;
    switch(tick % 3){
    case 1: add = c->arpX; break;
    case 2: add = c->arpY; break;
    default:add = 0; break;
    }

    int raw = (int)c->lastNote + (int)add;
    if(raw < 1) raw = 1;

    uint8_t si = c->inst;
    if(si >= MED_MAX_SAMPLES) si = 0;

    int nn = raw + (int)g_med.playTransp + (int)g_med.samples[si].transpose;
    if(nn < 1) nn = 1;

    uint16_t per = note_to_period(nn);
    per = apply_finetune_to_period(per, g_med.samples[si].finetune);
    if(per) c->stepFP = period_to_stepFP(per);
}


static void fx_porta_up(MedChan *c, uint8_t param){
    if(!c->period) return;
    uint16_t p = c->period;
    p = (p > param) ? (uint16_t)(p - param) : 1;
    c->period = p;
    c->stepFP = period_to_stepFP(p);
}
static void fx_porta_down(MedChan *c, uint8_t param){
    if(!c->period) return;
    uint32_t p = (uint32_t)c->period + (uint32_t)param;
    if(p > 65535u) p = 65535u;
    c->period = (uint16_t)p;
    c->stepFP = period_to_stepFP(c->period);
}
static void fx_tone_porta(MedChan *c){
    if(!c->period || !c->targetPeriod || !c->portaSpeed) return;
    uint16_t p = c->period;

    if(p < c->targetPeriod){
        uint32_t np = (uint32_t)p + (uint32_t)c->portaSpeed;
        if(np > c->targetPeriod) np = c->targetPeriod;
        c->period = (uint16_t)np;
    } else if(p > c->targetPeriod){
        int32_t np = (int32_t)p - (int32_t)c->portaSpeed;
        if(np < (int32_t)c->targetPeriod) np = (int32_t)c->targetPeriod;
        if(np < 1) np = 1;
        c->period = (uint16_t)np;
    }
    c->stepFP = period_to_stepFP(c->period);
}

static void fx_vibrato(MedChan *c){
    if(!c->period) return;
    int8_t sv = sine64(c->vibPos);
    int32_t delta = ((int32_t)sv * (int32_t)c->vibDepth) / 128;
    int32_t per = (int32_t)c->period + delta;
    if(per < 1) per = 1;
    if(per > 65535) per = 65535;
    c->stepFP = period_to_stepFP((uint16_t)per);
    c->vibPos = (uint8_t)(c->vibPos + c->vibSpeed);
}

static void fx_volslide(MedChan *c, uint8_t param){
    uint8_t x = (param >> 4) & 0x0F;
    uint8_t y = (param & 0x0F);
    int v = (int)c->vol + (int)x - (int)y;
    c->vol = clamp_u8_64(v);
}

// ---------------- note trigger ----------------
static void chan_retrigger(uint16_t t, uint8_t inst1)
{
    if(t >= g_med.numTracks) return;

    MedChan *c = &g_med.ch[t];

    uint8_t useInst1 = inst1 ? inst1 : c->lastInst1;
    uint8_t si = useInst1 ? (uint8_t)(useInst1 - 1) : c->inst;
    if(si >= MED_MAX_SAMPLES) si = 0;

    c->inst = si;
    // start OctaMED hold/decay envelope for this instrument
    c->envHold   = g_med.samples[si].hold;
    c->envDecay  = g_med.samples[si].decay;
    c->envActive = (c->envHold != 0 || c->envDecay != 0) ? 1 : 0;

    if(si < MED_MAX_SAMPLES && g_med.samples[si].valid)
    {
        c->active = 1;
        c->posFP  = 0;

        // Always ensure period matches current instrument transpose if we have a note.
        if(c->lastNote != 0)
        {
            int nn = (int)c->lastNote + (int)g_med.playTransp + (int)g_med.samples[si].transpose;
            if(nn < 1) nn = 1;

            uint16_t per = note_to_period(nn);
            per = apply_finetune_to_period(per, g_med.samples[si].finetune);
            if(per){
                c->period = per;
                c->targetPeriod = per;
            }
        }

        c->stepFP = period_to_stepFP(c->period);
    }
}

static void chan_trigger_note(uint16_t t, uint8_t inst1, uint8_t noteVal)
{
    if(t >= g_med.numTracks) return;
    if(noteVal == 0) return;

    MedChan *c = &g_med.ch[t];

    uint8_t useInst1 = inst1 ? inst1 : c->lastInst1;

    uint8_t si;
    if(useInst1){
        si = (uint8_t)(useInst1 - 1);
        c->lastInst1 = useInst1;
    } else {
        si = c->inst;
    }
    if(si >= MED_MAX_SAMPLES) si = 0;

    c->inst     = si;
    c->lastNote = noteVal;
    // start OctaMED hold/decay envelope for this instrument
    c->envHold   = g_med.samples[si].hold;
    c->envDecay  = g_med.samples[si].decay;
    c->envActive = (c->envHold != 0 || c->envDecay != 0) ? 1 : 0;

    int nn = (int)noteVal + (int)g_med.playTransp + (int)g_med.samples[si].transpose;
    if(nn < 1) nn = 1;




    uint16_t per = note_to_period(nn);
    per = apply_finetune_to_period(per, g_med.samples[si].finetune);



    c->period       = per;
    c->targetPeriod = per;

    if(g_med.samples[si].valid){
        c->active = 1;
        c->posFP  = 0;
    } else {
        c->active = 0;
    }

    c->stepFP = period_to_stepFP(c->period);
}

// ---------------- row command decode ----------------
static void apply_cmd_row(uint16_t t, uint8_t cmd, uint8_t param, uint8_t noteVal, uint8_t inst1)
{
    MedChan *c = &g_med.ch[t];
    uint8_t x = (param >> 4) & 0x0F;
    uint8_t y = (param & 0x0F);

    switch(cmd)
    {
    case 0x0: // Arpeggio (memory: keep X/Y)
        if(param){ c->arpX = x; c->arpY = y; }
        break;

    case 0x1: // Porta up (memory)
        if(param) c->mem_1 = param;
        break;

    case 0x1A: // OctaMED: Volume slide UP by param (0..15-ish)
        // Convert to PT-style Axy where x=param, y=0
        if(param) c->mem_1A = param;
        else      param = c->mem_1A;

        c->vol = clamp_u8_64((int)c->vol + (int)(param & 0x0F));
        break;

    case 0x2: // Porta down (memory)
        if(param) c->mem_2 = param;
        break;

    case 0x3: // Tone porta (speed memory)
        if(param) { c->portaSpeed = param; c->mem_3 = param; }
        else if(c->mem_3) c->portaSpeed = c->mem_3;

        if(noteVal){
            uint8_t useInst1 = inst1 ? inst1 : c->lastInst1;
            uint8_t si = useInst1 ? (uint8_t)(useInst1 - 1) : c->inst;
            if(si >= MED_MAX_SAMPLES) si = 0;

            int nn = (int)noteVal + (int)g_med.playTransp + (int)g_med.samples[si].transpose;
            if(nn < 1) nn = 1;

            uint16_t per = note_to_period(nn);
            per = apply_finetune_to_period(per, g_med.samples[si].finetune);
            c->targetPeriod = per;
            c->lastNote = noteVal;
        }
        break;

    case 0x5:
        // IMPORTANT MED QUIRK (matches your file):
        // Treat 5xx as TONE PORTAMENTO with SPEED=xx (NOT toneporta+volslide).
        if(param) { c->portaSpeed = param; c->mem_3 = param; }
        else if(c->mem_3) c->portaSpeed = c->mem_3;

        if(noteVal){
            uint8_t useInst1 = inst1 ? inst1 : c->lastInst1;
            uint8_t si = useInst1 ? (uint8_t)(useInst1 - 1) : c->inst;
            if(si >= MED_MAX_SAMPLES) si = 0;

            int nn = (int)noteVal + (int)g_med.playTransp + (int)g_med.samples[si].transpose;
            if(nn < 1) nn = 1;

            c->targetPeriod = note_to_period(nn);
            c->lastNote = noteVal;
        }
        break;

    case 0x4: // Vibrato (memory)
    case 0x6: // Vibrato + volslide (keep as-is if you later need it)
        if(param){
            if(x) c->vibSpeed = x;
            if(y) c->vibDepth = y;
            c->mem_4 = (uint8_t)((c->vibSpeed << 4) | (c->vibDepth & 0x0F));
        } else if(c->mem_4){
            c->vibSpeed = (c->mem_4 >> 4) & 0x0F;
            c->vibDepth = (c->mem_4 & 0x0F);
        }
        break;

    case 0x8: // Set panning 0..255
        c->pan = param;
        break;

    case 0x9: // ticks-per-line (1..0x20)

        //if(param > 0 && param <= 0x20){
        //    g_med.ticksPerLine = param;
        //    recompute_timing();
        //}
        if(param != 0){
            g_med.defTempo = (uint16_t)param;
            recompute_timing();
        }
        break;

    case 0xA: // Volume slide (memory)
        if(param) c->mem_A = param;
        break;

    case 0xB: // Position jump
    {
        uint16_t ord = param;
        if(ord >= g_med.songLen) ord = 0;
        schedule_flow(ord, 0);
    } break;

    case 0xC: // Set volume 0..64
        c->vol = clamp_u8_64(param);
        break;

    case 0xD: // pattern break heuristic / else volslide
        if(param) c->mem_D = param;
        else      param = c->mem_D;

        {
            uint8_t d0 = (param >> 4) & 0x0F;
            uint8_t d1 = (param & 0x0F);

            if(d0 <= 9 && d1 <= 9){
                uint16_t ord = (uint16_t)(g_med.curOrder + 1);
                if(ord >= g_med.songLen) ord = 0;
                schedule_flow(ord, (uint16_t)(d0 * 10u + d1));
            }
        }
        break;

    case 0xE: // Extended
        if(x == 0x6 && y == 0x0) c->loopRow = g_med.curRow;
        if(x == 0xC) c->noteCut = y;
        if(x == 0xD) c->noteDelay = y;
        if(x == 0xE){
            if(y > 0) g_med.patDelayCnt = (uint8_t)(y + 1);
        }
        break;

    case 0xF:
        if(param == 0xFF){
            c->active = 0; c->vol = 0; c->period = 0; c->stepFP = 0;
            break;
        }
        if(param >= 0x01 && param <= 0x20){
            g_med.ticksPerLine = param;
            recompute_timing();
            break;
        }
        if(param == 0x00){
            uint16_t ord = (uint16_t)(g_med.curOrder + 1);
            if(ord >= g_med.songLen) ord = 0;
            schedule_flow(ord, 0);
            break;
        }
        g_med.defTempo = (uint16_t)param;
        recompute_timing();
        break;

    default:
        break;
    }
}

// ---------------- process row/tick ----------------
static void process_tick0_E(uint16_t t, uint8_t param, uint16_t rowPlaying);
static void process_row(void)
{
    uint8_t pat = g_med.playSeq[g_med.curOrder];
    if(!g_med.patLoaded || g_med.patIndex != pat)
    {
        if(!load_pattern(pat)) return;
    }

    if(g_med.curRow >= g_med.patRows) g_med.curRow = 0;

    const uint16_t row0 = g_med.curRow;

    // ticks 1..N read this fixed row
    g_rowPlaying = row0;

    // assume normal advance at end-of-row unless something schedules flow or pattern delay
    g_rowAdvancePending = 1;

    for(uint16_t t=0; t<g_med.numTracks; t++)
    {
        MedChan *c = &g_med.ch[t];
        MedEvent e = g_med.pat[row0][t];

        uint8_t inst1 = e.inst;
        uint8_t note  = e.note;
        uint8_t cmd   = e.cmd;
        uint8_t par   = e.param;

        c->noteDelay = 0;
        c->noteCut   = 0;

        // Instrument update (MED instrument-only retrigger)
        if(inst1)
        {
            c->lastInst1 = inst1;

            uint8_t si = (uint8_t)(inst1 - 1);
            if(si >= MED_MAX_SAMPLES) si = 0;

            if(note != 0)
            {
                // note present: instrument applies now
                c->inst = si;

                if(g_med.samples[si].valid)
                    c->vol = g_med.samples[si].volume;
            }
            else
            {

                if(!c->active || c->period == 0)
                {
                    c->inst = si;
                    if(g_med.samples[si].valid)
                        c->vol = g_med.samples[si].volume;
                }


                //chan_retrigger(t, inst1);

                // If we changed instrument while a note is already defined, force pitch to new transpose.
                if(c->lastNote != 0)
                {
                    int nn = (int)c->lastNote + (int)g_med.playTransp + (int)g_med.samples[c->inst].transpose;
                    if(nn < 1) nn = 1;
                    uint16_t per = note_to_period(nn);
                    if(per){
                        c->period = per;
                        c->targetPeriod = per;
                        c->stepFP = period_to_stepFP(per);
                    }
                }
            }
        }

        // Row command decode first (sets noteDelay/noteCut/patDelayCnt, and may schedule flow)
        if(cmd || par)
            apply_cmd_row(t, cmd, par, note, inst1);

        // Tick-0-only E-subcommands (may schedule flow for E6)
        if(cmd == 0xE)
            process_tick0_E(t, par, row0);

        // Note trigger rules
        {
            uint8_t isTonePorta = (cmd == 0x3 || cmd == 0x5);
            uint8_t isNoteDelay = (cmd == 0xE) && (((par >> 4) & 0x0F) == 0xD) && ((par & 0x0F) != 0);

            if(note)
            {
                c->lastNote = note;

                if(isNoteDelay)
                {
                    // don't trigger now; will trigger at tick noteDelay
                }
                else if(isTonePorta)
                {
                    // classic: if toneporta and channel silent, start note now
                    if(!c->active || c->period == 0)
                        chan_trigger_note(t, inst1, note);
                }
                else
                {
                    chan_trigger_note(t, inst1, note);
                }
            }
        }
    }

    // pattern delay means "repeat this row's ticks", so block normal advance
    if(g_med.patDelayCnt)
        g_rowAdvancePending = 0;

    // also block normal advance if any flow was scheduled this row
    if(g_med.flowPending)
        g_rowAdvancePending = 0;
}

static  int rowstatic = 0;
static int rowTickState = 0;

static void process_tick0_E(uint16_t t, uint8_t param, uint16_t rowPlaying)
{
    MedChan *c = &g_med.ch[t];
    uint8_t sub = (param >> 4) & 0x0F;
    uint8_t val = (param & 0x0F);

    switch(sub)
    {
    case 0x1: fx_porta_up(c, val); break;                 // E1x
    case 0x2: fx_porta_down(c, val); break;               // E2x

    case 0x6: // E6x pattern loop (apply at end of row)
        if(val == 0)
        {
            c->loopRow = rowPlaying;
        }
        else
        {
            if(c->loopCount == 0)
            {
                c->loopCount = val;
                schedule_flow(g_med.curOrder, c->loopRow);
            }
            else
            {
                c->loopCount--;
                if(c->loopCount)
                    schedule_flow(g_med.curOrder, c->loopRow);
            }
        }
        break;

    case 0x8: c->pan = (uint8_t)(val * 17u); break;       // E8x fine pan

        // E9x in PT is "retrig note every x ticks" (tick0 is NOT special; it retrigs on tick multiples in tick handler).
        // So DO NOT force a tick0 retrig here.
        // case 0x9: break;

    case 0xA: c->vol = clamp_u8_64((int)c->vol + (int)val); break; // EAx
    case 0xB: c->vol = clamp_u8_64((int)c->vol - (int)val); break; // EBx
    default: break;
    }
}


static void process_tick(void)
{
    uint16_t rowPlaying = g_rowPlaying;
    char trackstring[64];

    if(!g_med.patLoaded) return;
    if(rowPlaying >= g_med.patRows) rowPlaying = 0;

    if(rowPlaying != rowstatic){
        rowTickState = 1;
        printf("\n");
    }

    for(uint16_t t=0; t<g_med.numTracks; t++)
    {
        MedChan *c = &g_med.ch[t];
        MedEvent e = g_med.pat[rowPlaying][t];

        if(rowTickState){
            memset(trackstring, 0x00, sizeof(trackstring));
            char *ptr = trackstring;

            //if(e.note)  ptr += sprintf(ptr, " %02X", e.note);  else ptr += sprintf(ptr, " --");
            char nb[4];
            format_med_note(e.note, nb);
            ptr += sprintf(ptr, " %s", nb);
            if(e.inst)  ptr += sprintf(ptr, " %02X", e.inst);  else ptr += sprintf(ptr, " --");
            if(e.cmd)   ptr += sprintf(ptr, " %02X", e.cmd);   else ptr += sprintf(ptr, " --");
            if(e.param) ptr += sprintf(ptr, " %02X", e.param); else ptr += sprintf(ptr, " --");
            printf(" | ");
            printf(trackstring);
        }

        // note cut / delay must run even if inactive
        if(c->noteCut && g_med.curTick == c->noteCut) c->vol = 0;

        if(c->noteDelay && g_med.curTick == c->noteDelay){
            uint8_t useInst1 = c->lastInst1 ? c->lastInst1 : (uint8_t)(c->inst + 1);
            if(c->lastNote) chan_trigger_note(t, useInst1, c->lastNote);
        }

        // OctaMED hold/decay envelope (tick-based)
        if(c->envActive)
        {
            if(c->envHold)
            {
                c->envHold--;
            }
            else if(c->envDecay)
            {
                // Scale decay to something audible but not insane.
                // (If you later confirm exact MED semantics, tweak this.)
                int dv = (int)(c->envDecay >> 4);
                if(dv < 1) dv = 1;

                int v = (int)c->vol - dv;
                c->vol = clamp_u8_64(v);

                if(c->vol == 0){
                    c->envActive = 0;
                    c->active = 0;     // <- important: actually silence the channel
                    c->stepFP = 0;
                }
            }
        }

        if(!c->active || c->period == 0) continue;

        // Memory rules
        if(e.cmd == 0x1 && e.param == 0) e.param = c->mem_1;
        if(e.cmd == 0x2 && e.param == 0) e.param = c->mem_2;
        if(e.cmd == 0x3 && e.param == 0) e.param = c->mem_3;
        if(e.cmd == 0xA && e.param == 0) e.param = c->mem_A;
        if(e.cmd == 0xD && e.param == 0) e.param = c->mem_D;

        switch(e.cmd)
        {
        case 0x0:
            if(e.param) fx_arpeggio(c, g_med.curTick);
            if((g_med.curTick % 3) == 0) c->stepFP = period_to_stepFP(c->period);
            break;

        case 0x1:
            if(e.param) fx_porta_up(c, e.param);
            break;


        case 0x2:
            if(e.param) fx_porta_down(c, e.param);
            break;

        case 0x3:
            if(e.param) c->portaSpeed = e.param;
            fx_tone_porta(c);
            break;

        case 0x4:
            if(e.param == 0 && c->mem_4){
                c->vibSpeed = (c->mem_4 >> 4) & 0x0F;
                c->vibDepth = (c->mem_4 & 0x0F);
            }
            fx_vibrato(c);
            break;

        case 0x5:
            // MED quirk: 5xx is tone portamento speed (not +volslide)
            if(e.param) c->portaSpeed = e.param;
            else if(c->mem_3) c->portaSpeed = c->mem_3;

            fx_tone_porta(c);
            break;

        case 0x6:
            if(e.param == 0 && c->mem_4){
                c->vibSpeed = (c->mem_4 >> 4) & 0x0F;
                c->vibDepth = (c->mem_4 & 0x0F);
            }
            fx_vibrato(c);
            fx_volslide(c, e.param ? e.param : c->mem_A);
            break;

        case 0xA:
            fx_volslide(c, e.param);
            break;

        case 0xD: {
            uint8_t d0 = (e.param >> 4) & 0x0F;
            uint8_t d1 = (e.param & 0x0F);
            if(!(d0 <= 9 && d1 <= 9)){
                fx_volslide(c, e.param);
            }
        } break;

        case 0xE: {
            uint8_t sub = (e.param >> 4) & 0x0F;
            uint8_t val = (e.param & 0x0F);

            // Tick-time E subcommands only (NO tick0-only here)
            if(sub == 0x9 && val != 0){
                if(g_med.curTick != 0 && ((g_med.curTick % val) == 0)){
                    chan_retrigger(t, 0);
                }
            }
        } break;

        default:
            break;
        }
    }

    rowstatic = rowPlaying;
    rowTickState = 0;
}

static void player_tick_advance(void)
{
    if(g_med.curTick == 0)
    {
        if(g_skipRowProcess)
        {
            // Repeating row due to pattern delay:
            // Do NOT re-run process_row(), but DO run tick processing for tick 0.
            g_skipRowProcess = 0;
            process_tick();              // <-- critical
        }
        else
        {
            process_row();               // row decode + triggers
        }
    }
    else
    {
        process_tick();                  // ticks 1..N-1
    }

    g_med.curTick++;
    if(g_med.curTick >= g_med.ticksPerLine)
    {
        g_med.curTick = 0;

        if(g_med.flowPending)
        {
            g_med.flowPending = 0;
            g_med.curOrder = g_med.flowOrder;
            g_med.curRow   = g_med.flowRow;
        }
        else
        {
            if(g_med.patDelayCnt)
            {
                g_med.patDelayCnt--;

                if(g_med.patDelayCnt)
                {
                    g_skipRowProcess = 1; // repeat same row ticks
                }
                else
                {
                    if(g_rowAdvancePending)
                        advance_row_normal();
                }
            }
            else
            {
                if(g_rowAdvancePending)
                    advance_row_normal();
            }
        }
    }
}

// ---------------- mixer ----------------
static void mix_one(int16_t *outL, int16_t *outR)
{
    int32_t sumL = 0;
    int32_t sumR = 0;

    int32_t mv = g_med.masterVol ? g_med.masterVol : 64;
    if(mv > 64) mv = 64;

    uint8_t chan[4] = {0, 1, 1, 0};

    for(uint16_t t=0;t<g_med.numTracks;t++)

    {
        //if(t != 2) continue;    // <<-- used to single out a channel for testing.. DO NOT remove until debugged fully
        MedChan *c = &g_med.ch[t];
        if(!c->active || c->stepFP == 0) continue;

        uint8_t si = c->inst;
        if(si >= MED_MAX_SAMPLES) continue;

        MedSample *s = &g_med.samples[si];
        if(!s->valid || !s->data || s->length == 0) continue;

        // ---- fetch THEN advance ----
        uint32_t idx = (c->posFP >> FP_SHIFT);

        if(s->loopLen){
            uint32_t loopEnd = s->loopStart + s->loopLen;
            if(idx >= loopEnd){
                uint32_t over = idx - loopEnd;
                idx = s->loopStart + (over % s->loopLen);
                c->posFP = (idx << FP_SHIFT);
            }
        } else {
            if(idx >= s->length){
                c->active = 0;
                continue;
            }
        }

        int32_t smp = (int32_t)s->data[idx]; // -128..127

        // one-shot proof that the voice is reaching the mixer
        c->posFP += c->stepFP;

        int32_t v = (int32_t)c->vol;
        if(v < 0)  v = 0;
        if(v > 64) v = 64;

        int32_t tv = (int32_t)g_med.trkVol[t & 15] ;
        if(tv < 0)  tv = 0;
        if(tv > 64) tv = 64;
        if(tv == 0) continue; // muted track

        // base sample -> 16-bit-ish
        int32_t out = (smp << 8);

        // volume chain
        out = (out * v) / 64;
        out = (out * tv) / 64;
        out = (out * mv) / 64;

        if(chan[t & 3]) sumL += (out / 2);
        else            sumR += (out / 2);
    }

    // crude headroom to avoid constant clamp saturation
    sumL >>= 2;  // /4
    sumR >>= 2;

    *outL = clamp16(sumL);
    *outR = clamp16(sumR);
}

// ---------------- API ----------------
int playMED_Load(const char *filename, uint32_t outSampleRate){
    memset(&g_med, 0, sizeof(g_med));
    g_med.outRate = 44100;//outSampleRate ? outSampleRate : 44100;

    g_med.paulaClock = AMIGA_CLOCK_PAL;
    g_med.eclock     = AMIGA_ECLOCK_PAL;

    if(!load_entire_file(filename)) return 0;
    if(!parse_headers()) return 0;
    if(!load_samples()) return 0;

    int ok=0, bad=0;
    for(uint32_t i = 0; i < g_med.numSamples; i++){
        if(g_med.samples[i].valid) ok++; else bad++;
    }
    printf("MED samples: ok=%d bad=%d (numsamples=%u)\n", ok, bad, g_med.numSamples);

    reset_player();

    // Useful sanity check: at 44.1kHz and MOD-ish timing, this should be ~882
    printf("MED: flags2=%02X tempo=%u speed=%u lpb=%u spt=%u (samples=%u)\n",
           g_med.flags2, g_med.defTempo, g_med.ticksPerLine, g_med.rowsPerBeat,
           g_med.samplesPerTickFP, (g_med.samplesPerTickFP >> FP_SHIFT));


    rowTickState =888;
    rowstatic = 888;
    return 1;
}

void playMED_Free(void){
    // no malloc -> nothing to free
    memset(&g_med, 0, sizeof(g_med));
    g_med_filebuf_len = 0;
}

uint32_t RenderMED_Interleaved(int16_t *out, uint32_t frames)
{
    if(!out || frames == 0) return 0;

    if(!g_med.file){
        memset(out, 0, frames * 2u * sizeof(int16_t));
        return frames;
    }

    for(uint32_t i=0;i<frames;i++){
        g_med.tickAccFP += TICK_FP_ONE;

        while(g_med.tickAccFP >= g_med.samplesPerTickFP){
            g_med.tickAccFP -= g_med.samplesPerTickFP;
            player_tick_advance();
        }

        int16_t L=0, R=0;
        mix_one(&L, &R);

        // headroom
        //L >>= 1;
        //R >>= 1;

        // mono time for test
        int16_t ff1 = (int16_t)(((int32_t)(L>>1) + (int32_t)R));;
        int16_t ff2 = (int16_t)(((int32_t)L + (int32_t)(R>>1)));;
        //ff = (L + R)>>1;
        ///

        out[i*2u + 0] = ff1;
        out[i*2u + 1] = ff2;
        //out[i*2u + 0] = L;
        //out[i*2u + 1] = R;
    }

    return frames;
}
