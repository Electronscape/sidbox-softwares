#include <stdio.h>

#include "cpu6502.h"
#include "vic.h"
#include "bus.h"
#include "cia.h"

// flags (match your sidplay.h style)
#define sFLAG_N 128
#define sFLAG_V 64
#define sFLAG_B 16
#define sFLAG_D 8
#define sFLAG_I 4
#define sFLAG_Z 2
#define sFLAG_C 1

// --- Opcode enum (yours) ---
__attribute__((const))
const enum {
    op_adc, op_and, op_asl, op_bcc, op_bcs, op_beq, op_bit, op_bmi, op_bne, op_bpl, op_brk, op_bvc, op_bvs, op_clc,
    op_cld, op_cli, op_clv, op_cmp, op_cpx, op_cpy, op_dec, op_dex, op_dey, op_eor, op_inc, op_inx, op_iny, op_jmp,
    op_jsr, op_lda, op_ldx, op_ldy, op_lsr, op_nop, op_ora, op_pha, op_php, op_pla, op_plp, op_rol, op_ror, op_rti,
    op_rts, op_sbc, op_sec, op_sed, op_sei, op_sta, op_stx, op_sty, op_tax, op_tay, op_tsx, op_txa, op_txs, op_tya,
    op_xxx
};

// addressing modes (yours)
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

// opcode tables (your pasted ones)
static byte cur_opc;

static const uint8_t page_cross_ok[256] = {
    // 1 only for opcodes that get +1 cycle on page cross
    // (loads/ALU reads + some compares), 0 otherwise.
    // I'll give you a safe starter mask below.
#if(0)
    /* 00-0F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 10-1F */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* 20-2F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 30-3F */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* 40-4F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 50-5F */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* 60-6F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 70-7F */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* 80-8F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 90-9F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* A0-AF */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* B0-BF */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* C0-CF */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* D0-DF */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,
    /* E0-EF */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* F0-FF */ 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0
#endif
    [0x11]=1, [0x19]=1, [0x1D]=1, // ORA (ind),Y / abs,Y / abs,X
    [0x31]=1, [0x39]=1, [0x3D]=1, // AND
    [0x51]=1, [0x59]=1, [0x5D]=1, // EOR
    [0x71]=1, [0x79]=1, [0x7D]=1, // ADC

    [0xB1]=1, [0xB9]=1, [0xBD]=1, // LDA
    [0xBC]=1,                     // LDY abs,X  <-- missing
    [0xBE]=1,                     // LDX abs,Y  <-- missing

    [0xD1]=1, [0xD9]=1, [0xDD]=1, // CMP
    [0xF1]=1, [0xF9]=1, [0xFD]=1, // SBC
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
    /* 0xA0 */  2,6,2,2,3,3,3,2,2,2,2,2,4,4,4,2,
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

// CPU state
static int cycles;
static byte bval;
static word wval;

static byte a, x, y, s, p;
static word pc;


// addressing helpers
static byte getaddr(int mode){
    word ad, ad2;
    switch(mode){
    case op_imp:  return 0;
    case op_imm:  return bus_read8(pc++);
    case op_abs:  ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); return bus_read8(ad);
    //case op_absx: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + x); if((ad2 & 0xff00) != (ad & 0xff00)) cycles++; return bus_read8(ad2);
    //case op_absy: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + y); if((ad2 & 0xff00) != (ad & 0xff00)) cycles++; return bus_read8(ad2);
    case op_zp:   ad = bus_read8(pc++); return bus_read8(ad);
    case op_zpx:  ad = (word)(bus_read8(pc++) + x); return bus_read8(ad & 0xff);
    case op_zpy:  ad = (word)(bus_read8(pc++) + y); return bus_read8(ad & 0xff);
    case op_indx: ad = (word)(bus_read8(pc++) + x); ad2 = bus_read8(ad & 0xff); ad++; ad2 |= (word)(bus_read8(ad & 0xff) << 8); return bus_read8(ad2);
    //case op_indy: ad = bus_read8(pc++); ad2 = bus_read8(ad); ad2 |= (word)(bus_read8((ad + 1) & 0xff) << 8); ad = (word)(ad2 + y); if((ad2 & 0xff00) != (ad & 0xff00)) cycles++; return bus_read8(ad);
    case op_acc:  return a;

    case op_absx:
        ad  = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
        ad2 = (word)(ad + x);
        if (((ad2 ^ ad) & 0xFF00) && page_cross_ok[cur_opc]) cycles++;
        return bus_read8(ad2);

    case op_absy:
        ad  = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
        ad2 = (word)(ad + y);
        if (((ad2 ^ ad) & 0xFF00) && page_cross_ok[cur_opc]) cycles++;
        return bus_read8(ad2);

    case op_indy: { // (ohh yeah bracket around this, declared tmp var)
        ad  = bus_read8(pc++);
        ad2 = bus_read8(ad) | ((word)bus_read8((ad + 1) & 0xFF) << 8);
        word ea = (word)(ad2 + y);
        if (((ea ^ ad2) & 0xFF00) && page_cross_ok[cur_opc]) cycles++;
        return bus_read8(ea);
    }


    }
    return 0;
}

