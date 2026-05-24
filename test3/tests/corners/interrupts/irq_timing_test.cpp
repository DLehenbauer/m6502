// test3/tests/corners/interrupts/irq_timing_test.cpp
//
// IRQ entry timing and bus pattern. /IRQ is active-low and level-sensitive.
//
// These tests are skip-gated until test3 exposes IRQ injection.
// Cpu::run() currently has no public hook to pulse irq_n mid-instruction,
// although ICpuShim::drive() accepts the pin.
//
// Coverage:
//   irq_at_instruction_boundary   IRQ enters after the current instruction.
//   irq_during_two_byte_instruction IRQ waits for a multi-cycle operation.
//   irq_blocked_by_i_flag         IRQ is masked while I=1, then accepted.
//   irq_handler_clears_b          Handler entry pushes P with B=0.
//   irq_then_brk_priority         BRK wins over a coincident IRQ.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

// TODO(test3): wire IRQ injection through the shim.

TEST(irq_at_instruction_boundary) {
    SKIP_UNLESS(0, "IRQ injection not yet implemented in shim");
    // TODO(test3): pulse IRQ via shim, expect handler entry via $FFFE/$FFFF

    Program prog = Program::start()
        .cli()
        .nop()
        .fail("IRQ must enter after the interrupted instruction");

    prog.brk_handler()
        .pass("IRQ entered after the interrupted instruction");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(irq_during_two_byte_instruction) {
    SKIP_UNLESS(0, "IRQ injection not yet implemented in shim");
    // TODO(test3): pulse IRQ via shim, expect handler entry via $FFFE/$FFFF

    Program prog = Program::start()
        .cli()
        .lda(Imm{0x42})
        .fail("IRQ must wait for the two-byte instruction");

    prog.brk_handler()
        .pass("IRQ waited for the two-byte instruction");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(irq_blocked_by_i_flag) {
    SKIP_UNLESS(0, "IRQ injection not yet implemented in shim");
    // TODO(test3): pulse IRQ via shim, expect handler entry via $FFFE/$FFFF

    Program prog = Program::start()
        .nop()
        .cli()
        .nop()
        .fail("IRQ must enter after CLI clears I");

    prog.brk_handler()
        .pass("IRQ entered after CLI cleared I");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(irq_handler_clears_b) {
    SKIP_UNLESS(0, "IRQ injection not yet implemented in shim");
    // TODO(test3): pulse IRQ via shim, expect handler entry via $FFFE/$FFFF

    Program prog = Program::start()
        .cli()
        .nop()
        .fail("IRQ must push P with B cleared");

    prog.brk_handler()
        .pass("IRQ pushed P with B cleared");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(irq_then_brk_priority) {
    SKIP_UNLESS(0, "IRQ injection not yet implemented in shim");
    // TODO(test3): pulse IRQ via shim, expect handler entry via $FFFE/$FFFF

    Program prog = Program::start()
        .cli()
        .brk()
        .fail("BRK must vector instead of falling through");

    prog.brk_handler()
        .pass("BRK took priority over coincident IRQ");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}
