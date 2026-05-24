// test3/tests/isa/nop_test.cpp
//
// Documented NOP. (Undocumented NMOS NOPs of varying lengths live in nmos_only/.)
//
// Coverage:
//   nop_implied   NOP is 1 byte, 2 cycles, no observable register effect.
//                 The 6502 spends cycle 2 reading PC+1 (a "wasted" fetch)
//                 and discards the byte.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(nop_implied) {
    // NOP at $0200, then pass() at $0201 to assert control reached
    // exactly PC+1 (not PC+2 from a regression that treats NOP as
    // 2-byte, and not PC+0 from a stuck-PC regression).
    Program prog = Program::start()
        .nop()
        .pass("NOP advanced PC by 1");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- NOP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
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
