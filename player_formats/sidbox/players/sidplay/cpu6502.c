#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

// Ensure types are defined if headers don't provide them

#include "cpu6502.h"
#include "vic.h"
#include "bus.h"
#include "cia.h"
#include "sid8579.h"

// --- Flags ---
#define sFLAG_N 128u
#define sFLAG_V 64u
#define sFLAG_B 16u
#define sFLAG_D 8u
#define sFLAG_I 4u
#define sFLAG_Z 2u
#define sFLAG_C 1u

// --- Opcode Enum ---
__attribute__((const))
const enum {
    iadc, iand, iasl, ibcc, ibcs, ibeq, ibit, ibmi, ibne, ibpl, ibrk, ibvc, ibvs, iclc,
    icld, icli, iclv, icmp, icpx, icpy, idec, idex, idey, ieor, iinc, iinx, iiny, ijmp,
    ijsr, ilda, ildx, ildy, ilsr, inop, iora, ipha, iphp, ipla, iplp, irol, iror, irti,
    irts, isbc, isec, ised, isei, ista, istx, isty, itax, itay, itsx, itxa, itxs, itya,
    ixxx,
    // ILLEGAL OPCODES (Added)
    ilax, isax, idcp, iisb, islo, irla, isre, irra
};

// --- Addressing Modes ---
/*
#define iimp  0u
#define iimm  1u
#define iabs  2u
#define iabsx 3u
#define iabsy 4u
#define izp   6u
#define izpx  7u
#define izpy  8u
#define iind  9u
#define iindx 10u
#define iindy 11u
#define iacc  12u
#define irel  13u
*/

__attribute__((const))
const enum {
    iimp    = 0u,
    iimm    = 1u,
    iabs    = 2u,
    iabsx   = 3u,
    iabsy   = 4u,
    izp     = 6u,
    izpx    = 7u,
    izpy    = 8u,
    iind    = 9u,
    iindx   = 10u,
    iindy   = 11u,
    iacc    = 12u,
    irel    = 13u,
};

// --- Tables ---
static byte cur_opc;

// Defines which instructions suffer a penalty cycle on page crossing (Reads=Yes, Stores=No)
static const uint8_t page_cross_ok[256] = {
    /* 0x00 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0x10 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0,  // 1C = NOP abs,X => +1 if cross
    /* 0x20 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0x30 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0,  // 3C = NOP abs,X
    /* 0x40 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0x50 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0,  // 5C = NOP abs,X
    /* 0x60 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0x70 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0,  // 7C = NOP abs,X
    /* 0x80 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0x90 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0xA0 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0xB0 */  0,1,0,1,1,1,1,0,  0,1,0,0,1,1,1,1,  // B3/BF = LAX => +1 if cross
    /* 0xC0 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0xD0 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0,  // DC = NOP abs,X
    /* 0xE0 */  0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,
    /* 0xF0 */  0,1,0,0,0,0,0,0,  0,1,0,0,1,1,0,0   // FC = NOP abs,X
};

