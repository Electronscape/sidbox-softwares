// cpu6502.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cpu6502.h"

#define READ8(addr)   (c->bus.read8(c->bus.user, (u16)(addr)))
#define WRITE8(a,v)   (c->bus.write8(c->bus.user, (u16)(a), (u8)(v)))

static inline u16 read16(cpu6502_t *c, u16 addr){
    u8 lo = READ8(addr);
    u8 hi = READ8((u16)(addr + 1));
    return (u16)(lo | ((u16)hi << 8));
}

static inline void setflags(cpu6502_t *c, u8 flag, int cond){
    if (cond) c->p |= flag;
    else      c->p &= (u8)~flag;
}

static inline void push(cpu6502_t *c, u8 v){
    WRITE8((u16)(0x0100u + c->sp), v);
    c->sp--;
}

static inline u8 pop(cpu6502_t *c){
    c->sp++;
    return READ8((u16)(0x0100u + c->sp));
}

// Addressing modes
enum {
    am_imp  = 0,
    am_imm  = 1,
    am_abs  = 2,
    am_absx = 3,
    am_absy = 4,
    am_zp   = 6,
    am_zpx  = 7,
    am_zpy  = 8,
    am_ind  = 9,
    am_indx = 10,
    am_indy = 11,
    am_acc  = 12,
    am_rel  = 13,
    am_xxx  = 255
};

// Opcode IDs (internal dispatch names)
enum {
    op_adc, op_and, op_asl, op_bcc, op_bcs, op_beq, op_bit, op_bmi, op_bne, op_bpl, op_brk,
    op_bvc, op_bvs, op_clc, op_cld, op_cli, op_clv, op_cmp, op_cpx, op_cpy, op_dec, op_dex,
    op_dey, op_eor, op_inc, op_inx, op_iny, op_jmp, op_jsr, op_lda, op_ldx, op_ldy, op_lsr,
    op_nop, op_ora, op_pha, op_php, op_pla, op_plp, op_rol, op_ror, op_rti, op_rts, op_sbc,
    op_sec, op_sed, op_sei, op_sta, op_stx, op_sty, op_tax, op_tay, op_tsx, op_txa, op_txs,
    op_tya, op_illegal
};

// NOTE: table type is u8, so no "byte" typedef needed.
static const u8 cpu6502_opcodes[256] = {
    op_brk, op_ora, op_illegal, op_illegal, op_illegal, op_ora, op_asl, op_illegal, op_php, op_ora, op_asl, op_illegal, op_illegal, op_ora, op_asl, op_illegal, op_bpl, op_ora,
    op_illegal, op_illegal, op_illegal, op_ora, op_asl, op_illegal, op_clc, op_ora, op_illegal, op_illegal, op_illegal, op_ora, op_asl, op_illegal, op_jsr, op_and, op_illegal, op_illegal, op_bit, op_and, op_rol,
    op_illegal, op_plp, op_and, op_rol, op_illegal, op_bit, op_and, op_rol, op_illegal, op_bmi, op_and, op_illegal, op_illegal, op_illegal, op_and, op_rol, op_illegal, op_sec, op_and, op_illegal, op_illegal,
    op_illegal, op_and, op_rol, op_illegal, op_rti, op_eor, op_illegal, op_illegal, op_illegal, op_eor, op_lsr, op_illegal, op_pha, op_eor, op_lsr, op_illegal, op_jmp, op_eor, op_lsr, op_illegal, op_bvc,
    op_eor, op_illegal, op_illegal, op_illegal, op_eor, op_lsr, op_illegal, op_cli, op_eor, op_illegal, op_illegal, op_illegal, op_eor, op_lsr, op_illegal, op_rts, op_adc, op_illegal, op_illegal, op_illegal, op_adc,
    op_ror, op_illegal, op_pla, op_adc, op_ror, op_illegal, op_jmp, op_adc, op_ror, op_illegal, op_bvs, op_adc, op_illegal, op_illegal, op_illegal, op_adc, op_ror, op_illegal, op_sei, op_adc, op_illegal,
    op_illegal, op_illegal, op_adc, op_ror, op_illegal, op_illegal, op_sta, op_illegal, op_illegal, op_sty, op_sta, op_stx, op_illegal, op_dey, op_illegal, op_txa, op_illegal, op_sty, op_sta, op_stx, op_illegal,
    op_bcc, op_sta, op_illegal, op_illegal, op_sty, op_sta, op_stx, op_illegal, op_tya, op_sta, op_txs, op_illegal, op_illegal, op_sta, op_illegal, op_illegal, op_ldy, op_lda, op_ldx, op_illegal, op_ldy,
    op_lda, op_ldx, op_illegal, op_tay, op_lda, op_tax, op_illegal, op_ldy, op_lda, op_ldx, op_illegal, op_bcs, op_lda, op_illegal, op_illegal, op_ldy, op_lda, op_ldx, op_illegal, op_clv, op_lda,
    op_tsx, op_illegal, op_ldy, op_lda, op_ldx, op_illegal, op_cpy, op_cmp, op_illegal, op_illegal, op_cpy, op_cmp, op_dec, op_illegal, op_iny, op_cmp, op_dex, op_illegal, op_cpy, op_cmp, op_dec,
    op_illegal, op_bne, op_cmp, op_illegal, op_illegal, op_illegal, op_cmp, op_dec, op_illegal, op_cld, op_cmp, op_illegal, op_illegal, op_illegal, op_cmp, op_dec, op_illegal, op_cpx, op_sbc, op_illegal, op_illegal,
    op_cpx, op_sbc, op_inc, op_illegal, op_inx, op_sbc, op_nop, op_illegal, op_cpx, op_sbc, op_inc, op_illegal, op_beq, op_sbc, op_illegal, op_illegal, op_illegal, op_sbc, op_inc, op_illegal, op_sed,
    op_sbc, op_illegal, op_illegal, op_illegal, op_sbc, op_inc, op_illegal
};

