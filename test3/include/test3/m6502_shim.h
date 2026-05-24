// test3/include/test3/m6502_shim.h
//
// ICpuShim wrapper around our own RTL 6502 core (rtl/cpu_6502.sv,
// verilated as Vcpu_6502 in build/m6502/). The RTL is plain synchronous
// SystemVerilog -- no settling clock dance -- so step() just toggles
// i_clk and evaluates.
//
// Clock model:
//   - The top-level pin is i_clk; the core derives o_phi2 = i_clk and
//     o_phi1 = ~i_clk. One full i_clk cycle == one 6502 cycle.
//   - step() advances by one phi half = one i_clk edge. Two step()s ==
//     one 6502 cycle.
//
// Internal register access uses --public-flat-rw (set in the Makefile)
// to expose register_acc / register_x / register_y / register_sp /
// program_counter / status_* via Verilator's rootp struct.

#pragma once

#include "test3/shim.h"

class Vcpu_6502;

namespace test3 {

class M6502Shim : public ICpuShim {
public:
    M6502Shim();
    ~M6502Shim() override;

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
    const char* name() const override { return "m6502"; }
    ResetSequence reset_sequence() const override;

private:
    Vcpu_6502* top_      = nullptr;
    bool       phi_high_ = false;   // true == clk high (phi2), false == clk low (phi1)
};

} // namespace test3
