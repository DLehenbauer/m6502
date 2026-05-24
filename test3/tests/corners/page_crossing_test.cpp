// test3/tests/corners/page_crossing_test.cpp

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(adc_absx_page_cross) {
    Program prog = Program::start()
        .lda(Imm{0x10})
        .ldx(Imm{0x10})
        .adc(AbsX{0x04F0})
        .pass("ADC absolute X page cross completed");

    prog.org(0x0500).bytes({0x32});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xA2,  R,    1, 0x0202, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // $A2 = LDX
        { /*    3.0 */ 0x0203, 0xA2,  R,    0, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x10,  R,    0, 0x0203, 0x10, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- ADC at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x10,  R,    1, 0x0204, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: ADC
        { /*    4.5 */ 0x0204, 0x7D,  R,    1, 0x0204, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // $7D = ADC
        { /*    5.0 */ 0x0205, 0x7D,  R,    0, 0x0205, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0xF0,  R,    0, 0x0205, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0206, 0xF0,  R,    0, 0x0206, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0206, 0x04,  R,    0, 0x0206, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0400, 0x04,  R,    0, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0400, 0x00,  R,    0, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0500, 0x00,  R,    0, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0500, 0x32,  R,    0, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0207: NOP flushes pipelined registers, JMP-self at $0208 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0207, 0x32,  R,    1, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    9.5 */ 0x0207, 0xEA,  R,    1, 0x0207, 0x10, 0x10, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*   10.0 */ 0x0208, 0xEA,  R,    0, 0x0208, 0x42, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*   10.5 */ 0x0208, 0x4C,  R,    0, 0x0208, 0x42, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*   11.0 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x42, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   11.5 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x42, 0x10, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(cmp_absx_page_cross) {
    Program prog = Program::start()
        .lda(Imm{0x32})
        .ldx(Imm{0x10})
        .cmp(AbsX{0x04F0})
        .pass("CMP absolute X page cross completed");

    prog.org(0x0500).bytes({0x32});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x32,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x32,  R,    1, 0x0202, 0x32, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xA2,  R,    1, 0x0202, 0x32, 0x00, 0x00, 0xFD, 0b00100100 },  // $A2 = LDX
        { /*    3.0 */ 0x0203, 0xA2,  R,    0, 0x0203, 0x32, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x10,  R,    0, 0x0203, 0x32, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- CMP at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x10,  R,    1, 0x0204, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: CMP
        { /*    4.5 */ 0x0204, 0xDD,  R,    1, 0x0204, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // $DD = CMP
        { /*    5.0 */ 0x0205, 0xDD,  R,    0, 0x0205, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0xF0,  R,    0, 0x0205, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0206, 0xF0,  R,    0, 0x0206, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0206, 0x04,  R,    0, 0x0206, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0400, 0x04,  R,    0, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0400, 0x00,  R,    0, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0500, 0x00,  R,    0, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0500, 0x32,  R,    0, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0207: NOP flushes pipelined registers, JMP-self at $0208 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0207, 0x32,  R,    1, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    9.5 */ 0x0207, 0xEA,  R,    1, 0x0207, 0x32, 0x10, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*   10.0 */ 0x0208, 0xEA,  R,    0, 0x0208, 0x32, 0x10, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*   10.5 */ 0x0208, 0x4C,  R,    0, 0x0208, 0x32, 0x10, 0x00, 0xFD, 0b00100111 },
        { /*   11.0 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x32, 0x10, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*   11.5 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x32, 0x10, 0x00, 0xFD, 0b00100111 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(and_absx_page_cross) {
    Program prog = Program::start()
        .lda(Imm{0xFF})
        .ldx(Imm{0x01})
        .and_(AbsX{0x04FF})
        .pass("AND absolute X page cross completed");

    prog.org(0x0500).bytes({0x0F});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xFF,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0xFF,  R,    1, 0x0202, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xA2,  R,    1, 0x0202, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // $A2 = LDX
        { /*    3.0 */ 0x0203, 0xA2,  R,    0, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x01,  R,    0, 0x0203, 0xFF, 0x00, 0x00, 0xFD, 0b10100100 },

        // --- AND at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x01,  R,    1, 0x0204, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // SYNC: AND
        { /*    4.5 */ 0x0204, 0x3D,  R,    1, 0x0204, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // $3D = AND
        { /*    5.0 */ 0x0205, 0x3D,  R,    0, 0x0205, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0xFF,  R,    0, 0x0205, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0206, 0xFF,  R,    0, 0x0206, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0206, 0x04,  R,    0, 0x0206, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0400, 0x04,  R,    0, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0400, 0x00,  R,    0, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0500, 0x00,  R,    0, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0500, 0x0F,  R,    0, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0207: NOP flushes pipelined registers, JMP-self at $0208 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0207, 0x0F,  R,    1, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    9.5 */ 0x0207, 0xEA,  R,    1, 0x0207, 0xFF, 0x01, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*   10.0 */ 0x0208, 0xEA,  R,    0, 0x0208, 0x0F, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*   10.5 */ 0x0208, 0x4C,  R,    0, 0x0208, 0x0F, 0x01, 0x00, 0xFD, 0b00100100 },
        { /*   11.0 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x0F, 0x01, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   11.5 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x0F, 0x01, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(sta_absx_always_pays) {
    Program prog = Program::start()
        .lda(Imm{0x42})
        .ldx(Imm{0x00})
        .sta(AbsX{0x0500})
        .pass("STA absolute X paid indexed write cycle");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x42,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x42,  R,    1, 0x0202, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xA2,  R,    1, 0x0202, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // $A2 = LDX
        { /*    3.0 */ 0x0203, 0xA2,  R,    0, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- STA at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0204, 0x00,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: STA
        { /*    4.5 */ 0x0204, 0x9D,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // $9D = STA
        { /*    5.0 */ 0x0205, 0x9D,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0205, 0x00,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0206, 0x00,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0206, 0x05,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0500, 0x05,  R,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    7.5 */ 0x0500, 0x00,  R,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    8.0 */ 0x0500, 0x42,  W,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0500
        { /*    8.5 */ 0x0500, 0x42,  W,    0, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // write $42 -> $0500

        // --- PASS at $0207: NOP flushes pipelined registers, JMP-self at $0208 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0207, 0x00,  R,    1, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    9.5 */ 0x0207, 0xEA,  R,    1, 0x0207, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*   10.0 */ 0x0208, 0xEA,  R,    0, 0x0208, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*   10.5 */ 0x0208, 0x4C,  R,    0, 0x0208, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*   11.0 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*   11.5 */ 0x0208, 0x4C,  R,    1, 0x0208, 0x42, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(branch_no_page_cross) {
    Program prog = Program::start(0x0200);
    Label taken = prog.org(0x0210).pass("BNE taken within page");
    prog.org(0x0200)
        .lda(Imm{0x01})
        .bne(taken)
        .fail("BNE must take branch when Z=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x01,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BNE at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x01,  R,    1, 0x0202, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: BNE
        { /*    2.5 */ 0x0202, 0xD0,  R,    1, 0x0202, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $D0 = BNE
        { /*    3.0 */ 0x0203, 0xD0,  R,    0, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x0C,  R,    0, 0x0203, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x0C,  R,    0, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0210: NOP flushes pipelined registers, JMP-self at $0211 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0210, 0xEA,  R,    1, 0x0210, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0210, 0xEA,  R,    1, 0x0210, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0211, 0xEA,  R,    0, 0x0211, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0211, 0x4C,  R,    0, 0x0211, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0211, 0x4C,  R,    1, 0x0211, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0211, 0x4C,  R,    1, 0x0211, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(branch_page_cross_taken) {
    Program prog = Program::start(0x02EE);
    Label taken = prog.org(0x0310).pass("BNE taken across page");
    prog.org(0x02EE)
        .lda(Imm{0x01})
        .bne(taken)
        .fail("BNE must take branch when Z=0");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $02EE ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x02EE, 0x02,  R,    1, 0x02EE, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x02EE, 0xA9,  R,    1, 0x02EE, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x02EF, 0xA9,  R,    0, 0x02EF, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x02EF, 0x01,  R,    0, 0x02EF, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- BNE at $02F0 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x02F0, 0x01,  R,    1, 0x02F0, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: BNE
        { /*    2.5 */ 0x02F0, 0xD0,  R,    1, 0x02F0, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $D0 = BNE
        { /*    3.0 */ 0x02F1, 0xD0,  R,    0, 0x02F1, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x02F1, 0x1E,  R,    0, 0x02F1, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x02F2, 0x1E,  R,    0, 0x02F2, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x02F2, 0xEA,  R,    0, 0x02F2, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0210, 0xEA,  R,    0, 0x0210, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0210, 0x00,  R,    0, 0x0210, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0310: NOP flushes pipelined registers, JMP-self at $0311 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0310, 0x00,  R,    1, 0x0310, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0310, 0xEA,  R,    1, 0x0310, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0311, 0xEA,  R,    0, 0x0311, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0311, 0x4C,  R,    0, 0x0311, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0311, 0x4C,  R,    1, 0x0311, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0311, 0x4C,  R,    1, 0x0311, 0x01, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
