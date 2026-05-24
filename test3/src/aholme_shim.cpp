// test3/src/aholme_shim.cpp

#include "test3/aholme_shim.h"
#include "test3/trace.h"
#include "Vchip_6502.h"
#include "Vchip_6502___024root.h"

#include <utility>

namespace test3 {

// ---------------------------------------------------------------------------
// Visual 6502 node numbers for the architectural registers.
//
// Source of truth:
//   https://github.com/trebonian/visual6502/blob/master/nodenames.js
//
// The aholme netlist (chip_6502_nodes.inc) uses the same numbering -- only
// a subset of the node names is `define`d there for Verilog-side use; the
// rest live here on the C++ side because we read them via Verilator's
// generated public storage of the node-state array `q[]`, not via Verilog
// port outputs.
//
// PCL/PCH use the primary `pcl*` / `pch*` storage nodes (not the
// pre-incremented `pclp*` / `pchp*` copies the CPU uses to compute PC+1
// ahead of time).
namespace {

constexpr int kA[8]   = {  737, 1234,  978,  162,  727,  858, 1136, 1653 };
constexpr int kX[8]   = { 1216,   98,    1, 1648,   85,  589,  448,  777 };
constexpr int kY[8]   = {   64, 1148,  573,  305,  989,  615,  115,  843 };
constexpr int kS[8]   = { 1403,  183,   81, 1532, 1702, 1098, 1212, 1435 };
constexpr int kPCL[8] = { 1139, 1022,  655, 1359,  900,  622,  377, 1611 };
constexpr int kPCH[8] = { 1670,  292,  502,  584,  948,   49, 1551,  205 };

// Status-register storage nodes. p0..p3, p6, p7 are real flip-flops in
// the silicon. p5 has no flip-flop (we force bit 5 to 1, matching the
// standard 6502 convention where P always reads with bit 5 set). p4 is
// not a real architectural B flag either -- visual6502 exposes a node at
// 1119 that the netlist drives during the push-P sequence to encode
// whether the entry was BRK/PHP (B=1) vs IRQ/NMI (B=0). Reading it here
// matches what visual6502 displays, so the trace stays comparable to
// visual6502 captures; treat it as "the bit that will be pushed" rather
// than as a latched architectural flag.
constexpr int kP0 =   32;  // C
constexpr int kP1 =  627;  // Z
constexpr int kP2 = 1553;  // I
constexpr int kP3 =  348;  // D
constexpr int kP4 = 1119;  // B (visual6502 synthetic push-status node)
constexpr int kP6 = 1625;  // V
constexpr int kP7 =   69;  // N

// chip_6502.v has `reg [NUM_NODES-1:0] q;` with NUM_NODES=1725, which
// Verilator packs into a VlWide<ceil(1725/32)> = VlWide<54>. Catch any
// mismatch at compile time so a future upstream change can't silently
// drift the storage we're reading from.
static_assert(decltype(std::declval<Vchip_6502___024root>()
                           .chip_6502__DOT__q)::Words == 54,
              "aholme chip_6502.q expected to be 1725 bits (54 EData words)");

inline uint8_t node_bit(const VlWide<54>& q, int node) {
    return static_cast<uint8_t>((q.at(node >> 5) >> (node & 31)) & 1u);
}

inline uint8_t pack_byte(const VlWide<54>& q, const int (&nodes)[8]) {
    uint8_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint8_t>(node_bit(q, nodes[i]) << i);
    return v;
}

} // namespace

// Canonical reset transition table for the aholme transistor-level
// core. Two rows per 6502 cycle (end-of-phi1, end-of-phi2). Cycles
// 0..15 are with reset asserted (`res` low); cycles 16..21 are after
// release before the first reset-vector fetch; cycles 22..23 are
// the $FFFC/$FFFD vector fetches that conclude reset. Cpu::reset()
// vets the actual captured trace against this table on every run.
//
// The data bytes for the vector-fetch rows (rows 45, 46, 47) are
// placeholders -- Cpu::reset() patches them from the loaded memory
// image before comparison. See the ResetSequence doc in shim.h.
//
// Captured from the aholme netlist (Verilator 5.044). To re-baseline
// after a deliberate change, copy the paste-ready block emitted by a
// failing reset() call into this array.
static const TraceRow kCanonicalResetTrace[] = {
    // --- Reset held (16 cycles) ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*    0.0 */ 0x00A8, 0x00,  R,    1, 0x00A8, 0x0A, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    0.5 */ 0x00A8, 0x00,  W,    1, 0x00A8, 0x0A, 0x00, 0x00, 0x00, 0b00100000 },
    { /*    1.0 */ 0x00A8, 0x00,  R,    0, 0x00A8, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    1.5 */ 0x00A8, 0x00,  R,    0, 0x00A8, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    2.0 */ 0x0000, 0x00,  R,    0, 0x0000, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    2.5 */ 0x0000, 0x00,  R,    0, 0x0000, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    3.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    3.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    4.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    4.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    5.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    5.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    6.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    6.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    7.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    7.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    8.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    8.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    9.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*    9.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   10.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   10.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   11.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   11.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   12.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   12.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   13.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   13.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   14.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   14.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   15.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   15.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    // --- Reset released; CPU's internal post-reset state machine ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*   16.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   16.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   17.0 */ 0x00FF, 0x00,  R,    1, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   17.5 */ 0x00FF, 0x00,  R,    1, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   18.0 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   18.5 */ 0x00FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   19.0 */ 0x0100, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   19.5 */ 0x0100, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   20.0 */ 0x01FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   20.5 */ 0x01FF, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   21.0 */ 0x01FE, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    { /*   21.5 */ 0x01FE, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0x00, 0b00100010 },
    // --- Vector fetches ($FFFC then $FFFD); data bytes patched by Cpu::reset() ---
    //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
    { /*   22.0 */ 0xFFFC, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // stale-carry from cycle 21 (data=mem[$01FE])
    { /*   22.5 */ 0xFFFC, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0xFD, 0b00100010 },  // data <- mem[$FFFC]
    { /*   23.0 */ 0xFFFD, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // data <- mem[$FFFC] (carry)
    { /*   23.5 */ 0xFFFD, 0x00,  R,    0, 0x00FF, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // data <- mem[$FFFD]
};

ResetSequence AholmeShim::reset_sequence() const {
    return ResetSequence{
        /*hold_cycles=*/         16,
        /*post_release_cycles=*/ 8,
        /*trace_begin=*/         &kCanonicalResetTrace[0],
        /*trace_end=*/           &kCanonicalResetTrace[
            sizeof(kCanonicalResetTrace) / sizeof(kCanonicalResetTrace[0])],
    };
}

AholmeShim::AholmeShim() {
    power_up();
}

AholmeShim::~AholmeShim() {
    delete top_;
}

void AholmeShim::power_up() {
    // Recreate the netlist so every q[] storage node returns to its
    // post-construction state. Cheaper than tracking which nodes
    // matter and zeroing them by hand; matches what the canonical
    // reset trace was captured against.
    delete top_;
    top_      = new Vchip_6502;
    phi_high_ = false;

    // All inputs at known idle.
    top_->clk = 0;
    top_->phi = 0;
    top_->res = 0;   // active-low; held low to start
    top_->so  = 1;   // not asserted
    top_->rdy = 1;
    top_->nmi = 1;   // not asserted
    top_->irq = 1;   // not asserted
    top_->dbi = 0;
    top_->eval();
}

void AholmeShim::step() {
    // Advance to the next phi edge. Run SETTLE_TICKS_PER_HALF `clk`
    // toggles to let the netlist's dynamic logic propagate, then flip
    // the `phi` input so the caller observes the just-ended half-cycle
    // before draining into the next one.
    for (int i = 0; i < SETTLE_TICKS_PER_HALF; ++i) {
        top_->clk = 1; top_->eval();
        top_->clk = 0; top_->eval();
    }

    phi_high_ = !phi_high_;
    top_->phi = phi_high_ ? 1 : 0;
    top_->eval();
}

void AholmeShim::set_reset(bool asserted) {
    // `res` is the active-low reset pin: low when reset is asserted.
    top_->res = asserted ? 0 : 1;
    top_->eval();
}

BusState AholmeShim::sample() const {
    BusState b{};
    b.addr = top_->ab;
    b.data = top_->dbo;
    b.rw   = (top_->rw != 0);
    b.sync = (top_->sync != 0);
    b.phi  = phi_high_;
    return b;
}

void AholmeShim::drive(uint8_t data_in,
                       bool irq_n, bool nmi_n,
                       bool rdy,  bool so_n) {
    top_->dbi = data_in;
    top_->irq = irq_n ? 1 : 0;
    top_->nmi = nmi_n ? 1 : 0;
    top_->rdy = rdy   ? 1 : 0;
    top_->so  = so_n  ? 1 : 0;
    top_->eval();
}

Regs AholmeShim::read_registers() const {
    // Read from `q` -- the latched node-state array updated on every
    // `posedge clk` (Verilator settling clock). After step() returns,
    // `q` holds the most recently settled values; subsequent phi
    // transitions don't disturb it until the next step()'s settle loop.
    const auto& q = top_->rootp->chip_6502__DOT__q;

    Regs r{};
    r.a = pack_byte(q, kA);
    r.x = pack_byte(q, kX);
    r.y = pack_byte(q, kY);
    r.s = pack_byte(q, kS);

    const uint8_t pcl = pack_byte(q, kPCL);
    const uint8_t pch = pack_byte(q, kPCH);
    r.pc = static_cast<uint16_t>(pch) << 8 | pcl;

    // Compose P with standard 6502 layout (bit 5 always 1; bit 4 is the
    // visual6502 push-status node -- see kP4 comment above).
    r.p = static_cast<uint8_t>(
        (node_bit(q, kP7) << 7) |
        (node_bit(q, kP6) << 6) |
        (             1u  << 5) |
        (node_bit(q, kP4) << 4) |
        (node_bit(q, kP3) << 3) |
        (node_bit(q, kP2) << 2) |
        (node_bit(q, kP1) << 1) |
        (node_bit(q, kP0)));

    return r;
}

} // namespace test3
