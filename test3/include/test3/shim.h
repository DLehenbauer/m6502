// test3/include/test3/shim.h
//
// The ICpuShim interface abstracts over different 6502 cores so a test
// can be written once and (in a future phase) run against either the
// aholme transistor-level netlist (Vchip_6502) or our own m6502 RTL
// (Vcpu_6502). In Phase 1 only AholmeShim exists.
//
// Step granularity: one shim->step() advances the simulation to the
// next phi edge -- i.e. by one phi half-period. Two step()s == one full
// 6502 cycle. After step() returns, sample() reflects the half-cycle
// that just ended; the runner then services the bus (presents read
// data or commits write data via drive()) before calling step() for
// the next half. How an edge is manufactured (settling iterations,
// internal clock toggling, etc.) is the shim's private business.

#pragma once

#include <cstdint>

namespace test3 {

// Forward declaration so ResetSequence can carry TraceRow pointers
// without dragging trace.h into shim.h (which would be circular).
struct TraceRow;

// A shim's canonical reset transition table, used by Cpu::reset()
// to vet that the core's reset sequence matches the known-good behavior.
//
// `trace_begin` / `trace_end` describe a contiguous array of
// `(hold_cycles + post_release_cycles) * 2` rows produced by:
//   1. asserting reset and running `hold_cycles` 6502 cycles, then
//   2. deasserting reset and running `post_release_cycles` 6502 cycles.
//
// The table covers the ENTIRE reset sequence, including the two
// reset-vector fetches at $FFFC then $FFFD that conclude reset on
// the 6502. The data bytes for those fetches are placeholders in the
// table; Cpu::reset() rewrites the last three data bytes from the
// loaded memory image before comparison:
//   rows[end-3].data <- mem[$FFFC]   (end-of-phi2 of $FFFC fetch)
//   rows[end-2].data <- mem[$FFFC]   (end-of-phi1 of $FFFD fetch, carry)
//   rows[end-1].data <- mem[$FFFD]   (end-of-phi2 of $FFFD fetch)
// rows[end-4] is the end-of-phi1 of the $FFFC fetch and carries the
// stale data byte from the prior cycle's read; the canonical table
// hardcodes that (the canonical reset reads $0000/$00FF/$00A8/$0100/
// $01FF/$01FE during its sequence -- if your program populates those
// addresses with non-zero values, you'll need to update the table).
struct ResetSequence {
    int             hold_cycles;
    int             post_release_cycles;
    const TraceRow* trace_begin;
    const TraceRow* trace_end;
};

// External CPU pins visible at the bus (sampled every step()).
struct BusState {
    uint16_t addr;   // ab
    uint8_t  data;   // dbo (only meaningful on writes; we still sample it always)
    bool     rw;     // 1 = read, 0 = write
    bool     sync;   // opcode-fetch indicator
    bool     phi;    // current half (false = phi1/low, true = phi2/high)
};

// 6502 architectural registers. Sampled by the runner at each phi edge
// (end-of-phi1 and end-of-phi2). For the aholme transistor-level core
// these are decoded directly from the visual6502 storage nodes; for the
// m6502 RTL core (future) they'll come from the synchronous debug port.
// A shim that cannot expose registers (e.g. a real-silicon logic-
// analyzer capture) clears the TF_REGS bit in its trace_fields() mask
// so the comparator skips the A/X/Y/S/P/PC columns. There is no
// per-row validity flag.
struct Regs {
    uint8_t  a;
    uint8_t  x;
    uint8_t  y;
    uint8_t  s;
    uint8_t  p;
    uint16_t pc;
};

// Bitmask describing which TraceRow columns a shim populates. The
// comparator filters its per-row comparison by ANDing the expected
// mask with the actual mask; columns absent from either side are
// not compared. Bus columns (addr/data/rw/sync) are always present,
// so they don't get their own bit -- they're implicit. Add bits as
// new optional column groups appear (e.g. cycle-stretch state from
// real-silicon captures).
enum TraceFields : uint32_t {
    TF_REGS = 1u << 0,   // a, x, y, s, p, pc
    TF_ALL  = TF_REGS,
};

// 6502 variant identifier. Bitmask so tests can match against a
// family of variants (e.g. CV_NMOS_LIKE for any test that exercises
// undocumented-NMOS or NMOS-specific bugs that 6510/6509 also have).
// Used by SKIP_UNLESS() in tests and by branches on cpu.variant()
// when the same opcode has divergent behavior across variants
// (decimal-mode flags, JMP-ind wrap, RMW timing, BRK+NMI hijack).
enum CpuVariant : uint32_t {
    CV_NMOS_6502  = 1u << 0,
    CV_6510       = 1u << 1,   // NMOS + on-chip I/O port at $00/$01
    CV_6509       = 1u << 2,   // NMOS + bank-register at $00/$01
    CV_CMOS_65C02 = 1u << 3,
    // Convenience masks.
    CV_NMOS_LIKE  = CV_NMOS_6502 | CV_6510 | CV_6509,
    CV_ALL        = CV_NMOS_LIKE | CV_CMOS_65C02,
};

class ICpuShim {
public:
    virtual ~ICpuShim() = default;