static const uint8_t base_cycles[256] = {
    /* 0x00 */  7,6,2,8,3,3,5,5,  3,2,2,2,4,4,6,6,   // 03/07/0F fixed (SLO)
    /* 0x10 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7,   // 13/17/1B/1F fixed (SLO)
    /* 0x20 */  6,6,2,8,3,3,5,5,  4,2,2,2,4,4,6,6,   // 23/27/2F fixed (RLA)
    /* 0x30 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7,   // 33/37/3B/3F fixed (RLA)
    /* 0x40 */  6,6,2,8,3,3,5,5,  3,2,2,2,3,4,6,6,   // 43/47/4F fixed (SRE)
    /* 0x50 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7,   // 53/57/5B/5F fixed (SRE)
    /* 0x60 */  6,6,2,8,3,3,5,5,  4,2,2,2,5,4,6,6,   // 63/67/6F fixed (RRA)
    /* 0x70 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7,   // 73/77/7B/7F fixed (RRA)
    /* 0x80 */  2,6,2,6,3,3,3,3,  2,2,2,2,4,4,4,4,   // 83/87/8F fixed (SAX)
    /* 0x90 */  2,6,2,2,4,4,4,4,  2,5,2,2,4,5,5,2,   // 97 fixed (SAX)
    /* 0xA0 */  2,6,2,6,3,3,3,3,  2,2,2,2,4,4,4,4,   // A3/A7/AF fixed (LAX)
    /* 0xB0 */  2,5,2,5,4,4,4,4,  2,4,2,4,4,4,4,4,   // B3/B7/BF fixed (LAX)
    /* 0xC0 */  2,6,2,8,3,3,5,5,  2,2,2,2,4,4,6,6,   // C3/C7/CF fixed (DCP)
    /* 0xD0 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7,   // D3/D7/DB/DF fixed (DCP)
    /* 0xE0 */  2,6,2,8,3,3,5,5,  2,2,2,2,4,4,6,6,   // E3/E7/EF fixed (ISB)
    /* 0xF0 */  2,5,2,8,4,4,6,6,  2,4,2,7,4,4,7,7    // F3/F7/FB/FF fixed (ISB)
};


// Table mapping hex to Opcode Enum
static const byte opcodes[256] = {
    /* 00 */  ibrk, iora, ixxx, islo, inop, iora, iasl, islo,    iphp, iora, iasl, ixxx, inop, iora, iasl, islo,
    /* 10 */  ibpl, iora, ixxx, islo, inop, iora, iasl, islo,    iclc, iora, inop, islo, inop, iora, iasl, islo,
    /* 20 */  ijsr, iand, ixxx, irla, ibit, iand, irol, irla,    iplp, iand, irol, ixxx, ibit, iand, irol, irla,
    /* 30 */  ibmi, iand, ixxx, irla, inop, iand, irol, irla,    isec, iand, inop, irla, inop, iand, irol, irla,
    /* 40 */  irti, ieor, ixxx, isre, inop, ieor, ilsr, isre,    ipha, ieor, ilsr, ixxx, ijmp, ieor, ilsr, isre,
    /* 50 */  ibvc, ieor, ixxx, isre, inop, ieor, ilsr, isre,    icli, ieor, inop, isre, inop, ieor, ilsr, isre,
    /* 60 */  irts, iadc, ixxx, irra, inop, iadc, iror, irra,    ipla, iadc, iror, ixxx, ijmp, iadc, iror, irra,
    /* 70 */  ibvs, iadc, ixxx, irra, inop, iadc, iror, irra,    isei, iadc, inop, irra, inop, iadc, iror, irra,
    /* 80 */  inop, ista, inop, isax, isty, ista, istx, isax,    idey, inop, itxa, ixxx, isty, ista, istx, isax,
    /* 90 */  ibcc, ista, ixxx, ixxx, isty, ista, istx, isax,    itya, ista, itxs, ixxx, inop, ista, ixxx, ixxx,
    /* A0 */  ildy, ilda, ildx, ilax, ildy, ilda, ildx, ilax,    itay, ilda, itax, ilax, ildy, ilda, ildx, ilax,
    /* B0 */  ibcs, ilda, ixxx, ilax, ildy, ilda, ildx, ilax,    iclv, ilda, itsx, ilax, ildy, ilda, ildx, ilax,
    /* C0 */  icpy, icmp, inop, idcp, icpy, icmp, idec, idcp,    iiny, icmp, idex, ixxx, icpy, icmp, idec, idcp,
    /* D0 */  ibne, icmp, ixxx, idcp, inop, icmp, idec, idcp,    icld, icmp, inop, idcp, inop, icmp, idec, idcp,
    /* E0 */  icpx, isbc, inop, iisb, icpx, isbc, iinc, iisb,    iinx, isbc, inop, isbc, icpx, isbc, iinc, iisb,
    /* F0 */  ibeq, isbc, ixxx, iisb, inop, isbc, iinc, iisb,    ised, isbc, inop, iisb, inop, isbc, iinc, iisb
};

