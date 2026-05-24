// test3/tests/corners/decimal_carry_chain_test.cpp
//
// BCD carry chains and NMOS decimal-mode flag corners.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(bcd_adc_99_plus_99) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCD ADC $99+$99 must produce A=$98 C=1");
    prog.org(0x0210)
        .sed()
        .clc()
        .lda(Imm{0x99})
        .adc(Imm{0x99})
        .bcc(wrong)
        .cmp(Imm{0x98})
        .bne(wrong)
        .pass("BCD ADC $99+$99 produces A=$98 C=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0210, 0xF8,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0211, 0xF8,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x18,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0212, 0x18,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xA9,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0213, 0xA9,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0213, 0x99,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0214 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0214, 0x99,  R,    1, 0x0214, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0214, 0x69,  R,    1, 0x0214, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $69 = ADC
        { /*    7.0 */ 0x0215, 0x69,  R,    0, 0x0215, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*    7.5 */ 0x0215, 0x99,  R,    0, 0x0215, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- BCC at $0216 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0216, 0x99,  R,    1, 0x0216, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: BCC
        { /*    8.5 */ 0x0216, 0x90,  R,    1, 0x0216, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $90 = BCC
        { /*    9.0 */ 0x0217, 0x90,  R,    0, 0x0217, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // dummy / operand
        { /*    9.5 */ 0x0217, 0xE8,  R,    0, 0x0217, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },

        // --- CMP at $0218 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   10.0 */ 0x0218, 0xE8,  R,    1, 0x0218, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // SYNC: CMP
        { /*   10.5 */ 0x0218, 0xC9,  R,    1, 0x0218, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // $C9 = CMP
        { /*   11.0 */ 0x0219, 0xC9,  R,    0, 0x0219, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // dummy / operand
        { /*   11.5 */ 0x0219, 0x98,  R,    0, 0x0219, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },

        // --- BNE at $021A ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x021A, 0x98,  R,    1, 0x021A, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // SYNC: BNE
        { /*   12.5 */ 0x021A, 0xD0,  R,    1, 0x021A, 0x98, 0x00, 0x00, 0xFD, 0b01101101 },  // $D0 = BNE
        { /*   13.0 */ 0x021B, 0xD0,  R,    0, 0x021B, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   13.5 */ 0x021B, 0xE4,  R,    0, 0x021B, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },

        // --- PASS at $021C: NOP flushes pipelined registers, JMP-self at $021D traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   14.0 */ 0x021C, 0xE4,  R,    1, 0x021C, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: NOP
        { /*   14.5 */ 0x021C, 0xEA,  R,    1, 0x021C, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // $EA = NOP
        { /*   15.0 */ 0x021D, 0xEA,  R,    0, 0x021D, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   15.5 */ 0x021D, 0x4C,  R,    0, 0x021D, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },
        { /*   16.0 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: JMP
        { /*   16.5 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x98, 0x00, 0x00, 0xFD, 0b01101111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_99_plus_01) {
    // NMOS BCD quirk: $99 + $01 produces A=$00, C=1, but Z reflects the
    // BINARY result ($9A), not the BCD-adjusted A. So Z=0 here, even
    // though the BCD-visible A is zero. The trace documents the actual
    // NMOS flag behavior; CMOS 65C02 fixes this so Z would track A.
    Program prog = Program::start()
        .sed()
        .clc()
        .lda(Imm{0x99})
        .adc(Imm{0x01})
        .pass("BCD ADC $99+$01: A=$00 C=1 (Z reflects binary $9A on NMOS)");

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
        { /*    5.5 */ 0x0203, 0x99,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x99,  R,    1, 0x0204, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0204, 0x69,  R,    1, 0x0204, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $69 = ADC
        { /*    7.0 */ 0x0205, 0x69,  R,    0, 0x0205, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x01,  R,    0, 0x0205, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0206, 0x01,  R,    1, 0x0206, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: NOP
        { /*    8.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $EA = NOP
        { /*    9.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x00, 0x00, 0x00, 0xFD, 0b10101101 },  // dummy / operand
        { /*    9.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x00, 0x00, 0x00, 0xFD, 0b10101101 },
        { /*   10.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x00, 0x00, 0x00, 0xFD, 0b10101101 },  // SYNC: JMP
        { /*   10.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x00, 0x00, 0x00, 0xFD, 0b10101101 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_50_plus_50_overflow_v) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCD ADC $50+$50 must produce A=$00 C=1");
    prog.org(0x0210)
        .sed()
        .clc()
        .lda(Imm{0x50})
        .adc(Imm{0x50})
        .bcc(wrong)
        .cmp(Imm{0x00})
        .bne(wrong)
        .pass("BCD ADC $50+$50 produces A=$00 C=1");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0210, 0xF8,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0211, 0xF8,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x18,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0212, 0x18,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xA9,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0213, 0xA9,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0213, 0x50,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0214 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0214, 0x50,  R,    1, 0x0214, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0214, 0x69,  R,    1, 0x0214, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*    7.0 */ 0x0215, 0x69,  R,    0, 0x0215, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0215, 0x50,  R,    0, 0x0215, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- BCC at $0216 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0216, 0x50,  R,    1, 0x0216, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: BCC
        { /*    8.5 */ 0x0216, 0x90,  R,    1, 0x0216, 0x50, 0x00, 0x00, 0xFD, 0b00101100 },  // $90 = BCC
        { /*    9.0 */ 0x0217, 0x90,  R,    0, 0x0217, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // dummy / operand
        { /*    9.5 */ 0x0217, 0xE8,  R,    0, 0x0217, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },

        // --- CMP at $0218 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   10.0 */ 0x0218, 0xE8,  R,    1, 0x0218, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // SYNC: CMP
        { /*   10.5 */ 0x0218, 0xC9,  R,    1, 0x0218, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // $C9 = CMP
        { /*   11.0 */ 0x0219, 0xC9,  R,    0, 0x0219, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // dummy / operand
        { /*   11.5 */ 0x0219, 0x00,  R,    0, 0x0219, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },

        // --- BNE at $021A ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x021A, 0x00,  R,    1, 0x021A, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // SYNC: BNE
        { /*   12.5 */ 0x021A, 0xD0,  R,    1, 0x021A, 0x00, 0x00, 0x00, 0xFD, 0b11101101 },  // $D0 = BNE
        { /*   13.0 */ 0x021B, 0xD0,  R,    0, 0x021B, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   13.5 */ 0x021B, 0xE4,  R,    0, 0x021B, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },

        // --- PASS at $021C: NOP flushes pipelined registers, JMP-self at $021D traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   14.0 */ 0x021C, 0xE4,  R,    1, 0x021C, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: NOP
        { /*   14.5 */ 0x021C, 0xEA,  R,    1, 0x021C, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // $EA = NOP
        { /*   15.0 */ 0x021D, 0xEA,  R,    0, 0x021D, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   15.5 */ 0x021D, 0x4C,  R,    0, 0x021D, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },
        { /*   16.0 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: JMP
        { /*   16.5 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x00, 0x00, 0x00, 0xFD, 0b01101111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_sbc_00_minus_01) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCD SBC $00-$01 must produce A=$99 C=0");
    prog.org(0x0210)
        .sed()
        .sec()
        .lda(Imm{0x00})
        .sbc(Imm{0x01})
        .bcs(wrong)
        .cmp(Imm{0x99})
        .bne(wrong)
        .pass("BCD SBC $00-$01 produces A=$99 C=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0210, 0xF8,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0211, 0xF8,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x38,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x38,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: SEC
        { /*    2.5 */ 0x0211, 0x38,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $38 = SEC
        { /*    3.0 */ 0x0212, 0x38,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xA9,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: LDA
        { /*    4.5 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // $A9 = LDA
        { /*    5.0 */ 0x0213, 0xA9,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    5.5 */ 0x0213, 0x00,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- SBC at $0214 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0214, 0x00,  R,    1, 0x0214, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: SBC
        { /*    6.5 */ 0x0214, 0xE9,  R,    1, 0x0214, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },  // $E9 = SBC
        { /*    7.0 */ 0x0215, 0xE9,  R,    0, 0x0215, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    7.5 */ 0x0215, 0x01,  R,    0, 0x0215, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- BCS at $0216 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0216, 0x01,  R,    1, 0x0216, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: BCS
        { /*    8.5 */ 0x0216, 0xB0,  R,    1, 0x0216, 0x00, 0x00, 0x00, 0xFD, 0b00101111 },  // $B0 = BCS
        { /*    9.0 */ 0x0217, 0xB0,  R,    0, 0x0217, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*    9.5 */ 0x0217, 0xE8,  R,    0, 0x0217, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- CMP at $0218 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   10.0 */ 0x0218, 0xE8,  R,    1, 0x0218, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: CMP
        { /*   10.5 */ 0x0218, 0xC9,  R,    1, 0x0218, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $C9 = CMP
        { /*   11.0 */ 0x0219, 0xC9,  R,    0, 0x0219, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*   11.5 */ 0x0219, 0x99,  R,    0, 0x0219, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- BNE at $021A ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x021A, 0x99,  R,    1, 0x021A, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: BNE
        { /*   12.5 */ 0x021A, 0xD0,  R,    1, 0x021A, 0x99, 0x00, 0x00, 0xFD, 0b10101100 },  // $D0 = BNE
        { /*   13.0 */ 0x021B, 0xD0,  R,    0, 0x021B, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*   13.5 */ 0x021B, 0xE4,  R,    0, 0x021B, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- PASS at $021C: NOP flushes pipelined registers, JMP-self at $021D traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   14.0 */ 0x021C, 0xE4,  R,    1, 0x021C, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: NOP
        { /*   14.5 */ 0x021C, 0xEA,  R,    1, 0x021C, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // $EA = NOP
        { /*   15.0 */ 0x021D, 0xEA,  R,    0, 0x021D, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*   15.5 */ 0x021D, 0x4C,  R,    0, 0x021D, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },
        { /*   16.0 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: JMP
        { /*   16.5 */ 0x021D, 0x4C,  R,    1, 0x021D, 0x99, 0x00, 0x00, 0xFD, 0b00101111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_adc_chain_two_ops) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCD ADC chain must propagate carry between operations");
    prog.org(0x0210)
        .sed()
        .clc()
        .lda(Imm{0x25})
        .adc(Imm{0x37})
        .bcs(wrong)
        .adc(Imm{0x50})
        .bcc(wrong)
        .cmp(Imm{0x12})
        .bne(wrong)
        .pass("BCD ADC chain propagates carry between operations");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0210, 0xF8,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0211, 0xF8,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x18,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLC
        { /*    2.5 */ 0x0211, 0x18,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $18 = CLC
        { /*    3.0 */ 0x0212, 0x18,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xA9,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: LDA
        { /*    4.5 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $A9 = LDA
        { /*    5.0 */ 0x0213, 0xA9,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    5.5 */ 0x0213, 0x25,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- ADC at $0214 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0214, 0x25,  R,    1, 0x0214, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*    6.5 */ 0x0214, 0x69,  R,    1, 0x0214, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*    7.0 */ 0x0215, 0x69,  R,    0, 0x0215, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    7.5 */ 0x0215, 0x37,  R,    0, 0x0215, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- BCS at $0216 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0216, 0x37,  R,    1, 0x0216, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: BCS
        { /*    8.5 */ 0x0216, 0xB0,  R,    1, 0x0216, 0x25, 0x00, 0x00, 0xFD, 0b00101100 },  // $B0 = BCS
        { /*    9.0 */ 0x0217, 0xB0,  R,    0, 0x0217, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*    9.5 */ 0x0217, 0xE8,  R,    0, 0x0217, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- ADC at $0218 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   10.0 */ 0x0218, 0xE8,  R,    1, 0x0218, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: ADC
        { /*   10.5 */ 0x0218, 0x69,  R,    1, 0x0218, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // $69 = ADC
        { /*   11.0 */ 0x0219, 0x69,  R,    0, 0x0219, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // dummy / operand
        { /*   11.5 */ 0x0219, 0x50,  R,    0, 0x0219, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },

        // --- BCC at $021A ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x021A, 0x50,  R,    1, 0x021A, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // SYNC: BCC
        { /*   12.5 */ 0x021A, 0x90,  R,    1, 0x021A, 0x62, 0x00, 0x00, 0xFD, 0b00101100 },  // $90 = BCC
        { /*   13.0 */ 0x021B, 0x90,  R,    0, 0x021B, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // dummy / operand
        { /*   13.5 */ 0x021B, 0xE4,  R,    0, 0x021B, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },

        // --- CMP at $021C ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   14.0 */ 0x021C, 0xE4,  R,    1, 0x021C, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // SYNC: CMP
        { /*   14.5 */ 0x021C, 0xC9,  R,    1, 0x021C, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // $C9 = CMP
        { /*   15.0 */ 0x021D, 0xC9,  R,    0, 0x021D, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // dummy / operand
        { /*   15.5 */ 0x021D, 0x12,  R,    0, 0x021D, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },

        // --- BNE at $021E ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   16.0 */ 0x021E, 0x12,  R,    1, 0x021E, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // SYNC: BNE
        { /*   16.5 */ 0x021E, 0xD0,  R,    1, 0x021E, 0x12, 0x00, 0x00, 0xFD, 0b11101101 },  // $D0 = BNE
        { /*   17.0 */ 0x021F, 0xD0,  R,    0, 0x021F, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   17.5 */ 0x021F, 0xE0,  R,    0, 0x021F, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },

        // --- PASS at $0220: NOP flushes pipelined registers, JMP-self at $0221 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   18.0 */ 0x0220, 0xE0,  R,    1, 0x0220, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: NOP
        { /*   18.5 */ 0x0220, 0xEA,  R,    1, 0x0220, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // $EA = NOP
        { /*   19.0 */ 0x0221, 0xEA,  R,    0, 0x0221, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // dummy / operand
        { /*   19.5 */ 0x0221, 0x4C,  R,    0, 0x0221, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },
        { /*   20.0 */ 0x0221, 0x4C,  R,    1, 0x0221, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // SYNC: JMP
        { /*   20.5 */ 0x0221, 0x4C,  R,    1, 0x0221, 0x12, 0x00, 0x00, 0xFD, 0b01101111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(bcd_sbc_chain_two_ops) {
    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200).fail("BCD SBC chain must propagate borrow between operations");
    prog.org(0x0210)
        .sed()
        .sec()
        .lda(Imm{0x80})
        .sbc(Imm{0x25})
        .bcc(wrong)
        .sbc(Imm{0x66})
        .bcs(wrong)
        .cmp(Imm{0x89})
        .bne(wrong)
        .pass("BCD SBC chain propagates borrow between operations");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- SED at $0210 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0210, 0x02,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0210, 0xF8,  R,    1, 0x0210, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0211, 0xF8,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0211, 0x38,  R,    0, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- SEC at $0211 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0211, 0x38,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: SEC
        { /*    2.5 */ 0x0211, 0x38,  R,    1, 0x0211, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $38 = SEC
        { /*    3.0 */ 0x0212, 0x38,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0212, 0xA9,  R,    0, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- LDA at $0212 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: LDA
        { /*    4.5 */ 0x0212, 0xA9,  R,    1, 0x0212, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // $A9 = LDA
        { /*    5.0 */ 0x0213, 0xA9,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*    5.5 */ 0x0213, 0x80,  R,    0, 0x0213, 0x66, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- SBC at $0214 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0214, 0x80,  R,    1, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },  // SYNC: SBC
        { /*    6.5 */ 0x0214, 0xE9,  R,    1, 0x0214, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },  // $E9 = SBC
        { /*    7.0 */ 0x0215, 0xE9,  R,    0, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },  // dummy / operand
        { /*    7.5 */ 0x0215, 0x25,  R,    0, 0x0215, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },

        // --- BCC at $0216 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0216, 0x25,  R,    1, 0x0216, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },  // SYNC: BCC
        { /*    8.5 */ 0x0216, 0x90,  R,    1, 0x0216, 0x80, 0x00, 0x00, 0xFD, 0b10101101 },  // $90 = BCC
        { /*    9.0 */ 0x0217, 0x90,  R,    0, 0x0217, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // dummy / operand
        { /*    9.5 */ 0x0217, 0xE8,  R,    0, 0x0217, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },

        // --- SBC at $0218 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   10.0 */ 0x0218, 0xE8,  R,    1, 0x0218, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // SYNC: SBC
        { /*   10.5 */ 0x0218, 0xE9,  R,    1, 0x0218, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // $E9 = SBC
        { /*   11.0 */ 0x0219, 0xE9,  R,    0, 0x0219, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // dummy / operand
        { /*   11.5 */ 0x0219, 0x66,  R,    0, 0x0219, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },

        // --- BCS at $021A ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x021A, 0x66,  R,    1, 0x021A, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // SYNC: BCS
        { /*   12.5 */ 0x021A, 0xB0,  R,    1, 0x021A, 0x55, 0x00, 0x00, 0xFD, 0b01101101 },  // $B0 = BCS
        { /*   13.0 */ 0x021B, 0xB0,  R,    0, 0x021B, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*   13.5 */ 0x021B, 0xE4,  R,    0, 0x021B, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- CMP at $021C ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   14.0 */ 0x021C, 0xE4,  R,    1, 0x021C, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: CMP
        { /*   14.5 */ 0x021C, 0xC9,  R,    1, 0x021C, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // $C9 = CMP
        { /*   15.0 */ 0x021D, 0xC9,  R,    0, 0x021D, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // dummy / operand
        { /*   15.5 */ 0x021D, 0x89,  R,    0, 0x021D, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },

        // --- BNE at $021E ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   16.0 */ 0x021E, 0x89,  R,    1, 0x021E, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // SYNC: BNE
        { /*   16.5 */ 0x021E, 0xD0,  R,    1, 0x021E, 0x89, 0x00, 0x00, 0xFD, 0b10101100 },  // $D0 = BNE
        { /*   17.0 */ 0x021F, 0xD0,  R,    0, 0x021F, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*   17.5 */ 0x021F, 0xE0,  R,    0, 0x021F, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },

        // --- PASS at $0220: NOP flushes pipelined registers, JMP-self at $0221 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   18.0 */ 0x0220, 0xE0,  R,    1, 0x0220, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: NOP
        { /*   18.5 */ 0x0220, 0xEA,  R,    1, 0x0220, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // $EA = NOP
        { /*   19.0 */ 0x0221, 0xEA,  R,    0, 0x0221, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // dummy / operand
        { /*   19.5 */ 0x0221, 0x4C,  R,    0, 0x0221, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },
        { /*   20.0 */ 0x0221, 0x4C,  R,    1, 0x0221, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // SYNC: JMP
        { /*   20.5 */ 0x0221, 0x4C,  R,    1, 0x0221, 0x89, 0x00, 0x00, 0xFD, 0b00101111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