static void setaddr(int mode, byte val){
    word ad, ad2;
    switch(mode){
    case op_abs:
        ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
        bus_write8(ad, val); return;
    case op_absx:
        ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
        ad2 = (word)(ad + x);
        bus_write8(ad2, val); return;
    case op_zp:
        ad = bus_read8(pc - 1); bus_write8(ad, val); return;
    case op_zpx:
        ad = (word)(bus_read8(pc - 1) + x); bus_write8(ad & 0xff, val); return;
    case op_acc:
        a = val; return;
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
    case op_indx: ad = (word)(bus_read8(pc++) + x); ad2 = bus_read8(ad & 0xff); ad++; ad2 |= (word)(bus_read8(ad & 0xff) << 8); bus_write8(ad2, val); return;
    case op_indy: ad = bus_read8(pc++); ad2 = bus_read8(ad); ad2 |= (word)(bus_read8((ad + 1) & 0xff) << 8); ad = (word)(ad2 + y); bus_write8(ad, val); return;
    case op_acc:  a = val; return;
    }
}

static inline void setflags(int flag, int cond){
    if(cond) p |= (byte)flag;
    else     p &= (byte)~flag;
}
static inline void push(byte val){
    bus_write8((word)(0x100 + s), val);
    if(s) s--;
}
static inline byte pop(void){
    if(s < 0xff) s++;
    return bus_read8((word)(0x100 + s));
}

static void branch(int take){
    int8_t dist = (int8_t)bus_read8(pc++);   // branch offset byte
    word oldpc = pc;
    word newpc = (word)(pc + dist);

    if(take){
        cycles += 1;                         // branch taken
        if((oldpc & 0xFF00) != (newpc & 0xFF00))
            cycles += 1;                     // page crossed
        pc = newpc;
    }
}
void cpuReset(void){
    cycles = 0;
    a = x = y = 0;
    //p = 0;
    p = (byte)(sFLAG_I | 0x20);

    s = 255;
    pc = bus_read16(0xfffc);
}
void cpuResetTo(word npc){
    cycles = 0;

    a = x = y = 0;
    p = 0;
    s = 255;
    pc = npc;
}


// additional stuff mainly used for RSID
void cpu_irq(void){
    cycles += 7;

    // all the other hardbits need 7 cycle updates too
    vic_step(7);    // THIS is here, but soon will be the CIA timers too
    cia_step_all(7);


    push((byte)(pc >> 8));
    push((byte)(pc & 0xFF));

    byte P = p;
    P &= (byte)~sFLAG_B;
    P |= 0x20;
    push(P);

    p |= sFLAG_I;
    pc = bus_read16(0xFFFE);
    //putchar('I');  // prove IRQ taken

}


/// VIC interrupt needs;



