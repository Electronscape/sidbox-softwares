// cpu6502_test.c
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "cpu6502.h"

#include "sid8579.h"
#include "bus.h"
/// to be removed later ///












// ---- tiny assert helpers ----
static int g_fail = 0;

static void expect_u8(const char *name, uint8_t got, uint8_t exp) {
    if (got != exp) {
        printf("FAIL %-18s got=$%02X exp=$%02X\n", name, got, exp);
        g_fail = 1;
    }
}

static void expect_u16(const char *name, uint16_t got, uint16_t exp) {
    if (got != exp) {
        printf("FAIL %-18s got=$%04X exp=$%04X\n", name, got, exp);
        g_fail = 1;
    }
}

static void expect_flag(const char *name, uint8_t p, uint8_t flag, int exp_set) {
    int got_set = (p & flag) != 0;
    if (got_set != !!exp_set) {
        printf("FAIL %-18s flag %s got=%d exp=%d (P=$%02X)\n",
               name, (flag==sFLAG_C)?"C":(flag==sFLAG_Z)?"Z":(flag==sFLAG_I)?"I":
                                         (flag==sFLAG_D)?"D":(flag==sFLAG_B)?"B":(flag==sFLAG_V)?"V":"N",
               got_set, !!exp_set, p);
        g_fail = 1;
    }
}

// Helper to run until BRK sentinel (your core sets PC=0 on BRK)
static void run_until_stop(cpu6502_t *c, int max_steps) {
    for (int i = 0; i < max_steps; i++) {
        int cycles = cpu6502_step(c);
        if(cycles <= 0) break;
        //sid_step(&sid, cycles);

        if (c->pc == 0) return;
    }
    printf("FAIL run_until_stop: exceeded max_steps\n");
    g_fail = 1;
}



static void trace_step(cpu6502_t *c, int step_i) {
    uint16_t pc_before = c->pc;
    uint8_t  opc = bus_read8(NULL, pc_before);

    uint8_t a0 = c->a, x0 = c->x, y0 = c->y, p0 = c->p, s0 = c->sp;

    int cyc = cpu6502_step(c);

    printf("[%04d] PC=$%04X OP=$%02X  "
           "A:%02X->%02X X:%02X->%02X Y:%02X->%02X "
           "S:%02X->%02X P:%02X->%02X  +%dcy\n",
           step_i, pc_before, opc,
           a0, c->a, x0, c->x, y0, c->y,
           s0, c->sp, p0, c->p, cyc);
}

static void run_until_stop_trace(cpu6502_t *c, int max_steps) {
    for (int i = 0; i < max_steps; i++) {
        if (c->pc == 0) return;
        trace_step(c, i);
    }
    printf("TRACE: exceeded max_steps\n");
    g_fail = 1;
}






















// JSR/RTS nesting test @ $8000
// Output should be: 1ABCD4\n
static const uint8_t prog_jsr_rts[] = {
    // $8000: print '1'
    0xA9, '1',             // LDA #'1'
    0x8D, 0x20, 0xD0,       // STA $D020

    // call sub1 at $8010
    0x20, 0x10, 0x80,       // JSR $8010

    // print '4'
    0xA9, '4',             // LDA #'4'
    0x8D, 0x20, 0xD0,       // STA $D020
    0x00,                   // BRK

    // pad to $8010 (we are currently at $800E)
    0xEA, 0xEA,             // NOP, NOP

    // $8010: sub1: print 'A', call sub2, print 'D', return
    0xA9, 'A',              // LDA #'A'
    0x8D, 0x20, 0xD0,       // STA $D020
    0x20, 0x20, 0x80,       // JSR $8020
    0xA9, 'D',              // LDA #'D'
    0x8D, 0x20, 0xD0,       // STA $D020
    0x60,                   // RTS

    // pad to $8020 (we are currently at $801E)
    0xEA, 0xEA,             // NOP, NOP

    // $8020: sub2: print 'B', 'C', return
    0xA9, 'B',              // LDA #'B'
    0x8D, 0x20, 0xD0,       // STA $D020
    0xA9, 'C',              // LDA #'C'
    0x8D, 0x20, 0xD0,       // STA $D020
    0x60                    // RTS
};



