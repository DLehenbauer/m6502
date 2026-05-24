// test3/src/cpu.cpp

#include "test3/cpu.h"
#include "test3/expect.h"

#include <cstring>
#include <vector>

namespace test3 {

void Cpu::power_up() {
    shim_.power_up();
    prog_               = nullptr;
    max_cycles_         = 9999ULL;
    term_pc_trap_       = true;
    term_write_enabled_ = false;
    term_write_addr_    = 0x0000;
    cycle_count_        = 0;
    half_cycle_         = 0;
    current_dbi_        = 0;
    std::memset(mem_, 0, Program::MEM_SIZE);
    logger_.reset();
}

void Cpu::load(const Program& prog) {
    std::memcpy(mem_, prog.data(), Program::MEM_SIZE);
    prog_ = &prog;
    // Reset per-run config so each test starts with the same defaults
    // even when the same Cpu is reused across tests (typical: the test
    // driver constructs one Cpu and passes it into every TEST()).
    max_cycles_         = 9999ULL;
    term_pc_trap_       = true;
    term_write_enabled_ = false;
    term_write_addr_    = 0x0000;
    cycle_count_        = 0;
    half_cycle_         = 0;
    current_dbi_        = 0;
    logger_.reset();
}

// One full 6502 cycle: step() to the end-of-phi1 edge, sample +
// service the bus, then step() to the end-of-phi2 edge, sample +
// commit + log. Outputs the just-completed cycle's bus snapshot via
// out parameters so callers can apply terminator checks.
//
// Logging is always on (callers that want to skip logging shouldn't
// call cycle()).
void Cpu::cycle(CycleInfo& out) {
    auto sample_bus = [&]() {
        BusState bus = shim_.sample();
        if (bus.rw) bus.data = current_dbi_;
        return bus;
    };

    // --- phi LOW half (phi1; address-establishment phase) ---
    shim_.step();

    // End-of-phi1 sample.
    BusState bus_p1 = sample_bus();
    out.addr = bus_p1.addr;
    out.rw   = bus_p1.rw;
    out.sync = bus_p1.sync;
    {
        Regs r = shim_.read_registers();
        logger_.append(bus_p1, r);
        ++half_cycle_;

        // Service reads: present the byte memory has for this address
        // before phi2 starts so the CPU latches it.
        if (out.rw) current_dbi_ = mem_read(out.addr);
        shim_.drive(current_dbi_,
                    /*irq_n=*/true, /*nmi_n=*/true,
                    /*rdy=*/true,   /*so_n=*/true);
    }

    // --- phi HIGH half (phi2; data-transfer phase) ---
    shim_.step();

    // End-of-phi2 sample. Everything for this cycle is now settled.
    BusState bus_p2 = sample_bus();
    out.data = bus_p2.data;
    {
        Regs r = shim_.read_registers();
        logger_.append(bus_p2, r);
        ++half_cycle_;

        // Commit writes from dbo to memory.
        if (!out.rw) mem_write(out.addr, bus_p2.data);
    }

    ++cycle_count_;
}

void Cpu::reset() {
    // Fresh state. Cycle / half-cycle counters and the trace log start
    // at zero so the captured reset sequence aligns with the shim's
    // canonical table.
    cycle_count_ = 0;
    half_cycle_  = 0;
    current_dbi_ = 0;
    logger_.reset();

    // Idle every CPU input before driving reset.
    shim_.drive(/*data_in=*/ 0, /*irq_n=*/ true, /*nmi_n=*/ true, /*rdy=*/ true, /*so_n=*/ true);

    const ResetSequence rs = shim_.reset_sequence();

    // Assert reset and cycle through the netlist's warm-up period.
    shim_.set_reset(/*asserted=*/true);
    CycleInfo ci{};
    for (int c = 0; c < rs.hold_cycles; ++c) cycle(ci);

    // Release reset; the CPU now runs its internal post-reset state
    // machine. These cycles are also logged.
    shim_.set_reset(/*asserted=*/false);
    for (int c = 0; c < rs.post_release_cycles; ++c) cycle(ci);

    // Verify the captured trace against the shim's canonical table.
    // The last three data bytes in the canonical table are placeholders
    // for the two reset-vector fetches at $FFFC then $FFFD -- patch
    // them from the loaded memory image so the compare reflects this
    // test's reset vector. (Row [end-4] is the end-of-phi1 of the
    // $FFFC fetch and carries the stale data byte from the prior
    // cycle's read; the canonical table hardcodes that.)
    const size_t expected_count =
        static_cast<size_t>(rs.trace_end - rs.trace_begin);
    std::vector<TraceRow> expected(rs.trace_begin, rs.trace_end);
    if (expected_count >= 4) {
        expected[expected_count - 3].data = mem_read(0xFFFC);
        expected[expected_count - 2].data = mem_read(0xFFFC);
        expected[expected_count - 1].data = mem_read(0xFFFD);
    }

    // A mismatch can mean any of:
    //   - the shim is broken (regressed),
    //   - the canonical table is stale (e.g. after a deliberate
    //     change to the shim or the trace format),
    //   - the test forgot to set a reset vector before calling
    //     reset() (so the patched data bytes disagree with what the
    //     CPU latched),
    //   - the test's program populates one of the addresses the
    //     canonical reset reads ($0000, $00A8, $00FF, $0100, $01FE,
    //     $01FF) with a non-zero value (the canonical assumes those
    //     are 0).
    // Either way we surface it via EXPECT_TRACE so it participates in
    // the framework's pass/fail accounting and verbose/fail-fast modes.
    if (!EXPECT_TRACE("[reset]", shim_.name(), shim_.trace_fields(),
                      expected.data(), expected_count, logger_.rows(),
                      "canonical reset trace must match shim '%s' (see diff above)",
                      shim_.name())) return;

    // Reset succeeded. Drop the reset+vector trace from the log and
    // zero the counters so the test's run loop sees half_cycle 0 at
    // the first opcode fetch.
    logger_.reset();
    cycle_count_ = 0;
    half_cycle_  = 0;
}

RunResult Cpu::run() {
    reset();

    uint16_t prev_sync_pc = 0xFFFF;
    int      same_pc      = 0;

    while (cycle_count_ < max_cycles_) {
        CycleInfo ci{};
        cycle(ci);

        // Terminator: Program::pass() / Program::fail() pseudo-op at
        // an opcode fetch. Takes precedence over the legacy
        // pc-trap/write terminators below: when a test declares
        // pass/fail addresses, it expects those to govern the run.
        if (prog_ && ci.sync) {
            if (const Terminator* t = prog_->terminator_for(ci.addr)) {
                if (t->kind == TermKind::Pass) {
                    return RunResult{true, half_cycle_, cycle_count_, t->msg};
                }
                // Fail: surface via EXPECT so it participates in the
                // framework's pass/fail accounting and fail-fast mode.
                ::test3::detail::report_fail(
                    t->src.file, t->src.line, __func__,
                    "prog.fail() terminator must not be reached",
                    "%s", t->msg);
                return RunResult{false, half_cycle_, cycle_count_, t->msg};
            }
        }

        // Terminator: bus write to magic address.
        if (term_write_enabled_ && !ci.rw && ci.addr == term_write_addr_) {
            return RunResult{true, half_cycle_, cycle_count_, nullptr};
        }

        // Terminator: PC trap (same SYNC PC twice in a row).
        if (term_pc_trap_ && ci.sync) {
            if (ci.addr == prev_sync_pc) {
                if (++same_pc >= 2) {
                    return RunResult{true, half_cycle_, cycle_count_, nullptr};
                }
            } else {
                same_pc      = 0;
                prev_sync_pc = ci.addr;
            }
        }
    }

    return RunResult{false, half_cycle_, cycle_count_, "max_cycles exceeded"};
}

} // namespace test3
