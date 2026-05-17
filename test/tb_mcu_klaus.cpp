// Fast C++ testbench for Klaus Dormann's 6502 tests on MCU with BRAM
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

#define MAX_CYCLES 100000000ULL  // 100M CPU cycles
#define PROGRESS_INTERVAL 1000000ULL  // 1M cycles

// Per-test parameters.  bram is loaded at runtime (rather than via INIT_FILE)
// so a single elaborated binary can run multiple programs back-to-back.
struct TestCase {
    const char* name;
    const char* hex;
    uint16_t    success_pc;
};
static const TestCase kTests[] = {
    {"functional", "6502_functional_test.hex", 0x3469},
    {"interrupt",  "6502_interrupt_test.hex",  0x06F5},
};

// Loader for .hex files (one byte per line, sequential from $0000).
static bool load_hex(Vtest_mcu_klaus* top, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: could not open hex file '%s'\n", path);
        return false;
    }
    auto& mem = top->rootp->test_mcu_klaus__DOT__bram__DOT__memory;
    unsigned byte;
    uint32_t addr = 0;
    while (addr < 65536 && fscanf(f, "%x", &byte) == 1) {
        mem[addr++] = (uint8_t)byte;
    }
    fclose(f);
    return true;
}

// Clock periods in time units:
// i_clk = 50MHz = 20ns period = 10ns half-period
// CPU runs at full speed (CPU_DIV=0, no division)

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
        if (!load_hex(top, tc.hex)) { rc = 1; break; }

        // Hold reset for several cycles
        top->rootp->test_mcu_klaus__DOT__i_reset_n = 0;
        for (int i = 0; i < 100; i++) {
            tick();
        }

        // Release reset
        top->rootp->test_mcu_klaus__DOT__i_reset_n = 1;

        // Wait for CPU init
        for (int i = 0; i < 200; i++) {
            tick();
        }

        printf("Starting Klaus 6502 %s test (MCU with BRAM)...\n", tc.name);

        uint64_t test_start_cycles = cpu_cycles;
        last_progress = cpu_cycles;
        uint64_t prev_cpu_cycles = cpu_cycles;
        prev_pc = 0xFFFF;
        same_pc_count = 0;
        bool done = false;

        while (cpu_cycles - test_start_cycles < MAX_CYCLES) {
            tick();

            // Only check on CPU clock falling edges (when cpu_cycles increments)
            if (cpu_cycles == prev_cpu_cycles)
                continue;
            prev_cpu_cycles = cpu_cycles;

            // Get current PC from CPU
            uint16_t pc = top->rootp->test_mcu_klaus__DOT__cpu_6502__DOT__program_counter;

            // Progress reporting (based on CPU cycles)
            if (cpu_cycles - last_progress >= PROGRESS_INTERVAL) {
                printf("Progress: %lluM CPU cycles, PC=$%04X\n",
                       (unsigned long long)((cpu_cycles - test_start_cycles) / 1000000), pc);
                last_progress = cpu_cycles;
            }

            // Trap detection: check if PC is stuck
            // Only check on instruction boundaries (first_microinstruction)
            if (top->rootp->test_mcu_klaus__DOT__cpu_6502__DOT__first_microinstruction) {
                if (pc == prev_pc) {
                    same_pc_count++;
                    if (same_pc_count >= 2) {
                        if (pc == tc.success_pc) {
                            printf("SUCCESS: %s test passed at PC=$%04X after %llu CPU cycles\n",
                                   tc.name, pc,
                                   (unsigned long long)(cpu_cycles - test_start_cycles));
                            done = true;
                        } else {
                            printf("TRAP: %s test failed at PC=$%04X after %llu CPU cycles\n",
                                   tc.name, pc,
                                   (unsigned long long)(cpu_cycles - test_start_cycles));
                            rc = 1;
                            done = true;
                        }
                        break;
                    }
                } else {
                    same_pc_count = 0;
                    prev_pc = pc;
                }
            }
        }

        if (!done) {
            printf("TIMEOUT: %s test did not complete within %llu CPU cycles\n",
                   tc.name, (unsigned long long)MAX_CYCLES);
            rc = 1;
            break;
        }
    }

#if VM_TRACE
    tfp->close();
#endif
    delete top;
    return rc;
}
