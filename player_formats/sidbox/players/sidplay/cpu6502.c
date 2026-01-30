#include <stdio.h>
#include <stdint.h>

// Ensure types are defined if headers don't provide them
typedef uint8_t byte;
typedef uint16_t word;

#include "cpu6502.h"
#include "vic.h"
#include "bus.h"
#include "cia.h"
#include "sid8579.h"

// --- Flags ---
#define sFLAG_N 128
#define sFLAG_V 64
#define sFLAG_B 16
#define sFLAG_D 8
#define sFLAG_I 4
#define sFLAG_Z 2
#define sFLAG_C 1

// --- Opcode Enum ---
__attribute__((const))
const enum {
    op_adc, op_and, op_asl, op_bcc, op_bcs, op_beq, op_bit, op_bmi, op_bne, op_bpl, op_brk, op_bvc, op_bvs, op_clc,
    op_cld, op_cli, op_clv, op_cmp, op_cpx, op_cpy, op_dec, op_dex, op_dey, op_eor, op_inc, op_inx, op_iny, op_jmp,
    op_jsr, op_lda, op_ldx, op_ldy, op_lsr, op_nop, op_ora, op_pha, op_php, op_pla, op_plp, op_rol, op_ror, op_rti,
    op_rts, op_sbc, op_sec, op_sed, op_sei, op_sta, op_stx, op_sty, op_tax, op_tay, op_tsx, op_txa, op_txs, op_tya,
    op_xxx
};

// --- Addressing Modes ---
#define op_imp  0
#define op_imm  1
#define op_abs  2
#define op_absx 3
#define op_absy 4
#define op_zp   6
#define op_zpx  7
#define op_zpy  8
#define op_ind  9
#define op_indx 10
#define op_indy 11
#define op_acc  12
#define op_rel  13

// --- Tables ---
static byte cur_opc;

// Defines which instructions suffer a penalty cycle on page crossing (Reads=Yes, Stores=No)
static const uint8_t page_cross_ok[256] = {
    /* 00 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 10 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0,
    /* 20 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 30 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0,
    /* 40 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 50 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0,
    /* 60 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 70 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0,
    /* 80 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* 90 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* A0 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* B0 */  0, 1, 0, 0, 1,    1,    1,    0,    0,    1,    0,    0,    1,    1,    1,    0,
    /* C0 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* D0 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0,
    /* E0 */  0, 0, 0, 0, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /* F0 */  0, 1, 0, 0, 0,    0,    0,    0,    0,    1,    0,    0,    0,    1,    0,    0
};

static const uint8_t base_cycles[256] = {
    /* 0x00 */  7,6,2,2,3,3,5,2,3,2,2,2,4,4,6,2,
    /* 0x10 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2,
    /* 0x20 */  6,6,2,2,3,3,5,2,4,2,2,2,4,4,6,2,
    /* 0x30 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2,
    /* 0x40 */  6,6,2,2,3,3,5,2,3,2,2,2,3,4,6,2,
    /* 0x50 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2,
    /* 0x60 */  6,6,2,2,3,3,5,2,4,2,2,2,5,4,6,2,
    /* 0x70 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2,
    /* 0x80 */  2,6,2,2,3,3,3,2,2,2,2,2,4,4,4,2,
    /* 0x90 */  2,6,2,2,4,4,4,2,2,5,2,2,4,5,5,2,
    /* 0xxA0 */ 2,6,2,2,3,3,3,2,2,2,2,2,4,4,4,2,
    /* 0xB0 */  2,5,2,2,4,4,4,2,2,4,2,2,4,4,4,2,
    /* 0xC0 */  2,6,2,2,3,3,5,2,2,2,2,2,4,4,6,2,
    /* 0xD0 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2,
    /* 0xE0 */  2,6,2,2,3,3,5,2,2,2,2,2,4,4,6,2,
    /* 0xF0 */  2,5,2,2,4,4,6,2,2,4,2,2,4,4,7,2
};

