/***************************************************************************

    Electronscape Audio GYM System FOr MegaDrive sound
    vgmplay.c

 ***************************************************************************/

#include "../../main.h"
#include <stdio.h>
#include <string.h>

#include "vgmplay.h"
#include "sn76496.h"
#include "ym2612.h"

uint8_t songmemory[(1024 * 1024) * 4];
uint8_t *psongmem;

void fseekmem(uint32_t addr){
    psongmem = &songmemory[addr];
}

#define true 1
#define false 0

#define uint8 unsigned char
//#define byte unsigned char

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

FILE *vgm;

//Song Data Variables
#define MAX_PCM_BUFFER_SIZE 44100 //In bytes
uint8_t pcmBuffer[MAX_PCM_BUFFER_SIZE];
uint32_t pcmBufferPosition = 0;

//File Stream
#define MAX_CMD_BUFFER 512
char cmdBuffer[MAX_CMD_BUFFER];
uint32_t bufferPos = 0;

uint32_t waitSamples = 0;
uint32_t loopOffset = 0;
uint16_t loopCount = 0;

unsigned char cmd;

int wait;



///////////////// trace ///////////////////////////////
// ---- poke trace (per audio chunk) ----
#define TRACE_BYTES_PER_CH 32

typedef struct {
    uint8_t n;
    uint8_t b[TRACE_BYTES_PER_CH];
} TraceBuf;

///////////////// trace ///////////////////////////////
///////////////// trace ///////////////////////////////
// Keep last N bytes per channel (trail)
#define TRAIL_BYTES  8   // <= keep this small for narrow output

typedef struct {
    uint8_t head;              // next write index
    uint8_t count;             // how many valid bytes (<=TRAIL_BYTES)
    uint8_t add_this_chunk;    // how many bytes were added since last clear
    uint8_t b[TRAIL_BYTES];
} TrailBuf;

static TrailBuf tr_fm[6];   // YM2612 channels 0..5
static TrailBuf tr_psg[4];  // PSG channels 0..3

static uint8_t psg_latch_ch = 0;   // 0..3
static uint8_t psg_latch_isvol = 0;

static inline void trace_clear(void)
{
    for (int i = 0; i < 6; i++) tr_fm[i].add_this_chunk = 0;
    for (int i = 0; i < 4; i++) tr_psg[i].add_this_chunk = 0;
}

static inline void trail_push(TrailBuf *t, uint8_t v)
{
    t->b[t->head] = v;
    t->head = (uint8_t)((t->head + 1u) % TRAIL_BYTES);
    if (t->count < TRAIL_BYTES) t->count++;
    t->add_this_chunk++;
}

static inline int ym_ch_from_reg(int port, uint8_t reg)
{
    int ch = (reg & 3);
    if (ch == 3) return -1;
    if (port) ch += 3;
    return ch;
}

static inline void trace_ym_write(int port, uint8_t reg, uint8_t data)
{
    int ch = ym_ch_from_reg(port, reg);
    if (ch < 0) return;

    // store reg,data so you see actual pokes
    trail_push(&tr_fm[ch], reg);
    trail_push(&tr_fm[ch], data);
}

static inline void trace_psg_write(uint8_t data)
{
    if (data & 0x80) {
        psg_latch_ch    = (data >> 5) & 3;
        psg_latch_isvol = (data >> 4) & 1;
        (void)psg_latch_isvol;
    }
    trail_push(&tr_psg[psg_latch_ch], data);
}

// write 2 hex chars into out (no snprintf)
static inline void put_hex2(char *out, int *pos, int max, uint8_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    if (*pos + 2 >= max) return;
    out[(*pos)++] = hx[(v >> 4) & 0xF];
    out[(*pos)++] = hx[(v >> 0) & 0xF];
}

static inline void put_ch(char *out, int *pos, int max, char c)
{
    if (*pos + 1 >= max) return;
    out[(*pos)++] = c;
}

// Print last TRAIL_BYTES bytes from ring, oldest->newest, no spaces.
// If not full, pads with '.' so it stays same width.
static void trail_dump_bytes(char *out, int *pos, int max, const TrailBuf *t)
{
    int pad = (int)TRAIL_BYTES - (int)t->count;

    // left pad (older missing)
    for (int i = 0; i < pad; i++) {
        put_ch(out, pos, max, '.');
        put_ch(out, pos, max, '.');
    }

    // oldest index = head - count
    int start = (int)t->head - (int)t->count;
    while (start < 0) start += TRAIL_BYTES;

    for (int i = 0; i < (int)t->count; i++) {
        uint8_t v = t->b[(start + i) % TRAIL_BYTES];
        put_hex2(out, pos, max, v);
    }
}

