// test3/src/main.cpp
//
// Test driver. Walks the global TEST() registry and runs each test
// against every available shim. Each test sees `Cpu& cpu` -- the
// driver constructs the shim+Cpu pair once per shim, reuses across
// tests in that shim, then tears down and moves to the next shim.
// Tests are decoupled from the shim implementation. Filtering via
// --filter=<substr>.

#include "test3/test.h"
#include "test3/cpu.h"
#include "test3/aholme_shim.h"
#include "test3/m6502_shim.h"

#include <verilated.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

namespace test3 {

std::vector<TestCase>& test_registry() {
    static std::vector<TestCase> g;
    return g;
}

static bool        g_failed      = false;
static std::string g_failmsg;
static bool        g_skipped     = false;
static std::string g_skipreason;

void mark_failed(const char* msg) {
    g_failed = true;
    if (msg) g_failmsg = msg;
}

bool current_failed() { return g_failed; }

void mark_skipped(const char* reason) {
    g_skipped = true;
    if (reason) g_skipreason = reason;
}

bool current_skipped() { return g_skipped; }
const char* current_skip_reason() {
    return g_skipreason.empty() ? "(no reason)" : g_skipreason.c_str();
}

} // namespace test3

static void usage(const char* argv0) {
    std::fprintf(stderr,
        "Usage: %s [--filter=SUBSTR] [--list] [--fail-fast] [-v|--verbose]\n"
        "\n"
        "  --filter=SUBSTR  run only tests whose name contains SUBSTR\n"
        "  --list           print test names and exit\n"
        "  --fail-fast      abort the process on the first failed EXPECT\n"
        "                   (default: record failure via mark_failed and continue)\n"
        "  -v, --verbose    print 'OK: <message>' for every successful EXPECT\n"
        "                   (failures are always printed)\n",
        argv0);
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    std::setvbuf(stderr, nullptr, _IOLBF, 0);

    const char* filter = nullptr;
    bool        list   = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--filter=", 9) == 0) {
            filter = argv[i] + 9;
        } else if (std::strcmp(argv[i], "--list") == 0) {
            list = true;
        } else if (std::strcmp(argv[i], "--fail-fast") == 0) {
            test3::set_fail_fast(true);
        } else if (std::strcmp(argv[i], "--verbose") == 0 ||
                   std::strcmp(argv[i], "-v") == 0) {
            test3::set_verbose(true);
        } else if (std::strcmp(argv[i], "--help") == 0 ||
                   std::strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            // Unknown flag -> usage + fail.
            std::fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    auto& reg = test3::test_registry();

    if (list) {
        for (const auto& tc : reg) {
            std::printf("%s\n", tc.name);
        }
        return 0;
    }

    // Available shims. Add new shims here as they come online; the
    // outer loop will then exercise every test against each shim,
    // with SKIP_UNLESS() inside tests filtering out tests that
    // don't apply to the current shim's variant.
    std::vector<std::unique_ptr<test3::ICpuShim>> shims;
    shims.emplace_back(std::make_unique<test3::AholmeShim>());
    shims.emplace_back(std::make_unique<test3::M6502Shim>());

    int run_count   = 0;
    int pass_count  = 0;
    int fail_count  = 0;
    int skip_count  = 0;

    for (const auto& shim_up : shims) {
        test3::ICpuShim& shim = *shim_up;
        std::printf("=== Shim: %s ===\n", shim.name());

        // Construct one Cpu per shim and reuse across all tests for
        // that shim. Cpu::load() resets per-run config (terminators,
        // max_cycles, logger, counters) so tests don't leak state.
        test3::Cpu cpu(shim);

        for (const auto& tc : reg) {
            if (filter && std::strstr(tc.name, filter) == nullptr) continue;

            ++run_count;
            std::printf("RUN   %s\n", tc.name);

            test3::g_failed   = false;
            test3::g_failmsg.clear();
            test3::g_skipped  = false;
            test3::g_skipreason.clear();

            // Restore the cpu to its post-construction baseline so
            // the test sees a pristine shim. Without this, netlist
            // node state from the previous test leaks into the
            // canonical reset trace check inside Cpu::reset().
            cpu.power_up();

            tc.fn(cpu);

            if (test3::g_skipped) {
                ++skip_count;
                std::printf("SKIP  %s (%s)\n", tc.name,
                            test3::current_skip_reason());
            } else if (test3::g_failed) {
                ++fail_count;
                std::printf("FAIL  %s: %s\n", tc.name,
                            test3::g_failmsg.empty() ? "(no message)"
                                                     : test3::g_failmsg.c_str());
            } else {
                ++pass_count;
                std::printf("PASS  %s\n", tc.name);
            }
        }
    }

    std::printf("\nSummary: %d run across %zu shim(s), "
                "%d passed, %d failed, %d skipped\n",
                run_count, shims.size(),
                pass_count, fail_count, skip_count);
    return fail_count == 0 ? 0 : 1;
}