// Table mapping hex to Addressing Mode
static const byte modes[256] = {
    /* 00 */ iimp, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iabs,  iabs,  iabs,  iabs,
    /* 10 */ irel, iindy, ixxx, iindy, ixxx, izpx, izpx, izpx,   iimp, iabsy, ixxx, iabsy, ixxx,  iabsx, iabsx, iabsx,
    /* 20 */ iabs, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iabs,  iabs,  iabs,  iabs,
    /* 30 */ irel, iindy, ixxx, iindy, ixxx, izpx, izpx, izpx,   iimp, iabsy, ixxx, iabsy, ixxx,  iabsx, iabsx, iabsx,
    /* 40 */ iimp, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iabs,  iabs,  iabs,  iabs,
    /* 50 */ irel, iindy, ixxx, iindy, ixxx, izpx, izpx, izpx,   iimp, iabsy, ixxx, iabsy, ixxx,  iabsx, iabsx, iabsx,
    /* 60 */ iimp, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iind,  iabs,  iabs,  iabs,
    /* 70 */ irel, iindy, ixxx, iindy, ixxx, izpx, izpx, izpx,   iimp, iabsy, ixxx, iabsy, ixxx,  iabsx, iabsx, iabsx,
    /* 80 */ iimm, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iabs,  iabs,  iabs,  iabs,
    /* 90 */ irel, iindy, ixxx, ixxx,  izpx, izpx, izpy, izpy,   iimp, iabsy, itxs, ixxx,  ixxx,  iabsx, iabsx, iabsy,
    /* A0 */ iimm, iindx, iimm, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, iimm,  iabs,  iabs,  iabs,  iabs,
    /* B0 */ irel, iindy, ixxx, iindy, izpx, izpx, izpy, izpy,   iimp, iabsy, iacc, iabsy, iabsx, iabsx, iabsy, iabsy,
    /* C0 */ iimm, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, ixxx,  iabs,  iabs,  iabs,  iabs,
    /* D0 */ irel, iindy, ixxx, iindy, izpx, izpx, izpx, izpx,   iimp, iabsy, iacc, iabsy, ixxx,  iabsx, iabsx, iabsx,
    /* E0 */ iimm, iindx, ixxx, iindx, izp,  izp,  izp,  izp,    iimp, iimm,  iacc, iimm,  iabs,  iabs,  iabs,  iabs,
    /* F0 */ irel, iindy, ixxx, iindy, izpx, izpx, izpx, izpx,   iimp, iabsy, iacc, iabsy, ixxx,  iabsx, iabsx, iabsx
};

// --- CPU State ---
static int cycles;
static byte bval;
static word wval;
static byte a, x, y, s, p;
static word pc;

// Helper variables
static uint8_t last_page_cross = 0;
static word last_ea = 0;
static uint8_t last_ea_valid = 0;

// --- Addressing Helper Functions ---

static byte getaddr(int mode) {
    word ad, ad2;
    last_page_cross = 0;
    last_ea_valid = 0;
    last_ea = 0;

    switch(mode) {
        case iimp:  return 0;
        case iimm:  return bus_read8(pc++);

        case iabs:
            ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
            last_ea = ad; last_ea_valid = 1;
            return bus_read8(ad);

        case izp:
            ad = bus_read8(pc++);
            last_ea = ad; last_ea_valid = 1;
            return bus_read8(ad);

        case izpx:
            ad = (word)(bus_read8(pc++) + x) & 0xFF;
            last_ea = ad; last_ea_valid = 1;
            return bus_read8(ad);

        case izpy:
            ad = (word)(bus_read8(pc++) + y) & 0xFF;
            last_ea = ad; last_ea_valid = 1;
            return bus_read8(ad);

        case iindx:
            ad = (word)(bus_read8(pc++) + x) & 0xFF;
            ad2 = bus_read8(ad);
            ad2 |= (word)(bus_read8((ad + 1) & 0xFF) << 8);
            last_ea = ad2; last_ea_valid = 1;
            return bus_read8(ad2);

        case iabsx: {
            ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
            ad2 = (word)(ad + x);
            last_page_cross = (((ad ^ ad2) & 0xFF00) != 0);
            last_ea = ad2; last_ea_valid = 1;
            return bus_read8(ad2);
        }

        case iabsy: {
            ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8);
            ad2 = (word)(ad + y);
            last_page_cross = (((ad ^ ad2) & 0xFF00) != 0);
            last_ea = ad2; last_ea_valid = 1;
            return bus_read8(ad2);
        }

        case iindy: {
            ad = bus_read8(pc++);
            ad2 = bus_read8(ad);
            ad2 |= ((word)bus_read8((ad + 1) & 0xFF) << 8);
            word ea = (word)(ad2 + y);
            last_page_cross = (((ad2 ^ ea) & 0xFF00) != 0);
            last_ea = ea; last_ea_valid = 1;
            return bus_read8(ea);
        }

        case iacc:
            return a;
    }
    return 0;
}