// Execute ONE opcode, return cycles consumed by that opcode.
// Returns 0 if CPU is halted (pc==0).
int cpuStep(void){
    if (!pc) return 0;

    int cycles_before = cycles;



    byte opc = bus_read8(pc++);
    cur_opc = opc;

    cycles += base_cycles[opc];

    int cmd  = opcodes[opc];
    int addr = modes[opc];
    int c;

    switch(cmd){
    case op_adc:{
        byte m = getaddr(addr);
        uint16_t sum = (uint16_t)a + (uint16_t)m + ((p & sFLAG_C) ? 1 : 0);

        setflags(sFLAG_C, sum & 0x100);
        setflags(sFLAG_V, (~(a ^ m) & (a ^ (byte)sum) & 0x80));

        a = (byte)sum;
        setflags(sFLAG_Z, !a);
        setflags(sFLAG_N, a & 0x80);
        break;
    }

    case op_and:
        bval = getaddr(addr); a &= bval;
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_asl:
        wval = getaddr(addr); wval <<= 1;
        setaddr(addr, (byte)wval);
        setflags(sFLAG_Z, !wval); setflags(sFLAG_N, wval & 0x80); setflags(sFLAG_C, wval & 0x100);
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
        setflags(sFLAG_Z, !(a & bval));
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_V, bval & 0x40);
        break;

    case op_brk:
        // pc = 0; // exit emulation loop - good for PSID
        // 6502 BRK behaves like an IRQ, but sets B flag in the pushed P,
        // and increments PC by one extra (BRK is 1 byte but acts like 2).
        pc++;                 // skip "padding" byte

        // push PC
        push((byte)(pc >> 8));
        push((byte)(pc & 0xFF));

        // push P with B set, bit5 set
        byte P = p;
        P |= sFLAG_B;
        P |= 0x20;
        push(P);

        // set I (disable further IRQs until RTI)
        p |= sFLAG_I;

        // jump via IRQ/BRK vector
        pc = bus_read16(0xFFFE);
        break;


    case op_clc: setflags(sFLAG_C, 0); break;
    case op_cld: setflags(sFLAG_D, 0); break;
    case op_cli: setflags(sFLAG_I, 0); break;
    case op_clv: setflags(sFLAG_V, 0); break;

    case op_cmp:
        bval = getaddr(addr);
        wval = (word)((unsigned short)a - bval);
        setflags(sFLAG_Z, !wval);
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, a >= bval);
        break;

    case op_cpx:
        bval = getaddr(addr);
        wval = (word)((unsigned short)x - bval);
        setflags(sFLAG_Z, !wval);
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, x >= bval);
        break;

    case op_cpy:
        bval = getaddr(addr);
        wval = (word)((unsigned short)y - bval);
        setflags(sFLAG_Z, !wval);
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, y >= bval);
        break;

    case op_dec:
        bval = getaddr(addr); bval--;
        setaddr(addr, bval);
        setflags(sFLAG_Z, !bval); setflags(sFLAG_N, bval & 0x80);
        break;

    case op_dex:
        x--; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
        break;

    case op_dey:
        y--; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
        break;

    case op_eor:
        bval = getaddr(addr); a ^= bval;
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_inc:
        bval = getaddr(addr); bval++;
        setaddr(addr, bval);
        setflags(sFLAG_Z, !bval); setflags(sFLAG_N, bval & 0x80);
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
            pc = bus_read16_wrap(wval);
        }
        break;

    case op_jsr:
        word target = bus_read8(pc++);
        target |= (word)(bus_read8(pc++) << 8);

        // pc is now the address of the *next* instruction
        word ret = (word)(pc - 1);     // last byte of operand
        push((byte)(ret >> 8));
        push((byte)(ret & 0xFF));

        pc = target;

        break;

    case op_lda:
        a = getaddr(addr);
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_ldx:
        x = getaddr(addr);
        setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
        break;

    case op_ldy:
        y = getaddr(addr);
        setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
        break;

    case op_lsr:
        bval = getaddr(addr);
        wval = bval; wval >>= 1;
        setaddr(addr, (byte)wval);
        setflags(sFLAG_Z, !wval);
        setflags(sFLAG_N, wval & 0x80);
        setflags(sFLAG_C, bval & 1);
        break;

    case op_nop:
        // IMPORTANT: NOP still costs cycles via addressing mode (implied = 2)
        // Your tables mark NOP as implied, so getaddr isn't called.
        // 2 cycles are taken up, already applied
        break;

    case op_ora:
        bval = getaddr(addr); a |= bval;
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_pha: push(a); break;  // stack ops have fixed timing (see note below)
    case op_php: push(p); break;

    case op_pla:
        a = pop();
        setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
        break;

    case op_plp: p = pop(); break;

    case op_rol:
        bval = getaddr(addr);
        c = !!(p & sFLAG_C);
        setflags(sFLAG_C, bval & 0x80);
        bval <<= 1; bval |= (byte)c;
        setaddr(addr, bval);
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_Z, !bval);
        break;

    case op_ror:
        bval = getaddr(addr);
        c = !!(p & sFLAG_C);
        setflags(sFLAG_C, bval & 1);
        bval >>= 1; bval |= (byte)(128 * c);
        setaddr(addr, bval);
        setflags(sFLAG_N, bval & 0x80);
        setflags(sFLAG_Z, !bval);
        break;

    case op_rti:
        p = pop();
        wval = pop();
        wval |= (word)(pop() << 8);
        pc = wval;
        break;

    case op_rts:
        wval = pop();
        wval |= (word)(pop() << 8);
        pc = (word)(wval + 1);
        break;

    case op_sbc:{
        byte m  = getaddr(addr);
        byte mi = (byte)(m ^ 0xFF);

        uint16_t sum = (uint16_t)a + (uint16_t)mi + ((p & sFLAG_C) ? 1 : 0);

        setflags(sFLAG_C, sum & 0x100);
        setflags(sFLAG_V, (~(a ^ mi) & (a ^ (byte)sum) & 0x80));  // using mi because that's what you're adding

        a = (byte)sum;
        setflags(sFLAG_Z, !a);
        setflags(sFLAG_N, a & 0x80);
        break;
    }

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
        // Unsupported opcode: treat as NOP-ish (2 cycles) to avoid freezing.
        break;
    }

    // RSID helping, this section CAN be skipped for PSID play back routines
    int spent = cycles - cycles_before; // log how many cycles this step too

    /// ** VIC HANDLING ** ///
    vic_step(spent);    // let the vic_step consume some of the cycles!
    cia_step_all(spent);
    //if (VIC_IRQ_LINE && !(p & sFLAG_I)) {
    if ((VIC_IRQ_LINE || CIA1_IRQ_LINE || CIA2_IRQ_LINE) && !(p & sFLAG_I)) {
        cpu_irq();
    }

    //if (VIC_IRQ_LINE) putchar('v');   // proves VIC line stays high



    return spent;       // return the cycles

}

