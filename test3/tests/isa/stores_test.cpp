// test3/tests/isa/stores_test.cpp
//
// Store instructions: STA, STX, STY across all addressing modes.
//
// Coverage:
//   STA  zp, zpx, abs, absx, absy, (ind,X), (ind),Y
//   STX  zp, zpy, abs
//   STY  zp, zpx, abs
//
// Store instructions do not update flags. The trace confirms bus timing
// and target-address formation.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(sta_zp) {
    Program prog = Program::start()
        .sta(ZP{0x20})
        .pass("STA zp completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- STA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: STA
        { /*    0.5 */ 0x0200, 0x85,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $85 = STA
        { /*    1.0 */ 0x0201, 0x85,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0020, 0x66,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0020
        { /*    2.5 */ 0x0020, 0x66,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0020

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_zpx) {
    Program prog = Program::start()
        .ldx(Imm{0x05})
        .sta(ZPX{0x30})
        .pass("STA zpx completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x95,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $95 = STA
        { /*    3.0 */ 0x0203, 0x95,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x30,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0030, 0x30,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0030, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0035, 0x66,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0035
        { /*    5.5 */ 0x0035, 0x66,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0035

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x00,  R,    1, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_abs) {
    Program prog = Program::start()
        .sta(Abs{0x0442})
        .pass("STA abs completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- STA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: STA
        { /*    0.5 */ 0x0200, 0x8D,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $8D = STA
        { /*    1.0 */ 0x0201, 0x8D,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x42,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x04,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0442, 0x66,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0442
        { /*    3.5 */ 0x0442, 0x66,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0442

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x04,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_absx) {
    Program prog = Program::start()
        .ldx(Imm{0x20})
        .sta(AbsX{0x04F0})
        .pass("STA absx completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x9D,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // $9D = STA
        { /*    3.0 */ 0x0203, 0x9D,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xF0,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xF0,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0410, 0x04,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0410, 0x00,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0510, 0x66,  W,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0510
        { /*    6.5 */ 0x0510, 0x66,  W,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0510

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x00,  R,    1, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_absy) {
    Program prog = Program::start()
        .ldy(Imm{0x30})
        .sta(AbsY{0x05E8})
        .pass("STA absy completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x30,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x30,  R,    1, 0x0202, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x99,  R,    1, 0x0202, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $99 = STA
        { /*    3.0 */ 0x0203, 0x99,  R,    0, 0x0203, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xE8,  R,    0, 0x0203, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xE8,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x05,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0518, 0x05,  R,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0518, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0618, 0x66,  W,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // write $66 -> $0618
        { /*    6.5 */ 0x0618, 0x66,  W,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // write $66 -> $0618

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x00,  R,    1, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_indx) {
    Program prog = Program::start();

    prog.org(0x0044).bytes({0x20, 0x07});
    prog.org(0x0200);
    prog.ldx(Imm{0x04})
        .sta(IndX{0x40})
        .pass("STA indx completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x81,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $81 = STA
        { /*    3.0 */ 0x0203, 0x81,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x40,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0040, 0x40,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0040, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0044, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0044, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0045, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0045, 0x07,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0720, 0x66,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0720
        { /*    7.5 */ 0x0720, 0x66,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0720

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x07,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_indy) {
    Program prog = Program::start();

    prog.org(0x0050).bytes({0xF0, 0x07});
    prog.org(0x0200);
    prog.ldy(Imm{0x30})
        .sta(IndY{0x50})
        .pass("STA indy completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x30,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x30,  R,    1, 0x0202, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x91,  R,    1, 0x0202, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $91 = STA
        { /*    3.0 */ 0x0203, 0x91,  R,    0, 0x0203, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x50,  R,    0, 0x0203, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0050, 0x50,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0050, 0xF0,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0051, 0xF0,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0051, 0x07,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0720, 0x07,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0720, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0820, 0x66,  W,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // write $66 -> $0820
        { /*    7.5 */ 0x0820, 0x66,  W,    0, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // write $66 -> $0820

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x00,  R,    1, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x30, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(stx_zp) {
    Program prog = Program::start()
        .ldx(Imm{0x7B})
        .stx(ZP{0x60})
        .pass("STX zp completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x7B,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x7B,  R,    1, 0x0202, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // SYNC: STX
        { /*    2.5 */ 0x0202, 0x86,  R,    1, 0x0202, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // $86 = STX
        { /*    3.0 */ 0x0203, 0x86,  R,    0, 0x0203, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x60,  R,    0, 0x0203, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0060, 0x7B,  W,    0, 0x0204, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // write $7B -> $0060
        { /*    4.5 */ 0x0060, 0x7B,  W,    0, 0x0204, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // write $7B -> $0060

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0204, 0x60,  R,    1, 0x0204, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x7B, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(stx_zpy) {
    Program prog = Program::start()
        .ldx(Imm{0x7C})
        .ldy(Imm{0x09})
        .stx(ZPY{0x70})
        .pass("STX zpy completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x7C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x7C,  R,    1, 0x0202, 0x66, 0x7C, 0x00, 0xFD, 0b00100100 },  // SYNC: LDY
        { /*    2.5 */ 0x0202, 0xA0,  R,    1, 0x0202, 0x66, 0x7C, 0x00, 0xFD, 0b00100100 },  // $A0 = LDY
        { /*    3.0 */ 0x0203, 0xA0,  R,    0, 0x0203, 0x66, 0x7C, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x09,  R,    0, 0x0203, 0x66, 0x7C, 0x00, 0xFD, 0b00100100 },

        // --- STX at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x09,  R,    1, 0x0204, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // SYNC: STX
        { /*    4.5 */ 0x0204, 0x96,  R,    1, 0x0204, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // $96 = STX
        { /*    5.0 */ 0x0205, 0x96,  R,    0, 0x0205, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x70,  R,    0, 0x0205, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0070, 0x70,  R,    0, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0070, 0x00,  R,    0, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0079, 0x7C,  W,    0, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // write $7C -> $0079
        { /*    7.5 */ 0x0079, 0x7C,  W,    0, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // write $7C -> $0079

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x00,  R,    1, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x7C, 0x09, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(stx_abs) {
    Program prog = Program::start()
        .ldx(Imm{0x7D})
        .stx(Abs{0x0920})
        .pass("STX abs completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x7D,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x7D,  R,    1, 0x0202, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // SYNC: STX
        { /*    2.5 */ 0x0202, 0x8E,  R,    1, 0x0202, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // $8E = STX
        { /*    3.0 */ 0x0203, 0x8E,  R,    0, 0x0203, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x20,  R,    0, 0x0204, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x09,  R,    0, 0x0204, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0920, 0x7D,  W,    0, 0x0205, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // write $7D -> $0920
        { /*    5.5 */ 0x0920, 0x7D,  W,    0, 0x0205, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // write $7D -> $0920

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x09,  R,    1, 0x0205, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x7D, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sty_zp) {
    Program prog = Program::start()
        .ldy(Imm{0x55})
        .sty(ZP{0x80})
        .pass("STY zp completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x55,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x55,  R,    1, 0x0202, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // SYNC: STY
        { /*    2.5 */ 0x0202, 0x84,  R,    1, 0x0202, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // $84 = STY
        { /*    3.0 */ 0x0203, 0x84,  R,    0, 0x0203, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x55,  W,    0, 0x0204, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // write $55 -> $0080
        { /*    4.5 */ 0x0080, 0x55,  W,    0, 0x0204, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // write $55 -> $0080

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x55, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sty_zpx) {
    Program prog = Program::start()
        .ldy(Imm{0x56})
        .ldx(Imm{0x0A})
        .sty(ZPX{0x90})
        .pass("STY zpx completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x56,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x56,  R,    1, 0x0202, 0x66, 0x00, 0x56, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xA2,  R,    1, 0x0202, 0x66, 0x00, 0x56, 0xFD, 0b00100100 },  // $A2 = LDX
        { /*    3.0 */ 0x0203, 0xA2,  R,    0, 0x0203, 0x66, 0x00, 0x56, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x0A,  R,    0, 0x0203, 0x66, 0x00, 0x56, 0xFD, 0b00100100 },

        // --- STY at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x0A,  R,    1, 0x0204, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // SYNC: STY
        { /*    4.5 */ 0x0204, 0x94,  R,    1, 0x0204, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // $94 = STY
        { /*    5.0 */ 0x0205, 0x94,  R,    0, 0x0205, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x90,  R,    0, 0x0205, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0090, 0x90,  R,    0, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0090, 0x00,  R,    0, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x009A, 0x56,  W,    0, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // write $56 -> $009A
        { /*    7.5 */ 0x009A, 0x56,  W,    0, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // write $56 -> $009A

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x00,  R,    1, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x0A, 0x56, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sty_abs) {
    Program prog = Program::start()
        .ldy(Imm{0x57})
        .sty(Abs{0x0A20})
        .pass("STY abs completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x57,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x57,  R,    1, 0x0202, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // SYNC: STY
        { /*    2.5 */ 0x0202, 0x8C,  R,    1, 0x0202, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // $8C = STY
        { /*    3.0 */ 0x0203, 0x8C,  R,    0, 0x0203, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x20,  R,    0, 0x0204, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x0A,  R,    0, 0x0204, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0A20, 0x57,  W,    0, 0x0205, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // write $57 -> $0A20
        { /*    5.5 */ 0x0A20, 0x57,  W,    0, 0x0205, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // write $57 -> $0A20

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x0A,  R,    1, 0x0205, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x57, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
