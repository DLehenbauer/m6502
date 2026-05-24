// test3/tests/isa/shift_test.cpp
//
// Shifts and rotates: ASL, LSR, ROL, ROR.
// Accumulator tests cover result flags and carry behavior.
// Memory tests cover read-modify-write bus timing across representative modes.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(asl_acc_basic) {
    Program prog = Program::start()
        .asl()
        .pass("ASL accumulator completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ASL at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ASL
        { /*    0.5 */ 0x0200, 0x0A,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $0A = ASL
        { /*    1.0 */ 0x0201, 0x0A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_acc_carry_out) {
    Program prog = Program::start()
        .lda(Imm{0x81})
        .asl()
        .pass("ASL accumulator carry out completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x81,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ASL at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x81,  R,    1, 0x0202, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: ASL
        { /*    2.5 */ 0x0202, 0x0A,  R,    1, 0x0202, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },  // $0A = ASL
        { /*    3.0 */ 0x0203, 0x0A,  R,    0, 0x0203, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x81, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x02, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x02, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x02, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x02, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_acc_to_zero) {
    Program prog = Program::start()
        .lda(Imm{0x80})
        .asl()
        .pass("ASL accumulator to zero completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ASL at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: ASL
        { /*    2.5 */ 0x0202, 0x0A,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $0A = ASL
        { /*    3.0 */ 0x0203, 0x0A,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_zp) {
    Program prog = Program::start()
        .asl(ZP{0x80})
        .pass("ASL zero page completed");

    prog.org(0x0080).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ASL at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ASL
        { /*    0.5 */ 0x0200, 0x06,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $06 = ASL
        { /*    1.0 */ 0x0201, 0x06,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    3.5 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    4.0 */ 0x0080, 0x84,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0080
        { /*    4.5 */ 0x0080, 0x84,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x42,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_zpx) {
    Program prog = Program::start()
        .ldx(Imm{0x05})
        .asl(ZPX{0x80})
        .pass("ASL zero page X completed");

    prog.org(0x0085).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ASL at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: ASL
        { /*    2.5 */ 0x0202, 0x16,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $16 = ASL
        { /*    3.0 */ 0x0203, 0x16,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0085, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0085, 0x42,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0085, 0x42,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0085
        { /*    6.5 */ 0x0085, 0x42,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0085
        { /*    7.0 */ 0x0085, 0x84,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0085
        { /*    7.5 */ 0x0085, 0x84,  W,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0085

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x42,  R,    1, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_abs) {
    Program prog = Program::start()
        .asl(Abs{0x2400})
        .pass("ASL absolute completed");

    prog.org(0x2400).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ASL at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ASL
        { /*    0.5 */ 0x0200, 0x0E,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $0E = ASL
        { /*    1.0 */ 0x0201, 0x0E,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x24,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x2400, 0x24,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x2400, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x2400, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $2400
        { /*    4.5 */ 0x2400, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $2400
        { /*    5.0 */ 0x2400, 0x84,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $2400
        { /*    5.5 */ 0x2400, 0x84,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $2400

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0203, 0x42,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    7.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lsr_acc_basic) {
    Program prog = Program::start()
        .lsr()
        .pass("LSR accumulator completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LSR at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LSR
        { /*    0.5 */ 0x0200, 0x4A,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4A = LSR
        { /*    1.0 */ 0x0201, 0x4A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lsr_acc_to_zero) {
    Program prog = Program::start()
        .lda(Imm{0x01})
        .lsr()
        .pass("LSR accumulator to zero completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x01,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LSR at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x01,  R,    1, 0x0202, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: LSR
        { /*    2.5 */ 0x0202, 0x4A,  R,    1, 0x0202, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $4A = LSR
        { /*    3.0 */ 0x0203, 0x4A,  R,    0, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lsr_zp) {
    Program prog = Program::start()
        .lsr(ZP{0x80})
        .pass("LSR zero page completed");

    prog.org(0x0080).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LSR at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LSR
        { /*    0.5 */ 0x0200, 0x46,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $46 = LSR
        { /*    1.0 */ 0x0201, 0x46,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    3.5 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    4.0 */ 0x0080, 0x21,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $21 -> $0080
        { /*    4.5 */ 0x0080, 0x21,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $21 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x42,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lsr_absx) {
    Program prog = Program::start()
        .ldx(Imm{0x05})
        .lsr(AbsX{0x2400})
        .pass("LSR absolute X completed");

    prog.org(0x2405).bytes({0x01});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LSR at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: LSR
        { /*    2.5 */ 0x0202, 0x5E,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $5E = LSR
        { /*    3.0 */ 0x0203, 0x5E,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2405, 0x24,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2405, 0x01,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x2405, 0x01,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x2405, 0x01,  R,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x2405, 0x01,  W,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100101 },  // write $01 -> $2405
        { /*    7.5 */ 0x2405, 0x01,  W,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100101 },  // write $01 -> $2405
        { /*    8.0 */ 0x2405, 0x00,  W,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // write $00 -> $2405
        { /*    8.5 */ 0x2405, 0x00,  W,    0, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // write $00 -> $2405

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0205, 0x01,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    9.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*   10.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*   10.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },
        { /*   11.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*   11.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x05, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(rol_acc_basic) {
    Program prog = Program::start()
        .clc()
        .rol()
        .pass("ROL accumulator completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CLC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLC
        { /*    0.5 */ 0x0200, 0x18,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $18 = CLC
        { /*    1.0 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x2A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ROL at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x2A,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ROL
        { /*    2.5 */ 0x0201, 0x2A,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $2A = ROL
        { /*    3.0 */ 0x0202, 0x2A,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0xCC, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(rol_acc_with_carry_in) {
    Program prog = Program::start()
        .sec()
        .lda(Imm{0x80})
        .rol()
        .pass("ROL accumulator carry in completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xA9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: LDA
        { /*    2.5 */ 0x0201, 0xA9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $A9 = LDA
        { /*    3.0 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- ROL at $0203 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x80,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // SYNC: ROL
        { /*    4.5 */ 0x0203, 0x2A,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // $2A = ROL
        { /*    5.0 */ 0x0204, 0x2A,  R,    0, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(rol_zp) {
    Program prog = Program::start()
        .clc()
        .rol(ZP{0x80})
        .pass("ROL zero page completed");

    prog.org(0x0080).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CLC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLC
        { /*    0.5 */ 0x0200, 0x18,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $18 = CLC
        { /*    1.0 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x26,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ROL at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x26,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ROL
        { /*    2.5 */ 0x0201, 0x26,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $26 = ROL
        { /*    3.0 */ 0x0202, 0x26,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0080, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    5.5 */ 0x0080, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    6.0 */ 0x0080, 0x84,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0080
        { /*    6.5 */ 0x0080, 0x84,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $84 -> $0080

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0203, 0x42,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    8.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    8.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    9.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ror_acc_basic) {
    Program prog = Program::start()
        .clc()
        .ror()
        .pass("ROR accumulator completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CLC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLC
        { /*    0.5 */ 0x0200, 0x18,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $18 = CLC
        { /*    1.0 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x6A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ROR at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x6A,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ROR
        { /*    2.5 */ 0x0201, 0x6A,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $6A = ROR
        { /*    3.0 */ 0x0202, 0x6A,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x33, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ror_acc_with_carry_in) {
    Program prog = Program::start()
        .sec()
        .lda(Imm{0x01})
        .ror()
        .pass("ROR accumulator carry in completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xA9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: LDA
        { /*    2.5 */ 0x0201, 0xA9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $A9 = LDA
        { /*    3.0 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x01,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- ROR at $0203 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x01,  R,    1, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: ROR
        { /*    4.5 */ 0x0203, 0x6A,  R,    1, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // $6A = ROR
        { /*    5.0 */ 0x0204, 0x6A,  R,    0, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100101 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ror_zp) {
    Program prog = Program::start()
        .clc()
        .ror(ZP{0x80})
        .pass("ROR zero page completed");

    prog.org(0x0080).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- CLC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLC
        { /*    0.5 */ 0x0200, 0x18,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $18 = CLC
        { /*    1.0 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x66,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ROR at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x66,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ROR
        { /*    2.5 */ 0x0201, 0x66,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $66 = ROR
        { /*    3.0 */ 0x0202, 0x66,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0080, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    5.5 */ 0x0080, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0080
        { /*    6.0 */ 0x0080, 0x21,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $21 -> $0080
        { /*    6.5 */ 0x0080, 0x21,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $21 -> $0080

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0203, 0x42,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
