// Fast C++ testbench for Klaus Dormann's 6502 tests on MCU with BRAM.
// (See: https://github.com/Klaus2m5/6502_65C02_functional_tests)
//
// Run with: make -f Makefile.mcu_klaus run
// Run with waves: WAVES=1 make -f Makefile.mcu_klaus run

#include <verilated.h>
#if VM_TRACE
#include <verilated_vcd_c.h>
#endif
#include "Vtest_mcu_klaus.h"
#include "Vtest_mcu_klaus___024root.h"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>

#define MAX_CYCLES 100000000ULL  // 100M CPU cycles
#define PROGRESS_INTERVAL 1000000ULL  // 1M cycles

// Clock periods in time units:
// i_clk = 50MHz = 20ns period = 10ns half-period
// CPU runs at full speed (CPU_DIV=0, no division)

// Per-instruction test result returned from TestCheckFn callback.
enum class TestResult {
    Continue,   // Keep running
    Pass,       // Stop with success
    Fail        // Stop with failure
};

// Test check function, invoked once per CPU instruction boundary.
//   pc      - the program counter at the about-to-fetch opcode
//   trapped - true when this PC has been seen twice in a row at boundary
//             (CPU is in a tight self-loop)
//   mem     - pointer to the 64 KiB BRAM contents
using TestCheckFn = TestResult (*)(uint16_t pc, bool trapped, const uint8_t* mem);

struct TestCase {
    const char* name;       // Test name for reporting
    const char* bin;        // Path to test binary (relative to executable)
    uint16_t    start_pc;   // Address of test entry point
    TestCheckFn check;      // Per-instruction verdict callback
};

// Klaus's functional test ends with a `JMP *` self-trap.  Trapping at $3469
// is success; trapping anywhere else means a subtest's failure trap fired.
static TestResult check_functional(uint16_t pc, bool trapped, const uint8_t* /*mem*/) {
    if (trapped) {
        return pc == 0x3469
            ? TestResult::Pass
            : TestResult::Fail;
    }

    return TestResult::Continue;
}

// Decimal test halts at the `db $db` byte (DONE label).  PASS iff the ERROR
// variable in zero page is 0; otherwise FAIL. Any unexpected trap is also FAIL.
static TestResult check_decimal(uint16_t pc, bool trapped, const uint8_t* mem) {
    if (pc == 0x024B) {
        return mem[0x000B] == 0
            ? TestResult::Pass
            : TestResult::Fail;
    }

    if (trapped) {
        return TestResult::Fail;
    }

    return TestResult::Continue;
}

// Klaus's interrupt test's `success` macro expands to `jmp *` (a self-trap).
// The automated IRQ / BRK / NMI test ends at $06F5 with this trap.
//
// The two later `success` macros at $070F and $072C live inside the "manual
// tests for the WAI / STP opcode of the 65c02" sections, which require an
// external IRQ to continue past the WAI instruction.
static TestResult check_interrupt(uint16_t pc, bool trapped, const uint8_t* /*mem*/) {
    if (trapped) {
        return pc == 0x06F5
            ? TestResult::Pass
            : TestResult::Fail;
    }

    return TestResult::Continue;
}

// Each test program is assembled using the AS65 assembler:
// https://www.kingswood-consulting.co.uk/assemblers/
static const TestCase kTests[] = {
    // AS65 -l -m -w -h0 -o6502_functional_test.bin 6502_functional_test.a65
    {"functional", "6502_functional_test.bin", /* start_pc: */ 0x0400, check_functional},

    // AS65 -l -m -w -h0 -o6502_decimal_test.bin 6502_decimal_test.a65
    {"decimal",    "6502_decimal_test.bin",    /* start_pc: */ 0x0200, check_decimal},

    // AS65 -l -m -w -h0 -o6502_interrupt_test.bin 6502_interrupt_test.a65
    {"interrupt",  "6502_interrupt_test.bin",  /* start_pc: */ 0x0400, check_interrupt},
};

// Load a test binary image into bram via `--public-flat-rw`.
static bool load_bin(Vtest_mcu_klaus* top, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "ERROR: could not open binary file '%s'\n", path);
        return false;
    }

    auto& mem = top->rootp->test_mcu_klaus__DOT__bram__DOT__memory;
    const size_t mem_size = sizeof(mem) / sizeof(mem[0]);

    // Zero memory before loading the test image in case we relax the 64 KiB
    // size requirement in the future.
    memset(&mem[0], 0, sizeof(mem));

    const size_t n = fread(&mem[0], sizeof(mem[0]), mem_size, f);
    fclose(f);

    // Currently, we expect test binaries to be exactly 64 KiB, but this could be
    // relaxed in the future if needed.
    if (n != mem_size) {
        fprintf(stderr, "ERROR: %s: expected %zu bytes, read %zu\n", path, mem_size, n);
        return false;
    }
    return true;
}