static const byte opcodes[256] = {
    op_brk, op_ora, op_xxx, op_xxx, op_xxx, op_ora, op_asl, op_xxx, op_php, op_ora, op_asl, op_xxx, op_xxx, op_ora, op_asl, op_xxx,
    op_bpl, op_ora, op_xxx, op_xxx, op_xxx, op_ora, op_asl, op_xxx, op_clc, op_ora, op_xxx, op_xxx, op_xxx, op_ora, op_asl, op_xxx,
    op_jsr, op_and, op_xxx, op_xxx, op_bit, op_and, op_rol, op_xxx, op_plp, op_and, op_rol, op_xxx, op_bit, op_and, op_rol, op_xxx,
    op_bmi, op_and, op_xxx, op_xxx, op_xxx, op_and, op_rol, op_xxx, op_sec, op_and, op_xxx, op_xxx, op_xxx, op_and, op_rol, op_xxx,
    op_rti, op_eor, op_xxx, op_xxx, op_xxx, op_eor, op_lsr, op_xxx, op_pha, op_eor, op_lsr, op_xxx, op_jmp, op_eor, op_lsr, op_xxx,
    op_bvc, op_eor, op_xxx, op_xxx, op_xxx, op_eor, op_lsr, op_xxx, op_cli, op_eor, op_xxx, op_xxx, op_xxx, op_eor, op_lsr, op_xxx,
    op_rts, op_adc, op_xxx, op_xxx, op_xxx, op_adc, op_ror, op_xxx, op_pla, op_adc, op_ror, op_xxx, op_jmp, op_adc, op_ror, op_xxx,
    op_bvs, op_adc, op_xxx, op_xxx, op_xxx, op_adc, op_ror, op_xxx, op_sei, op_adc, op_xxx, op_xxx, op_xxx, op_adc, op_ror, op_xxx,
    op_xxx, op_sta, op_xxx, op_xxx, op_sty, op_sta, op_stx, op_xxx, op_dey, op_xxx, op_txa, op_xxx, op_sty, op_sta, op_stx, op_xxx,
    op_bcc, op_sta, op_xxx, op_xxx, op_sty, op_sta, op_stx, op_xxx, op_tya, op_sta, op_txs, op_xxx, op_xxx, op_sta, op_xxx, op_xxx,
    op_ldy, op_lda, op_ldx, op_xxx, op_ldy, op_lda, op_ldx, op_xxx, op_tay, op_lda, op_tax, op_xxx, op_ldy, op_lda, op_ldx, op_xxx,
    op_bcs, op_lda, op_xxx, op_xxx, op_ldy, op_lda, op_ldx, op_xxx, op_clv, op_lda, op_tsx, op_xxx, op_ldy, op_lda, op_ldx, op_xxx,
    op_cpy, op_cmp, op_xxx, op_xxx, op_cpy, op_cmp, op_dec, op_xxx, op_iny, op_cmp, op_dex, op_xxx, op_cpy, op_cmp, op_dec, op_xxx,
    op_bne, op_cmp, op_xxx, op_xxx, op_xxx, op_cmp, op_dec, op_xxx, op_cld, op_cmp, op_xxx, op_xxx, op_xxx, op_cmp, op_dec, op_xxx,
    op_cpx, op_sbc, op_xxx, op_xxx, op_cpx, op_sbc, op_inc, op_xxx, op_inx, op_sbc, op_nop, op_xxx, op_cpx, op_sbc, op_inc, op_xxx,
    op_beq, op_sbc, op_xxx, op_xxx, op_xxx, op_sbc, op_inc, op_xxx, op_sed, op_sbc, op_xxx, op_xxx, op_xxx, op_sbc, op_inc, op_xxx
};

static const byte modes[256] = {
    op_imp, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_xxx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_xxx, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_abs, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_xxx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_xxx, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_imp, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_xxx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_xxx, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_imp, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_ind, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_xxx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_xxx, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_imm, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_zpx, op_zpx, op_zpy, op_xxx, op_imp, op_absy, op_acc, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_imm, op_indx, op_imm, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_zpx, op_zpx, op_zpy, op_xxx, op_imp, op_absy, op_acc, op_xxx, op_absx, op_absx, op_absy, op_xxx,
    op_imm, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_zpx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_acc, op_xxx, op_xxx, op_absx, op_absx, op_xxx,
    op_imm, op_indx, op_xxx, op_xxx, op_zp, op_zp, op_zp, op_xxx, op_imp, op_imm, op_acc, op_xxx, op_abs, op_abs, op_abs, op_xxx,
    op_rel, op_indy, op_xxx, op_xxx, op_zpx, op_zpx, op_zpx, op_xxx, op_imp, op_absy, op_acc, op_xxx, op_xxx, op_absx, op_absx, op_xxx
};