// Compact fixed-ish width line:
// F0+2:11223344 | F1+0:........ | ... | P0+1:AA......
static void trace_format_line(char *out, int outsz)
{
    int pos = 0;
    if (!out || outsz < 32) return;

    for (int ch = 0; ch < 6; ch++) {
        put_ch(out, &pos, outsz, 'F');
        put_ch(out, &pos, outsz, (char)('0' + ch));
        put_ch(out, &pos, outsz, '+');
        // single hex-ish digit for “new bytes this chunk” (caps at 9)
        uint8_t a = tr_fm[ch].add_this_chunk;
        if (a > 9) a = 9;
        put_ch(out, &pos, outsz, (char)('0' + a));
        put_ch(out, &pos, outsz, ':');

        trail_dump_bytes(out, &pos, outsz, &tr_fm[ch]);

        if (ch != 5) { put_ch(out, &pos, outsz, ' '); put_ch(out, &pos, outsz, '|'); put_ch(out, &pos, outsz, ' '); }
    }

    put_ch(out, &pos, outsz, ' ');
    put_ch(out, &pos, outsz, '|');
    put_ch(out, &pos, outsz, ' ');

    for (int ch = 0; ch < 4; ch++) {
        put_ch(out, &pos, outsz, 'P');
        put_ch(out, &pos, outsz, (char)('0' + ch));
        put_ch(out, &pos, outsz, '+');
        uint8_t a = tr_psg[ch].add_this_chunk;
        if (a > 9) a = 9;
        put_ch(out, &pos, outsz, (char)('0' + a));
        put_ch(out, &pos, outsz, ':');

        trail_dump_bytes(out, &pos, outsz, &tr_psg[ch]);

        if (ch != 3) { put_ch(out, &pos, outsz, ' '); put_ch(out, &pos, outsz, '|'); put_ch(out, &pos, outsz, ' '); }
    }

    put_ch(out, &pos, outsz, '\n');
    out[pos] = 0;
}




void freadmem(uint8_t *mem, uint32_t len){
    int i;
    for(i = 0; i < len; i ++){
        *(mem++) = *(psongmem++);
    }
}



void FillBuffer() {
    //vgm.readBytes(cmdBuffer, MAX_CMD_BUFFER);
    //fread(cmdBuffer, 1, MAX_CMD_BUFFER, vgm);
    freadmem(cmdBuffer, MAX_CMD_BUFFER);
}

unsigned char GetByte() {
    if(bufferPos == MAX_CMD_BUFFER) {
        bufferPos = 0;
        FillBuffer();
    }
    return cmdBuffer[bufferPos++];
}


void ClearBuffers() {
    int i;
    pcmBufferPosition = 0;
    bufferPos = 0;
    for(i = 0; i < MAX_CMD_BUFFER; i++)
        cmdBuffer[i] = 0;
    for(i = 0; i < MAX_PCM_BUFFER_SIZE; i++)
        pcmBuffer[i] = 0;
}



static uint32_t parse_uint32_le(const uint8_t *data) {
    return (uint32_t)(data[0])
    | (uint32_t)(data[1]) << 8
        | (uint32_t)(data[2]) << 16
        | (uint32_t)(data[3]) << 24;
}

static uint32_t parse_uint16_le(const uint8_t *data) {
    return (uint16_t)(data[0])
    | (uint16_t)(data[1]) << 8;
}


void openVgmFile(unsigned char *filename) {
    int i, cb;
    unsigned char b[2];
    vgm = fopen(filename, "rb");

    psongmem = songmemory;
    ////fread(cmdBuffer, 1, MAX_CMD_BUFFER, vgm);
    fread(songmemory, 1, (1024 * 10124 * 4),  vgm);

    waitSamples = 0;
    loopOffset = 0;
    loopCount = 0;

    FillBuffer();
    for(i = bufferPos; i<0x17; i++) GetByte(); //Ignore the unimportant VGM header data

    for (i = bufferPos; i < 0x1B; i++ ) { //0x18->0x1B : Get wait Samples count
        waitSamples += (uint32_t)(GetByte()) << ( 8 * i );
    }

    for ( i = bufferPos; i < 0x1F; i++ ) { //0x1C->0x1F : Get loop offset Postition
        loopOffset += (uint32_t)(GetByte()) << ( 8 * i );
    }
    for ( i = bufferPos; i < 0x40; i++ ) GetByte(); //Go to VGM data start

    printf("Init YM2612...\r\n");
    YM2612Init();
    printf("Configuring...\r\n");
    YM2612Config(2);
    printf("Reset.\r\n");
    YM2612ResetChip();
    printf("Go!\r\n");

    printf("loop offset: 0x%X\n", loopOffset - 0x1C);

    SN76496Init(0, 3579545, 0, 44100);
    //void SN76496Write(int chip, int data);
    //void SN76496Update(int chip, uint16 *buffer, int length);
}