static const u8 cpu6502_modes[256] = {
    am_imp, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp, am_imm,
    am_acc, am_xxx,  am_abs, am_abs, am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_xxx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_xxx,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_abs, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_xxx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_xxx,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_imp, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_xxx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_xxx,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_imp, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_ind, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_xxx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_xxx,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_imm, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_zpx,  am_zpx,  am_zpy,  am_xxx, am_imp,
    am_absy,am_acc,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_imm, am_indx, am_imm, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_zpx,  am_zpx,  am_zpy,  am_xxx, am_imp,
    am_absy,am_acc,  am_xxx, am_absx,am_absx, am_absy, am_xxx,
    am_imm, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_zpx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_acc,  am_xxx, am_xxx, am_absx, am_absx, am_xxx,
    am_imm, am_indx, am_xxx, am_xxx, am_zp,   am_zp,   am_zp,   am_xxx, am_imp,
    am_imm, am_acc,  am_xxx, am_abs, am_abs,  am_abs,  am_xxx,
    am_rel, am_indy, am_xxx, am_xxx, am_zpx,  am_zpx,  am_zpx,  am_xxx, am_imp,
    am_absy,am_acc,  am_xxx, am_xxx, am_absx, am_absx, am_xxx
};


#define PACK_OPMODE(op, mode)  ((uint16_t)(uint8_t)(op) | ((uint16_t)(uint8_t)(mode) << 8))
#define UNPACK_OP(x)           ((uint8_t)((x) & 0xFF))
#define UNPACK_MODE(x)         ((uint8_t)((x) >> 8))

static uint16_t cpu6502_decode[256];   // op in low byte, mode in high byte



// If you don’t want externs, keep them static in this file.
// (I’m doing extern to keep this snippet short.)
static u8 getaddr(cpu6502_t *c, int mode, int *cycles){
    u16 ad, ad2;
    switch (mode) {
    case am_imp:  *cycles += 2; return 0;
    case am_imm:  *cycles += 2; return READ8(c->pc++);
    case am_abs:  *cycles += 4; ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8; return READ8(ad);
    case am_absx: *cycles += 4; ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8; ad2 = (u16)(ad + c->x);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++; return READ8(ad2);
    case am_absy: *cycles += 4; ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8; ad2 = (u16)(ad + c->y);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++; return READ8(ad2);
    case am_zp:   *cycles += 3; ad = READ8(c->pc++); return READ8(ad);
    case am_zpx:  *cycles += 4; ad = (u8)(READ8(c->pc++) + c->x); return READ8((u16)(ad & 0xFF));
    case am_zpy:  *cycles += 4; ad = (u8)(READ8(c->pc++) + c->y); return READ8((u16)(ad & 0xFF));
    case am_indx: *cycles += 6; ad = (u8)(READ8(c->pc++) + c->x);
        ad2 = READ8((u16)(ad & 0xFF));
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        return READ8(ad2);
    case am_indy: *cycles += 5; ad = READ8(c->pc++);
        ad2 = READ8(ad);
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        ad = (u16)(ad2 + c->y);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++;
        return READ8(ad);
    case am_acc:  *cycles += 2; return c->a;
    default: return 0;
    }
}

