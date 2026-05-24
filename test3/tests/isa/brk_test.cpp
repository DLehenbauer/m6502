// test3/tests/isa/brk_test.cpp
//
// Software interrupt and return: BRK, RTI.
// (Hardware-IRQ-induced BRK behaviors live in corners/interrupts/.)
//
// Coverage:
//   brk_stacks_pc_plus_two   BRK pushes PC+2 (skipping the padding byte),
//                            stacks P with B=1 and bit5=1, fetches the
//                            vector from $FFFE/$FFFF, and sets the I flag
//                            in the handler.
//   brk_rti_roundtrip        BRK enters a handler that executes RTI;
//                            control returns to BRK_pc+2 with the
//                            pre-BRK flags restored (B/bit5 discarded).

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(brk_stacks_pc_plus_two) {
    // BRK at $0200 emits a 2-byte instruction: opcode $00 at $0200 +
    // signature byte $42 at $0201 (the latter is dummy-fetched but
    // never executes, since BRK pushes PC+2). If BRK regresses to
    // only pushing PC+1, control would return to $0202 after the
    // handler; we park a fail() trap there so that regression is
    // caught with a clear message instead of just a trace mismatch.
    // The BRK vector ($FFFE/$FFFF) lands at a pass() trap so the run
    // terminates cleanly inside the handler.
    Program prog = Program::start()
        .brk()
        .fail("BRK pushed PC+1 instead of PC+2");

    prog.brk_handler()
        .pass("BRK reached handler via $FFFE/$FFFF");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- BRK at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BRK
        { /*    0.5 */ 0x0200, 0x00,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $00 = BRK
        { /*    1.0 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x42,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x01FD, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    2.5 */ 0x01FD, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    3.0 */ 0x01FC, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    3.5 */ 0x01FC, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    4.0 */ 0x01FB, 0x36,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack
        { /*    4.5 */ 0x01FB, 0x36,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack
        { /*    5.0 */ 0xFFFE, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    5.5 */ 0xFFFE, 0x06,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    6.0 */ 0xFFFF, 0x06,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    6.5 */ 0xFFFF, 0x02,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch

        // --- PASS at $0206: NOP flushes pipelined registers, JMP-self at $0207 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0206, 0x02,  R,    1, 0x0206, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // SYNC: NOP
        { /*    7.5 */ 0x0206, 0xEA,  R,    1, 0x0206, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // $EA = NOP
        { /*    8.0 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // dummy / operand
        { /*    8.5 */ 0x0207, 0x4C,  R,    0, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },
        { /*    9.0 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // SYNC: JMP
        { /*    9.5 */ 0x0207, 0x4C,  R,    1, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(brk_rti_roundtrip) {
    // BRK at $0200 vectors to a handler at $0500 that immediately
    // executes RTI. RTI pops P and PC; PC = BRK_pc + 2 = $0202, where
    // pass() is parked. Reaching pass() proves the round-trip closed
    // and that B/bit5 were discarded from the popped P (P returns to
    // its pre-BRK value $26, not the stacked $36).
    Program prog = Program::start()
        .brk()
        .pass("returned from BRK via RTI");

    prog.brk_handler()
        .rti()
        .fail("RTI did not return to BRK_pc+2");

    cpu.load(prog);

    static const TraceRow expected[] = {
        // --- BRK at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BRK
        { /*    0.5 */ 0x0200, 0x00,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $00 = BRK
        { /*    1.0 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x42,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x01FD, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    2.5 */ 0x01FD, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    3.0 */ 0x01FC, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    3.5 */ 0x01FC, 0x02,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $02 -> stack
        { /*    4.0 */ 0x01FB, 0x36,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack
        { /*    4.5 */ 0x01FB, 0x36,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // push $36 -> stack
        { /*    5.0 */ 0xFFFE, 0x42,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    5.5 */ 0xFFFE, 0x06,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    6.0 */ 0xFFFF, 0x06,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch
        { /*    6.5 */ 0xFFFF, 0x02,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // vector fetch

        // --- RTI at $0206 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    7.0 */ 0x0206, 0x02,  R,    1, 0x0206, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // SYNC: RTI
        { /*    7.5 */ 0x0206, 0x40,  R,    1, 0x0206, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // $40 = RTI
        { /*    8.0 */ 0x0207, 0x40,  R,    0, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // dummy / operand
        { /*    8.5 */ 0x0207, 0xEA,  R,    0, 0x0207, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },
        { /*    9.0 */ 0x01FA, 0xEA,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*    9.5 */ 0x01FA, 0x00,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*   10.0 */ 0x01FB, 0x00,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*   10.5 */ 0x01FB, 0x36,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*   11.0 */ 0x01FC, 0x36,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*   11.5 */ 0x01FC, 0x02,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFA, 0b00100110 },  // stack read
        { /*   12.0 */ 0x01FD, 0x02,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // stack read
        { /*   12.5 */ 0x01FD, 0x02,  R,    0, 0x0208, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // stack read

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*   13.0 */ 0x0202, 0x02,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*   13.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*   14.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*   14.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*   15.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*   15.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