void process_command(void)
{
    unsigned char address, data;

    cmd = GetByte();

    // IMPORTANT: most commands do NOT consume time.
    // Only wait commands change time. So reset every command.
    wait = 0;

    switch(cmd)
    {
    case 0x4F:
        data = GetByte();
        SN76496Write(0, 0x06);
        SN76496Write(0, data);
        break;

    case 0x50:
        data = GetByte();
        SN76496Write(0, data);
        trace_psg_write((uint8_t)data);

        break;

    case 0x52:
        address = GetByte();
        data    = GetByte();
        YM2612Write(0, address);
        YM2612Write(1, data);
        trace_ym_write(0, (uint8_t)address, (uint8_t)data);

        break;

    case 0x53:
        address = GetByte();
        data    = GetByte();
        YM2612Write(2, address);
        YM2612Write(3, data);
        trace_ym_write(1, (uint8_t)address, (uint8_t)data);

        break;

    case 0x61: {
        // wait n samples (16-bit little endian)
        uint32_t n = (uint32_t)GetByte();
        n |= (uint32_t)GetByte() << 8;
        wait = (int)n;
        break;
    }

    case 0x62:
        wait = 735;    // 1/60 sec at 44.1k
        break;

    case 0x63:
        wait = 882;    // 1/50 sec at 44.1k
        break;

    case 0x70: case 0x71: case 0x72: case 0x73:
    case 0x74: case 0x75: case 0x76: case 0x77:
    case 0x78: case 0x79: case 0x7A: case 0x7B:
    case 0x7C: case 0x7D: case 0x7E: case 0x7F:
        // VGM spec: 0x70-0x7F = wait (n+1) samples
        wait = (cmd & 0x0F) + 1;
        break;

    case 0x80: case 0x81: case 0x82: case 0x83:
    case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x88: case 0x89: case 0x8A: case 0x8B:
    case 0x8C: case 0x8D: case 0x8E: case 0x8F:
        // DAC write + wait n samples (low nibble)
        YM2612Write(0, 0x2A);
        YM2612Write(1, pcmBuffer[pcmBufferPosition++]);
        wait = (cmd & 0x0F);
        break;

    case 0x67: {
        GetByte(); // 0x66
        GetByte(); // data type
        pcmBufferPosition = bufferPos;
        uint32_t PCMdataSize = 0;
        PCMdataSize |= (uint32_t)GetByte();
        PCMdataSize |= (uint32_t)GetByte() << 8;
        PCMdataSize |= (uint32_t)GetByte() << 16;
        PCMdataSize |= (uint32_t)GetByte() << 24;

        for (uint32_t i = 0; i < PCMdataSize; i++) {
            uint8_t b = (uint8_t)GetByte();
            if (i < MAX_PCM_BUFFER_SIZE) pcmBuffer[i] = b;
        }
        break;
    }

    case 0xE0: {
        uint32_t pos = 0;
        pos |= (uint32_t)GetByte();
        pos |= (uint32_t)GetByte() << 8;
        pos |= (uint32_t)GetByte() << 16;
        pos |= (uint32_t)GetByte() << 24;
        pcmBufferPosition = pos;
        break;
    }

    case 0x66:
        if(loopOffset == 0) loopOffset = 64;
        loopCount++;
        //fseek(vgm, loopOffset - 0x1c, SEEK_SET);
        fseekmem(loopOffset - 0x1c);
        FillBuffer();
        printf("Song Looped: 0x%08X\n", loopOffset - 0x1c);
        bufferPos = 0;
        break;

    default:
        // unknown/unhandled command: leave wait=0
        break;
    }
}






void stepVGM(int32_t *sndbuffer, int frames){
    static int pending = 0;

    // temp mono buffer for PSG (SN76496) output
    // period is 882 in your setup, so 2048 is plenty
    static int16_t sn_mono[1024];

    // clear stereo interleaved output (L,R,L,R...)
    memset(sndbuffer, 0, (size_t)frames * 2u * sizeof(int32_t));


    int32_t *p = sndbuffer;


    char outputstringline[2048];


    int8_t th5 = 0;
    while (frames > 0)
    {
        trace_clear();
        while (pending <= 0)
        {
            process_command();
            pending = wait;
        }

        int n = (pending < frames) ? pending : frames;
        if (n > (int)(sizeof(sn_mono) / sizeof(sn_mono[0])))
            n = (int)(sizeof(sn_mono) / sizeof(sn_mono[0]));  // safety clamp

        // --- SN76496: produces mono 16-bit samples ---
        memset(sn_mono, 0, (size_t)n * sizeof(int16_t));
        SN76496Update(0, sn_mono, n);

        // add mono into stereo mix buffer
        for (int i = 0; i < n; i++) {
            int32_t s = (int32_t)sn_mono[i];
            p[i*2 + 0] += s;
            p[i*2 + 1] += s;
        }

        // --- YM2612: produces stereo and ADDS into p ---
        YM2612Update(p, n);
        // print the pokes that happened during the command processing for this chunk

        p       += n * 2;   // stereo
        frames  -= n;
        pending -= n;
    }
    //trace_format_line(outputstringline, (int)sizeof(outputstringline));
    //printf("%s", outputstringline);

}




uint32_t RenderGYMBlock(int16_t *left, int16_t *right, uint32_t frames){
    // TODO: this would be rendered here, and then passed through left and right

    return frames;
}
