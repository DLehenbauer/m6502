// test3/tests/corners/interrupts/brk_nmi_hijack_test.cpp
//
// NMOS BRK+NMI hijack hardware bug: if /NMI asserts during BRK's
// stack-push or vector-fetch sequence, original NMOS silicon vectors
// through $FFFA/$FFFB while retaining BRK's stacked P with B=1.
// The handler cannot distinguish the hijack from BRK by reading P.
// It must inspect the stacked PC.
//
// Repository memory records the core divergence: m6502 RTL does NOT
// exhibit the NMOS 6502 BRK+NMI hijack hardware bug; the aholme
// transistor-level reference core does. test2/tb_chip_6502_klaus.cpp
// treats Klaus's $075C trap as PASS for the aholme NMOS quirk.
//
// These tests are skip-gated until test3 exposes IRQ/NMI injection.
// Cpu::run() currently has no public hook to pulse nmi_n or irq_n
// mid-instruction, although ICpuShim::drive() accepts both pins.
//
// Coverage:
//   brk_then_nmi_within_brk_sequence_hijack
//   brk_then_nmi_after_brk_pushes
//   nmi_during_irq_pushes_hijack

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(brk_then_nmi_within_brk_sequence_hijack) {
    SKIP_UNLESS(0, "BRK+NMI hijack test requires NMI injection (not yet wired)");
    // TODO(test3): pulse NMI mid-BRK, expect either aholme bug behavior or m6502 RTL correct behavior

    Program prog = Program::start()
        .brk();

    prog.brk_handler()
        .pass("BRK+NMI within BRK sequence path terminates");
    prog.nmi_handler()
        .pass("BRK+NMI hijack path terminates");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(brk_then_nmi_after_brk_pushes) {
    SKIP_UNLESS(0, "BRK+NMI hijack test requires NMI injection (not yet wired)");
    // TODO(test3): pulse NMI mid-BRK, expect either aholme bug behavior or m6502 RTL correct behavior

    Program prog = Program::start()
        .brk();

    prog.brk_handler()
        .pass("BRK after pushes path terminates");
    prog.nmi_handler()
        .pass("BRK+NMI after pushes hijack path terminates");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(nmi_during_irq_pushes_hijack) {
    SKIP_UNLESS(0, "BRK+NMI hijack test requires NMI injection (not yet wired)");
    // TODO(test3): pulse NMI mid-BRK, expect either aholme bug behavior or m6502 RTL correct behavior

    Program prog = Program::start()
        .cli()
        .pass("IRQ path terminates after future injection support");

    prog.brk_handler()
        .pass("IRQ/BRK vector path terminates");
    prog.nmi_handler()
        .pass("NMI during IRQ pushes hijack path terminates");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}
