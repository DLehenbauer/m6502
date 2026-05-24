// test3/src/m6502_shim.cpp

#include "test3/m6502_shim.h"
#include "test3/trace.h"
#include "Vcpu_6502.h"
#include "Vcpu_6502___024root.h"

namespace test3 {

// Canonical reset transition table for the m6502 RTL. Two rows per
// 6502 cycle (end-of-phi1, end-of-phi2). Cycles 0..7 are with reset
// asserted (i_reset_n low); cycles 8..15 are after release, including
// the INIT state machine (cpu_6502.sv INIT_CYCLES=6) and the
// reset-vector fetches at $FFFC/$FFFD that conclude reset.
//
// The data bytes for the vector-fetch rows (end-3, end-2, end-1) are
// placeholders -- Cpu::reset() patches them from the loaded memory
// image before comparison (see the ResetSequence doc in shim.h).
//
// Captured against rtl/cpu_6502.sv via Verilator 5.044. Notable
// differences from the aholme transistor-level reset trace:
//   - Hold sequence keeps addr at $0000 (aholme cycles through
//     $00A8/$0000/$00FF). The RTL drives a write of $00 in the very
//     first half-cycle (a property of the synchronous register-clear
//     reset behavior, not visible on real silicon).
//   - Post-reset SP = $00 (aholme: $FD). The RTL does not perform the
//     three implicit SP decrements that the real 6502's microcode runs
//     during reset; SP just retains whatever value the reset clears
//     it to ($00 here).
//   - Post-reset A = $00 (aholme: $66). A is not architecturally
//     defined after reset on a real 6502; aholme exposes the random
//     transistor settling value, the RTL just initializes to zero.
//   - I flag is set at the vector fetch (cycle 14.5) rather than
//     earlier in the reset sequence.
static const TraceRow kCanonicalResetTrace[] = {
    // --- Reset held (8 cycles) ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*    0.0 */ 0x0000, 0x00,  W,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    0.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    1.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    1.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    2.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    2.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    3.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    3.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    4.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    4.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    5.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    5.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    6.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    6.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    7.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    7.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    // --- Reset released; INIT_CYCLES=6 cycles ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*    8.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    8.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    9.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    9.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   10.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   10.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   11.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   11.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   12.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   12.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   13.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   13.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    { /*   14.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100000 },
    // --- Vector fetches ($FFFC then $FFFD); data bytes patched by Cpu::reset() ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*   14.5 */ 0xFFFC, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100100 },  // data <- mem[$FFFC]
    { /*   15.0 */ 0xFFFC, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100100 },  // data <- mem[$FFFC] (carry)
    { /*   15.5 */ 0xFFFD, 0x00,  R,    0, 0x0000, 0x00, 0x00, 0x00, 0x00, 0b00100100 },  // data <- mem[$FFFD]
};

ResetSequence M6502Shim::reset_sequence() const {
    return ResetSequence{
        /*hold_cycles=*/         8,
        /*post_release_cycles=*/ 8,
        /*trace_begin=*/         &kCanonicalResetTrace[0],
        /*trace_end=*/           &kCanonicalResetTrace[
            sizeof(kCanonicalResetTrace) / sizeof(kCanonicalResetTrace[0])],
    };
}

M6502Shim::M6502Shim() {
    power_up();
}

M6502Shim::~M6502Shim() {
    delete top_;
}

void M6502Shim::power_up() {
    delete top_;
    top_      = new Vcpu_6502;
    phi_high_ = false;

    top_->i_clk     = 0;
    top_->i_reset_n = 0;   // active-low; held asserted
    top_->i_rdy     = 1;
    top_->i_nmi_n   = 1;
    top_->i_irq_n   = 1;
    top_->i_so_n    = 1;
    top_->i_bus_data = 0;
    top_->i_debug_sel = 0;
    top_->eval();
}

void M6502Shim::step() {
    // One phi half-period == one i_clk toggle. phi_high_ tracks the
    // half that just ended.
    phi_high_ = !phi_high_;
    top_->i_clk = phi_high_ ? 1 : 0;
    top_->eval();
}

void M6502Shim::set_reset(bool asserted) {
    top_->i_reset_n = asserted ? 0 : 1;
    top_->eval();
}

BusState M6502Shim::sample() const {
    BusState b{};
    b.addr = top_->o_bus_addr;
    b.data = top_->o_bus_data;
    b.rw   = (top_->o_rw != 0);
    b.sync = (top_->o_sync != 0);
    b.phi  = phi_high_;
    return b;
}

void M6502Shim::drive(uint8_t data_in,
                      bool irq_n, bool nmi_n,
                      bool rdy,  bool so_n) {
    top_->i_bus_data = data_in;
    top_->i_irq_n    = irq_n ? 1 : 0;
    top_->i_nmi_n    = nmi_n ? 1 : 0;
    top_->i_rdy      = rdy   ? 1 : 0;
    top_->i_so_n     = so_n  ? 1 : 0;
    top_->eval();
}

Regs M6502Shim::read_registers() const {
    const auto& r = *top_->rootp;
    Regs out{};
    out.a  = r.cpu_6502__DOT__register_acc;
    out.x  = r.cpu_6502__DOT__register_x;
    out.y  = r.cpu_6502__DOT__register_y;
    out.s  = r.cpu_6502__DOT__register_sp;
    out.pc = r.cpu_6502__DOT__program_counter;
    out.p  = static_cast<uint8_t>(
        (r.cpu_6502__DOT__status_negative  << 7) |
        (r.cpu_6502__DOT__status_overflow  << 6) |
        (                              1u  << 5) |
        (                              0u  << 4) |   // B not latched architecturally
        (r.cpu_6502__DOT__status_decimal   << 3) |
        (r.cpu_6502__DOT__status_interrupt << 2) |
        (r.cpu_6502__DOT__status_zero      << 1) |
        (r.cpu_6502__DOT__status_carry));
    return out;
}

} // namespace test3