// --- CPU State ---
static int cycles;
static byte bval;
static word wval;
static byte a, x, y, s, p;
static word pc;

// Helper variables
static uint8_t last_page_cross = 0;

// --- Addressing Helper Functions ---

static byte getaddr(int mode) {
    word ad, ad2;
    last_page_cross = 0; // Reset flag

    switch(mode) {
    case op_imp:  return 0;
    case op_imm:  return bus_read8(pc++);
    case op_abs:  ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); return bus_read8(ad);
    case op_zp:   ad = bus_read8(pc++); return bus_read8(ad);
    case op_zpx:  ad = (word)(bus_read8(pc++) + x); return bus_read8(ad & 0xff);
    case op_zpy:  ad = (word)(bus_read8(pc++) + y); return bus_read8(ad & 0xff);

    case op_indx:
        ad = (word)(bus_read8(pc++) + x);
        ad2 = bus_read8(ad & 0xff);
        // Corrected index wrap behavior
        ad2 |= (word)(bus_read8((ad + 1) & 0xff) << 8);
        return bus_read8(ad2);

    case op_absx: {
        ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
        ad2 = (word)(ad + x);
        last_page_cross = (((ad ^ ad2) & 0xFF00) != 0);
        return bus_read8(ad2);
    }

    case op_absy: {
        ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
        ad2 = (word)(ad + y);
        last_page_cross = (((ad ^ ad2) & 0xFF00) != 0);
        return bus_read8(ad2);
    }

    case op_indy: {
        ad = bus_read8(pc++);
        // Pointer is in zero page, wrap arithmetic
        ad2 = bus_read8(ad);
        ad2 |= ((word)bus_read8((ad + 1) & 0xFF) << 8);
        word ea = (word)(ad2 + y);
        last_page_cross = (((ad2 ^ ea) & 0xFF00) != 0);
        return bus_read8(ea);
    }

    case op_acc:  return a;
    }
    return 0;
}

static void setaddr(int mode, byte val) {
    word ad, ad2;
    switch(mode) {
    case op_abs: {
        ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
        bus_write8(ad, val);
        break;
    }
    case op_absx: {
        ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
        ad2 = (word)(ad + x);
        bus_write8(ad2, val);
        break;
    }
    case op_absy: {
        ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
        ad2 = (word)(ad + y);
        bus_write8(ad2, val);
        break;
    }
    case op_zp: {
        ad = bus_read8(pc - 1);
        bus_write8(ad, val);
        break;
    }
    case op_zpx: {
        ad = (word)(bus_read8(pc - 1) + x);
        bus_write8(ad & 0xff, val);
        break;
    }
    case op_acc: {
        a = val;
        break;
    }
    default: break;
    }
}

static void putaddr(int mode, byte val){
    word ad, ad2;
    switch(mode){
    case op_abs:  ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); bus_write8(ad, val); return;
    case op_absx: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + x); bus_write8(ad2, val); return;
    case op_absy: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + y); bus_write8(ad2, val); return;
    case op_zp:   ad = bus_read8(pc++); bus_write8(ad, val); return;
    case op_zpx:  ad = (word)(bus_read8(pc++) + x); bus_write8(ad & 0xff, val); return;
    case op_zpy:  ad = (word)(bus_read8(pc++) + y); bus_write8(ad & 0xff, val); return;
    case op_indx:
        ad = (word)(bus_read8(pc++) + x);
        ad2 = bus_read8(ad & 0xff);
        ad++; // increment inside ZP
        ad2 |= (word)(bus_read8(ad & 0xff) << 8);
        bus_write8(ad2, val); return;
    case op_indy:
        ad = bus_read8(pc++);
        ad2 = bus_read8(ad);
        ad2 |= (word)(bus_read8((ad + 1) & 0xff) << 8);
        ad = (word)(ad2 + y);
        bus_write8(ad, val); return;
    case op_acc:  a = val; return;
    }
}

