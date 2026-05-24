// test3/include/test3/trace.h
//
// Per-half-cycle trace data structures for the test3 framework.
//
// Concept: each 6502 cycle has two silicon-stable observation windows
// (see test3/README.md for the datasheet-derived rationale):
//   - end of phi1, just before posedge phi2 ("setup" window)
//   - end of phi2, just before negedge phi2 ("settle" window)
//
// The runner samples at each of these and appends one TraceRow per
// half-cycle, producing 2 rows per 6502 cycle.
//
// Row N maps to:
//   cycle = N / 2
//   phase = (N % 2 == 0) ? END_OF_PHI1 : END_OF_PHI2
//
// Each test embeds an `expected[]` array of TraceRow values; on a run,
// the actual stream is compared lockstep against expected. On the first
// divergence the framework prints a paste-ready C++ block of the
// *actual* trace that the author can copy back into the test source.

#pragma once

#include "test3/shim.h"
#include "test3/expect.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace test3 {

// Forward-declared to avoid pulling program.h into every trace.h
// consumer. trace.cpp includes program.h to do the actual src_for()
// lookup.
class Program;

// Use lowercase r / w so the in-source tables read like assembler.
enum Rw : uint8_t { R = 1, W = 0 };

// Which silicon-stable observation window this row was sampled in.
enum Phase : uint8_t {
    END_OF_PHI1 = 0,  // just before posedge phi2
    END_OF_PHI2 = 1,  // just before negedge phi2
};

struct TraceRow {
    uint16_t addr;
    uint8_t  data;
    uint8_t  rw;          // Rw enum
    uint8_t  sync;        // 0 or 1
    // Architectural registers; only compared when the shim's
    // trace_fields() mask includes TF_REGS. Shims that don't expose
    // registers leave these zero. Field order (PC, A, X, Y, S, P)
    // matches Klaus Dormann's test convention and MAME/py65/VICE
    // debuggers: PC-first because "where am I" frames everything else.
    uint16_t pc;
    uint8_t  a;
    uint8_t  x;
    uint8_t  y;
    uint8_t  s;
    uint8_t  p;
};

inline bool rows_equal(const TraceRow& a, const TraceRow& b, uint32_t mask) {
    if (a.addr != b.addr) return false;
    if (a.data != b.data) return false;
    if (a.rw   != b.rw)   return false;
    if (a.sync != b.sync) return false;
    if (mask & TF_REGS) {
        if (a.pc != b.pc) return false;
        if (a.a != b.a) return false;
        if (a.x != b.x) return false;
        if (a.y != b.y) return false;
        if (a.s != b.s) return false;
        if (a.p != b.p) return false;
    }
    return true;
}

// Flat per-half-cycle appender. No delta suppression -- every sample
// becomes a row. The runner calls append() once at end-of-phi1 and once
// at end-of-phi2 for each 6502 cycle, so a row's array index is its
// half-cycle index (cycle = index/2; phase = phi1 if index%2==0 else phi2).
class TraceLogger {
public:
    void reset() { rows_.clear(); }

    void append(const BusState& bus,
                const Regs& regs) {
        TraceRow r{};
        r.addr       = bus.addr;
        r.data       = bus.data;
        r.rw         = bus.rw ? R : W;
        r.sync       = bus.sync ? 1 : 0;
        r.a  = regs.a;
        r.x  = regs.x;
        r.y  = regs.y;
        r.s  = regs.s;
        r.p  = regs.p;
        r.pc = regs.pc;
        rows_.push_back(r);
    }

    const std::vector<TraceRow>& rows() const { return rows_; }

private:
    std::vector<TraceRow> rows_;
};

// Compare expected vs actual. If they match exactly, returns true.
// Otherwise prints a side-by-side diff to stderr and emits the full
// actual trace formatted as a paste-ready C++ block. `fields_mask` is
// the shim's trace_fields() bitmask -- columns absent from the mask
// are excluded from comparison and printed as "_" placeholders.
bool compare_and_report(const char* test_name,
                        const char* core_name,
                        uint32_t fields_mask,
                        const TraceRow* expected,
                        size_t expected_count,
                        const std::vector<TraceRow>& actual);

// Source-aware overload. When `prog` is non-null, the diff dumper
// looks up the source location of the byte at expected[first_diff].addr
// in the Program's source map and prints
//     source:   <file>:<line>
// under the diff, pointing at the line of test code that emitted the
// offending byte. Use via EXPECT_TRACE_FROM (below) from any test
// that has a Program in scope.
bool compare_and_report(const char* test_name,
                        const char* core_name,
                        uint32_t fields_mask,
                        const TraceRow* expected,
                        size_t expected_count,
                        const std::vector<TraceRow>& actual,
                        const Program* prog);

// Print just the actual table as a paste-ready C++ block. When
// `prog` is non-null, the printer also emits section headers at each
// SYNC boundary, collapses landing-pad-NOP + JMP-self trap pairs
// into a single PASS/FAIL header (kind looked up via
// prog->terminator_for()), and tags each row with a short trailing
// comment naming what the cycle is doing.
void print_actual_paste_block(uint32_t fields_mask,
                              const std::vector<TraceRow>& actual,
                              const Program* prog);