static void setaddr(cpu6502_t *c, int mode, u8 val, int *cycles){
    u16 ad, ad2;
    switch (mode) {
    case am_abs:
        *cycles += 2;
        ad = READ8((u16)(c->pc - 2));
        ad |= (u16)READ8((u16)(c->pc - 1)) << 8;
        WRITE8(ad, val);
        return;
    case am_absx:
        *cycles += 3;
        ad = READ8((u16)(c->pc - 2));
        ad |= (u16)READ8((u16)(c->pc - 1)) << 8;
        ad2 = (u16)(ad + c->x);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)--;
        WRITE8(ad2, val);
        return;
    case am_zp:
        *cycles += 2;
        ad = READ8((u16)(c->pc - 1));
        WRITE8(ad, val);
        return;
    case am_zpx:
        *cycles += 2;
        ad = (u8)(READ8((u16)(c->pc - 1)) + c->x);
        WRITE8((u16)(ad & 0xFF), val);
        return;
    case am_acc:
        c->a = val;
        return;
    default:
        return;
    }
}
static void putaddr(cpu6502_t *c, int mode, u8 val, int *cycles){
    u16 ad, ad2;
    switch (mode) {
    case am_abs:
        *cycles += 4;
        ad = READ8(c->pc++);
        ad |= (u16)READ8(c->pc++) << 8;
        WRITE8(ad, val);
        return;
    case am_absx:
        *cycles += 4;
        ad = READ8(c->pc++);
        ad |= (u16)READ8(c->pc++) << 8;
        ad2 = (u16)(ad + c->x);
        WRITE8(ad2, val);
        return;
    case am_absy:
        *cycles += 4;
        ad = READ8(c->pc++);
        ad |= (u16)READ8(c->pc++) << 8;
        ad2 = (u16)(ad + c->y);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++;
        WRITE8(ad2, val);
        return;
    case am_zp:
        *cycles += 3;
        ad = READ8(c->pc++);
        WRITE8(ad, val);
        return;
    case am_zpx:
        *cycles += 4;
        ad = (u8)(READ8(c->pc++) + c->x);
        WRITE8((u16)(ad & 0xFF), val);
        return;
    case am_zpy:
        *cycles += 4;
        ad = (u8)(READ8(c->pc++) + c->y);
        WRITE8((u16)(ad & 0xFF), val);
        return;
    case am_indx:
        *cycles += 6;
        ad = (u8)(READ8(c->pc++) + c->x);
        ad2 = READ8((u16)(ad & 0xFF));
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        WRITE8(ad2, val);
        return;
    case am_indy:
        *cycles += 5;
        ad = READ8(c->pc++);
        ad2 = READ8(ad);
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        ad = (u16)(ad2 + c->y);
        WRITE8(ad, val);
        return;
    case am_acc:
        *cycles += 2;
        c->a = val;
        return;
    default:
        return;
    }
}


static void branch(cpu6502_t *c, int cond, int *cycles){
    int8_t dist = (int8_t)getaddr(c, am_imm, cycles);
    u16 target = (u16)(c->pc + dist);
    if (cond) {
        *cycles += ((c->pc & 0xFF00) != (target & 0xFF00)) ? 2 : 1;
        c->pc = target;
    }
}

void cpu6502_init(cpu6502_t *c, cpu6502_bus_t bus){
    if (!c) return;
    *c = (cpu6502_t){0};
    c->bus = bus;
    c->sp = 0xFF;

    // Build packed decode table once
    for (int i = 0; i < 256; i++) {
        cpu6502_decode[i] = PACK_OPMODE(cpu6502_opcodes[i], cpu6502_modes[i]);
    }
}

void cpu6502_reset(cpu6502_t *c){
    if (!c) return;
    c->a = c->x = c->y = 0;
    c->p = 0x00;
    c->sp = 0xFD;
    c->pc = read16(c, 0xFFFC);
}

void cpu6502_reset_to(cpu6502_t *c, u16 pc){
    if (!c) return;
    c->a = c->x = c->y = 0;
    c->p = 0x24;
    c->sp = 0xFD;
    c->pc = pc;
}

static inline void push16(cpu6502_t *c, u16 v){
    push(c, (u8)(v >> 8));
    push(c, (u8)(v & 0xFF));
}

static inline u16 vec16(cpu6502_t *c, u16 addr){
    u8 lo = READ8(addr);
    u8 hi = READ8((u16)(addr + 1));
    return (u16)(lo | ((u16)hi << 8));
}

// 6502 pushes P with B=0 for IRQ/NMI, and bit5 (unused) is usually 1.
// We'll force bit5=1 and B=0.
static inline u8 pack_p_for_irq(cpu6502_t *c){
    u8 p = c->p;
    p &= (u8)~sFLAG_B;
    p |= 0x20;
    return p;
}

