// test3/tests/isa/logic_test.cpp
//
// Bitwise logic: AND, ORA, EOR, BIT.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110.
// AND, ORA, and EOR update N from bit 7 of the result and Z from
// result == 0. BIT updates N from bit 7 of the operand, V from bit 6
// of the operand, and Z from (A & operand) == 0.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(and_imm_zero) {
    // AND: $66 & $00 -> $00. N clears from result bit 7, Z sets from result == 0.
    Program prog = Program::start()
        .and_(Imm{0x00})
        .pass("AND immediate produced zero");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- AND at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: AND
        { /*    0.5 */ 0x0200, 0x29,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $29 = AND
        { /*    1.0 */ 0x0201, 0x29,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(and_imm_keeps_bits) {
    // AND: $66 & $0F -> $06. N stays clear from result bit 7, Z clears from result != 0.
    Program prog = Program::start()
        .and_(Imm{0x0F})
        .pass("AND immediate kept selected bits");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- AND at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: AND
        { /*    0.5 */ 0x0200, 0x29,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $29 = AND
        { /*    1.0 */ 0x0201, 0x29,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x0F,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x0F,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x06, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x06, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x06, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x06, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ora_imm_sets_n) {
    // ORA: $66 | $99 -> $FF. N sets from result bit 7, Z clears from result != 0.
    Program prog = Program::start()
        .ora(Imm{0x99})
        .pass("ORA immediate set bit 7");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ORA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ORA
        { /*    0.5 */ 0x0200, 0x09,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $09 = ORA
        { /*    1.0 */ 0x0201, 0x09,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x99,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x99,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ora_zp) {
    Program prog = Program::start();

    prog.org(0x0080).bytes({0x18});
    prog.org(0x0200)
        // ORA: $66 | $18 -> $7E. N stays clear from result bit 7, Z clears from result != 0.
        .ora(ZP{0x80})
        .pass("ORA zero-page read operand");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ORA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ORA
        { /*    0.5 */ 0x0200, 0x05,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $05 = ORA
        { /*    1.0 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x18,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x7E, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x7E, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x7E, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x7E, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ora_abs) {
    Program prog = Program::start();

    prog.org(0x1234).bytes({0x80});
    prog.org(0x0200)
        // ORA: $66 | $80 -> $E6. N sets from result bit 7, Z clears from result != 0.
        .ora(Abs{0x1234})
        .pass("ORA absolute read operand");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ORA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ORA
        { /*    0.5 */ 0x0200, 0x0D,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $0D = ORA
        { /*    1.0 */ 0x0201, 0x0D,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x34,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x34,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x12,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x1234, 0x12,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x1234, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x80,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0xE6, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0xE6, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0xE6, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0xE6, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(eor_imm_zero) {
    // EOR: $66 ^ $66 -> $00. N clears from result bit 7, Z sets from result == 0.
    Program prog = Program::start()
        .eor(Imm{0x66})
        .pass("EOR immediate produced zero");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- EOR at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: EOR
        { /*    0.5 */ 0x0200, 0x49,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $49 = EOR
        { /*    1.0 */ 0x0201, 0x49,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x66,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x66,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(eor_zp) {
    Program prog = Program::start();

    prog.org(0x0081).bytes({0xE6});
    prog.org(0x0200)
        // EOR: $66 ^ $E6 -> $80. N sets from result bit 7, Z clears from result != 0.
        .eor(ZP{0x81})
        .pass("EOR zero-page read operand");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- EOR at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: EOR
        { /*    0.5 */ 0x0200, 0x45,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $45 = EOR
        { /*    1.0 */ 0x0201, 0x45,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x81,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0081, 0x81,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0081, 0xE6,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0xE6,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bit_zp_sets_nv_clears_z) {
    Program prog = Program::start();

    prog.org(0x0082).bytes({0xC2});
    prog.org(0x0200)
        // BIT: operand $C2 sets N from bit 7 and V from bit 6. A & operand != 0 clears Z.
        .bit(ZP{0x82})
        .pass("BIT zero-page set N and V while clearing Z");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BIT at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BIT
        { /*    0.5 */ 0x0200, 0x24,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $24 = BIT
        { /*    1.0 */ 0x0201, 0x24,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x82,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0082, 0x82,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0082, 0xC2,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0xC2,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bit_abs_sets_z) {
    Program prog = Program::start();

    prog.org(0x1235).bytes({0x99});
    prog.org(0x0200)
        // BIT: operand $99 sets N from bit 7 and clears V from bit 6. A & operand == 0 sets Z.
        .bit(Abs{0x1235})
        .pass("BIT absolute set Z from masked result");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- BIT at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BIT
        { /*    0.5 */ 0x0200, 0x2C,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $2C = BIT
        { /*    1.0 */ 0x0201, 0x2C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x35,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x35,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x12,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x1235, 0x12,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x1235, 0x99,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x99,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100110 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100110 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b10100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