// Reset the CPU and have it begin executing at `start_pc` by transiently
// overwriting the reset vector ($FFFC/$FFFD). The original vector bytes are
// restored once the CPU's PC reaches start_pc, so any later reset still hits
// whatever the loaded image installed at $FFFC.
//
// `tick` is a callable that advances the simulation by one half-clock. Returns
// true once PC == start_pc; false if start_pc isn't reached within a reasonable
// bound (reset hold + CPU init + a small margin).
static bool reset_to_start_pc(Vtest_mcu_klaus* top, std::function<void()> tick, uint16_t start_pc) {
    auto& mem = top->rootp->test_mcu_klaus__DOT__bram__DOT__memory;
    const uint8_t orig_vec_lo = mem[0xFFFC];
    const uint8_t orig_vec_hi = mem[0xFFFD];
    mem[0xFFFC] = (uint8_t)(start_pc & 0xFF);
    mem[0xFFFD] = (uint8_t)(start_pc >> 8);

    // Hold reset for several cycles
    top->rootp->test_mcu_klaus__DOT__i_reset_n = 0;
    for (int i = 0; i < 100; i++) tick();

    // Release reset and tick until the CPU has advanced to `start_pc`.
    top->rootp->test_mcu_klaus__DOT__i_reset_n = 1;
    for (int i = 0; i < 200; i++) {
        tick();
        if (top->rootp->test_mcu_klaus__DOT__cpu_6502__DOT__program_counter == start_pc) {
            mem[0xFFFC] = orig_vec_lo;
            mem[0xFFFD] = orig_vec_hi;
            return true;
        }
    }

    // `start_pc` not reached within reasonable time.
    return false;
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    Vtest_mcu_klaus* top = new Vtest_mcu_klaus;

#if VM_TRACE
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("trace.vcd");
    printf("VCD tracing enabled: trace.vcd\n");
#endif

    // Initialize - hold reset low
    top->i_clk = 0;
    top->rootp->test_mcu_klaus__DOT__i_reset_n = 0;

    uint64_t time_units = 0;
    uint64_t cpu_cycles = 0;
    uint64_t last_progress = 0;
    uint16_t prev_pc = 0xFFFF;
    int same_pc_count = 0;
    const TestCase* current = nullptr;

    // Helper to advance simulation by one time unit
    auto tick = [&]() {
        // i_clk toggles every time unit (50MHz)
        top->i_clk = !top->i_clk;
        top->eval();
        // Count CPU cycles on falling edge (with CPU_DIV=0, every clock is a CPU cycle)
        if (top->i_clk == 0 && time_units > 0) {
            cpu_cycles++;
        }
#if VM_TRACE
        tfp->dump(time_units * 10);  // 10ns per time unit
#endif
        time_units++;
    };

    int rc = 0;
    for (const TestCase& tc : kTests) {
        current = &tc;

        if (!load_bin(top, tc.bin)) {
            rc = 1;
            break;
        }

        if (!reset_to_start_pc(top, tick, tc.start_pc)) {
            fprintf(stderr, "ERROR: %s test did not reach start_pc=$%04X after reset\n",
                    tc.name, tc.start_pc);
            rc = 1;
            break;
        }

        printf("Starting 6502 %s test (MCU with BRAM)...\n", tc.name);

        cpu_cycles = 0;
        last_progress = 0;
        prev_pc = 0xFFFF;
        same_pc_count = 0;
        uint64_t prev_cpu_cycles = 0;
        uint16_t pc = 0;
        TestResult result = TestResult::Continue;

        while (cpu_cycles < MAX_CYCLES && result == TestResult::Continue) {
            tick();

            // Only check on CPU clock falling edges (when cpu_cycles increments)
            if (cpu_cycles == prev_cpu_cycles)
                continue;
            prev_cpu_cycles = cpu_cycles;

            // Get current PC from CPU
            pc = top->rootp->test_mcu_klaus__DOT__cpu_6502__DOT__program_counter;

            // Progress reporting (based on CPU cycles)
            if (cpu_cycles - last_progress >= PROGRESS_INTERVAL) {
                printf("Progress: %lluM CPU cycles, PC=$%04X\n",
                       (unsigned long long)(cpu_cycles / 1000000), pc);
                last_progress = cpu_cycles;
            }

            // Trap detection: check if PC is stuck
            // Only check on instruction boundaries (first_microinstruction)
            if (top->rootp->test_mcu_klaus__DOT__cpu_6502__DOT__first_microinstruction) {
                bool trapped = false;
                if (pc == prev_pc) {
                    if (++same_pc_count >= 2) trapped = true;
                } else {
                    same_pc_count = 0;
                    prev_pc = pc;
                }

                auto& m = top->rootp->test_mcu_klaus__DOT__bram__DOT__memory;
                result = current->check(pc, trapped, &m[0]);
            }
        }

        // On pass, continue to next test.
        if (result == TestResult::Pass) {
            printf("SUCCESS: %s test passed at PC=$%04X after %llu CPU cycles\n",
                   current->name, pc, (unsigned long long)cpu_cycles);
            continue;
        }

        // Otherwise stop and report failure or timeout.
        if (result == TestResult::Fail) {
            printf("FAIL: %s test failed at PC=$%04X after %llu CPU cycles\n",
                   current->name, pc, (unsigned long long)cpu_cycles);
        } else {
            printf("TIMEOUT: %s test did not complete within %llu CPU cycles\n",
                   current->name, (unsigned long long)MAX_CYCLES);
        }

        rc = 1;
        break;
    }

#if VM_TRACE
    tfp->close();
#endif
    delete top;
    return rc;
}
