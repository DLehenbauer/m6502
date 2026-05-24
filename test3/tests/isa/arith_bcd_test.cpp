// test3/tests/isa/arith_bcd_test.cpp
//
// Decimal-mode arithmetic: ADC, SBC with D flag set.
// NMOS leaves N/V/Z undefined after decimal-mode ADC/SBC. These tests
// document what aholme produces.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110, D = 0.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(bcd_adc_simple) {
    Program prog = Program::start()
        .sed()
        .clc()
        .lda(Imm{0x15})
        .adc(Imm{0x27})
        .pass("decimal ADC without carry completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x15,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x15,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0204, 0x69,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*    7.0 */ 0x0205, 0x69,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x27,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x27,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_with_carry_in) {
    Program prog = Program::start()
        .sed()
        .sec()
        .lda(Imm{0x15})
        .adc(Imm{0x27})
        .pass("decimal ADC with carry-in completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: SEC
        { /*    2.5 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $38 = SEC
        { /*    3.0 */ 0x0202, 0x38,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x15,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x15,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: ADC
        { /*    6.5 */ 0x0204, 0x69,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // $69 = ADC
        { /*    7.0 */ 0x0205, 0x69,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x27,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x27,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x43, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x43, 0x00, 0x00, 0xFD, 0b00101100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x43, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x43, 0x00, 0x00, 0xFD, 0b00101100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_carry_out) {
    Program prog = Program::start()
        .sed()
        .clc()
        .lda(Imm{0x65})
        .adc(Imm{0x55})
        .pass("decimal ADC with carry-out completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x65,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x65,  R,    1, 0x0204, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0204, 0x69,  R,    1, 0x0204, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*    7.0 */ 0x0205, 0x69,  R,    0, 0x0205, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x55,  R,    0, 0x0205, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x55,  R,    1, 0x0206, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x65, 0x00, 0x00, 0xFD, 0b00101100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x20, 0x00, 0x00, 0xFD, 0b11101101 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x20, 0x00, 0x00, 0xFD, 0b11101101 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x20, 0x00, 0x00, 0xFD, 0b11101101 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x20, 0x00, 0x00, 0xFD, 0b11101101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_invalid_nibble) {
    Program prog = Program::start()
        .sed()
        .clc()
        .lda(Imm{0x0A})
        .adc(Imm{0x01})
        .pass("decimal ADC with invalid BCD digit completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x0A,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x0A,  R,    1, 0x0204, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0204, 0x69,  R,    1, 0x0204, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*    7.0 */ 0x0205, 0x69,  R,    0, 0x0205, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x01,  R,    0, 0x0205, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x01,  R,    1, 0x0206, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x0A, 0x00, 0x00, 0xFD, 0b00101100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x11, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x11, 0x00, 0x00, 0xFD, 0b00101100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x11, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x11, 0x00, 0x00, 0xFD, 0b00101100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_sbc_simple) {
    Program prog = Program::start()
        .sed()
        .sec()
        .lda(Imm{0x42})
        .sbc(Imm{0x15})
        .pass("decimal SBC without borrow completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: SEC
        { /*    2.5 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $38 = SEC
        { /*    3.0 */ 0x0202, 0x38,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- SBC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x42,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: SBC
        { /*    6.5 */ 0x0204, 0xE9,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },  // $E9 = SBC
        { /*    7.0 */ 0x0205, 0xE9,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x15,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x15,  R,    1, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00101101 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x27, 0x00, 0x00, 0xFD, 0b00101101 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x27, 0x00, 0x00, 0xFD, 0b00101101 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x27, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x27, 0x00, 0x00, 0xFD, 0b00101101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_sbc_borrow_out) {
    Program prog = Program::start()
        .sed()
        .sec()
        .lda(Imm{0x15})
        .sbc(Imm{0x42})
        .pass("decimal SBC with borrow-out completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: SEC
        { /*    2.5 */ 0x0201, 0x38,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $38 = SEC
        { /*    3.0 */ 0x0202, 0x38,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x15,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- SBC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x15,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: SBC
        { /*    6.5 */ 0x0204, 0xE9,  R,    1, 0x0204, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // $E9 = SBC
        { /*    7.0 */ 0x0205, 0xE9,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x42,  R,    0, 0x0205, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x42,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x15, 0x00, 0x00, 0xFD, 0b00101101 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x73, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x73, 0x00, 0x00, 0xFD, 0b10101100 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x73, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x73, 0x00, 0x00, 0xFD, 0b10101100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_sbc_with_borrow_in) {
    Program prog = Program::start()
        .sed()
        .clc()
        .lda(Imm{0x42})
        .sbc(Imm{0x15})
        .pass("decimal SBC with borrow-in completes");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xA9,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0202, 0xA9,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0203, 0xA9,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- SBC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x42,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: SBC
        { /*    6.5 */ 0x0204, 0xE9,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // $E9 = SBC
        { /*    7.0 */ 0x0205, 0xE9,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x15,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x15,  R,    1, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00101100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x26, 0x00, 0x00, 0xFD, 0b00101101 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x26, 0x00, 0x00, 0xFD, 0b00101101 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x26, 0x00, 0x00, 0xFD, 0b00101101 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x26, 0x00, 0x00, 0xFD, 0b00101101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