static int cpu_handle_interrupts(cpu6502_t *c){
    // NMI edge-latched (highest priority)
    if (c->nmi_latch)
    {
        c->nmi_latch = 0;

        // push PC, push P, set I
        push16(c, c->pc);
        push(c, pack_p_for_irq(c));
        c->p |= sFLAG_I;

        // jump to NMI vector
        c->pc = vec16(c, 0xFFFA);

        return 7; // NMI takes 7 cycles
    }

    // IRQ level-triggered, only if I flag clear
    if (c->irq_line && !(c->p & sFLAG_I)) {

        push16(c, c->pc);
        push(c, pack_p_for_irq(c));
        c->p |= sFLAG_I;

        c->pc = vec16(c, 0xFFFE);

        return 7; // IRQ takes 7 cycles
    }

    return 0;
}


// Assert/clear IRQ/NMI lines (for later RSID)
void cpu6502_irq(cpu6502_t *c, int level){
    c->irq_line = !!level;//(u8)(level != 0);
}

void cpu6502_nmi(cpu6502_t *c, int level){
    // NMI is edge-triggered on real 6502; latch rising edge.
    if (level && !c->nmi_line) c->nmi_latch = 1;
    c->nmi_line = !!level;//(u8)(level != 0);
}


static u16 ea(cpu6502_t *c, int mode, int *cycles){
    u16 ad, ad2;
    switch (mode) {
    case am_abs:
        *cycles += 4;
        ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8;
        return ad;

    case am_absx:
        *cycles += 4;
        ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8;
        ad2 = (u16)(ad + c->x);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++;
        return ad2;

    case am_absy:
        *cycles += 4;
        ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8;
        ad2 = (u16)(ad + c->y);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++;
        return ad2;

    case am_zp:
        *cycles += 3;
        return (u16)READ8(c->pc++);

    case am_zpx:
        *cycles += 4;
        return (u16)(u8)(READ8(c->pc++) + c->x);

    case am_zpy:
        *cycles += 4;
        return (u16)(u8)(READ8(c->pc++) + c->y);

    case am_indx:
        *cycles += 6;
        ad = (u8)(READ8(c->pc++) + c->x);
        ad2 = READ8((u16)(ad & 0xFF));
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        return ad2;

    case am_indy:
        *cycles += 5;
        ad = READ8(c->pc++);
        ad2 = READ8(ad);
        ad2 |= (u16)READ8((u16)((ad + 1) & 0xFF)) << 8;
        ad = (u16)(ad2 + c->y);
        if ((ad2 & 0xFF00) != (ad & 0xFF00)) (*cycles)++;
        return ad;

    // Only used by JMP (ind)
    case am_ind:
        *cycles += 5;
        ad = READ8(c->pc++); ad |= (u16)READ8(c->pc++) << 8;
        {
            // 6502 JMP(ind) page-wrap bug
            u8 lo = READ8(ad);
            u16 adhi = (u16)((ad & 0xFF00) | ((ad + 1) & 0x00FF));
            u8 hi = READ8(adhi);
            return (u16)(lo | ((u16)hi << 8));
        }

    default:
        *cycles +=1;
        return 0;
    }
}

static u8 rdop(cpu6502_t *c, int mode, int *cycles, u16 *out_addr){
    if (out_addr) *out_addr = 0;

    switch (mode) {
    case am_imm:  *cycles += 2; return READ8(c->pc++);
    case am_acc:  *cycles += 2; return c->a;
    case am_imp:  *cycles += 2; return 0;
    default: {
        u16 a = ea(c, mode, cycles);
        if (out_addr) *out_addr = a;
        return READ8(a);
    }}
}

extern uint8_t roms_loaded; //nice

