// test3/tests/isa/arith_test.cpp
//
// Binary-mode arithmetic: ADC, SBC. Decimal mode lives in arith_bcd_test.cpp.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(adc_imm_no_carry_no_overflow) {
    Program prog = Program::start()
        .adc(Imm{0x10})
        .pass("ADC immediate without carry completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ADC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ADC
        { /*    0.5 */ 0x0200, 0x69,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $69 = ADC
        { /*    1.0 */ 0x0201, 0x69,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_imm_carry_out) {
    Program prog = Program::start()
        .adc(Imm{0xC0})
        .pass("ADC immediate carry out completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ADC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ADC
        { /*    0.5 */ 0x0200, 0x69,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $69 = ADC
        { /*    1.0 */ 0x0201, 0x69,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xC0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0xC0,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x26, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x26, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x26, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x26, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_imm_overflow) {
    Program prog = Program::start()
        .lda(Imm{0x50})
        .adc(Imm{0x50})
        .pass("ADC immediate overflow completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x50,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x50,  R,    1, 0x0202, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x69,  R,    1, 0x0202, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },  // $69 = ADC
        { /*    3.0 */ 0x0203, 0x69,  R,    0, 0x0203, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x50,  R,    0, 0x0203, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x50,  R,    1, 0x0204, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x50, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0xA0, 0x00, 0x00, 0xFD, 0b11100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0xA0, 0x00, 0x00, 0xFD, 0b11100100 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0xA0, 0x00, 0x00, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0xA0, 0x00, 0x00, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_imm_zero_with_carry) {
    Program prog = Program::start()
        .lda(Imm{0x00})
        .adc(Imm{0x00})
        .pass("ADC immediate zero result completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x69,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $69 = ADC
        { /*    3.0 */ 0x0203, 0x69,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x00,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_zp) {
    Program prog = Program::start()
        .adc(ZP{0x80})
        .pass("ADC zero page completed");

    prog.org(0x0080).bytes({0x10}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ADC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ADC
        { /*    0.5 */ 0x0200, 0x65,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $65 = ADC
        { /*    1.0 */ 0x0201, 0x65,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x10,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_zpx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .adc(ZPX{0x80})
        .pass("ADC zero page indexed by X completed");

    prog.org(0x0084).bytes({0x20}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x75,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $75 = ADC
        { /*    3.0 */ 0x0203, 0x75,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0084, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0084, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_abs) {
    Program prog = Program::start()
        .adc(Abs{0x2400})
        .pass("ADC absolute completed");

    prog.org(0x2400).bytes({0x10}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ADC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ADC
        { /*    0.5 */ 0x0200, 0x6D,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $6D = ADC
        { /*    1.0 */ 0x0201, 0x6D,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x24,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x2400, 0x24,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x2400, 0x10,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x10,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x76, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_absx) {
    Program prog = Program::start()
        .ldx(Imm{0x03})
        .adc(AbsX{0x2400})
        .pass("ADC absolute indexed by X completed");

    prog.org(0x2403).bytes({0x20}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x03,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x03,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x7D,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // $7D = ADC
        { /*    3.0 */ 0x0203, 0x7D,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2403, 0x24,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2403, 0x20,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x20,  R,    1, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x86, 0x03, 0x00, 0xFD, 0b11100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x86, 0x03, 0x00, 0xFD, 0b11100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x86, 0x03, 0x00, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x86, 0x03, 0x00, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_absy) {
    Program prog = Program::start()
        .ldy(Imm{0x05})
        .adc(AbsY{0x2400})
        .pass("ADC absolute indexed by Y completed");

    prog.org(0x2405).bytes({0x30}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x79,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $79 = ADC
        { /*    3.0 */ 0x0203, 0x79,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x2405, 0x24,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x2405, 0x30,  R,    0, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x30,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x96, 0x00, 0x05, 0xFD, 0b11100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x96, 0x00, 0x05, 0xFD, 0b11100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x96, 0x00, 0x05, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x96, 0x00, 0x05, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_indx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .adc(IndX{0x40})
        .pass("ADC indexed indirect completed");

    prog.org(0x0044).bytes({0x00, 0x24}).org(0x0200);
    prog.org(0x2400).bytes({0x20}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x61,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $61 = ADC
        { /*    3.0 */ 0x0203, 0x61,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x40,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0040, 0x40,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0040, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0044, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0044, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0045, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0045, 0x24,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x2400, 0x24,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x2400, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x20,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x86, 0x04, 0x00, 0xFD, 0b11100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(adc_indy) {
    Program prog = Program::start()
        .ldy(Imm{0x05})
        .adc(IndY{0x50})
        .pass("ADC indirect indexed completed");

    prog.org(0x0050).bytes({0x00, 0x24}).org(0x0200);
    prog.org(0x2405).bytes({0x10}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ADC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    2.5 */ 0x0202, 0x71,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $71 = ADC
        { /*    3.0 */ 0x0203, 0x71,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
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
        { /*    8.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x76, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x76, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x76, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x76, 0x00, 0x05, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sbc_imm_simple) {
    Program prog = Program::start()
        .sec()
        .sbc(Imm{0x10})
        .pass("SBC immediate completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xE9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SBC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xE9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: SBC
        { /*    2.5 */ 0x0201, 0xE9,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $E9 = SBC
        { /*    3.0 */ 0x0202, 0xE9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x10,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x10,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sbc_imm_borrow_out) {
    Program prog = Program::start()
        .lda(Imm{0x10})
        .sec()
        .sbc(Imm{0x66})
        .pass("SBC immediate borrow out completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: SEC
        { /*    2.5 */ 0x0202, 0x38,  R,    1, 0x0202, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // $38 = SEC
        { /*    3.0 */ 0x0203, 0x38,  R,    0, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xE9,  R,    0, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- SBC at $0203 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0xE9,  R,    1, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: SBC
        { /*    4.5 */ 0x0203, 0xE9,  R,    1, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },  // $E9 = SBC
        { /*    5.0 */ 0x0204, 0xE9,  R,    0, 0x0204, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x66,  R,    0, 0x0204, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x66,  R,    1, 0x0205, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x10, 0x00, 0x00, 0xFD, 0b00100101 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0xAA, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0xAA, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0xAA, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0xAA, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sbc_imm_overflow_neg) {
    Program prog = Program::start()
        .lda(Imm{0x80})
        .sec()
        .sbc(Imm{0x01})
        .pass("SBC immediate overflow completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: SEC
        { /*    2.5 */ 0x0202, 0x38,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $38 = SEC
        { /*    3.0 */ 0x0203, 0x38,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xE9,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- SBC at $0203 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0xE9,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // SYNC: SBC
        { /*    4.5 */ 0x0203, 0xE9,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // $E9 = SBC
        { /*    5.0 */ 0x0204, 0xE9,  R,    0, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x01,  R,    0, 0x0204, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x01,  R,    1, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x80, 0x00, 0x00, 0xFD, 0b10100101 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x7F, 0x00, 0x00, 0xFD, 0b01100101 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x7F, 0x00, 0x00, 0xFD, 0b01100101 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x7F, 0x00, 0x00, 0xFD, 0b01100101 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x7F, 0x00, 0x00, 0xFD, 0b01100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sbc_zp) {
    Program prog = Program::start()
        .sec()
        .sbc(ZP{0x80})
        .pass("SBC zero page completed");

    prog.org(0x0080).bytes({0x10}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xE5,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SBC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xE5,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: SBC
        { /*    2.5 */ 0x0201, 0xE5,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $E5 = SBC
        { /*    3.0 */ 0x0202, 0xE5,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x10,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0203, 0x10,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    5.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    6.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    6.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    7.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    7.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sbc_abs) {
    Program prog = Program::start()
        .sec()
        .sbc(Abs{0x2400})
        .pass("SBC absolute completed");

    prog.org(0x2400).bytes({0x10}).org(0x0200);

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xED,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SBC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xED,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: SBC
        { /*    2.5 */ 0x0201, 0xED,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $ED = SBC
        { /*    3.0 */ 0x0202, 0xED,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    4.0 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x24,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    5.0 */ 0x2400, 0x24,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    5.5 */ 0x2400, 0x10,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x10,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x56, 0x00, 0x00, 0xFD, 0b00100101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