// --- Helper Macros/Functions ---

static inline void setflags(int flag, int cond){
    if(cond) p |= (byte)flag;
    else     p &= (byte)~flag;
}

static inline void push(byte val){
    bus_write8((word)(0x100 + s), val);
    s--;
}

static inline byte pop(void){
    s++;
    return bus_read8((word)(0x100 + s));
}

static void branch(int take){
    int8_t dist = (int8_t)bus_read8(pc++);
    word oldpc = pc;
    word newpc = (word)(pc + dist);

    if(take){
        cycles += 1;
        if((oldpc & 0xFF00) != (newpc & 0xFF00))
            cycles += 1;
        pc = newpc;
    }
}

#define APPLY_PAGE_CROSS() do { if (last_page_cross && page_cross_ok[cur_opc]) cycles += 1; } while(0)


// --- Interrupt Handling ---

void cpu_irq(void){
    push((byte)(pc >> 8));
    push((byte)(pc & 0xFF));

    byte P = p;
    P &= (byte)~sFLAG_B;
    P |= 0x20;
    push(P);

    p |= sFLAG_I;
    pc = bus_read16(0xFFFE);
}

void cpu_nmi(void){
    push((byte)(pc >> 8));
    push((byte)(pc & 0xFF));

    byte P = p;
    P &= (byte)~sFLAG_B;
    P |= 0x20;
    push(P);

    p |= sFLAG_I;
    pc = bus_read16(0xFFFA);
}

// --- Lifecycle ---

void cpuReset(void){
    cycles = 0;
    a = x = y = 0;
    p = (byte)(sFLAG_I | 0x20);
    s = 0xFD;
    pc = bus_read16(0xfffc);
}

void cpuResetTo(word npc){
    cycles = 0;
    a = x = y = 0;
    p = (byte)(sFLAG_I | 0x20);
    s = 0xFF;
    pc = npc;
}


// --- Main Execution Loop ---

