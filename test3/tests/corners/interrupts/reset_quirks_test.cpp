// test3/tests/corners/interrupts/reset_quirks_test.cpp
//
// Reset-vector setup and post-reset state coverage.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(reset_jumps_to_vector_at_FFFC_FFFD) {
    Program prog = Program::start();

    prog.reset_handler(0x0500)
        .pass("reset vector $FFFC/$FFFD reached $0500");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- PASS at $0500: NOP flushes pipelined registers, JMP-self at $0501 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0500, 0x05,  R,    1, 0x0500, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0500, 0xEA,  R,    1, 0x0500, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0501, 0xEA,  R,    0, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0501, 0x4C,  R,    0, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    2.5 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(reset_sets_i_flag) {
    Program prog = Program::start()
        .pass("reset sets I in P before first instruction");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    2.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(reset_initializes_sp) {
    Program prog = Program::start()
        .pass("reset leaves S at $FD before first instruction");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- PASS at $0200: NOP flushes pipelined registers, JMP-self at $0201 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0200, 0xEA,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0201, 0xEA,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x4C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    2.5 */ 0x0201, 0x4C,  R,    1, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(reset_vector_via_label) {
    Label target{0x0203};
    Program prog = Program::start();

    prog.reset_handler(target)
        .jmp(target)
        .pass("reset_handler(Label) kept cursor at program entry");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0203, 0x02,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    2.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(reset_handler_no_arg_uses_cursor) {
    Program prog = Program::start();

    prog.org(0x0400);
    prog.reset_handler()
        .pass("reset_handler() used the current cursor");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- PASS at $0400: NOP flushes pipelined registers, JMP-self at $0401 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0400, 0x04,  R,    1, 0x0400, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    0.5 */ 0x0400, 0xEA,  R,    1, 0x0400, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    1.0 */ 0x0401, 0xEA,  R,    0, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0401, 0x4C,  R,    0, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0401, 0x4C,  R,    1, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    2.5 */ 0x0401, 0x4C,  R,    1, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