int cpu6502_step(cpu6502_t *c){
    if (!c || !c->bus.read8 || !c->bus.write8) return 0;

    int cycles = 0;


    // optional interrupt handling at instruction boundary
    cycles += cpu_handle_interrupts(c);


    u8 opc = READ8(c->pc++);
    //int cmd  = cpu6502_opcodes[opc];
    //int mode = cpu6502_modes[opc];
    uint16_t d = cpu6502_decode[opc];
    int cmd  = UNPACK_OP(d);
    int mode = UNPACK_MODE(d);


    if (!roms_loaded && c->pc >= 0xE000) {
        //printf("RSID executed ROM-space @ %04X (but ROM not active)\n", c->pc);
    }

    u8 bval;
    u16 wval;
    int carry;
    u16 addr = 0;

    switch (cmd) {

    // -------------------- LOAD/STORE --------------------
    case op_lda:
        c->a = rdop(c, mode, &cycles, NULL);
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
        break;

    case op_ldx:
        c->x = rdop(c, mode, &cycles, NULL);
        setflags(c, sFLAG_Z, c->x == 0);
        setflags(c, sFLAG_N, c->x & 0x80);
        break;

    case op_ldy:
        c->y = rdop(c, mode, &cycles, NULL);
        setflags(c, sFLAG_Z, c->y == 0);
        setflags(c, sFLAG_N, c->y & 0x80);
        break;

    case op_sta:
        addr = ea(c, mode, &cycles);
        WRITE8(addr, c->a);
        break;

    case op_stx:
        addr = ea(c, mode, &cycles);
        WRITE8(addr, c->x);
        break;

    case op_sty:
        addr = ea(c, mode, &cycles);
        WRITE8(addr, c->y);
        break;

    // -------------------- TRANSFERS --------------------
    case op_tax: cycles += 2; c->x = c->a;  setflags(c,sFLAG_Z,c->x==0); setflags(c,sFLAG_N,c->x&0x80); break;
    case op_tay: cycles += 2; c->y = c->a;  setflags(c,sFLAG_Z,c->y==0); setflags(c,sFLAG_N,c->y&0x80); break;
    case op_txa: cycles += 2; c->a = c->x;  setflags(c,sFLAG_Z,c->a==0); setflags(c,sFLAG_N,c->a&0x80); break;
    case op_tya: cycles += 2; c->a = c->y;  setflags(c,sFLAG_Z,c->a==0); setflags(c,sFLAG_N,c->a&0x80); break;
    case op_tsx: cycles += 2; c->x = c->sp; setflags(c,sFLAG_Z,c->x==0); setflags(c,sFLAG_N,c->x&0x80); break;
    case op_txs: cycles += 2; c->sp = c->x; break;

    // -------------------- INC/DEC REG --------------------
    case op_inx: cycles += 2; c->x++; setflags(c,sFLAG_Z,c->x==0); setflags(c,sFLAG_N,c->x&0x80); break;
    case op_dex: cycles += 2; c->x--; setflags(c,sFLAG_Z,c->x==0); setflags(c,sFLAG_N,c->x&0x80); break;
    case op_iny: cycles += 2; c->y++; setflags(c,sFLAG_Z,c->y==0); setflags(c,sFLAG_N,c->y&0x80); break;
    case op_dey: cycles += 2; c->y--; setflags(c,sFLAG_Z,c->y==0); setflags(c,sFLAG_N,c->y&0x80); break;

    // -------------------- LOGIC --------------------
    case op_and:
        bval = rdop(c, mode, &cycles, NULL);
        c->a &= bval;
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
        break;

    case op_ora:
        bval = rdop(c, mode, &cycles, NULL);
        c->a |= bval;
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
        break;

    case op_eor:
        bval = rdop(c, mode, &cycles, NULL);
        c->a ^= bval;
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
        break;

    case op_bit: {
        u8 v = rdop(c, mode, &cycles, NULL);
        setflags(c, sFLAG_Z, (c->a & v) == 0);
        setflags(c, sFLAG_N, v & 0x80);
        setflags(c, sFLAG_V, v & 0x40);
    } break;

    // -------------------- ADC/SBC --------------------
    case op_adc: {
        u8 v = rdop(c, mode, &cycles, NULL);
        u16 sum = (u16)c->a + (u16)v + ((c->p & sFLAG_C) ? 1 : 0);
        setflags(c, sFLAG_C, sum & 0x100);
        // overflow: (~(A^V) & (A^R)) & 0x80
        setflags(c, sFLAG_V, (~(c->a ^ v) & (c->a ^ (u8)sum)) & 0x80);
        c->a = (u8)sum;
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
    } break;

    case op_sbc: {
        u8 v = rdop(c, mode, &cycles, NULL) ^ 0xFF;
        u16 sum = (u16)c->a + (u16)v + ((c->p & sFLAG_C) ? 1 : 0);
        setflags(c, sFLAG_C, sum & 0x100);
        setflags(c, sFLAG_V, (~(c->a ^ v) & (c->a ^ (u8)sum)) & 0x80);
        c->a = (u8)sum;
        setflags(c, sFLAG_Z, c->a == 0);
        setflags(c, sFLAG_N, c->a & 0x80);
    } break;

    // -------------------- COMPARES --------------------
    case op_cmp: {
        u8 v = rdop(c, mode, &cycles, NULL);
        u8 r = (u8)(c->a - v);
        setflags(c, sFLAG_C, c->a >= v);
        setflags(c, sFLAG_Z, r == 0);
        setflags(c, sFLAG_N, r & 0x80);
    } break;

    case op_cpx: {
        u8 v = rdop(c, mode, &cycles, NULL);
        u8 r = (u8)(c->x - v);
        setflags(c, sFLAG_C, c->x >= v);
        setflags(c, sFLAG_Z, r == 0);
        setflags(c, sFLAG_N, r & 0x80);
    } break;

    case op_cpy: {
        u8 v = rdop(c, mode, &cycles, NULL);
        u8 r = (u8)(c->y - v);
        setflags(c, sFLAG_C, c->y >= v);
        setflags(c, sFLAG_Z, r == 0);
        setflags(c, sFLAG_N, r & 0x80);
    } break;

    // -------------------- SHIFTS/ROTATES (acc or memory) --------------------
    case op_asl: {
        u16 a = 0; u8 v = rdop(c, mode, &cycles, &a);
        setflags(c, sFLAG_C, v & 0x80);
        v <<= 1;
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, v & 0x80);
        if (mode == am_acc) c->a = v; else WRITE8(a, v);
        if (mode != am_acc) cycles += 2; // RMW internal timing fudge
    } break;

    case op_lsr: {
        u16 a = 0; u8 v = rdop(c, mode, &cycles, &a);
        setflags(c, sFLAG_C, v & 0x01);
        v >>= 1;
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, 0);
        if (mode == am_acc) c->a = v; else WRITE8(a, v);
        if (mode != am_acc) cycles += 2;
    } break;

    case op_rol: {
        u16 a = 0; u8 v = rdop(c, mode, &cycles, &a);
        u8 c_in = (c->p & sFLAG_C) ? 1 : 0;
        setflags(c, sFLAG_C, v & 0x80);
        v = (u8)((v << 1) | c_in);
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, v & 0x80);
        if (mode == am_acc) c->a = v; else WRITE8(a, v);
        if (mode != am_acc) cycles += 2;
    } break;

    case op_ror: {
        u16 a = 0; u8 v = rdop(c, mode, &cycles, &a);
        u8 c_in = (c->p & sFLAG_C) ? 0x80 : 0x00;
        setflags(c, sFLAG_C, v & 0x01);
        v = (u8)((v >> 1) | c_in);
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, v & 0x80);
        if (mode == am_acc) c->a = v; else WRITE8(a, v);
        if (mode != am_acc) cycles += 2;
    } break;

    // -------------------- INC/DEC MEMORY --------------------
    case op_inc: {
        u16 a = ea(c, mode, &cycles);
        u8 v = (u8)(READ8(a) + 1);
        WRITE8(a, v);
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, v & 0x80);
        cycles += 2;
    } break;

    case op_dec: {
        u16 a = ea(c, mode, &cycles);
        u8 v = (u8)(READ8(a) - 1);
        WRITE8(a, v);
        setflags(c, sFLAG_Z, v == 0);
        setflags(c, sFLAG_N, v & 0x80);
        cycles += 2;
    } break;

    // -------------------- STACK --------------------
    case op_pha: cycles += 3; push(c, c->a); break;
  //case op_php: cycles += 3; push(c, (u8)(c->p | sFLAG_B)); break; // B set when pushed
    case op_php: cycles += 3; push(c, (u8)(c->p | sFLAG_B | 0x20)); break; // B set when pushed
    case op_pla: cycles += 4; c->a = pop(c); setflags(c,sFLAG_Z,c->a==0); setflags(c,sFLAG_N,c->a&0x80); break;
  //case op_plp: cycles += 4; c->p = pop(c); break;
    case op_plp: cycles += 4; c->p = (u8)(pop(c) | 0x20); break;


    // -------------------- FLAGS --------------------
    case op_clc: cycles += 2; c->p &= (u8)~sFLAG_C; break;
    case op_sec: cycles += 2; c->p |= sFLAG_C; break;
    case op_cli: cycles += 2; c->p &= (u8)~sFLAG_I; break;
    case op_sei: cycles += 2; c->p |= sFLAG_I; break;
    case op_clv: cycles += 2; c->p &= (u8)~sFLAG_V; break;
    case op_cld: cycles += 2; c->p &= (u8)~sFLAG_D; break;
    case op_sed: cycles += 2; c->p |= sFLAG_D; break;

    // -------------------- BRANCHES --------------------
    case op_bne: branch(c, !(c->p & sFLAG_Z), &cycles); break;
    case op_beq: branch(c,  (c->p & sFLAG_Z), &cycles); break;
    case op_bpl: branch(c, !(c->p & sFLAG_N), &cycles); break;
    case op_bmi: branch(c,  (c->p & sFLAG_N), &cycles); break;
    case op_bcc: branch(c, !(c->p & sFLAG_C), &cycles); break;
    case op_bcs: branch(c,  (c->p & sFLAG_C), &cycles); break;
    case op_bvc: branch(c, !(c->p & sFLAG_V), &cycles); break;
    case op_bvs: branch(c,  (c->p & sFLAG_V), &cycles); break;

    // -------------------- JUMPS/RTS/RTI --------------------
    case op_jmp:
        if (mode == am_ind) {
            c->pc = ea(c, am_ind, &cycles); // ea(ind) returns target already
        } else {
            cycles += 3;
            u16 lo = READ8(c->pc++);
            u16 hi = READ8(c->pc++);
            c->pc = (u16)(lo | (hi << 8));
        }
        break;

    case op_jsr: {
        u16 target = (u16)READ8(c->pc++);
        target |= (u16)READ8(c->pc++) << 8;
        u16 ret = (u16)(c->pc - 1);
        push(c, (u8)(ret >> 8));
        push(c, (u8)(ret & 0xFF));
        c->pc = target;
        cycles += 6;
    } break;

    case op_rts:
        cycles += 6;
        wval = pop(c);
        wval |= (u16)pop(c) << 8;
        c->pc = (u16)(wval + 1);
        break;

    case op_rti:
        cycles += 6;
        c->p = (u8)(pop(c) | 0x20);
        wval = pop(c);
        wval |= (u16)pop(c) << 8;
        c->pc = wval;

        break;

    // -------------------- NOP/BRK --------------------
    case op_nop:
        cycles += 2;
        break;

    case op_brk:
        // BRK behaves like IRQ/NMI vector fetch, but pushes PC+2 and sets B in pushed P.
        cycles += 7;

        // PC currently points to next byte after opcode fetch, BRK has a padding byte => PC+1
        // Many emus model it as PC already incremented by 1 for the padding.
        c->pc++; // skip padding byte

        push16(c, c->pc);                        // push PC (now effectively PC+2 from original opcode address)
        u8 p = c->p | sFLAG_B | 0x20;            // B set in pushed copy, bit5 set
        push(c, p);
        c->p |= sFLAG_I;                         // set interrupt disable
        c->pc = vec16(c, 0xFFFE);                // jump to IRQ/BRK vector
        break;

    default:
        // Unimplemented/illegal
        //c->pc = 0;   // hard stop so tests don't "pass" quietly
        cycles += 2;
        break;
    }


    return cycles;
}

