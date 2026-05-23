// Tiny smoke test for the aholme transistor-level chip_6502, used to
// validate the Verilator testbench harness (bus servicing, reset, cycle
// counting, SYNC-based instruction boundary detection) before pointing
// it at the much-longer Klaus suite.
//
// The program runs entirely from RAM ($0400-...), uses zero-page memory
// as scratch, and ends in a `JMP *` self-loop so we can detect "done".
//
// Build: g++ embeds tb_smoke.cpp into the same Verilator build dir as
// the Klaus harness; see Makefile.aholme_smoke.

#include <verilated.h>
#include "Vchip_6502.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static constexpr int kClkPerPhiHalf = 8;

static constexpr size_t MEM_SIZE = 0x10000;
static uint8_t g_mem[MEM_SIZE];

// Hand-assembled smoke test, loaded at $0400. Reset vector points here.
//
//   $0400  A9 42       LDA #$42
//   $0402  85 10       STA $10
//   $0404  A2 05       LDX #$05
//   $0406  86 11       STX $11
//   $0408  A0 99       LDY #$99
//   $040A  84 12       STY $12
//   $040C  A9 01       LDA #$01
//   $040E  18          CLC
//   $040F  69 02       ADC #$02         ; A = 3
//   $0411  85 13       STA $13
//   $0413  A9 00       LDA #$00
//   $0415  85 20       STA $20          ; clear loop counter
//   $0417  E6 20       INC $20          ; loop: counter++
//   $0419  A5 20       LDA $20
//   $041B  C9 05       CMP #$05         ; counter == 5?
//   $041D  D0 F8       BNE $0417        ;   no, branch back
//   $041F  4C 1F 04    JMP $041F        ; done: self-trap
//
// Expected post-conditions when the CPU traps at $041F:
//   mem[$10] == $42, mem[$11] == $05, mem[$12] == $99
//   mem[$13] == $03 (LDA #$01 + ADC #$02 with carry clear)
//   mem[$20] == $05 (loop ran 5 times)
//
static const uint8_t kSmokeProgram[] = {
    0xA9, 0x42,        // LDA #$42
    0x85, 0x10,        // STA $10
    0xA2, 0x05,        // LDX #$05
    0x86, 0x11,        // STX $11
    0xA0, 0x99,        // LDY #$99
    0x84, 0x12,        // STY $12
    0xA9, 0x01,        // LDA #$01
    0x18,              // CLC
    0x69, 0x02,        // ADC #$02
    0x85, 0x13,        // STA $13
    0xA9, 0x00,        // LDA #$00
    0x85, 0x20,        // STA $20
    0xE6, 0x20,        // INC $20         <-- loop target = $0417
    0xA5, 0x20,        // LDA $20
    0xC9, 0x05,        // CMP #$05
    0xD0, 0xF8,        // BNE -8 -> $0417
    0x4C, 0x1F, 0x04,  // JMP $041F (self trap)
};
static constexpr uint16_t kProgStart = 0x0400;
static constexpr uint16_t kTrapPc    = 0x041F;

static inline uint8_t  bus_read (uint16_t a)             { return g_mem[a]; }
static inline void     bus_write(uint16_t a, uint8_t d)  { g_mem[a] = d; }

struct Sim {
    Vchip_6502* top = nullptr;
    uint64_t    cpu_cycles = 0;

    void clk_tick() {
        top->clk = 1; top->eval();
        top->clk = 0; top->eval();
    }

    struct CycleInfo { uint16_t ab; bool sync; bool rw; uint8_t dbo; };