static void setaddr(int mode, byte val) {
    // Prefer the effective address captured by getaddr().
    // This fixes several nasty cases (especially illegal RMW ops) where recomputing from pc-1/pc-2
    // can write to the wrong place if PC has moved differently than you expect.
    if (mode == iacc) { a = val; return; }

    if (last_ea_valid) {
        bus_write8(last_ea, val);
        return;
    }

    // Fallback (should rarely happen in this core)
    word ad, ad2;
    switch(mode) {
        case iabs: {
            ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
            bus_write8(ad, val);
            break;
        }
        case iabsx: {
            ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
            ad2 = (word)(ad + x);
            bus_write8(ad2, val);
            break;
        }
        case iabsy: {
            ad = bus_read8(pc - 2); ad |= (word)(bus_read8(pc - 1) << 8);
            ad2 = (word)(ad + y);
            bus_write8(ad2, val);
            break;
        }
        case izp: {
            ad = bus_read8(pc - 1);
            bus_write8(ad, val);
            break;
        }
        case izpx: {
            ad = (word)(bus_read8(pc - 1) + x);
            bus_write8(ad & 0xff, val);
            break;
        }
        default:
            // If we got here, something called setaddr without getaddr first.
            // Don't spam prints; just ignore.
            break;
    }
}

static void putaddr(int mode, byte val){
    word ad, ad2;
    switch(mode){
        case iabs:  ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); bus_write8(ad, val); return;
        case iabsx: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + x); bus_write8(ad2, val); return;
        case iabsy: ad = bus_read8(pc++); ad |= (word)(bus_read8(pc++) << 8); ad2 = (word)(ad + y); bus_write8(ad2, val); return;
        case izp:   ad = bus_read8(pc++); bus_write8(ad, val); return;
        case izpx:  ad = (word)(bus_read8(pc++) + x); bus_write8(ad & 0xff, val); return;
        case izpy:  ad = (word)(bus_read8(pc++) + y); bus_write8(ad & 0xff, val); return;
        case iindx:
            ad = (word)(bus_read8(pc++) + x);
            ad2 = bus_read8(ad & 0xff);
            ad++; // increment inside ZP
            ad2 |= (word)(bus_read8(ad & 0xff) << 8);
            bus_write8(ad2, val); return;
        case iindy:
            ad = bus_read8(pc++);
            ad2 = bus_read8(ad);
            ad2 |= (word)(bus_read8((ad + 1) & 0xff) << 8);
            ad = (word)(ad2 + y);
            bus_write8(ad, val); return;
        case iacc:  a = val; return;
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
    cycles = 7;
    a = x = y = 0;
    p = (byte)(sFLAG_I);// | 0x20);
    s = 0xFD;
    pc = bus_read16(0xfffc);
}

