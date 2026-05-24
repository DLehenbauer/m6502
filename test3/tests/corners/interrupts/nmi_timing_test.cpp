// test3/tests/corners/interrupts/nmi_timing_test.cpp
//
// NMI entry timing. /NMI is active-low and edge-triggered.
//
// Coverage:
//   nmi_at_instruction_boundary  NMI assertion during an instruction enters
//                                the handler after that instruction finishes.
//   nmi_not_blocked_by_i_flag    NMI bypasses I, unlike IRQ.
//   nmi_handler_pushes_pc_and_p  Handler entry pushes return PC and P with B=0.
//   nmi_edge_triggered_not_level A held-low /NMI level fires one NMI.
//   nmi_vector_fetch             Handler entry fetches $FFFA/$FFFB.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(nmi_at_instruction_boundary) {
    SKIP_UNLESS(0, "NMI injection not yet implemented");
    // TODO(test3): assert NMI edge, expect handler entry via $FFFA/$FFFB

    Program prog = Program::start()
        .nop()
        .fail("NMI must enter after the interrupted instruction");

    prog.nmi_handler()
        .pass("NMI entered after the interrupted instruction");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(nmi_not_blocked_by_i_flag) {
    SKIP_UNLESS(0, "NMI injection not yet implemented");
    // TODO(test3): assert NMI edge, expect handler entry via $FFFA/$FFFB

    Program prog = Program::start()
        .sei()
        .nop()
        .fail("NMI must bypass the I flag");

    prog.nmi_handler()
        .pass("NMI bypassed the I flag");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(nmi_handler_pushes_pc_and_p) {
    SKIP_UNLESS(0, "NMI injection not yet implemented");
    // TODO(test3): assert NMI edge, expect handler entry via $FFFA/$FFFB

    Program prog = Program::start()
        .nop()
        .fail("NMI must push return state before handler entry");

    prog.nmi_handler()
        .pass("NMI pushed return PC and P with B=0");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(nmi_edge_triggered_not_level) {
    SKIP_UNLESS(0, "NMI injection not yet implemented");
    // TODO(test3): assert NMI edge, expect handler entry via $FFFA/$FFFB

    Program prog = Program::start()
        .nop()
        .nop()
        .fail("held-low /NMI must not retrigger without a new edge");

    prog.nmi_handler()
        .pass("held-low /NMI fired once");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(nmi_vector_fetch) {
    SKIP_UNLESS(0, "NMI injection not yet implemented");
    // TODO(test3): assert NMI edge, expect handler entry via $FFFA/$FFFB

    Program prog = Program::start()
        .nop()
        .fail("NMI must fetch the $FFFA/$FFFB vector");

    prog.nmi_handler(0x0500)
        .pass("NMI fetched the $FFFA/$FFFB vector");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}