int cpu6502_run(cpu6502_t *c, int cycle_budget){
    int used = 0;
    while (used < cycle_budget) {
        int ccy = cpu6502_step(c);
        if (ccy <= 0) break;
        used += ccy;
        if (c->pc == 0) break; // sentinel
    }
    return used;
}

int cpu6502_jsr(cpu6502_t *c, u16 addr, u8 a_reg)
{
    int total_cycles = 0;

    // Make sure stack is sane (real 6502 reset value)
    // If you already set this in reset/reset_to, you can remove this line.
    if (c->sp == 0) c->sp = 0xFD;

    // Load A as PSID expects: A = song_index (0-based) for INIT.
    c->a = a_reg;

    // Push return address-1 so RTS returns to PC==0x0000.
    // RTS pulls addr then does PC = addr + 1.
    // So we want pulled addr = 0xFFFF -> PC becomes 0x0000.
    c->bus.write8(c->bus.user, (u16)(0x0100u | c->sp), 0xFF); c->sp--;
    c->bus.write8(c->bus.user, (u16)(0x0100u | c->sp), 0xFF); c->sp--;

    // Jump into routine
    c->pc = addr;

    // Run until it returns to sentinel
    while (c->pc != 0x0000) {
        int cyc = cpu6502_step(c);
        if (cyc <= 0) break;       // safety
        total_cycles += cyc;
    }

    return total_cycles;
}