int cpuStep(void){
    if (!pc && s == 0xFF) return 0; // Halted

    cycles = 0;
    int cycles_start = cycles;

    byte opc = bus_read8(pc++);
    cur_opc = opc;

    cycles += base_cycles[opc];

    int cmd  = opcodes[opc];
    int addr = modes[opc];
    int c_flag;

    switch(cmd){

    case op_adc:{
        byte m = getaddr(addr);
        APPLY_PAGE_CROSS();

        if (p & sFLAG_D) {
            // NMOS 6502 Decimal Mode
            uint16_t low = (a & 0x0F) + (m & 0x0F) + ((p & sFLAG_C) ? 1 : 0);
            uint16_t high = (a >> 4) + (m >> 4) + (low > 9);
            if (low > 9) low += 6;
            setflags(sFLAG_V, ((a ^ (high << 4)) & (m ^ (high << 4)) & 0x80));
            if (high > 9) high += 6;
            setflags(sFLAG_C, high > 15);
            a = (low & 0x0F) | ((high & 0x0F) << 4);
        } else {
            uint16_t sum = (uint16_t)a + (uint16_t)m + ((p & sFLAG_C) ? 1 : 0);
            setflags(sFLAG_V, (~(a ^ m) & (a ^ (byte)sum) & 0x80));
            setflags(sFLAG_C, sum > 0xFF);
            a = (byte)sum;
        }
        setflags(sFLAG_Z, !a);
        setflags(sFLAG_N, a & 0x80);
        break;
    }

    case op_sbc:{
        byte m = getaddr(addr);
        APPLY_PAGE_CROSS();

        if (p & sFLAG_D) {
            // NMOS 6502 Decimal Mode SBC
            uint16_t low = (a & 0x0F) - (m & 0x0F) - ((p & sFLAG_C) ? 0 : 1);
            uint16_t high = (a >> 4) - (m >> 4) - ((low & 0x10) ? 1 : 0);

            if (low & 0x10) low -= 6;
            if (high & 0x10) high -= 6;

            // Flags are typically calculated on the binary result for NMOS
            uint16_t diff = (uint16_t)a - (uint16_t)m - ((p & sFLAG_C) ? 0 : 1);
            setflags(sFLAG_V, ((a ^ m) & (a ^ (byte)diff) & 0x80));
            setflags(sFLAG_C, diff < 0x100);

            a = (low & 0x0F) | ((high & 0x0F) << 4);
        } else {
            // Normal Binary SBC
            uint16_t diff = (uint16_t)a - (uint16_t)m - ((p & sFLAG_C) ? 0 : 1);
            setflags(sFLAG_V, ((a ^ m) & (a ^ (byte)diff) & 0x80));
            setflags(sFLAG_C, diff < 0x100);
            a = (byte)diff;
        }
        setflags(sFLAG_Z, !a);
        setflags(sFLAG_N, a & 0x80);
        break;
    }

    case op_and:
        bval = getaddr(addr); a &= bval;
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_asl:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        wval = (word)bval << 1;
        setaddr(addr, (byte)wval);
        setflags(sFLAG_Z, !(byte)wval);
        setflags(sFLAG_N, (byte)wval & 0x80);
        setflags(sFLAG_C, wval & 0x100);
        break;

    case op_bcc: branch(!(p & sFLAG_C)); break;
    case op_bcs: branch( (p & sFLAG_C)); break;
    case op_bne: branch(!(p & sFLAG_Z)); break;
    case op_beq: branch( (p & sFLAG_Z)); break;
    case op_bpl: branch(!(p & sFLAG_N)); break;
    case op_bmi: branch( (p & sFLAG_N)); break;
    case op_bvc: branch(!(p & sFLAG_V)); break;
    case op_bvs: branch( (p & sFLAG_V)); break;

    case op_bit:
        bval = getaddr(addr);
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !(a & bval));
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_V, bval & 0x40);
        break;

    case op_brk:
        pc++; // Padding byte
        push((byte)(pc >> 8));
        push((byte)(pc & 0xFF));
        push((byte)(p | sFLAG_B | 0x20));
        p |= sFLAG_I;
        pc = bus_read16(0xFFFE);
        break;

    case op_clc: setflags(sFLAG_C, 0); break;
    case op_cld: setflags(sFLAG_D, 0); break;
    case op_cli: setflags(sFLAG_I, 0); break;
    case op_clv: setflags(sFLAG_V, 0); break;

    case op_cmp:
        bval = getaddr(addr);
        APPLY_PAGE_CROSS();
        wval = (word)((uint16_t)a - bval);
        setflags(sFLAG_Z, !(wval & 0xFF));
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, a >= bval);
        break;

    case op_cpx:
        bval = getaddr(addr);
        APPLY_PAGE_CROSS();
        wval = (word)((uint16_t)x - bval);
        setflags(sFLAG_Z, !(wval & 0xFF));
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, x >= bval);
        break;

    case op_cpy:
        bval = getaddr(addr);
        APPLY_PAGE_CROSS();
        wval = (word)((uint16_t)y - bval);
        setflags(sFLAG_Z, !(wval & 0xFF));
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, y >= bval);
        break;

    case op_dec:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        bval--;
        setaddr(addr, bval);
        setflags(sFLAG_Z, !bval);
        setflags(sFLAG_N, bval & 0x80);
        break;

    case op_dex:
        x--; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
        break;

    case op_dey:
        y--; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
        break;

    case op_eor:
        bval = getaddr(addr); a ^= bval;
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_inc:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        bval++;
        setaddr(addr, bval);
        setflags(sFLAG_Z, !bval);
        setflags(sFLAG_N, bval & 0x80);
        break;

    case op_inx:
        x++; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
        break;

    case op_iny:
        y++; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
        break;

    case op_jmp:
        wval = bus_read8(pc++);
        wval |= (word)(bus_read8(pc++) << 8);
        if(addr == op_abs){
            pc = wval;
        } else {
            // Re-applying NMOS Page Wrap bug: JMP ($xxFF) reads high byte from $xx00
            pc = bus_read8(wval) | (bus_read8((wval & 0xFF00) | ((wval + 1) & 0xFF)) << 8);
        }
        break;

    case op_jsr:
    {
        word t_addr = bus_read8(pc++);
        // Push address of the last byte of instruction (pc-1 + 1)
        push((byte)(pc >> 8));
        push((byte)(pc & 0xFF));
        t_addr |= (word)(bus_read8(pc) << 8);
        pc = t_addr;
    }
    break;

    case op_lda:
        a = getaddr(addr);
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_ldx:
        x = getaddr(addr);
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
        break;

    case op_ldy:
        y = getaddr(addr);
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
        break;

    case op_lsr:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        setflags(sFLAG_C, bval & 1);
        wval = bval >> 1;
        setaddr(addr, (byte)wval);
        setflags(sFLAG_Z, !(byte)wval);
        setflags(sFLAG_N, 0);
        break;

    case op_nop:
        break;

    case op_ora:
        bval = getaddr(addr); a |= bval;
        APPLY_PAGE_CROSS();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_pha: push(a); break;
    case op_php: push((byte)(p | sFLAG_B | 0x20)); break;

    case op_pla:
        a = pop();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_plp:
        p = pop();
        p |= 0x20;
        p &= ~sFLAG_B;
        break;

    case op_rol:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        c_flag = !!(p & sFLAG_C);
        setflags(sFLAG_C, bval & 0x80);
        bval = (byte)((bval << 1) | c_flag);
        setaddr(addr, bval);
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_Z, !bval);
        break;

    case op_ror:
        bval = getaddr(addr);
        if (addr != op_acc) setaddr(addr, bval); // Dummy write
        c_flag = !!(p & sFLAG_C);
        setflags(sFLAG_C, bval & 1);
        bval = (byte)((bval >> 1) | (c_flag << 7));
        setaddr(addr, bval);
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_Z, !bval);
        break;

    case op_rti:
        p = pop();
        p |= 0x20;
        p &= ~sFLAG_B;
        wval = pop();
        wval |= (word)(pop() << 8);
        pc = wval;
        break;

    case op_rts:
        wval = pop();
        wval |= (word)(pop() << 8);
        pc = (word)(wval + 1);
        break;

    case op_sec: setflags(sFLAG_C, 1); break;
    case op_sed: setflags(sFLAG_D, 1); break;
    case op_sei: setflags(sFLAG_I, 1); break;

    case op_sta: putaddr(addr, a); break;
    case op_stx: putaddr(addr, x); break;
    case op_sty: putaddr(addr, y); break;

    case op_tax: x = a; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80); break;
    case op_tay: y = a; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80); break;
    case op_tsx: x = s; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80); break;
    case op_txa: a = x; setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80); break;
    case op_txs: s = x; break;
    case op_tya: a = y; setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80); break;

    default:
        break;
    }

    return cycles;
}

