// test3/tests/isa/branches_test.cpp
//
// Conditional branches: BCC/BCS/BEQ/BMI/BNE/BPL/BVC/BVS.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110.
//
// Coverage:
//   All 8 NMOS branches, both taken and not taken.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(bpl_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BPL takes branch when N=0");
    prog.org(0x0210)
        .bpl(taken)
        .fail("BPL must take branch when N=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BPL at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BPL
        { /*    0.5 */ 0x0210, 0x10,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $10 = BPL
        { /*    1.0 */ 0x0211, 0x10,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0212, 0xEE,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0212, 0xEA,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bpl_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BPL must not take branch when N=1");
    prog.org(0x0210)
        .lda(Imm{0x80})
        .bpl(wrong)
        .pass("BPL does not take branch when N=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0210, 0xA9,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0211, 0xA9,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x80,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BPL at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0x80,  R,    1, 0x0212, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: BPL
        { /*    2.5 */ 0x0212, 0x10,  R,    1, 0x0212, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $10 = BPL
        { /*    3.0 */ 0x0213, 0x10,  R,    0, 0x0213, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0214: NOP flushes pipelined registers, JMP-self at $0215 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0214, 0xEC,  R,    1, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0214, 0xEA,  R,    1, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0215, 0xEA,  R,    0, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0215, 0x4C,  R,    0, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bmi_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BMI takes branch when N=1");
    prog.org(0x0210)
        .lda(Imm{0x80})
        .bmi(taken)
        .fail("BMI must take branch when N=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0210, 0xA9,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0211, 0xA9,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x80,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BMI at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0x80,  R,    1, 0x0212, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: BMI
        { /*    2.5 */ 0x0212, 0x30,  R,    1, 0x0212, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $30 = BMI
        { /*    3.0 */ 0x0213, 0x30,  R,    0, 0x0213, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0214, 0xEC,  R,    0, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    4.5 */ 0x0214, 0xEA,  R,    0, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    6.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    6.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    7.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bmi_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BMI must not take branch when N=0");
    prog.org(0x0210)
        .bmi(wrong)
        .pass("BMI does not take branch when N=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BMI at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BMI
        { /*    0.5 */ 0x0210, 0x30,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $30 = BMI
        { /*    1.0 */ 0x0211, 0x30,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0212: NOP flushes pipelined registers, JMP-self at $0213 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0xEE,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0212, 0xEA,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0213, 0xEA,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0x4C,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bvc_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BVC takes branch when V=0");
    prog.org(0x0210)
        .bvc(taken)
        .fail("BVC must take branch when V=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BVC at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BVC
        { /*    0.5 */ 0x0210, 0x50,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $50 = BVC
        { /*    1.0 */ 0x0211, 0x50,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0212, 0xEE,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0212, 0xEA,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bvc_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BVC must not take branch when V=1");
    prog.org(0x0080).bytes({0x40});
    prog.org(0x0210)
        .bit(ZP{0x80})
        .bvc(wrong)
        .pass("BVC does not take branch when V=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BIT at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BIT
        { /*    0.5 */ 0x0210, 0x24,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $24 = BIT
        { /*    1.0 */ 0x0211, 0x24,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x80,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x40,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BVC at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0212, 0x40,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: BVC
        { /*    3.5 */ 0x0212, 0x50,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $50 = BVC
        { /*    4.0 */ 0x0213, 0x50,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // dummy / operand
        { /*    4.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },

        // --- PASS at $0214: NOP flushes pipelined registers, JMP-self at $0215 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0214, 0xEC,  R,    1, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0214, 0xEA,  R,    1, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $EA = NOP
        { /*    6.0 */ 0x0215, 0xEA,  R,    0, 0x0215, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // dummy / operand
        { /*    6.5 */ 0x0215, 0x4C,  R,    0, 0x0215, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },
        { /*    7.0 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bvs_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BVS takes branch when V=1");
    prog.org(0x0080).bytes({0x40});
    prog.org(0x0210)
        .bit(ZP{0x80})
        .bvs(taken)
        .fail("BVS must take branch when V=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BIT at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BIT
        { /*    0.5 */ 0x0210, 0x24,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $24 = BIT
        { /*    1.0 */ 0x0211, 0x24,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x80,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x40,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BVS at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0212, 0x40,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: BVS
        { /*    3.5 */ 0x0212, 0x70,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $70 = BVS
        { /*    4.0 */ 0x0213, 0x70,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // dummy / operand
        { /*    4.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },
        { /*    5.0 */ 0x0214, 0xEC,  R,    0, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // dummy / operand
        { /*    5.5 */ 0x0214, 0xEA,  R,    0, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $EA = NOP
        { /*    7.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // dummy / operand
        { /*    7.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },
        { /*    8.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b01100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bvs_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BVS must not take branch when V=0");
    prog.org(0x0210)
        .bvs(wrong)
        .pass("BVS does not take branch when V=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BVS at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BVS
        { /*    0.5 */ 0x0210, 0x70,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $70 = BVS
        { /*    1.0 */ 0x0211, 0x70,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0212: NOP flushes pipelined registers, JMP-self at $0213 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0xEE,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0212, 0xEA,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0213, 0xEA,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0x4C,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcc_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BCC takes branch when C=0");
    prog.org(0x0210)
        .bcc(taken)
        .fail("BCC must take branch when C=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BCC at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BCC
        { /*    0.5 */ 0x0210, 0x90,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $90 = BCC
        { /*    1.0 */ 0x0211, 0x90,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0212, 0xEE,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0212, 0xEA,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcc_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCC must not take branch when C=1");
    prog.org(0x0210)
        .sec()
        .bcc(wrong)
        .pass("BCC does not take branch when C=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0210, 0x38,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0211, 0x38,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x90,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BCC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x90,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: BCC
        { /*    2.5 */ 0x0211, 0x90,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $90 = BCC
        { /*    3.0 */ 0x0212, 0x90,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xED,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0213: NOP flushes pipelined registers, JMP-self at $0214 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0213, 0xED,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    4.5 */ 0x0213, 0xEA,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    5.0 */ 0x0214, 0xEA,  R,    0, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x0214, 0x4C,  R,    0, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    6.0 */ 0x0214, 0x4C,  R,    1, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    6.5 */ 0x0214, 0x4C,  R,    1, 0x0214, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcs_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BCS takes branch when C=1");
    prog.org(0x0210)
        .sec()
        .bcs(taken)
        .fail("BCS must take branch when C=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0210, 0x38,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0211, 0x38,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xB0,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BCS at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0xB0,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: BCS
        { /*    2.5 */ 0x0211, 0xB0,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $B0 = BCS
        { /*    3.0 */ 0x0212, 0xB0,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xED,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    4.0 */ 0x0213, 0xED,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    4.5 */ 0x0213, 0xEA,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    5.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    6.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    6.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    7.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    7.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcs_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCS must not take branch when C=0");
    prog.org(0x0210)
        .bcs(wrong)
        .pass("BCS does not take branch when C=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BCS at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BCS
        { /*    0.5 */ 0x0210, 0xB0,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $B0 = BCS
        { /*    1.0 */ 0x0211, 0xB0,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0212: NOP flushes pipelined registers, JMP-self at $0213 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0xEE,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0212, 0xEA,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0213, 0xEA,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0x4C,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bne_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BNE takes branch when Z=0");
    prog.org(0x0210)
        .lda(Imm{0x01})
        .bne(taken)
        .fail("BNE must take branch when Z=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0210, 0xA9,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0211, 0xA9,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x01,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BNE at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0x01,  R,    1, 0x0212, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: BNE
        { /*    2.5 */ 0x0212, 0xD0,  R,    1, 0x0212, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $D0 = BNE
        { /*    3.0 */ 0x0213, 0xD0,  R,    0, 0x0213, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0214, 0xEC,  R,    0, 0x0214, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0214, 0xEA,  R,    0, 0x0214, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bne_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BNE must not take branch when Z=1");
    prog.org(0x0210)
        .bne(wrong)
        .pass("BNE does not take branch when Z=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BNE at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BNE
        { /*    0.5 */ 0x0210, 0xD0,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $D0 = BNE
        { /*    1.0 */ 0x0211, 0xD0,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0212: NOP flushes pipelined registers, JMP-self at $0213 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0xEE,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0212, 0xEA,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0213, 0xEA,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0x4C,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0213, 0x4C,  R,    1, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(beq_taken) {
    Program prog = Program::start(0x0210);
    Label taken = prog.org(0x0200).pass("BEQ takes branch when Z=1");
    prog.org(0x0210)
        .beq(taken)
        .fail("BEQ must take branch when Z=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BEQ at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BEQ
        { /*    0.5 */ 0x0210, 0xF0,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F0 = BEQ
        { /*    1.0 */ 0x0211, 0xF0,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0xEE,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0212, 0xEE,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0212, 0xEA,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(beq_not_taken) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BEQ must not take branch when Z=0");
    prog.org(0x0210)
        .lda(Imm{0x01})
        .beq(wrong)
        .pass("BEQ does not take branch when Z=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0210, 0xA9,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0211, 0xA9,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x01,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BEQ at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0212, 0x01,  R,    1, 0x0212, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: BEQ
        { /*    2.5 */ 0x0212, 0xF0,  R,    1, 0x0212, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $F0 = BEQ
        { /*    3.0 */ 0x0213, 0xF0,  R,    0, 0x0213, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0213, 0xEC,  R,    0, 0x0213, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0214: NOP flushes pipelined registers, JMP-self at $0215 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0214, 0xEC,  R,    1, 0x0214, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0214, 0xEA,  R,    1, 0x0214, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0215, 0xEA,  R,    0, 0x0215, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0215, 0x4C,  R,    0, 0x0215, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0215, 0x4C,  R,    1, 0x0215, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
