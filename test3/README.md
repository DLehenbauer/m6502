# test3/ -- half-cycle-accurate Verilator test framework

Goal: an exhaustive, half-cycle-accurate 6502 test suite that can validate
any candidate core. Tests are self-contained C++: code is assembled inline
via a fluent `Program` builder with addressing-mode operand types
(`prog.lda(Imm{0x42}).sta(ZP{0x80}).brk()`), and the expected bus-state
trace lives in the source as a per-half-cycle table.

**Status: Phase 1 + trace-granularity refactor + aholme register
peeking.** A single smoke test passes end-to-end against Andrew
Holme's transistor-level core (`Vchip_6502`, sources vendored under
`test2/aholme/`). The framework, build, and authoring workflow are
proven, and per-half-cycle traces now include the aholme core's
A/X/Y/S/P/PC. Subsequent phases will add broader opcode coverage,
parallel test execution, the m6502 shim, etc.

## Build & run

```
make -C test3            # build
make -C test3 run        # build + run all tests
make -C test3 run ARGS="--filter=smoke"
make -C test3 run ARGS="--list"
make -C test3 clean
```

Top-level shortcut:

```
make test-test3
```

## Authoring workflow

The framework uses an in-source "expected trace" table. Workflow:

1. Write a `TEST(name) { ... }` body. Each test receives a `Cpu& cpu`
   parameter (the driver constructs the shim+Cpu once and passes it
   into every test, keeping tests shim-agnostic). Assemble your
   program (terminate it with `prog.pass(...)` reachable from the
   normal exit path), then:
   ```cpp
   cpu.load(prog);
   EXPECT_PASS(expected);   // expected[] starts empty
   ```
2. Run it: the test FAILS and prints a paste-ready block of the actual
   per-half-cycle trace to stderr. Each row is annotated with its
   cycle number and phase (`end-of-phi1` / `end-of-phi2`).
3. Copy that block into your test source, replacing `expected[]`.
4. Re-run: the test now PASSES.

`EXPECT_PASS(expected)` is a one-liner that runs the CPU, asserts the
run terminated via `prog.pass()` (not `prog.fail()` / max_cycles),
and matches the captured trace against `expected[]`. The Program
installed by the preceding `cpu.load(prog)` is threaded through via
`cpu.program()` so trace-diff failures still annotate the offending
byte with its emitting source line.

If the underlying core's behavior changes later, the comparison will
fail and surface the diff (plus the new full actual table).

See `tests/smoke_test.cpp` for a worked example.

## Granularity: why per-half-cycle (and not finer)

The runner steps the aholme netlist at **1/8 phi resolution** (16 steps
per 6502 cycle) so the ~1700-node netlist has enough iterations to
settle every half-cycle. That's a **simulation** detail.

For **comparison / logging** we sample twice per 6502 cycle, at the two
externally-stable observation windows defined by the NMOS 6502
datasheet:

- **End of phi1** (just before posedge phi2): `ab` / `rw` / `sync` for
  this cycle are established; the data bus shows the previous cycle's
  residual or in-transition value.
- **End of phi2** (just before negedge phi2): everything for this cycle
  is fully settled -- `ab` / `rw` / `sync` long-stable, data finalized
  (read presented or write driven).

This is the **maximum granularity at which aholme can be
deterministically compared to a physical NMOS 6502** (e.g. a
logic-analyzer capture). Going finer captures noise:

| Reference | State at sub-half-cycle sample point |
|---|---|
| aholme | mid-settling of netlist `eval()` -- Verilator-scheduler artifact |
| physical NMOS 6502 | analog propagation in progress -- per-chip / temp / voltage |
| m6502 RTL (future) | identical to phi-edge (purely synchronous on phi2) |

### Datasheet anchors

The two stable windows correspond to the NMOS 6502 datasheet's
guaranteed-valid intervals (1 MHz part shown):

| Signal | Validity window |
|---|---|
| `ab` (address)         | from t_ADS (<=225 ns) after posedge phi2 through t_HA (>=30 ns) after negedge phi2 |
| `rw`                   | same as `ab` |
| `sync`                 | same as `ab` |
| `dbo` (write data)     | from t_MDS (<=200 ns) after posedge phi2 to negedge phi2 |
| `dbi` (read data)      | must be valid t_DSR (>=50 ns) before negedge phi2 |

The phi edges themselves are transition points (50-225 ns of analog
propagation); sampling there would risk catching mid-transition values
that differ between chips.

## Layout

```
test3/
  Makefile                 # Verilator + C++ build
  include/test3/
    program.h              # Program (64 KiB byte emitter) + fluent opcodes (lda/sta/jmp/...) + Label / at() / data helpers
    shim.h                 # ICpuShim interface + BusState/Regs
    aholme_shim.h          # AholmeShim (Vchip_6502 wrapper)
    trace.h                # TraceRow, TraceLogger, comparator, paste printer
    cpu.h                  # Cpu (load/reset/terminator/run/log)
    test.h                 # TEST() macro + registry
  src/
    aholme_shim.cpp
    trace.cpp
    cpu.cpp
    main.cpp               # CLI parsing + serial test driver
  tests/
    smoke_test.cpp           # First end-to-end test (kept as tutorial)
    isa/                     # Per-instruction tests (loads, stores, branches, ...)
      nmos_only/             #   Undocumented NMOS opcodes (LAX, SAX, ...)
    corners/                 # Cross-cutting behaviors (decimal mode, page
      interrupts/            #   crossing, RMW timing, IRQ/NMI/BRK timing)
```

## Suite organization

Tests are organized along two axes:

1. **Primary: instruction family.** Files under `tests/isa/` group
   closely related mnemonics (`loads_test.cpp` has LDA/LDX/LDY across
   all addressing modes; `branches_test.cpp` has all eight Bxx
   conditional branches; etc). Mirrors how 6502 reference docs and
   `Program`'s API are organized.

2. **Secondary: cross-cutting behavioral corners.** Behaviors that
   span many opcodes (decimal-mode ADC/SBC, +1-cycle page-crossing
   penalty, RMW bus pattern, interrupt timing) live in
   `tests/corners/` instead of being smeared across per-instruction
   files.

### TEST() naming

Inside a `*_test.cpp` file, write multiple `TEST()` cases, one per
(opcode, addressing mode, behavioral corner) tuple:

```
<mnemonic>_<addressing-mode>[_<corner>]
```

Examples: `lda_imm`, `lda_absx_no_cross`, `lda_absx_page_cross`,
`jmp_ind_at_page_boundary`, `brk_simple`.

### Multi-variant support (infrastructure for later)

The driver loops over all available shims (today only `aholme`,
NMOS 6502) and runs every test against each. Shims declare their
variant via `ICpuShim::variant()` returning a `CpuVariant`
(`CV_NMOS_6502`, `CV_6510`, `CV_6509`, `CV_CMOS_65C02`); convenience
mask `CV_NMOS_LIKE` covers the three NMOS-family variants.

The framework includes `SKIP_UNLESS(variant_mask, "reason")` so
that tests added for variant-only opcodes or behaviors can be
skipped cleanly when the loaded shim doesn't match. The driver
reports SKIPs separately from PASS/FAIL so missed coverage is
visible rather than silently green. Summary line: `N run across
M shim(s), P passed, F failed, S skipped`.

When a CMOS shim (or other variant) is added later, the established
patterns are:

- **Common opcode, identical behavior** -- one TEST(), no variant
  check. Stays in `tests/isa/<group>_test.cpp`.
- **Variant-only opcode** (STZ on 65C02; undocumented NMOS opcodes)
  -- TEST() in a variant-specific subdir (e.g. `isa/cmos_only/` --
  not present today; will be added when needed) with
  `SKIP_UNLESS(mask, ...)` at the top.
- **Same opcode, divergent expected[]** (JMP-ind wrap, decimal flags,
  RMW timing, BRK+NMI hijack) -- one TEST() with `cpu.variant()`
  branching between two `static const TraceRow expected[]` tables,
  each followed by its own `EXPECT_PASS(expected);` (`sizeof / sizeof`
  needs a C array, not a pointer). Today these tests just assert the
  NMOS/aholme behavior.

## Trace row schema

```cpp
struct TraceRow {
    uint16_t addr;        // ab pin
    uint8_t  data;        // bidirectional bus: dbi for reads, dbo for writes
    uint8_t  rw;          // R or W (lowercase r/w in source for readability)
    uint8_t  sync;        // 0 or 1
    uint16_t pc;
    uint8_t  a, x, y, s, p;
};
```

A row's *array index* IS its half-cycle index (cycle = `index/2`,
phase = phi1 if `index%2==0` else phi2). The paste-block printer
emits each row with a leading `/* N.X */` comment decoding that
index as cycle `N`, phase `0`=phi1 / `5`=phi2 for readability.

Which trace columns are meaningful is a property of the shim, not of
individual rows. `ICpuShim::trace_fields()` returns a bitmask (see
`TraceFields` in `shim.h`); the comparator skips columns absent from
the mask. The aholme shim returns `TF_ALL` because it decodes
A/X/Y/S/P/PC by reading visual6502 storage nodes directly out of the
Verilator-generated node-state array. A future real-silicon-capture
shim that can only observe external pins would clear `TF_REGS`, and
the same `expected[]` tables would still match on bus state alone.
Tests pass the mask via `runner.trace_fields()` to the EXPECT_TRACE
macros.

Sampling happens at end-of-phi1 and end-of-phi2, so register values
typically appear unchanged across both halves of a cycle and update
at the half-cycle granularity the netlist commits.

The `P` byte uses the standard 6502 layout (`NV-BDIZC`): bit 5 is
forced to 1 (there is no bit-5 flip-flop in the silicon), and bit 4
reads from visual6502's synthetic "B" node (1119) -- that node tracks
what the netlist will push as the B bit during a PHP/BRK/IRQ/NMI
sequence, not a true latched architectural flag. Trace tables print
`P` as a C++14 binary literal (e.g. `0b00100100`) so the individual
flag bits are readable inline at their natural positions.

## Bus-data column semantics

The `data` column always reflects what is on the bidirectional bus at
the sample point:

- **Reads**: `data` = the byte the runner has presented on `dbi`. At
  end-of-phi1 this is typically the previous cycle's value (since
  presentation happens at posedge phi2). At end-of-phi2 it is the byte
  the CPU latched.
- **Writes**: `data` = `dbo` from the CPU. At end-of-phi1 it may be
  stale (CPU hasn't yet driven); at end-of-phi2 it is the byte being
  written.

This matches what a logic analyzer would record on a real bus.

## Deferred to later phases

- `M6502Shim` (wraps `Vcpu_6502`) + `--core=` switch + cross-core diff.
  Register access via the m6502 debug port.
- `fork()`-based `--jobs=N` parallel test execution.
- Full ISA opcode coverage; per-addressing-mode test files.
- Build-time codegen for exhaustive opcode/operand/flag sweeps.
- Transition-order (not position-based) comparison mode, useful for
  cross-validation against real-silicon LA captures at sub-half-cycle
  resolution.