    // Bus model:
    //   - Address `ab` and `rw` are established by the CPU during phi1
    //     (phi low). At the rising edge of phi (start of phi2) they are
    //     stable for the cycle.
    //   - For READS, memory must present data on `dbi` during phi2 so
    //     the CPU can latch it: we update `dbi` at posedge phi.
    //   - For WRITES, the CPU drives `dbo` during phi2; we capture it
    //     at the END of phi2 (negedge phi).
    //
    // Returns the per-cycle bus snapshot.
    CycleInfo cpu_cycle() {
        // phi LOW half (phi1 = address phase)
        top->phi = 0;
        for (int i = 0; i < kClkPerPhiHalf; ++i) clk_tick();

        // Posedge phi: sample address + control; present read data.
        const uint16_t ab   = top->ab;
        const bool     rw   = top->rw != 0;
        const bool     sync = top->sync != 0;
        if (rw) top->dbi = bus_read(ab);

        // phi HIGH half (phi2 = data phase)
        top->phi = 1;
        for (int i = 0; i < kClkPerPhiHalf; ++i) clk_tick();

        // Negedge phi: capture write data.
        const uint8_t dbo = top->dbo;
        if (!rw) bus_write(ab, dbo);

        ++cpu_cycles;
        return CycleInfo{ab, sync, rw, dbo};
    }
};

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    Sim sim;
    sim.top = new Vchip_6502;

    sim.top->clk = 0;
    sim.top->phi = 0;
    sim.top->res = 0;
    sim.top->so  = 0;
    sim.top->rdy = 1;
    sim.top->nmi = 1;
    sim.top->irq = 1;
    sim.top->dbi = 0;

    // Lay down the program + reset vector in main memory.
    std::memset(g_mem, 0, sizeof(g_mem));
    std::memcpy(&g_mem[kProgStart], kSmokeProgram, sizeof(kSmokeProgram));
    g_mem[0xFFFC] = static_cast<uint8_t>(kProgStart & 0xFF);
    g_mem[0xFFFD] = static_cast<uint8_t>(kProgStart >> 8);

    // Hold reset, warm up the netlist, then release.
    for (int i = 0; i < 32; ++i) sim.cpu_cycle();
    sim.top->res = 1;

    // Run until either we trap at kTrapPc, hit a hard cycle limit, or
    // something else self-traps.
    constexpr uint64_t MAX_CYCLES = 5000;
    uint16_t prev_pc = 0xFFFF;
    int same_pc = 0;
    uint16_t pc = 0;
    bool started = false;
    bool ok = false;

    std::printf("Smoke test: program at $%04X, expected trap at $%04X\n",
                kProgStart, kTrapPc);

    while (sim.cpu_cycles < MAX_CYCLES) {
        auto ci = sim.cpu_cycle();
        if (ci.sync) {
            pc = ci.ab;
            if (!started) {
                std::printf("First SYNC at PC=$%04X (cycle %llu)\n",
                            pc, (unsigned long long)sim.cpu_cycles);
                started = true;
            }
            if (pc == prev_pc) {
                if (++same_pc >= 2) {
                    std::printf("Trap detected at PC=$%04X (cycle %llu)\n",
                                pc, (unsigned long long)sim.cpu_cycles);
                    ok = (pc == kTrapPc);
                    break;
                }
            } else {
                same_pc = 0;
                prev_pc = pc;
            }
        }
    }

    if (!ok) {
        std::printf("FAIL: did not reach %s. Last PC=$%04X after %llu cycles\n",
                    "kTrapPc", pc, (unsigned long long)sim.cpu_cycles);
        delete sim.top;
        return 1;
    }

    // Verify expected memory contents.
    struct Expect { uint16_t addr; uint8_t want; const char* name; };
    static const Expect kExpect[] = {
        {0x0010, 0x42, "STA #$42 -> $10"},
        {0x0011, 0x05, "STX #$05 -> $11"},
        {0x0012, 0x99, "STY #$99 -> $12"},
        {0x0013, 0x03, "ADC #$01+#$02 -> $13"},
        {0x0020, 0x05, "INC loop count -> $20"},
    };

    bool all_pass = true;
    for (const auto& e : kExpect) {
        uint8_t got = g_mem[e.addr];
        std::printf("  $%04X: want $%02X got $%02X  %s   %s\n",
                    e.addr, e.want, got,
                    (got == e.want) ? "PASS" : "FAIL", e.name);
        if (got != e.want) all_pass = false;
    }

    std::printf("%s after %llu cycles\n",
                all_pass ? "SUCCESS" : "FAIL",
                (unsigned long long)sim.cpu_cycles);

    delete sim.top;
    return all_pass ? 0 : 1;
}
