// test3/tests/isa/transfers_test.cpp
//
// Register transfers: TAX, TAY, TXA, TYA, TSX, TXS.
// All are 1-byte / 2-cycle implied instructions.
//
// TAX/TAY/TXA/TYA/TSX update N and Z from the result.
// TXS is the odd one out: it copies X into S without touching any flag.
//
// Post-reset register state lets us exercise all 6 without first
// loading a source register:
//   A = $66   (canonical aholme post-reset A)
//   X = $00
//   Y = $00
//   S = $FD
//   P = 0b00100110 (Z = 1, I = 1, bit 5 = 1)
//
// So:
//   TAX  $66 -> X         N=0, Z=0
//   TAY  $66 -> Y         N=0, Z=0
//   TXA  $00 -> A         N=0, Z=1 (Z stays set, A goes 0)
//   TYA  $00 -> A         N=0, Z=1
//   TSX  $FD -> X         N=1 (bit 7 set), Z=0
//   TXS  $00 -> S         flags unchanged (Z stays 1)

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(tax_implied) {
    // TAX: A($66) -> X. Z cleared, N stays 0.
    Program prog = Program::start()
        .tax()
        .pass("TAX completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TAX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TAX
        { /*    0.5 */ 0x0200, 0xAA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $AA = TAX
        { /*    1.0 */ 0x0201, 0xAA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x66, 0x00, 0xFD, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(tay_implied) {
    // TAY: A($66) -> Y. Z cleared, N stays 0.
    Program prog = Program::start()
        .tay()
        .pass("TAY completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TAY at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TAY
        { /*    0.5 */ 0x0200, 0xA8,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A8 = TAY
        { /*    1.0 */ 0x0201, 0xA8,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x66, 0xFD, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(txa_implied) {
    // TXA: X($00) -> A. A goes $66 -> $00, Z stays set (now from A).
    Program prog = Program::start()
        .txa()
        .pass("TXA completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TXA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TXA
        { /*    0.5 */ 0x0200, 0x8A,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $8A = TXA
        { /*    1.0 */ 0x0201, 0x8A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(tya_implied) {
    // TYA: Y($00) -> A. A goes $66 -> $00, Z set.
    Program prog = Program::start()
        .tya()
        .pass("TYA completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TYA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TYA
        { /*    0.5 */ 0x0200, 0x98,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $98 = TYA
        { /*    1.0 */ 0x0201, 0x98,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x00, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(tsx_implied) {
    // TSX: S($FD) -> X. N set (bit 7 of $FD), Z cleared.
    Program prog = Program::start()
        .tsx()
        .pass("TSX completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TSX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TSX
        { /*    0.5 */ 0x0200, 0xBA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $BA = TSX
        { /*    1.0 */ 0x0201, 0xBA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0xFD, 0x00, 0xFD, 0b10100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(txs_implied) {
    // TXS: X($00) -> S. S goes $FD -> $00. Flags must NOT change
    // (TXS is the only transfer that doesn't touch N/Z) -- the trace
    // check confirms P stays at the post-reset value.
    Program prog = Program::start()
        .txs()
        .pass("TXS completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- TXS at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: TXS
        { /*    0.5 */ 0x0200, 0x9A,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $9A = TXS
        { /*    1.0 */ 0x0201, 0x9A,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0x00, 0b00100110 },  // SYNC: NOP
        { /*    2.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0x00, 0b00100110 },  // $EA = NOP
        { /*    3.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0x00, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0x00, 0b00100110 },
        { /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0x00, 0b00100110 },  // SYNC: JMP
        { /*    4.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0x00, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

