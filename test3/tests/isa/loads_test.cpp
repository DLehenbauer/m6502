// test3/tests/isa/loads_test.cpp
//
// Load instructions: LDA, LDX, LDY across all documented addressing modes.
// Each test ends with pass() so the landing-pad NOP exposes register latency.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(lda_imm) {
    /*
     * LDA #$80 loads a negative value into A.
     * N sets, and Z clears from the loaded operand.
     */
    Program prog = Program::start()
        .lda(Imm{0x80})
        .pass("LDA immediate completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x80, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_zp) {
    /*
     * LDA $80 reads zero-page data from $0080.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .lda(ZP {0x80})
        .pass("LDA zero page completed");

    prog.org(0x0080).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA5,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A5 = LDA
        { /*    1.0 */ 0x0201, 0xA5,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_zpx) {
    /*
     * LDA $80,X uses X=$05 to address $0085.
     * The operand is $80, so N sets and Z clears.
     */
    Program prog = Program::start()
        .ldx(Imm{0x05})
        .lda(ZPX{0x80})
        .pass("LDA zero page X completed");

    prog.org(0x0085).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xB5,  R,    1, 0x0202, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // $B5 = LDA
        { /*    3.0 */ 0x0203, 0xB5,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0085, 0x00,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0085, 0x80,  R,    0, 0x0204, 0x66, 0x05, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x05, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_abs) {
    /*
     * LDA $0400 reads absolute data from $0400.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .lda(Abs{0x0400})
        .pass("LDA absolute completed");

    prog.org(0x0400).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xAD,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $AD = LDA
        { /*    1.0 */ 0x0201, 0xAD,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x04,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0400, 0x04,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0400, 0x42,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x42,  R,    1, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_absx) {
    /*
     * LDA $0400,X uses X=$03 to address $0403 without a page cross.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .ldx(Imm{0x03})
        .lda(AbsX{0x0400})
        .pass("LDA absolute X completed");

    prog.org(0x0403).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x03,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x03,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xBD,  R,    1, 0x0202, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // $BD = LDA
        { /*    3.0 */ 0x0203, 0xBD,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0403, 0x04,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0403, 0x00,  R,    0, 0x0205, 0x66, 0x03, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x00,  R,    1, 0x0205, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x00, 0x03, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_absx_page_cross) {
    /*
     * LDA $04F0,X uses X=$20 to address $0510 with a page cross.
     * The operand is $80, so N sets and Z clears.
     */
    Program prog = Program::start()
        .ldx(Imm{0x20})
        .lda(AbsX{0x04F0})
        .pass("LDA absolute X page cross completed");

    prog.org(0x0510).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xBD,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // $BD = LDA
        { /*    3.0 */ 0x0203, 0xBD,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xF0,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xF0,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0410, 0x04,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0410, 0x00,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0510, 0x00,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0510, 0x80,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x80,  R,    1, 0x0205, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x80, 0x20, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_absy) {
    /*
     * LDA $0400,Y uses Y=$04 to address $0404 without a page cross.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldy(Imm{0x04})
        .lda(AbsY{0x0400})
        .pass("LDA absolute Y completed");

    prog.org(0x0404).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xB9,  R,    1, 0x0202, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // $B9 = LDA
        { /*    3.0 */ 0x0203, 0xB9,  R,    0, 0x0203, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0404, 0x04,  R,    0, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0404, 0x42,  R,    0, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x42,  R,    1, 0x0205, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x42, 0x00, 0x04, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_absy_page_cross) {
    /*
     * LDA $04F8,Y uses Y=$10 to address $0508 with a page cross.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .ldy(Imm{0x10})
        .lda(AbsY{0x04F8})
        .pass("LDA absolute Y page cross completed");

    prog.org(0x0508).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x10,  R,    1, 0x0202, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xB9,  R,    1, 0x0202, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // $B9 = LDA
        { /*    3.0 */ 0x0203, 0xB9,  R,    0, 0x0203, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xF8,  R,    0, 0x0203, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xF8,  R,    0, 0x0204, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0408, 0x04,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0408, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0508, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0508, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x10, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x00,  R,    1, 0x0205, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x00, 0x00, 0x10, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_indx) {
    /*
     * LDA ($20,X) uses X=$04 to select the pointer at $24/$25.
     * The pointer addresses $0408. The operand is $80, so N sets.
     */
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .lda(IndX{0x20})
        .pass("LDA indexed indirect completed");

    prog.org(0x0024).bytes({0x08, 0x04});
    prog.org(0x0408).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xA1,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $A1 = LDA
        { /*    3.0 */ 0x0203, 0xA1,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0020, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0020, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0024, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0024, 0x08,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0025, 0x08,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0025, 0x04,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0408, 0x04,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0408, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x80, 0x04, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_indy) {
    /*
     * LDA ($30),Y reads pointer $30/$31 and adds Y=$03.
     * The target is $0413. The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldy(Imm{0x03})
        .lda(IndY{0x30})
        .pass("LDA indirect indexed completed");

    prog.org(0x0030).bytes({0x10, 0x04});
    prog.org(0x0413).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x03,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x03,  R,    1, 0x0202, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xB1,  R,    1, 0x0202, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // $B1 = LDA
        { /*    3.0 */ 0x0203, 0xB1,  R,    0, 0x0203, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x30,  R,    0, 0x0203, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0030, 0x30,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0030, 0x10,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0031, 0x10,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0031, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0413, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0413, 0x42,  R,    0, 0x0204, 0x66, 0x00, 0x03, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0204, 0x42,  R,    1, 0x0204, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x42, 0x00, 0x03, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(lda_indy_page_cross) {
    /*
     * LDA ($40),Y reads pointer $40/$41 and adds Y=$20.
     * The target is $0510 after a page cross. The operand is $00, so Z sets.
     */
    Program prog = Program::start()
        .ldy(Imm{0x20})
        .lda(IndY{0x40})
        .pass("LDA indirect indexed page cross completed");

    prog.org(0x0040).bytes({0xF0, 0x04});
    prog.org(0x0510).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // SYNC: LDA
        { /*    2.5 */ 0x0202, 0xB1,  R,    1, 0x0202, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // $B1 = LDA
        { /*    3.0 */ 0x0203, 0xB1,  R,    0, 0x0203, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x40,  R,    0, 0x0203, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0040, 0x40,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0040, 0xF0,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0041, 0xF0,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0041, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0410, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0410, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0510, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0510, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x00,  R,    1, 0x0204, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x00, 0x00, 0x20, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_imm) {
    /*
     * LDX #$80 loads a negative value into X.
     * N sets, and Z clears from the loaded operand.
     */
    Program prog = Program::start()
        .ldx(Imm{0x80})
        .pass("LDX immediate completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x80,  R,    1, 0x0202, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_zp) {
    /*
     * LDX $81 reads zero-page data from $0081.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .ldx(ZP {0x81})
        .pass("LDX zero page completed");

    prog.org(0x0081).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A6 = LDX
        { /*    1.0 */ 0x0201, 0xA6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x81,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0081, 0x81,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0081, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_zpy) {
    /*
     * LDX $81,Y uses Y=$05 to address $0086.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldy(Imm{0x05})
        .ldx(ZPY{0x81})
        .pass("LDX zero page Y completed");

    prog.org(0x0086).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x05,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x05,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xB6,  R,    1, 0x0202, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // $B6 = LDX
        { /*    3.0 */ 0x0203, 0xB6,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x81,  R,    0, 0x0203, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0081, 0x81,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0081, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0086, 0x00,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0086, 0x42,  R,    0, 0x0204, 0x66, 0x00, 0x05, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x42,  R,    1, 0x0204, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x42, 0x05, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_abs) {
    /*
     * LDX $0418 reads absolute data from $0418.
     * The operand is $80, so N sets and Z clears.
     */
    Program prog = Program::start()
        .ldx(Abs{0x0418})
        .pass("LDX absolute completed");

    prog.org(0x0418).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xAE,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $AE = LDX
        { /*    1.0 */ 0x0201, 0xAE,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x04,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0418, 0x04,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0418, 0x80,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x80,  R,    1, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x80, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_absy) {
    /*
     * LDX $0418,Y uses Y=$04 to address $041C without a page cross.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .ldy(Imm{0x04})
        .ldx(AbsY{0x0418})
        .pass("LDX absolute Y completed");

    prog.org(0x041C).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xBE,  R,    1, 0x0202, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // $BE = LDX
        { /*    3.0 */ 0x0203, 0xBE,  R,    0, 0x0203, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x18,  R,    0, 0x0203, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x18,  R,    0, 0x0204, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x041C, 0x04,  R,    0, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x041C, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x00,  R,    1, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x00, 0x04, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldx_absy_page_cross) {
    /*
     * LDX $04F0,Y uses Y=$20 to address $0510 with a page cross.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldy(Imm{0x20})
        .ldx(AbsY{0x04F0})
        .pass("LDX absolute Y page cross completed");

    prog.org(0x0510).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDX at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // SYNC: LDX
        { /*    2.5 */ 0x0202, 0xBE,  R,    1, 0x0202, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // $BE = LDX
        { /*    3.0 */ 0x0203, 0xBE,  R,    0, 0x0203, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xF0,  R,    0, 0x0203, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xF0,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0410, 0x04,  R,    0, 0x0205, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0410, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0510, 0x00,  R,    0, 0x0205, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0510, 0x42,  R,    0, 0x0205, 0x66, 0x00, 0x20, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x42,  R,    1, 0x0205, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x42, 0x20, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_imm) {
    /*
     * LDY #$00 loads zero into Y.
     * Z sets, and N clears from the loaded operand.
     */
    Program prog = Program::start()
        .ldy(Imm{0x00})
        .pass("LDY immediate completed");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA0,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A0 = LDY
        { /*    1.0 */ 0x0201, 0xA0,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_zp) {
    /*
     * LDY $82 reads zero-page data from $0082.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldy(ZP {0x82})
        .pass("LDY zero page completed");

    prog.org(0x0082).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xA4,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A4 = LDY
        { /*    1.0 */ 0x0201, 0xA4,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x82,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0082, 0x82,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0082, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0202, 0x42,  R,    1, 0x0202, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    3.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    4.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    5.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x42, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_zpx) {
    /*
     * LDY $82,X uses X=$06 to address $0088.
     * The operand is $80, so N sets and Z clears.
     */
    Program prog = Program::start()
        .ldx(Imm{0x06})
        .ldy(ZPX{0x82})
        .pass("LDY zero page X completed");

    prog.org(0x0088).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x06,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x06,  R,    1, 0x0202, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },  // SYNC: LDY
        { /*    2.5 */ 0x0202, 0xB4,  R,    1, 0x0202, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },  // $B4 = LDY
        { /*    3.0 */ 0x0203, 0xB4,  R,    0, 0x0203, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x82,  R,    0, 0x0203, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0082, 0x82,  R,    0, 0x0204, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0082, 0x00,  R,    0, 0x0204, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0088, 0x00,  R,    0, 0x0204, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0088, 0x80,  R,    0, 0x0204, 0x66, 0x06, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    7.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },  // dummy / operand
        { /*    7.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },
        { /*    8.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x06, 0x80, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_abs) {
    /*
     * LDY $0420 reads absolute data from $0420.
     * The operand is $00, so Z sets and N clears.
     */
    Program prog = Program::start()
        .ldy(Abs{0x0420})
        .pass("LDY absolute completed");

    prog.org(0x0420).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDY
        { /*    0.5 */ 0x0200, 0xAC,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $AC = LDY
        { /*    1.0 */ 0x0201, 0xAC,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x20,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x04,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0420, 0x04,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0420, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0203, 0x00,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_absx) {
    /*
     * LDY $0420,X uses X=$04 to address $0424 without a page cross.
     * The operand is $42, so N and Z both clear.
     */
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .ldy(AbsX{0x0420})
        .pass("LDY absolute X completed");

    prog.org(0x0424).bytes({0x42});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: LDY
        { /*    2.5 */ 0x0202, 0xBC,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $BC = LDY
        { /*    3.0 */ 0x0203, 0xBC,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x20,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x20,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0424, 0x04,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0424, 0x42,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0205, 0x42,  R,    1, 0x0205, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x42, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(ldy_absx_page_cross) {
    /*
     * LDY $04F0,X uses X=$20 to address $0510 with a page cross.
     * The operand is $80, so N sets and Z clears.
     */
    Program prog = Program::start()
        .ldx(Imm{0x20})
        .ldy(AbsX{0x04F0})
        .pass("LDY absolute X page cross completed");

    prog.org(0x0510).bytes({0x80});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x20,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- LDY at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x20,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // SYNC: LDY
        { /*    2.5 */ 0x0202, 0xBC,  R,    1, 0x0202, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // $BC = LDY
        { /*    3.0 */ 0x0203, 0xBC,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0xF0,  R,    0, 0x0203, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0xF0,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x04,  R,    0, 0x0204, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0410, 0x04,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0410, 0x00,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0510, 0x00,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0510, 0x80,  R,    0, 0x0205, 0x66, 0x20, 0x00, 0xFD, 0b00100100 },

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0205, 0x80,  R,    1, 0x0205, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    8.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },  // dummy / operand
        { /*    8.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },
        { /*    9.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x20, 0x80, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