//////////////// ROMS FOR TESTING CPU ///////////////////////////

// JSR/RTS nesting test @ $8000
// Output should be: 1ABCD4\n
static const uint8_t prog_jsr_rts[] = {
    // main @ $8000
    0xA9,'1',        0x8D,0x20,0xD0,     // LDA #'1' ; STA $D020
    0x20,0x10,0x80,                      // JSR $8010
    0xA9,'4',        0x8D,0x20,0xD0,     // LDA #'4' ; STA $D020
    0xA9,'\n',       0x8D,0x20,0xD0,     // newline
    0x00,                                 // BRK

    // pad to $8010
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,

    // subA @ $8010
    0xA9,'A',        0x8D,0x20,0xD0,     // print 'A'
    0x20,0x20,0x80,                      // JSR $8020
    0xA9,'D',        0x8D,0x20,0xD0,     // print 'D'
    0x60,                                 // RTS

    // pad to $8020
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,

    // subB @ $8020
    0xA9,'B',        0x8D,0x20,0xD0,     // print 'B'
    0x20,0x30,0x80,                      // JSR $8030
    0xA9,'C',        0x8D,0x20,0xD0,     // print 'C'
    0x60,                                 // RTS

    // pad to $8030
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,

    // subC @ $8030
    0xA9,'C',        0x8D,0x20,0xD0,     // print 'C' (extra marker)
    0x60                                  // RTS
};