uint8_t getIFlagStatus(){
    return (p & sFLAG_I);
}

int cpuGetPC(void) { return pc; }
void cpuSetPC(word npc) { pc = npc; }

// Debug helper
void cpu_force_cli(void){
    p &= (byte)~sFLAG_I;
}

// External Access helpers
void cpu_set_regs(byte A, byte X, byte Y) { a = A; x = X; y = Y; }
void cpu_set_a(byte A) { a = A; }
byte cpu_get_a(void) { return a; }
void cpu_set_sp(byte S) { s = S; }
void cpu_push_byte(byte val) { push(val); }


//void cpu_set_regs(byte A, byte X, byte Y) { a = A; x = X; y = Y; }
void cpu_set_x(byte X) { x = X; }
void cpu_set_y(byte Y) { y = Y; }
void cpu_set_p(byte P) { p = P; }
void cpu_set_s(byte S) { s = S; }

byte cpu_get_x(void) { return x; }
byte cpu_get_y(void) { return y; }
byte cpu_get_p(void) { return p; }
byte cpu_get_s(void) { return s; }


// --- Execution Wrappers ---

int cpu_call_jsr(word target){
    int total = 0;
    uint16_t saved_pc = pc;

    push(0xFF); // high
    push(0xFF); // low

    pc = target;

    while (pc){
        total += cpuStep();
    }

    pc = saved_pc;
    return total;
}

int cpu_call_jsr_resetting(word npc, byte na){
    int total = 0;
    cycles = 0;

    a = na;
    x = 0;
    y = 0;
    p = (byte)(0x20 | sFLAG_I);
    s = 0xFF;
    pc = npc;

    push(0xFF);
    push(0xFF);

    while(pc){
        total += cpuStep();
    }
    return total;
}
