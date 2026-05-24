// test3/tests/isa/cmp_test.cpp
//
// Compares: CMP, CPX, CPY. Set N/Z/C from (reg - operand).
// A, X, and Y stay unchanged.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110.
//
// Coverage:
//   CMP  imm equal/less/greater, zp, zpx, abs, absx, absy, (ind,X), (ind),Y
//   CPX  imm equal/less/greater, zp, abs
//   CPY  imm equal/less/greater, zp, abs

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(cmp_imm_equal) {
    Program prog = Program::start()
        .cmp(Imm{0x66})
        .pass("CMP immediate equal completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CMP
        { /*    0.5 */ 0x0200, 0xC9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C9 = CMP
        { /*    1.0 */ 0x0201, 0xC9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x66,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x66,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_imm_less) {
    Program prog = Program::start()
        .cmp(Imm{0x80})
        .pass("CMP immediate less completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CMP
        { /*    0.5 */ 0x0200, 0xC9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C9 = CMP
        { /*    1.0 */ 0x0201, 0xC9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_imm_greater) {
    Program prog = Program::start()
        .cmp(Imm{0x10})
        .pass("CMP immediate greater completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CMP
        { /*    0.5 */ 0x0200, 0xC9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C9 = CMP
        { /*    1.0 */ 0x0201, 0xC9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_zp) {
    Program prog = Program::start()
        .cmp(ZP{0x80})
        .pass("CMP zero page completed");

    prog.org(0x0080).bytes({0x66});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CMP
        { /*    0.5 */ 0x0200, 0xC5,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C5 = CMP
        { /*    1.0 */ 0x0201, 0xC5,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x66,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x66,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_abs) {
    Program prog = Program::start()
        .cmp(Abs{0x2400})
        .pass("CMP absolute completed");

    prog.org(0x2400).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CMP
        { /*    0.5 */ 0x0200, 0xCD,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $CD = CMP
        { /*    1.0 */ 0x0201, 0xCD,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x24,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x2400, 0x24,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x2400, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x80,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_zpx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .cmp(ZPX{0x80})
        .pass("CMP zero page indexed by X completed");

    prog.org(0x0084).bytes({0x10});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CMP at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    2.5 */ 0x0202, 0xD5,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $D5 = CMP
        { /*    3.0 */ 0x0203, 0xD5,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0084, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0084, 0x10,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x10,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100101 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_absx) {
    Program prog = Program::start()
        .ldx(Imm{0x03})
        .cmp(AbsX{0x2400})
        .pass("CMP absolute indexed by X completed");

    prog.org(0x2403).bytes({0x66});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x03,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CMP at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x03,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    2.5 */ 0x0202, 0xDD,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // $DD = CMP
        { /*    3.0 */ 0x0203, 0xDD,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2403, 0x24,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2403, 0x66,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x66,  R,    1, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x03, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x03, 0x00, 0xFD, 0b00100111 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x03, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x03, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_absy) {
    Program prog = Program::start()
        .ldy(Imm{0x05})
        .cmp(AbsY{0x2400})
        .pass("CMP absolute indexed by Y completed");

    prog.org(0x2405).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CMP at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    2.5 */ 0x0202, 0xD9,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $D9 = CMP
        { /*    3.0 */ 0x0203, 0xD9,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2405, 0x24,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2405, 0x80,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x80,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x00, 0x05, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x00, 0x05, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x05, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x05, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_indx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .cmp(IndX{0x40})
        .pass("CMP indexed indirect completed");

    prog.org(0x0044).bytes({0x00, 0x24});
    prog.org(0x2400).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CMP at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    2.5 */ 0x0202, 0xC1,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $C1 = CMP
        { /*    3.0 */ 0x0203, 0xC1,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x40,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0040, 0x40,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0040, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0044, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0044, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0045, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0045, 0x24,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x2400, 0x24,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x2400, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_indy) {
    Program prog = Program::start()
        .ldy(Imm{0x05})
        .cmp(IndY{0x50})
        .pass("CMP indirect indexed completed");

    prog.org(0x0050).bytes({0x00, 0x24});
    prog.org(0x2405).bytes({0x10});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CMP at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    2.5 */ 0x0202, 0xD1,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $D1 = CMP
        { /*    3.0 */ 0x0203, 0xD1,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x50,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0050, 0x50,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0050, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0051, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0051, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x2405, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x2405, 0x10,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0204, 0x10,  R,    1, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100101 },  // dummy / operand
        { /*    8.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100101 },
        { /*    9.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    9.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpx_imm_equal) {
    Program prog = Program::start()
        .ldx(Imm{0x44})
        .cpx(Imm{0x44})
        .pass("CPX immediate equal completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: CPX
        { /*    2.5 */ 0x0202, 0xE0,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $E0 = CPX
        { /*    3.0 */ 0x0203, 0xE0,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x44,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x44,  R,    1, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100111 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpx_imm_less) {
    Program prog = Program::start()
        .ldx(Imm{0x10})
        .cpx(Imm{0x20})
        .pass("CPX immediate less completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: CPX
        { /*    2.5 */ 0x0202, 0xE0,  R,    1, 0x0202, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },  // $E0 = CPX
        { /*    3.0 */ 0x0203, 0xE0,  R,    0, 0x0203, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x10, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x10, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x10, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x10, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x10, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpx_imm_greater) {
    Program prog = Program::start()
        .ldx(Imm{0x80})
        .cpx(Imm{0x7F})
        .pass("CPX immediate greater completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: CPX
        { /*    2.5 */ 0x0202, 0xE0,  R,    1, 0x0202, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $E0 = CPX
        { /*    3.0 */ 0x0203, 0xE0,  R,    0, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x7F,  R,    0, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x7F,  R,    1, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x80, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x80, 0x00, 0xFD, 0b00100101 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x80, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x80, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpx_zp) {
    Program prog = Program::start()
        .ldx(Imm{0x44})
        .cpx(ZP{0x90})
        .pass("CPX zero page completed");

    prog.org(0x0090).bytes({0x20});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: CPX
        { /*    2.5 */ 0x0202, 0xE4,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $E4 = CPX
        { /*    3.0 */ 0x0203, 0xE4,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x90,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0090, 0x90,  R,    0, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0090, 0x20,  R,    0, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    6.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100101 },
        { /*    7.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    7.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpx_abs) {
    Program prog = Program::start()
        .ldx(Imm{0x44})
        .cpx(Abs{0x2410})
        .pass("CPX absolute completed");

    prog.org(0x2410).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: CPX
        { /*    2.5 */ 0x0202, 0xEC,  R,    1, 0x0202, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $EC = CPX
        { /*    3.0 */ 0x0203, 0xEC,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x10,  R,    0, 0x0203, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x10,  R,    0, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2410, 0x24,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2410, 0x80,  R,    0, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x80,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x44, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x44, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x44, 0x00, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x44, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x44, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpy_imm_equal) {
    Program prog = Program::start()
        .ldy(Imm{0x44})
        .cpy(Imm{0x44})
        .pass("CPY immediate equal completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: CPY
        { /*    2.5 */ 0x0202, 0xC0,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $C0 = CPY
        { /*    3.0 */ 0x0203, 0xC0,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x44,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x44,  R,    1, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100111 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpy_imm_less) {
    Program prog = Program::start()
        .ldy(Imm{0x10})
        .cpy(Imm{0x20})
        .pass("CPY immediate less completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // SYNC: CPY
        { /*    2.5 */ 0x0202, 0xC0,  R,    1, 0x0202, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // $C0 = CPY
        { /*    3.0 */ 0x0203, 0xC0,  R,    0, 0x0203, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpy_imm_greater) {
    Program prog = Program::start()
        .ldy(Imm{0x80})
        .cpy(Imm{0x7F})
        .pass("CPY immediate greater completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },  // SYNC: CPY
        { /*    2.5 */ 0x0202, 0xC0,  R,    1, 0x0202, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },  // $C0 = CPY
        { /*    3.0 */ 0x0203, 0xC0,  R,    0, 0x0203, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x7F,  R,    0, 0x0203, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x7F,  R,    1, 0x0204, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x80, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x80, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x80, 0xFD, 0b00100101 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x80, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x80, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpy_zp) {
    Program prog = Program::start()
        .ldy(Imm{0x44})
        .cpy(ZP{0x91})
        .pass("CPY zero page completed");

    prog.org(0x0091).bytes({0x20});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: CPY
        { /*    2.5 */ 0x0202, 0xC4,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $C4 = CPY
        { /*    3.0 */ 0x0203, 0xC4,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x91,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0091, 0x91,  R,    0, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0091, 0x20,  R,    0, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100101 },  // dummy / operand
        { /*    6.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100101 },
        { /*    7.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    7.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cpy_abs) {
    Program prog = Program::start()
        .ldy(Imm{0x44})
        .cpy(Abs{0x2420})
        .pass("CPY absolute completed");

    prog.org(0x2420).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x44,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CPY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x44,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: CPY
        { /*    2.5 */ 0x0202, 0xCC,  R,    1, 0x0202, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $CC = CPY
        { /*    3.0 */ 0x0203, 0xCC,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x20,  R,    0, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2420, 0x24,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2420, 0x80,  R,    0, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x80,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x44, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x00, 0x44, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x00, 0x44, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x44, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x44, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
