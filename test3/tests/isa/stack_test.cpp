// test3/tests/isa/stack_test.cpp
//
// Stack push/pop: PHA, PLA, PHP, PLP.
// All are 1-byte instructions but take 3 cycles (push) or 4 cycles
// (pop) thanks to the extra stack-pointer dummy-read.
//
// Post-reset A = $66, X = Y = $00, S = $FD, P = 0b00100110.
//
// Coverage:
//   pha_implied                 PHA writes A to $01FD, S goes $FD -> $FC.
//   pha_pla_roundtrip           PHA + PLA returns A to its original value;
//                               Z cleared, S restored.
//   php_implied                 PHP writes P|B|bit5 ($26 | $30 = $36) to
//                               stack. B is set in pushed value even though
//                               B is not a real flag.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(pha_implied) {
    // PHA: push A ($66) to $01FD, S goes $FD -> $FC.
    Program prog = Program::start()
        .pha()
        .pass("PHA completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- PHA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: PHA
        { /*    0.5 */ 0x0200, 0x48,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $48 = PHA
        { /*    1.0 */ 0x0201, 0x48,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x01FD, 0x66,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $66 -> stack
        { /*    2.5 */ 0x01FD, 0x66,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $66 -> stack

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },
        { /*    5.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(pha_pla_roundtrip) {
    // PHA pushes A; PLA pops it back. After both, A is unchanged but
    // S has cycled $FD -> $FC -> $FD.
    Program prog = Program::start()
        .pha()
        .pla()
        .pass("PHA/PLA roundtrip completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- PHA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: PHA
        { /*    0.5 */ 0x0200, 0x48,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $48 = PHA
        { /*    1.0 */ 0x0201, 0x48,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x68,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x01FD, 0x66,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $66 -> stack
        { /*    2.5 */ 0x01FD, 0x66,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $66 -> stack

        // --- PLA at $0201 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0201, 0x68,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // SYNC: PLA
        { /*    3.5 */ 0x0201, 0x68,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // $68 = PLA
        { /*    4.0 */ 0x0202, 0x68,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },
        { /*    5.0 */ 0x01FC, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // stack read
        { /*    5.5 */ 0x01FC, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // stack read
        { /*    6.0 */ 0x01FD, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // stack read
        { /*    6.5 */ 0x01FD, 0x66,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // stack read

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0202, 0x66,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    7.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    8.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    8.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    9.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    9.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

TEST(php_implied) {
    // PHP: push P with B and bit-5 set. Post-reset P = $26, so the
    // pushed byte is $26 | $30 = $36.
    Program prog = Program::start()
        .php()
        .pass("PHP completed");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- PHP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: PHP
        { /*    0.5 */ 0x0200, 0x08,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $08 = PHP
        { /*    1.0 */ 0x0201, 0x08,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x01FD, 0x36,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack
        { /*    2.5 */ 0x01FD, 0x36,  W,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack

        // --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    3.0 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // SYNC: NOP
        { /*    3.5 */ 0x0201, 0xEA,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // $EA = NOP
        { /*    4.0 */ 0x0202, 0xEA,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0202, 0x4C,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },
        { /*    5.0 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // SYNC: JMP
        { /*    5.5 */ 0x0202, 0x4C,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFC, 0b00100110 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}