void print_actual_paste_block(uint32_t fields_mask,
                              const std::vector<TraceRow>& actual);

} // namespace test3

// Compare an expected vs captured trace and surface the result through
// the EXPECT framework. compare_and_report prints a side-by-side diff
// and a paste-ready actual block on mismatch; EXPECT prints the
// standard FAIL block (or an "OK:" line in --verbose mode) and
// participates in the framework's pass/fail accounting and
// --fail-fast mode.
//
// `label` and `core_name` are forwarded to compare_and_report's
// "FAIL: <label> [core=<core_name>]" diff header. `fields_mask` is
// the shim's trace_fields() bitmask -- typically `shim.trace_fields()`
// or `runner.trace_fields()`. The trailing __VA_ARGS__ is the
// spec-style EXPECT message (mandatory) plus optional printf-style
// args -- see EXPECT for the convention.
//
// Like EXPECT, this is an expression of type bool equal to "did the
// traces match?". Use the short-circuit form to bail out:
//
//     if (!EXPECT_TRACE("[reset]", shim_.name(), shim_.trace_fields(),
//                       expected.data(), expected.size(), logger_.rows(),
//                       "canonical reset trace must match shim '%s'",
//                       shim_.name())) return;
#define EXPECT_TRACE(label, core_name, fields_mask, expected_ptr, expected_count, actual, ...) \
    __extension__ ({                                                                            \
        const bool traces_match = ::test3::compare_and_report(                                  \
            (label), (core_name), (fields_mask),                                                \
            (expected_ptr), (expected_count),                                                   \
            (actual));                                                                          \
        EXPECT(traces_match, __VA_ARGS__);                                                      \
    })

// Source-annotated variant of EXPECT_TRACE: takes a `Program*` as the
// first argument. On mismatch the diff dumper appends the source line
// of the emit at expected[first_diff].addr, so failure output points
// directly at the line of the test that produced the offending byte.
//
//     EXPECT_TRACE_FROM(&prog,
//                       "smoke", shim.name(), shim.trace_fields(),
//                       expected, expected_count, runner.logger().rows(),
//                       "captured trace must match expected[]");
//
// Pass `nullptr` for the Program* to opt out of annotation (rarely
// useful -- use plain EXPECT_TRACE in that case).
#define EXPECT_TRACE_FROM(prog_ptr, label, core_name, fields_mask, expected_ptr, expected_count, actual, ...) \
    __extension__ ({                                                                                          \
        const bool traces_match = ::test3::compare_and_report(                                                \
            (label), (core_name), (fields_mask),                                                              \
            (expected_ptr), (expected_count),                                                                 \
            (actual), (prog_ptr));                                                                            \
        EXPECT(traces_match, __VA_ARGS__);                                                                    \
    })

// EXPECT_PASS(expected) -- one-shot assertion: runs the CPU until a
// terminator fires, asserts that the run terminated via prog.pass()
// (not prog.fail() / max_cycles), and matches the captured trace
// against expected[].
//
// Must be called inside a TEST(name) { ... } body so that `cpu` is in
// scope (the TEST() macro provides it as Cpu&). The Program must
// already be loaded:
//
//     Program prog = Program::start().lda(Imm{0x42})...brk();
//     auto done = prog.pass("...");
//     prog.brk_handler(done);
//
//     cpu.load(prog);
//     EXPECT_PASS(expected);
//
// `expected` must be a C array (length deduced via sizeof / sizeof).
// The Program installed by the preceding cpu.load() is threaded into
// EXPECT_TRACE_FROM via cpu.program() so trace-diff failures still
// annotate the offending byte with its emitting source line. The
// test name in diagnostics comes from __func__ (auto-captured).
//
// Returns bool (overall pass/fail), so callers can chain with the
// short-circuit form:
//
//     if (!EXPECT_PASS(expected)) return;  // also OK as a discarded value
#define EXPECT_PASS(expected)                                                              \
    __extension__ ({                                                                       \
        const ::test3::RunResult _t3_res = cpu.run();                                      \
        bool _t3_ok = EXPECT(_t3_res.ok,                                                   \
            "run must terminate via prog.pass() "                                          \
            "(got message=%s, %llu cycles, %llu half-cycles)",                             \
            _t3_res.message ? _t3_res.message : "?",                                       \
            (unsigned long long)_t3_res.cycles,                                            \
            (unsigned long long)_t3_res.half_cycles);                                      \
        if (_t3_ok) {                                                                      \
            _t3_ok = EXPECT_TRACE_FROM(cpu.program(),                                      \
                __func__, cpu.shim_name(), cpu.trace_fields(),                             \
                (expected),                                                                \
                sizeof(expected) / sizeof((expected)[0]),                                  \
                cpu.logger().rows(),                                                       \
                "captured trace must match expected[] (see diff above)");                  \
        }                                                                                  \
        _t3_ok;                                                                            \
    })