int cpuGetPC(void) { return pc; }

void cpuSetPC(word npc) { pc = npc; }


// for debugging only //
void cpu_force_cli(void){
    p &= (byte)~sFLAG_I;
    putchar('C'); // prove it runs

}


int cpu_call_jsr_resetting(word npc, byte na){
    int total = 0;
    cycles = 0;

    a = na;
    x = 0;
    y = 0;
    //p = 0;
    p = (byte)(0x20 | sFLAG_I);   // start init with IRQs masked
    s = 255;
    pc = npc;

    push(0xFF); // high
    push(0xFF); // low


    while(pc){
        total += cpuStep();
    }
    return total;
}


// cpu6502.c additions

void cpu_set_regs(byte A, byte X, byte Y) { a = A; x = X; y = Y; }
void cpu_set_a(byte A) { a = A; }
byte cpu_get_a(void) { return a; }

// Emulate: JSR target; run until it returns.
// DOES NOT reset CPU state.
int cpu_call_jsr(word target){
    int total = 0;
    uint16_t saved_pc    = pc;

    // Push fake return address = $FFFF.
    // RTS will pop it, add 1, pc becomes $0000 -> stops your loop.
    push(0xFF); // high
    push(0xFF); // low

    pc = target;

    while (pc){
        total += cpuStep();
    }

    pc = saved_pc;

    return total;
}



