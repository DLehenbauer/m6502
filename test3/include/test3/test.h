// test3/include/test3/test.h
//
// Tiny hand-rolled C++ test registry.
//
// Each TEST(name) { ... } block compiles into a function with a static
// registrar that adds it to a global vector. main() walks the list,
// optionally filtered by --filter=<substr>, runs each in sequence, and
// prints a pass/fail summary.
//
// Phase 1: serial execution only. (Fork-based --jobs=N comes later.)

#pragma once

#include <cstddef>
#include <vector>

namespace test3 {

class Cpu;  // forward; see test3/cpu.h

using TestFn = void (*)(Cpu&);

struct TestCase {
    const char* name;
    TestFn      fn;
};

// Returns the global list of registered tests.
std::vector<TestCase>& test_registry();

// Used by the TEST() macro; appends one entry to the registry.
struct TestRegistrar {
    TestRegistrar(const char* name, TestFn fn) {
        test_registry().push_back(TestCase{name, fn});
    }
};

// On a fatal assertion inside a TEST(), the body should set this and
// return. The runner sets it to false at the start of each test.
void mark_failed(const char* msg);
bool current_failed();

// Mark the current test as skipped (not applicable to this shim
// variant). Reported in the driver's output as "SKIP <name> (<reason>)"
// and aggregated separately from pass/fail in the summary. Tests
// invoke this via the SKIP_UNLESS() macro below.
void mark_skipped(const char* reason);
bool current_skipped();
const char* current_skip_reason();

// Runtime mode accessors used by the EXPECT() macro (see expect.h).
// Both default to false and are toggled by main.cpp's CLI parser.
void set_fail_fast(bool v);
bool fail_fast();
void set_verbose(bool v);
bool verbose();

} // namespace test3

// Token-pasting helpers
#define TEST3_CAT_(a, b) a##b
#define TEST3_CAT(a, b)  TEST3_CAT_(a, b)

// TEST(name) declares a test function with signature `void(Cpu&)`.
// The test driver constructs a Cpu (with the chosen shim) once and
// passes it into every test, so tests are decoupled from the shim
// implementation. Cpu::load() resets per-run config (terminators,
// max_cycles, logger, half-cycle counter) so each test starts clean.
//
// Inside the body, refer to the Cpu as `cpu`:
//   TEST(my_test) {
//       Program prog = Program::start()...;
//       cpu.load(prog);
//       RunResult res = cpu.run();
//       ...
//   }
#define TEST(test_name)                                                       \
    static void TEST3_CAT(test3_test_, test_name)(::test3::Cpu& cpu);         \
    static ::test3::TestRegistrar TEST3_CAT(test3_registrar_, test_name)(     \
        #test_name, &TEST3_CAT(test3_test_, test_name));                      \
    static void TEST3_CAT(test3_test_, test_name)(::test3::Cpu& cpu)

// Mark the current test as skipped if `cpu.variant()` is not in
// `variant_mask` and return. Use for tests that exercise an opcode
// or behavior that doesn't exist on the loaded shim's variant
// (e.g. STZ is 65C02-only; the NMOS BRK+NMI hijack is NMOS-only).
//
//     TEST(stz_abs) {
//         SKIP_UNLESS(::test3::CV_CMOS_65C02, "STZ is 65C02-only");
//         ...
//     }
//
// The driver reports SKIP separately from PASS/FAIL so missed
// coverage (e.g. a test that never ran on any shim) is visible
// rather than silently green.
#define SKIP_UNLESS(variant_mask, reason)                                      \
    do {                                                                       \
        if (!(cpu.variant() & (variant_mask))) {                               \
            ::test3::mark_skipped(reason);                                     \
            return;                                                            \
        }                                                                      \
    } while (0)