    // Hold or release reset. When `asserted` is true, the active-low
    // reset pin goes LOW (CPU is in reset). When false, the pin goes
    // HIGH (normal operation). The runner is responsible for stepping
    // the clock during reset hold so the netlist can warm up.
    virtual void set_reset(bool asserted) = 0;

    // Restore the shim to its constructor-fresh "just powered on"
    // state: clear all internal storage (netlist nodes, RTL registers,
    // logic-analyzer cursor, etc.) and re-idle every input pin. The
    // canonical reset trace was captured from this state; calling
    // reset() without first calling power_up() on a shim that has
    // already been run will see stale internal state and diverge from
    // the canonical table. The test driver calls this before every
    // test so individual tests don't have to.
    virtual void power_up() = 0;

    // Advance the simulation to the next phi edge. On return, the bus
    // outputs are valid for the half-cycle that just ended; the caller
    // should sample(), then drive() inputs for the next half, then
    // call step() again. Two step()s == one 6502 cycle.
    virtual void step() = 0;

    // Sample current externally-visible bus state.
    virtual BusState sample() const = 0;

    // Drive the CPU inputs: data_in on the data-in bus + the level-
    // sensitive control pins (active-low). Called by the runner once
    // per phi half (at the appropriate edge); also persists between
    // step() calls.
    virtual void drive(uint8_t data_in,
                       bool irq_n, bool nmi_n,
                       bool rdy,  bool so_n) = 0;

    // Read architectural register state. Should only be called at phi
    // edges. AholmeShim decodes them from the visual6502 storage nodes
    // exposed by Verilator on the generated model. Shims that don't
    // expose registers should return zeroed Regs (and clear TF_REGS in
    // trace_fields()).
    virtual Regs read_registers() const = 0;

    // Bitmask of optional TraceRow columns this shim populates. Used
    // by the trace comparator to skip columns the shim can't provide.
    virtual uint32_t trace_fields() const = 0;

    // Which 6502 variant this shim implements. Used by SKIP_UNLESS()
    // in tests and by cpu.variant() branches inside tests for
    // opcodes whose behavior differs across variants. Returns a
    // single-bit value (e.g. CV_NMOS_6502); tests compare against
    // bitmasks (e.g. CV_NMOS_LIKE) via bitwise AND.
    virtual CpuVariant variant() const = 0;

    // Identifier for diagnostics (e.g. "aholme", "m6502").
    virtual const char* name() const = 0;

    // The shim's canonical reset transition table. Cpu::reset()
    // calls this once per run to retrieve the expected behavior and
    // verifies the actual captured trace against it.
    virtual ResetSequence reset_sequence() const = 0;
};

} // namespace test3