// RMW INC/DEC on zero page
// Expected: "I D\n" (with a space) if both pass
static const uint8_t prog_rmw_inc_dec[] = {
    // init zp $10 = 1
    0xA9,0x01,        0x85,0x10,          // LDA #$01 ; STA $10

    // INC $10 -> should be 2
    0xE6,0x10,                             // INC $10
    0xA5,0x10,        0xC9,0x02,          // LDA $10 ; CMP #$02
    0xF0,0x08,                             // BEQ inc_ok   (FIXED: was 0x07)

    0xA9,'i',         0x8D,0x20,0xD0,     // fail -> 'i'
    0x4C,0x19,0x80,                        // JMP after_inc (FIXED target)

    // inc_ok @ $8014
    0xA9,'I',         0x8D,0x20,0xD0,     // pass -> 'I'

    // after_inc @ $8019
    0xA9,' ',         0x8D,0x20,0xD0,     // print space

    // DEC $10 -> should be 1 again
    0xC6,0x10,                             // DEC $10
    0xA5,0x10,        0xC9,0x01,          // LDA $10 ; CMP #$01
    0xF0,0x08,                             // BEQ dec_ok   (FIXED: was 0x07)

    0xA9,'d',         0x8D,0x20,0xD0,     // fail -> 'd'
    0x4C,0x33,0x80,                        // JMP done (FIXED target)

    // dec_ok @ $802E
    0xA9,'D',         0x8D,0x20,0xD0,     // pass -> 'D'

    // done @ $8033
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
    0x70,0x08,        // BVS adc_ok   (was 0x07; must land on LDA #'A')
    0xA9,'a',         // fail -> 'a'
    0x8D,0x20,0xD0,   // STA $D020
    0x4C,0x15,0x80,   // JMP after_adc (was 0x18,0x80)

    // adc_ok @ $8010:
    0xA9,'A',
    0x8D,0x20,0xD0,

    // after_adc @ $8015:
    0xA9,' ',
    0x8D,0x20,0xD0,

    // ---- SBC overflow: 0x80 - 0x01 = 0x7F, V should set ----
    0x38,             // SEC
    0xB8,             // CLV
    0xA9,0x80,        // LDA #$80
    0xE9,0x01,        // SBC #$01
    0x70,0x08,        // BVS sbc_ok   (was 0x07)
    0xA9,'s',         // fail -> 's'
    0x8D,0x20,0xD0,   // STA $D020
    0x4C,0x2F,0x80,   // JMP done (was 0x2E,0x80)

    // sbc_ok @ $802A:
    0xA9,'S',
    0x8D,0x20,0xD0,

    // done @ $802F:
    0xA9,'\n',
    0x8D,0x20,0xD0,
    0x00              // BRK
};


// Branch across page boundary correctness test
// We arrange PC so BEQ jumps from $80FE to $8102.

#define NOP10  0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA,0xEA
#define NOP20  NOP10, NOP10
#define NOP40  NOP20, NOP20
#define NOP80  NOP40, NOP40
#define NOP160 NOP80, NOP80
#define NOP240 NOP160, NOP80
#define NOP252 NOP240, NOP10, 0xEA,0xEA  // 240 + 10 + 2 = 252

static const uint8_t prog_branch_page[] = {
    NOP252,

    0xA9,0x00,        // $80FC LDA #$00
    0xF0,0x02,        // $80FE BEQ +2  (to $8102)
    0x00,             // $8100 BRK (skipped)
    0xA9,'P',         // $8101 LDA #'P'
    0x8D,0x20,0xD0,   // $8103 STA $D020
    0xA9,'\n',
    0x8D,0x20,0xD0,
    0x00
};

static const uint8_t prog_branch_tiny[] = {
    0xA9,0x00,        // $80FC: LDA #$00  (Z=1)
    0xF0,0x01,        // $80FE: BEQ +1    (lands at $8101) ✅ page cross
    0x00,             // $8100: BRK (skipped)
    0xA9,'P',         // $8101: LDA #'P'
    0x8D,0x20,0xD0,   // $8103: STA $D020
    0x00              // $8106: BRK
};

// Put this at $81FC, run from $81FC.
// BEQ at $81FE jumps to $8202 (page-cross guaranteed).
static const uint8_t prog_branch_cross4[] = {
    0xA9,0x00,        // $81FB: LDA #$00   (Z=1)
    0xF0,0x03,        // $81FD: BEQ +3     (from $81FF -> $8202) page-cross ✅
    0x00,             // $81FF: BRK        (should be skipped)
    0xEA,             // $8200: NOP        (padding)
    0xEA,             // $8201: NOP        (padding)
    0xA9,'X',         // $8202: LDA #'X'
    0x8D,0x20,0xD0,   // $8204: STA $D020
    0x00              // $8207: BRK
};

extern uint8_t ram[];
int do_cpuTest(void) {
    cpu6502_t c;
    cpu6502_bus_t bus = {0};
    bus.user = NULL;
    bus.read8 = bus_read8;
    bus.write8 = bus_write8;




    //sid_init(&sid, 985248, 44100);


    // ----------------------------
    // Test 1: RESET vector loads PC
    // ----------------------------
    clear_ram();
    ram[0xFFFC] = 0x00;
    ram[0xFFFD] = 0x80;

    cpu6502_init(&c, bus);
    cpu6502_reset(&c);
    expect_u16("reset PC", c.pc, 0x8000);



    {
        clear_ram();

        uint16_t start = 0x8000;

        uint8_t prog[] = {
          0xA2, 0x00,             // LDX #$00
          // loop:
          0xBD, 0x0F, 0x80,       // LDA $800F,X   (message)  ✅ fixed
          0xF0, 0x07,             // BEQ done
          0x8D, 0x20, 0xD0,       // STA $D020     (print char)
          0xE8,                   // INX
          0x4C, 0x02, 0x80,       // JMP loop
          // done:
          0x00,                   // BRK
          // message @ $800F
          'H','E','L','L','O',' ',
          'W','O','R','L','D','\n',0
        };

        load_prog(start, prog, sizeof(prog));

        cpu6502_init(&c, bus);
        cpu6502_reset(&c);

        printf("\n--- HELLO WORLD OUTPUT ---\n");
        run_until_stop(&c, 200);
        printf("\n--- END OUTPUT ---\n");
    }


    // Summary
    if (!g_fail) {
        printf("ALL TESTS PASSED ✅\n");
        return 0;
    } else {
        printf("TESTS FAILED ❌\n");
        return 1;
    }
}





























