// test3/include/test3/cpu.h
//
// Cpu glues a Program (the assembled 6502 code + a 64 KiB memory
// image) to an ICpuShim. It owns the bus-service loop: at each step it
// drives the shim, samples the bus, and at phi edges performs reads
// (presenting data) and writes (committing to memory). It logs deltas
// into a TraceLogger and stops when both (a) the expected trace is
// fully consumed AND (b) the configured terminator fires.

#pragma once

#include "test3/program.h"
#include "test3/shim.h"
#include "test3/trace.h"

#include <cstdint>
#include <cstddef>

namespace test3 {

struct RunResult {
    bool        ok;             // did the run terminate cleanly?
    uint64_t    half_cycles;    // phi half-periods executed (post-reset)
    uint64_t    cycles;          // 6502 cycles executed (post-reset)
    // Terminator message. On success, this is the msg passed to the
    // Program::pass() that fired (or nullptr if termination was via a
    // legacy terminator like set_terminator_pc_trap). On failure, this
    // describes why the run failed (Program::fail() msg, "max_cycles
    // exceeded", etc.). Tests can inspect this to distinguish
    // acceptable variant paths from each other.
    const char* message;
};

class Cpu {
public:
    explicit Cpu(ICpuShim& shim) : shim_(shim) {}

    // Restore the cpu (shim + per-test state) to its "just powered
    // on" baseline: delegates to ICpuShim::power_up() so the netlist
    // returns to constructor-fresh state, then re-initializes the
    // per-run config (terminators, max_cycles, counters, logger) so
    // subsequent load()/run() calls see the same defaults as a
    // freshly-constructed Cpu. The test driver calls this between
    // tests; individual tests should not need to.
    void power_up();

    // Replace the runner's working memory image with the program's.
    // After this, set_terminator_* / run() refer to this memory. The
    // reset vector must already be set on the Program (via
    // Program::reset_handler) before load().
    void load(const Program& prog);

    // Terminator predicates. Multiple can be enabled at once; whichever
    // fires first ends the run.
    //
    // PC-trap: opcode-fetch (SYNC) at the same PC twice in a row.
    void set_terminator_pc_trap(bool enable = true) { term_pc_trap_ = enable; }
    
    // Magic-write: a bus write to `addr` (with optional value match).
    void set_terminator_write(uint16_t addr) {
        term_write_enabled_ = true;
        term_write_addr_    = addr;
    }

    // Cap on total 6502 cycles to run (post-reset). Hitting this is a
    // failure ("timed out"). Defaults to 9999 -- matches the cycle
    // column padding (9999.5) so timeouts stay within the trace
    // table's natural width. Tests that legitimately need more can
    // call set_max_cycles().
    void set_max_cycles(uint64_t n) { max_cycles_ = n; }

    // Get the trace logger so tests can introspect / dump after a run.
    TraceLogger& logger() { return logger_; }

    // Identifier for the loaded shim (e.g. "aholme"); forwarded to
    // trace diagnostics so tests can write `cpu.shim_name()` without
    // having to know which concrete ICpuShim is in use.
    const char* shim_name() const { return shim_.name(); }

    // The Program installed by the most recent load() (or nullptr if
    // none). EXPECT_PASS threads this into EXPECT_TRACE_FROM so the
    // trace-diff dumper can annotate failures with the source line
    // that emitted the offending byte, without forcing the test to
    // pass the Program reference explicitly.
    const Program* program() const { return prog_; }

    // Bitmask of optional TraceRow columns the loaded shim produces.
    // Pass to EXPECT_TRACE / EXPECT_TRACE_FROM so the comparator
    // skips columns the shim can't provide.
    uint32_t trace_fields() const { return shim_.trace_fields(); }

    // Which 6502 variant the loaded shim implements. Tests use this
    // to skip variant-only opcodes (via SKIP_UNLESS) or to branch
    // between divergent expected[] tables.
    CpuVariant variant() const { return shim_.variant(); }

    // Drive all CPU inputs to their idle (deasserted) state and run
    // the shim's canonical reset sequence (hold + post-release cycles
    // per ICpuShim::reset_sequence()). The post-release portion of
    // the canonical sequence includes the two reset-vector fetches at
    // $FFFC then $FFFD; reset() patches those rows' data bytes from
    // the currently loaded memory image before comparing the captured
    // trace against the shim's canonical reset table.
    //
    // A mismatch can mean the shim regressed, the canonical table
    // is stale, the test forgot to set a reset vector before calling
    // reset(), or the test's program populates one of the addresses
    // the canonical reset reads ($0000, $00A8, $00FF, $0100, $01FE,
    // $01FF) with a non-zero value (the canonical assumes those are
    // 0). Either way we surface the failure via EXPECT so it
    // participates in the framework's pass/fail accounting and
    // --fail-fast / --verbose modes. On mismatch reset() prints the
    // diff (via compare_and_report) plus a FAIL block and returns
    // early.
    //
    // The reset vector must be set before calling reset() -- on the
    // Program via Program::reset_handler() prior to load().
    //
    // On success, the cpu's cycle counters and trace logger are
    // cleared so the test's subsequent `run()` sees half_cycle 0 at
    // the first opcode fetch (SYNC=1) at the vectored PC.
    //
    // Idempotent: safe to call repeatedly to re-reset the CPU between
    // sub-tests. Tests that drive the CPU directly (without going
    // through run()) can call this themselves.
    void reset();

    // Execute the program until the terminator fires (or max_cycles).
    // Returns the run result. The delta trace is in logger().rows().
    RunResult run();

    // Memory accessors (the shim's bus service goes through these).
    uint8_t mem_read(uint16_t addr) const  { return mem_[addr]; }
    void    mem_write(uint16_t addr, uint8_t v) { mem_[addr] = v; }

private:
    // Per-cycle bookkeeping passed back out of cycle() so run() can
    // apply terminator checks.
    struct CycleInfo {
        uint16_t addr;
        uint8_t  data;
        bool     rw;
        bool     sync;
    };
    void cycle(CycleInfo& out);

    ICpuShim& shim_;
    uint8_t   mem_[Program::MEM_SIZE]{};
    const Program* prog_         = nullptr;  // captured in load() for terminator lookup
    uint64_t  max_cycles_         = 9999ULL;
    bool      term_pc_trap_       = true;
    bool      term_write_enabled_ = false;
    uint16_t  term_write_addr_    = 0x0000;
    TraceLogger logger_;

    // State carried across cycle() calls within a single run.
    uint64_t cycle_count_ = 0;
    uint64_t half_cycle_  = 0;
    uint8_t  current_dbi_ = 0;
};

} // namespace test3
