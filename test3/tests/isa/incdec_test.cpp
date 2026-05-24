// test3/tests/isa/incdec_test.cpp
//
// Increment/decrement: register (INX/INY/DEX/DEY) and memory (INC/DEC).
// Register variants are 1-byte / 2-cycle implied.
// Memory variants are 5-7 cycle read-modify-write instructions.
//
// Visible-register latency note: the aholme core's X/Y storage nodes
// update one cycle later than the status flags. After an INX/DEX, the
// architectural X result first appears in the trace on the dummy-fetch
// cycle of the NEXT instruction (not the next SYNC). pass() / fail()
// already emit a landing-pad NOP before the JMP-self for exactly this
// reason, so tests just chain `.inx().pass(...)` and the X result lands
// on the NOP's dummy fetch.
//
// Post-reset A=$66, X=Y=$00, S=$FD, P=%00100110. Register coverage:
//   INX  $00 -> $01      Z=0, N=0
//   INY  $00 -> $01      Z=0, N=0
//   DEX  $00 -> $FF      Z=0, N=1 (underflow)
//   DEY  $00 -> $FF      Z=0, N=1
//   DEX, INX  $00 -> $FF -> $00  Z=1, N=0 (wrap-around)

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(inx_implied) {
    // INX: X $00 -> $01. NOP after so X update lands in trace.
    Program prog = Program::start()
        .inx()
        .pass("INX completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- INX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INX
        { /*    0.5 */ 0x0200, 0xE8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E8 = INX
        { /*    1.0 */ 0x0201, 0xE8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x01, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x01, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x01, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x01, 0x00, 0xFD, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(iny_implied) {
    // INY: Y $00 -> $01.
    Program prog = Program::start()
        .iny()
        .pass("INY completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- INY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INY
        { /*    0.5 */ 0x0200, 0xC8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C8 = INY
        { /*    1.0 */ 0x0201, 0xC8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x01, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x01, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x01, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x01, 0xFD, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(dex_implied) {
    // DEX: X $00 -> $FF (underflow). N set.
    Program prog = Program::start()
        .dex()
        .pass("DEX completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- DEX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEX
        { /*    0.5 */ 0x0200, 0xCA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $CA = DEX
        { /*    1.0 */ 0x0201, 0xCA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(dey_implied) {
    // DEY: Y $00 -> $FF (underflow). N set.
    Program prog = Program::start()
        .dey()
        .pass("DEY completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- DEY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEY
        { /*    0.5 */ 0x0200, 0x88,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $88 = DEY
        { /*    1.0 */ 0x0201, 0x88,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0xFF, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0xFF, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0xFF, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0xFF, 0xFD, 0b10100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(inx_wraps_to_zero) {
    // DEX leaves X at $FF; INX wraps to $00. Z set, N cleared.
    Program prog = Program::start()
        .dex()
        .inx()
        .pass("INX wrapped $FF -> $00");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- DEX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEX
        { /*    0.5 */ 0x0200, 0xCA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $CA = DEX
        { /*    1.0 */ 0x0201, 0xCA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xE8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- INX at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xE8,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INX
        { /*    2.5 */ 0x0201, 0xE8,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E8 = INX
        { /*    3.0 */ 0x0202, 0xE8,  R,    0, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    4.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0xFF, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    5.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(inc_zp) {
    Program prog = Program::start()
        .inc(ZP{0x80})
        .pass("INC zero page completed");

    prog.org(0x0080).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- INC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INC
        { /*    0.5 */ 0x0200, 0xE6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E6 = INC
        { /*    1.0 */ 0x0201, 0xE6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x41,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x41,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $0080
        { /*    3.5 */ 0x0080, 0x41,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $0080
        { /*    4.0 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080
        { /*    4.5 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x41,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0x42,
           "INC zero page result must be $42 (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(inc_zp_to_zero) {
    Program prog = Program::start()
        .inc(ZP{0x80})
        .pass("INC zero page wrapped to zero");

    prog.org(0x0080).bytes({0xFF});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- INC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INC
        { /*    0.5 */ 0x0200, 0xE6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E6 = INC
        { /*    1.0 */ 0x0201, 0xE6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0xFF,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0xFF,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $FF -> $0080
        { /*    3.5 */ 0x0080, 0xFF,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $FF -> $0080
        { /*    4.0 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080
        { /*    4.5 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0xFF,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0x00,
           "INC zero page result must be $00 (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(inc_zp_to_neg) {
    Program prog = Program::start()
        .inc(ZP{0x80})
        .pass("INC zero page set negative");

    prog.org(0x0080).bytes({0x7F});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- INC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INC
        { /*    0.5 */ 0x0200, 0xE6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E6 = INC
        { /*    1.0 */ 0x0201, 0xE6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x7F,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x7F,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $7F -> $0080
        { /*    3.5 */ 0x0080, 0x7F,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $7F -> $0080
        { /*    4.0 */ 0x0080, 0x80,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $80 -> $0080
        { /*    4.5 */ 0x0080, 0x80,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $80 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x7F,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0x80,
           "INC zero page result must be $80 (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(inc_zpx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .inc(ZPX{0x80})
        .pass("INC zero page X completed");

    prog.org(0x0084).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- INC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: INC
        { /*    2.5 */ 0x0202, 0xF6,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $F6 = INC
        { /*    3.0 */ 0x0203, 0xF6,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0084, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0084, 0x41,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0084, 0x41,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $0084
        { /*    6.5 */ 0x0084, 0x41,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $0084
        { /*    7.0 */ 0x0084, 0x42,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0084
        { /*    7.5 */ 0x0084, 0x42,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0084

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x41,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0084) == 0x42,
           "INC zero page X result must be $42 (got $%02X)",
           cpu.mem_read(0x0084));
}

TEST(inc_abs) {
    Program prog = Program::start()
        .inc(Abs{0x1234})
        .pass("INC absolute completed");

    prog.org(0x1234).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- INC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INC
        { /*    0.5 */ 0x0200, 0xEE,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EE = INC
        { /*    1.0 */ 0x0201, 0xEE,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x34,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x34,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x12,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x1234, 0x12,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x1234, 0x41,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x1234, 0x41,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $1234
        { /*    4.5 */ 0x1234, 0x41,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $1234
        { /*    5.0 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234
        { /*    5.5 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0203, 0x41,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x1234) == 0x42,
           "INC absolute result must be $42 (got $%02X)",
           cpu.mem_read(0x1234));
}

TEST(inc_absx) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .inc(AbsX{0x1234})
        .pass("INC absolute X completed");

    prog.org(0x1238).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- INC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: INC
        { /*    2.5 */ 0x0202, 0xFE,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $FE = INC
        { /*    3.0 */ 0x0203, 0xFE,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x34,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x34,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x12,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x1238, 0x12,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x1238, 0x41,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $1238
        { /*    7.5 */ 0x1238, 0x41,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $1238
        { /*    8.0 */ 0x1238, 0x42,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1238
        { /*    8.5 */ 0x1238, 0x42,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1238

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0205, 0x41,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    9.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*   10.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*   10.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*   11.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   11.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x1238) == 0x42,
           "INC absolute X result must be $42 (got $%02X)",
           cpu.mem_read(0x1238));
}

TEST(dec_zp) {
    Program prog = Program::start()
        .dec(ZP{0x80})
        .pass("DEC zero page completed");

    prog.org(0x0080).bytes({0x43});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- DEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEC
        { /*    0.5 */ 0x0200, 0xC6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C6 = DEC
        { /*    1.0 */ 0x0201, 0xC6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x43,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x43,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $0080
        { /*    3.5 */ 0x0080, 0x43,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $0080
        { /*    4.0 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080
        { /*    4.5 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x43,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0x42,
           "DEC zero page result must be $42 (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(dec_zp_to_zero) {
    Program prog = Program::start()
        .dec(ZP{0x80})
        .pass("DEC zero page reached zero");

    prog.org(0x0080).bytes({0x01});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- DEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEC
        { /*    0.5 */ 0x0200, 0xC6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C6 = DEC
        { /*    1.0 */ 0x0201, 0xC6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x01,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x01,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $01 -> $0080
        { /*    3.5 */ 0x0080, 0x01,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $01 -> $0080
        { /*    4.0 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080
        { /*    4.5 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x01,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0x00,
           "DEC zero page result must be $00 (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(dec_zp_to_neg) {
    Program prog = Program::start()
        .dec(ZP{0x80})
        .pass("DEC zero page set negative");

    prog.org(0x0080).bytes({0x00});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- DEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEC
        { /*    0.5 */ 0x0200, 0xC6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $C6 = DEC
        { /*    1.0 */ 0x0201, 0xC6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080
        { /*    3.5 */ 0x0080, 0x00,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $00 -> $0080
        { /*    4.0 */ 0x0080, 0xFF,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $FF -> $0080
        { /*    4.5 */ 0x0080, 0xFF,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $FF -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x00,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x0080) == 0xFF,
           "DEC zero page result must be $FF (got $%02X)",
           cpu.mem_read(0x0080));
}

TEST(dec_abs) {
    Program prog = Program::start()
        .dec(Abs{0x1234})
        .pass("DEC absolute completed");

    prog.org(0x1234).bytes({0x43});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- DEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEC
        { /*    0.5 */ 0x0200, 0xCE,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $CE = DEC
        { /*    1.0 */ 0x0201, 0xCE,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x34,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x34,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x12,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x1234, 0x12,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x1234, 0x43,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x1234, 0x43,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $1234
        { /*    4.5 */ 0x1234, 0x43,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $1234
        { /*    5.0 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234
        { /*    5.5 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0203, 0x43,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    if (!EXPECT_PASS(expected)) return;

    EXPECT(cpu.mem_read(0x1234) == 0x42,
           "DEC absolute result must be $42 (got $%02X)",
           cpu.mem_read(0x1234));
}