// RMW INC/DEC on zero page
// Expected: "I D\n" (with a space) if both pass
static const uint8_t prog_rmw_inc_dec[] = {
    // init zp $10 = 1
    0xA9,0x01,        0x85,0x10,          // LDA #$01 ; STA $10

    // INC $10 -> should be 2
    0xE6,0x10,                             // INC $10
    0xA5,0x10,        0xC9,0x02,          // LDA $10 ; CMP #$02
    0xF0,0x07,                             // BEQ inc_ok
    0xA9,'i',         0x8D,0x20,0xD0,     // fail -> 'i'
    0x4C,0x1C,0x80,                        // JMP after_inc
    // inc_ok:
    0xA9,'I',         0x8D,0x20,0xD0,     // pass -> 'I'
    // after_inc:
    0xA9,' ',         0x8D,0x20,0xD0,     // print space

    // DEC $10 -> should be 1 again
    0xC6,0x10,                             // DEC $10
    0xA5,0x10,        0xC9,0x01,          // LDA $10 ; CMP #$01
    0xF0,0x07,                             // BEQ dec_ok
    0xA9,'d',         0x8D,0x20,0xD0,     // fail -> 'd'
    0x4C,0x32,0x80,                        // JMP done
    // dec_ok:
    0xA9,'D',         0x8D,0x20,0xD0,     // pass -> 'D'

    // done:
    0xA9,'\n',        0x8D,0x20,0xD0,
    0x00
};



// ADC/SBC overflow (V flag) test
// Expected: "A S\n"
static const uint8_t prog_adc_sbc_v[] = {
    // ---- ADC overflow: 0x50 + 0x50 = 0xA0, V should set ----
    0x18,             // CLC
    0xB8,             // CLV
    0xA9,0x50,        // LDA #$50
    0x69,0x50,        // ADC #$50
    0x70,0x07,        // BVS adc_ok
    0xA9,'a',         0x8D,0x20,0xD0,   // fail -> 'a'
    0x4C,0x18,0x80,                      // JMP after_adc
    // adc_ok:
    0xA9,'A',         0x8D,0x20,0xD0,
    // after_adc:
    0xA9,' ',         0x8D,0x20,0xD0,

    // ---- SBC overflow: 0x80 - 0x01 = 0x7F, V should set ----
    0x38,             // SEC (6502 SBC uses carry as NOT-borrow)
    0xB8,             // CLV
    0xA9,0x80,        // LDA #$80
    0xE9,0x01,        // SBC #$01  => 0x7F, V=1
    0x70,0x07,        // BVS sbc_ok
    0xA9,'s',         0x8D,0x20,0xD0,   // fail -> 's'
    0x4C,0x2E,0x80,                      // JMP done
    // sbc_ok:
    0xA9,'S',         0x8D,0x20,0xD0,

    // done
    0xA9,'\n',        0x8D,0x20,0xD0,
    0x00
};


// Branch across page boundary correctness test
// We arrange PC so BEQ jumps from $80FE to $8102.
static const uint8_t prog_branch_page[] = {
    // Fill up to reach $80FE with harmless NOPs.
    // Put the useful code near the end.
    // NOTE: this assumes you load at $8000 exactly.
    // Size here: 0xFE bytes total before BEQ instruction.
    // We'll do it simple: 0xFE-6 bytes of NOP, then:
    //   LDA #$00 ; (sets Z)
    //   BEQ +2 across page
    //   BRK (should be skipped)
    //   LDA #'P' ; STA $D020 ; '\n' ; BRK

    // 0xF8 NOPs (248)
    // (If you prefer, generate these in code rather than typing them.)
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,
    0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,

    // near $80FC-ish: make Z=1 then BEQ
    0xA9,0x00,        // LDA #$00  (Z=1)
    0xF0,0x02,        // BEQ +2    (jumps over BRK)
    0x00,             // BRK (should be skipped if branch works)
    0xA9,'P',         // LDA #'P'
    0x8D,0x20,0xD0,   // STA $D020
    0xA9,'\n',
    0x8D,0x20,0xD0,
    0x00
};