void cpuResetTo(word npc){
    cycles = 7;
    a = x = y = 0;
    p = (byte)(sFLAG_I);// | 0x20);
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

        case iadc: {
            byte val = getaddr(addr);
            APPLY_PAGE_CROSS();

            int c = (p & sFLAG_C) ? 1 : 0;

            if (p & sFLAG_D) {
                // Decimal Mode (BCD) - fixed
                int bin_sum = (int)a + (int)val + c;
                setflags(sFLAG_V, (~((int)a ^ (int)val) & ((int)a ^ bin_sum)) & 0x80);

                int al = (a & 0x0F) + (val & 0x0F) + c;
                int ah = (a >> 4) + (val >> 4);

                if (al > 9) { al += 6; ah++; }
                if (ah > 9) { ah += 6; }

                setflags(sFLAG_C, ah > 0x0F);
                a = (byte)(((ah << 4) & 0xF0) | (al & 0x0F));

                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            } else {
                // Binary Mode (Safe from C integer bugs)
                int sum = (int)a + (int)val + c;
                setflags(sFLAG_V, (~((int)a ^ (int)val) & ((int)a ^ sum)) & 0x80);
                setflags(sFLAG_C, sum > 0xFF);
                a = (byte)(sum & 0xFF);
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            }
            break;
        }

        case isbc: {
            byte val = getaddr(addr);
            APPLY_PAGE_CROSS();

            int c = (p & sFLAG_C) ? 1 : 0;

            if (p & sFLAG_D) {
                // Decimal Mode
                uint16_t diff = (uint16_t)a - (uint16_t)val - (1 - c);
                uint16_t al = (a & 0x0F) - (val & 0x0F) - (1 - c);
                uint16_t ah = (a >> 4) - (val >> 4);

                if (al & 0x10) { al -= 6; ah--; }
                if (ah & 0x10) { ah -= 6; }

                setflags(sFLAG_V, ((a ^ val) & (a ^ diff)) & 0x80);
                setflags(sFLAG_C, diff < 0x100);

                a = (byte)((ah << 4) | (al & 0x0F));
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            } else {
                // Binary Mode - SAFE IMPLEMENTATION
                int val_inv = (int)(val ^ 0xFF);
                int sum = (int)a + val_inv + c;

                setflags(sFLAG_V, (((int)a ^ (int)val) & ((int)a ^ sum)) & 0x80);
                setflags(sFLAG_C, sum > 0xFF);
                a = (byte)(sum & 0xFF);
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            }
            break;
        }

            // --- ILLEGAL OPCODES (Required for RSID/Demos) ---
        case ilax: // LDA + LDX
            a = getaddr(addr);
            APPLY_PAGE_CROSS();
            x = a;
            setflags(sFLAG_Z, !a);
            setflags(sFLAG_N, a & 0x80);
            break;

        case isax: // Store A & X
            bval = a & x;
            putaddr(addr, bval);
            break;

        case idcp: // DEC memory + CMP A
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            bval--;
            setaddr(addr, bval);
            // CMP logic
            wval = (word)((uint16_t)a - bval);
            setflags(sFLAG_Z, !(wval & 0xFF));
            setflags(sFLAG_N, wval & 0x80);
            setflags(sFLAG_C, a >= bval);
            break;

        case iisb: // INC memory + SBC A
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            bval++;
            setaddr(addr, bval);
            // SBC logic (Binary)
            {
                int c = (p & sFLAG_C) ? 1 : 0;
                int val_inv = (int)(bval ^ 0xFF);
                int sum = (int)a + val_inv + c;
                setflags(sFLAG_V, (((int)a ^ (int)bval) & ((int)a ^ sum)) & 0x80);
                setflags(sFLAG_C, sum > 0xFF);
                a = (byte)(sum & 0xFF);
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            }
            break;

        case islo: // ASL + ORA
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            c_flag = bval & 0x80;
            bval <<= 1;
            setaddr(addr, bval);
            setflags(sFLAG_C, c_flag);
            a |= bval;
            setflags(sFLAG_Z, !a);
            setflags(sFLAG_N, a & 0x80);
            break;

        case irla: // ROL + AND
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            c_flag = (p & sFLAG_C) ? 1 : 0;
            {
                int new_c = (bval & 0x80) ? 1 : 0;
                bval = (byte)((bval << 1) | c_flag);
                setaddr(addr, bval);
                setflags(sFLAG_C, new_c);
                a &= bval;
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            }
            break;

        case isre: // LSR + EOR
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            setflags(sFLAG_C, bval & 1);
            bval >>= 1;
            setaddr(addr, bval);
            a ^= bval;
            setflags(sFLAG_Z, !a);
            setflags(sFLAG_N, a & 0x80);
            break;

        case irra: // ROR + ADC
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // dummy write (RMW)
            c_flag = (p & sFLAG_C) ? 1 : 0;
            {
                int new_c = bval & 1;
                bval = (byte)((bval >> 1) | ((byte)c_flag << 7));
                setaddr(addr, bval);
                setflags(sFLAG_C, new_c);
                // ADC logic (Binary)
                int sum = (int)a + (int)bval + new_c;
                setflags(sFLAG_V, (~((int)a ^ (int)bval) & ((int)a ^ sum)) & 0x80);
                setflags(sFLAG_C, sum > 0xFF);
                a = (byte)(sum & 0xFF);
                setflags(sFLAG_Z, !a);
                setflags(sFLAG_N, a & 0x80);
            }
            break;

        case iand:
            bval = getaddr(addr); a &= bval;
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
            break;

        case iasl:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            wval = (word)bval << 1;
            setaddr(addr, (byte)wval);
            setflags(sFLAG_Z, !(byte)wval);
            setflags(sFLAG_N, (byte)wval & 0x80);
            setflags(sFLAG_C, wval & 0x100);
            break;

        case ibcc: branch(!(p & sFLAG_C)); break;
        case ibcs: branch( (p & sFLAG_C)); break;
        case ibne: branch(!(p & sFLAG_Z)); break;
        case ibeq: branch( (p & sFLAG_Z)); break;
        case ibpl: branch(!(p & sFLAG_N)); break;
        case ibmi: branch( (p & sFLAG_N)); break;
        case ibvc: branch(!(p & sFLAG_V)); break;
        case ibvs: branch( (p & sFLAG_V)); break;

        case ibit:
            bval = getaddr(addr);
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !(a & bval));
            setflags(sFLAG_N, bval & 0x80);
            setflags(sFLAG_V, bval & 0x40);
            break;

        case ibrk:
            pc++; // Padding byte
            push((byte)(pc >> 8));
            push((byte)(pc & 0xFF));
            push((byte)(p | sFLAG_B | 0x20));
            p |= sFLAG_I;
            pc = bus_read16(0xFFFE);
            break;

        case iclc: setflags(sFLAG_C, 0); break;
        case icld: setflags(sFLAG_D, 0); break;
        case icli: setflags(sFLAG_I, 0); break;
        case iclv: setflags(sFLAG_V, 0); break;

        case icmp:
            bval = getaddr(addr);
            APPLY_PAGE_CROSS();
            wval = (word)((uint16_t)a - bval);
            setflags(sFLAG_Z, !(wval & 0xFF));
            setflags(sFLAG_N, wval & 0x80);
            setflags(sFLAG_C, a >= bval);
            break;

        case icpx:
            bval = getaddr(addr);
            APPLY_PAGE_CROSS();
            wval = (word)((uint16_t)x - bval);
            setflags(sFLAG_Z, !(wval & 0xFF));
            setflags(sFLAG_N, wval & 0x80);
            setflags(sFLAG_C, x >= bval);
            break;

        case icpy:
            bval = getaddr(addr);
            APPLY_PAGE_CROSS();
            wval = (word)((uint16_t)y - bval);
            setflags(sFLAG_Z, !(wval & 0xFF));
            setflags(sFLAG_N, wval & 0x80);
            setflags(sFLAG_C, y >= bval);
            break;

        case idec:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            bval--;
            setaddr(addr, bval);
            setflags(sFLAG_Z, !bval);
            setflags(sFLAG_N, bval & 0x80);
            break;

        case idex:
            x--; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
            break;

        case idey:
            y--; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
            break;

        case ieor:
            bval = getaddr(addr); a ^= bval;
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
            break;

        case iinc:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            bval++;
            setaddr(addr, bval);
            setflags(sFLAG_Z, !bval);
            setflags(sFLAG_N, bval & 0x80);
            break;

        case iinx:
            x++; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
            break;

        case iiny:
            y++; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
            break;

        case ijmp:
            wval = bus_read8(pc++);
            wval |= (word)(bus_read8(pc++) << 8);
            if(addr == iabs){
                pc = wval;
            } else {
                // Re-applying NMOS Page Wrap bug: JMP ($xxFF) reads high byte from $xx00
                pc = bus_read8(wval) | (bus_read8((wval & 0xFF00) | ((wval + 1) & 0xFF)) << 8);
            }
            break;

        case ijsr: {
            // do not change this, this is specific usecase (AI doesnt understand the pc++, pc, pc, running a redundency check
            byte lo = bus_read8(pc++);
            byte hi = bus_read8(pc);
            word target = (word)lo | ((word)hi << 8);

            // 6502 pushes (PC-1) after fetching operand
            word ret = (word)(pc);
            push((byte)(ret >> 8));
            push((byte)(ret & 0xFF));

            pc = target;
            break;
        }

        case ilda:
            a = getaddr(addr);
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
            break;

        case ildx:
            x = getaddr(addr);
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80);
            break;

        case ildy:
            y = getaddr(addr);
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80);
            break;

        case ilsr:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            setflags(sFLAG_C, bval & 1);
            wval = bval >> 1;
            setaddr(addr, (byte)wval);
            setflags(sFLAG_Z, !(byte)wval);
            setflags(sFLAG_N, 0);
            break;

        case inop:
            break;

        case iora:
            bval = getaddr(addr); a |= bval;
            APPLY_PAGE_CROSS();
            setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
            break;

        case ipha: push(a); break;
        case iphp: push((byte)(p | sFLAG_B | 0x20)); break;

        case ipla:
            a = pop();
            setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80);
            break;

        case iplp:
            p = pop();
            p |= 0x20;
            p &= ~sFLAG_B;
            break;

        case irol:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            c_flag = !!(p & sFLAG_C);
            setflags(sFLAG_C, bval & 0x80);
            bval = (byte)((bval << 1) | c_flag);
            setaddr(addr, bval);
            setflags(sFLAG_N, bval & 0x80);
            setflags(sFLAG_Z, !bval);
            break;

        case iror:
            bval = getaddr(addr);
            if (addr != iacc) setaddr(addr, bval); // Dummy write
            c_flag = !!(p & sFLAG_C);
            setflags(sFLAG_C, bval & 1);
            bval = (byte)((bval >> 1) | (c_flag << 7));
            setaddr(addr, bval);
            setflags(sFLAG_N, bval & 0x80);
            setflags(sFLAG_Z, !bval);
            break;

        case irti:
            p = pop();
            p |= 0x20;
            p &= ~sFLAG_B;
            wval = pop();
            wval |= (word)(pop() << 8);
            pc = wval;
            break;

        case irts:
            wval = pop();
            wval |= (word)(pop() << 8);
            pc = (word)(wval + 1);
            break;

        case isec: setflags(sFLAG_C, 1); break;
        case ised: setflags(sFLAG_D, 1); break;
        case isei: setflags(sFLAG_I, 1); break;

        case ista: putaddr(addr, a); break;
        case istx: putaddr(addr, x); break;
        case isty: putaddr(addr, y); break;

        case itax: x = a; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80); break;
        case itay: y = a; setflags(sFLAG_Z, !y); setflags(sFLAG_N, y & 0x80); break;
        case itsx: x = s; setflags(sFLAG_Z, !x); setflags(sFLAG_N, x & 0x80); break;
        case itxa: a = x; setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80); break;
        case itxs: s = x; break;
        case itya: a = y; setflags(sFLAG_Z, !a); setflags(sFLAG_N, a & 0x80); break;

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

void cpu_force_sei(void) {
    p |= sFLAG_I;
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
// used by the PSID though
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
