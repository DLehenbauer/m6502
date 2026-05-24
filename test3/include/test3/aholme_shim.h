// test3/include/test3/aholme_shim.h
//
// ICpuShim wrapper around Andrew Holme's transistor-level chip_6502
// (Visual 6502 netlist auto-generated to Verilog). Verilator builds it
// as Vchip_6502; the sources live in test2/aholme/ (vendored unmodified
// from http://www.aholme.co.uk/6502/SRC/Core/).
//
// Step model:
//   - One step() == one phi half-period: do SETTLE_TICKS_PER_HALF
//     `clk` toggles (so the ~1700 dynamic nodes settle), then flip
//     the netlist's `phi` input. Two step()s == one 6502 cycle.
//   - The `clk` line is a Verilator/FPGA settling clock -- a simulation
//     artifact, NOT a real 6502 pin. The real master clock is `phi`.
//
// Bus servicing (i.e. presenting read data, capturing write data) lives
// in Cpu, not the shim. The shim is just the netlist plus its
// pin-level inputs/outputs.

#pragma once

#include "test3/shim.h"

class Vchip_6502;

namespace test3 {

class AholmeShim : public ICpuShim {
public:
    AholmeShim();
    ~AholmeShim() override;

    void set_reset(bool asserted) override;
    void power_up() override;
    void step() override;
    BusState sample() const override;
    void drive(uint8_t data_in,
               bool irq_n, bool nmi_n,
               bool rdy,  bool so_n) override;
    Regs read_registers() const override;
    uint32_t trace_fields() const override { return TF_ALL; }
    CpuVariant variant() const override { return CV_NMOS_6502; }
    const char* name() const override { return "aholme"; }
    ResetSequence reset_sequence() const override;

private:
    // Number of `clk` toggles per phi half. Implementation detail of
    // the transistor-level netlist's dynamic-logic settling -- not
    // exposed in the ICpuShim contract.
    static constexpr int SETTLE_TICKS_PER_HALF = 8;

    Vchip_6502* top_       = nullptr;
    bool        phi_high_  = false; // current phi level
};

} // namespace test3
