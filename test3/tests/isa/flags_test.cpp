// test3/tests/isa/flags_test.cpp
//
// Single-flag setters/clearers: CLC, SEC, CLD, SED, CLI, SEI, CLV.
// All are 1-byte / 2-cycle implied instructions that toggle exactly
// one bit in P. (No SEV instruction exists on the 6502; V is set
// externally via the SO pin or as a side-effect of BIT/ADC/SBC.)
//
// Post-reset P = 0b00100110 (--1B-IZ-): bit 5 = 1, I = 1, Z = 1,
// everything else clear. The "set" tests assert each flag goes 0 -> 1;
// the "clear" tests need the flag set first (we set it with the
// matching set instruction).

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(sec_implied) {
    // SEC: set carry. C starts clear post-reset; SEC sets it.
    Program prog = Program::start()
        .sec()
        .pass("SEC completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(clc_implied) {
    // CLC after SEC: verify C goes 1 -> 0.
    Program prog = Program::start()
        .sec()
        .clc()
        .pass("CLC completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- SEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEC
        { /*    0.5 */ 0x0200, 0x38,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $38 = SEC
        { /*    1.0 */ 0x0201, 0x38,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x18,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLC at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // SYNC: CLC
        { /*    2.5 */ 0x0201, 0x18,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // $18 = CLC
        { /*    3.0 */ 0x0202, 0x18,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100111 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(sei_implied) {
    // SEI: set interrupt-disable. I is already set post-reset; SEI is
    // still observable on the bus (the opcode fetch + 1-cycle delay)
    // and leaves I = 1.
    Program prog = Program::start()
        .sei()
        .pass("SEI completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- SEI at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SEI
        { /*    0.5 */ 0x0200, 0x78,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $78 = SEI
        { /*    1.0 */ 0x0201, 0x78,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(cli_implied) {
    // CLI: clear interrupt-disable. I = 1 post-reset; CLI clears it.
    Program prog = Program::start()
        .cli()
        .pass("CLI completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- CLI at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLI
        { /*    0.5 */ 0x0200, 0x58,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $58 = CLI
        { /*    1.0 */ 0x0201, 0x58,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(sed_implied) {
    // SED: set decimal. D = 0 post-reset.
    Program prog = Program::start()
        .sed()
        .pass("SED completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(cld_implied) {
    // CLD after SED: verify D goes 1 -> 0.
    Program prog = Program::start()
        .sed()
        .cld()
        .pass("CLD completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- SED at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: SED
        { /*    0.5 */ 0x0200, 0xF8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $F8 = SED
        { /*    1.0 */ 0x0201, 0xF8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xD8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- CLD at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xD8,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // SYNC: CLD
        { /*    2.5 */ 0x0201, 0xD8,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // $D8 = CLD
        { /*    3.0 */ 0x0202, 0xD8,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00101110 },

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    4.0 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    4.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    5.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    5.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    6.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    6.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(clv_implied) {
    // CLV: clear overflow. V starts clear post-reset, so this test
    // verifies CLV is observable on the bus and leaves V = 0 without
    // disturbing other flags. (A V-was-set CLV test belongs alongside
    // BIT or ADC, which can set V; see corners/ when those land.)
    Program prog = Program::start()
        .clv()
        .pass("CLV completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- CLV at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: CLV
        { /*    0.5 */ 0x0200, 0xB8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $B8 = CLV
        { /*    1.0 */ 0x0201, 0xB8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

