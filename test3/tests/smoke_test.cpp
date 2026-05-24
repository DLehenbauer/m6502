// test3/tests/smoke_test.cpp
//
// First end-to-end smoke test for the test3 framework. Assembles a
// tiny program at the default entry ($0200), runs it on the
// AholmeShim, and compares the captured bus-state delta trace against
// an embedded expected[] table.
//
// Authoring workflow:
//   1. Run with `expected[]` empty (or stale) -> test FAILS and prints
//      a paste-ready actual table to stderr.
//   2. Copy the actual block into this file, replacing `expected[]`.
//   3. Re-run -> test PASSES.
//
// Program:
//   $0200  A9 42       LDA #$42
//   $0202  85 80       STA $80      ; store $42 -> $80
//   $0204  00 42       BRK          ; vector $FFFE/$FFFF -> $0500
//                                   ; $42 at $0205 is the BRK signature (dummy-fetched)
//   $0500  4C 00 05    JMP $0500    ; pseudo-op: prog.pass(); run ends here
//
// Termination: prog.pass() at $0500 registers a run-terminator that
// fires on the first opcode fetch at $0500, so the trace ends
// deterministically two half-cycles into the JMP-self.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(smoke_lda_sta_brk) {
    // Build the program image.
    Program prog = Program::start()
        .lda(Imm{0x42})
        .sta(ZP {0x80})
        .brk();

    // BRK exit lands at a pass-terminator: reaching its address ends
    // the run cleanly with the recorded message in RunResult::message.
    prog.brk_handler(0x0500)
        .pass("BRK terminated cleanly at $0500");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- LDA at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDA
        { /*    0.5 */ 0x0200, 0xA9,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A9 = LDA
        { /*    1.0 */ 0x0201, 0xA9,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x42,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- STA at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x42,  R,    1, 0x0202, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: STA
        { /*    2.5 */ 0x0202, 0x85,  R,    1, 0x0202, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // $85 = STA
        { /*    3.0 */ 0x0203, 0x85,  R,    0, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x42,  W,    0, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080
        { /*    4.5 */ 0x0080, 0x42,  W,    0, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080

        // --- BRK at $0204 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0204, 0x80,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: BRK
        { /*    5.5 */ 0x0204, 0x00,  R,    1, 0x0204, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // $00 = BRK
        { /*    6.0 */ 0x0205, 0x00,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0205, 0x42,  R,    0, 0x0205, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x01FD, 0x02,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $02 -> stack
        { /*    7.5 */ 0x01FD, 0x02,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $02 -> stack
        { /*    8.0 */ 0x01FC, 0x06,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $06 -> stack
        { /*    8.5 */ 0x01FC, 0x06,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $06 -> stack
        { /*    9.0 */ 0x01FB, 0x34,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $34 -> stack
        { /*    9.5 */ 0x01FB, 0x34,  W,    0, 0x0206, 0x42, 0x00, 0x00, 0xFD, 0b00100100 },  // push $34 -> stack
        { /*   10.0 */ 0xFFFE, 0x42,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // vector fetch
        { /*   10.5 */ 0xFFFE, 0x00,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // vector fetch
        { /*   11.0 */ 0xFFFF, 0x00,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // vector fetch
        { /*   11.5 */ 0xFFFF, 0x05,  R,    0, 0x0206, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // vector fetch

        // --- PASS at $0500: NOP flushes pipelined registers, JMP-self at $0501 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   12.0 */ 0x0500, 0x05,  R,    1, 0x0500, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // SYNC: NOP
        { /*   12.5 */ 0x0500, 0xEA,  R,    1, 0x0500, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // $EA = NOP
        { /*   13.0 */ 0x0501, 0xEA,  R,    0, 0x0501, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // dummy / operand
        { /*   13.5 */ 0x0501, 0x4C,  R,    0, 0x0501, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },
        { /*   14.0 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // SYNC: JMP
        { /*   14.5 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x42, 0x00, 0x00, 0xFA, 0b00100100 },  // $4C = JMP

    };

    EXPECT_PASS(expected);
}
