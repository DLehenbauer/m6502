---
name: test3-tests
description: Author and update half-cycle accurate 6502 tests under test3/tests/. Use when adding new TEST() cases under test3/tests/isa/ or tests/corners/, refreshing expected[] trace tables, or building 6502 program images via the Program fluent API.
---

# Writing test3 tests

Tests live under `test3/tests/`. Each test asserts cycle-accurate bus behavior
by comparing a captured trace against an embedded `expected[]` table. The
framework supplies the program builder, the cycle driver, the trace logger,
and the comparator; tests just describe the program and the expected trace.

## Test scaffold

```cpp
// test3/tests/isa/<group>_test.cpp
#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;            // file scope, not per-TEST

TEST(<mnemonic>_<mode>[_<corner>]) {
    Program prog = Program::start()
        ...;

    cpu.load(prog);

    static const TraceRow expected[] = {
        // (start empty; capture and paste after first run)
    };

    EXPECT_PASS(expected);
}
```

`TEST(name)` declares a function taking `Cpu& cpu`. The driver constructs
one `Cpu` per shim, calls `cpu.power_up()` before every test (so the shim
returns to constructor-fresh state), and passes the same `cpu` into each
test body. Tests never call `cpu.power_up()` or `cpu.reset()` directly.

## Authoring workflow

1. Write the program with `Program::start()....`.
2. Leave `expected[]` empty and run. The test fails and prints a
   fully-annotated paste-ready trace between `BEGIN actual` / `END
   actual` markers: every SYNC boundary gets a section header naming
   the instruction (or a combined `PASS at $XXXX: ...` header for the
   framework's auto-inserted trap pair), and every row has a trailing
   `// SYNC: XYZ` / `// dummy / operand` / `// push ... -> stack` /
   `// vector fetch` / etc. comment.
3. Copy the block between those markers, replacing `expected[]`. No
   manual annotation needed.
4. Re-run; test passes.

### Batch paste-back (multiple tests at once)

When authoring many tests in parallel, the script `paste_expected.py`
(co-located with this skill) automates step 3. Workflow:

```sh
cd test3
make run > /tmp/test_run.log 2>&1   # captures every FAIL paste block
python3 .github/skills/test3-tests/paste_expected.py
make run                            # all tests now pass
```

The script scans the log for `FAIL: test3_test_<name>` blocks, parses
the `BEGIN actual` / `END actual` body, finds the matching `TEST(<name>)`
in any `test3/tests/**/*.cpp`, and replaces that test's empty
`static const TraceRow expected[] = { };` with the captured rows. Tests
that already pass are untouched.

Example trace block layout:

```cpp
// --- BRK opcode fetch + dummy fetch of signature byte at PC+1 ---
//    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
{ /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: BRK
{ /*    0.5 */ 0x0200, 0x00,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $00 = BRK
```

When the captured trace ends in a `pass()` / `fail()` trap (the
common case), the printer groups the auto-inserted landing-pad NOP
together with the JMP-self under one header and labels it `PASS` or
`FAIL` based on the terminator kind registered for the trap address:

```cpp
// --- PASS at $0201: NOP flushes pipelined registers, JMP-self at $0202 traps ---
//    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
{ /*    2.0 */ 0x0201, 0xEA,  R,    1, 0x0201, ... },  // SYNC: NOP
...
{ /*    4.0 */ 0x0202, 0x4C,  R,    1, 0x0202, ... },  // SYNC: JMP
```

`/* N.X */` is the cycle.phase prefix: `.0` is end-of-phi1, `.5` is
end-of-phi2.

## Program assembly style

Prefer fluent chaining when emitting instruction streams. Break the chain
and start a new (unindented) statement for **labels, org, and handler
declarations** -- they read like assembly labels:

```cpp
Program prog = Program::start()
    .brk()
    .fail("BRK pushed PC+1 instead of PC+2");

prog.brk_handler()                 // label-like, starts new statement
    .pass("BRK reached handler via $FFFE/$FFFF");
```

```cpp
Program prog = Program::start()
    .lda(Imm{0x42})
    .sta(ZP {0x80})
    .brk();

prog.brk_handler(0x0500)           // forward-declared handler
    .pass("BRK terminated cleanly at $0500");
```

Use default addresses whenever the test doesn't care about the specific
address. `Program::start()` puts the entry at `$0200` (the first RAM page
above the zero page and stack). The no-arg `brk_handler()` /
`nmi_handler()` / `reset_handler()` overloads vector to the current cursor
so the handler body chains directly:

```cpp
prog.org(0x0500).brk_handler().rti();
```

## Padding and sentinel bytes

When you need a placeholder byte (BRK signature, dummy data, fill values,
"unused" memory), prefer these in order:

1. **`$42`** -- preferred
2. `$02`
3. `$22`
4. `$62`

All four are JAM/KIL instructions on the NMOS 6502 (illegal opcodes that
lock the CPU), so a regression that mis-executes the byte fails loudly
instead of silently doing nothing. The ranking reflects how
unambiguously each value lands across CMOS/PCE/65CE02/65C816 variants --
`$42` is the most distinctive across those families, `$62` the least.

`brk()` already uses `$42` as its default 2-byte signature; pass a
specific value via `brk(Imm{0xNN})` only when the test cares about the
signature byte.

## Visible-register latency on aholme

The aholme shim decodes A/X/Y/S from visual6502 storage nodes, and
those nodes update on different timing than the status flags:

- **Status flag bits (N/V/Z/I/C/D)** become visible in the `P` column
  on the SYNC of the next instruction.
- **A/X/Y registers** become visible one cycle *later* than that --
  in the row at the *dummy-fetch* of the next instruction (not its
  SYNC).

`pass()` and `fail()` already handle this by emitting a landing-pad
NOP before the JMP-self. The NOP's dummy-fetch cycle is where any
A/X/Y update from the immediately-preceding instruction lands in the
trace. Tests do not need to add their own `.nop()` before
`.pass()` / `.fail()`:

```cpp
// Just works -- the X=$01 result lands in the trace on the auto-
// inserted NOP's dummy-fetch row.
Program prog = Program::start().inx().pass("INX done");
```

The returned `LabelOrProgramRef` points at the NOP entry, so the
landing pad remains a valid vector / branch target.

## Run terminators

End every run with a `pass()` or `fail()` pseudo-op so the driver knows
when to stop. Both emit a 3-byte `JMP self` at the cursor and register a
terminator at that address; `Cpu::run()` ends the run when an opcode
fetch (SYNC=1) hits it.

- `pass(msg)` -- successful termination. `RunResult.ok == true`.
- `fail(msg)` -- failure. Trips an `EXPECT(false, ...)` so the run shows
  up in pass/fail counts and `--fail-fast`.

### Prefer explicit traps over relying on trace diffs

The `expected[]` trace check catches almost any regression because the
cycle-by-cycle bus values diverge. But a trace mismatch reports as a
row-level diff ("row 4 expected addr=$01FD, got addr=$01FE"), which is
slow to diagnose. An explicit `pass()` / `fail()` trap at a meaningful
PC reports as a one-line message ("BRK pushed PC+1 instead of PC+2"),
which points straight at the failure mode.

Use traps as the **primary** assertion whenever the test boils down to
"control reaches this address" or "control must not reach this
address". The trace match then serves as the secondary
belt-and-suspenders check.

Common patterns:

```cpp
// Catch fall-through: BRK should vector away from $0201; if it
// doesn't, executing the byte at $0201 hits the fail trap.
Program prog = Program::start()
    .brk()
    .fail("BRK pushed PC+1 instead of PC+2");

// Assert "control reaches here": RTI from BRK handler should return
// to BRK_pc + 2.
prog.pass("returned from BRK via RTI");

// Verify a specific branch was taken: park pass() on the expected
// path and fail() on the wrong path.
prog.lda(Imm{0x00})
    .bne(taken_path)              // branch should NOT be taken (Z=1)
    .pass("BNE correctly not taken on Z=1");
prog.brk_handler(taken_path = prog.label())
    .fail("BNE incorrectly taken on Z=1");
```

When a trap can't fit at the natural fall-through PC (e.g. its 3-byte
JMP-self would clobber the next instruction needed by the test), note
that in a comment and let the trace check carry the assertion. This is
the exception, not the default.

`pass()` and `fail()` return a `LabelOrProgramRef` -- usable directly
as a vector target via implicit Label conversion, or chainable via the
forwarded emit ops.

## Handler setters

`brk_handler`, `nmi_handler`, `reset_handler` each have six overloads
(three argument forms x `&` / `&&` ref-qualifiers):

```cpp
prog.brk_handler(0x0500)              // raw addr: patches vector + moves cursor
    .rti();
prog.brk_handler(some_label)          // Label: patches vector, cursor unchanged
prog.brk_handler();                   // no arg: vector -> current cursor
```

`Program::start(addr)` writes a default reset vector pointing at `addr`
but does not flag it as user-set, so an explicit `prog.reset_handler(...)`
silently overrides the default. A second explicit `reset_handler`
trips the must-set-exactly-once `EXPECT`.

The BRK/IRQ and NMI vectors are pre-wired to `fail()` traps in high RAM
so unexpected interrupts surface as FAIL ("unexpected BRK or IRQ" /
"unexpected NMI"); tests that exercise BRK/IRQ/NMI override via
`brk_handler` / `nmi_handler`.

## Variant gating

If a test exercises an opcode or behavior that doesn't exist on every
shim variant, gate it with `SKIP_UNLESS` so the driver reports SKIP
separately from PASS/FAIL:

```cpp
TEST(stz_abs) {
    SKIP_UNLESS(CV_CMOS_65C02, "STZ is 65C02-only");
    ...
}
```

Variant masks include `CV_NMOS_6502`, `CV_6510`, `CV_6509`,
`CV_CMOS_65C02`, plus the convenience masks `CV_NMOS_LIKE` (NMOS|6510|6509)
and `CV_ALL`.

For opcodes whose **behavior** differs across variants (decimal-mode
flags, JMP-ind wrap, RMW bus pattern, BRK+NMI hijack), branch on
`cpu.variant()` and run two separate `EXPECT_PASS(expected)` calls
with distinct static arrays. A ternary doesn't work because
`EXPECT_PASS` deduces length via `sizeof / sizeof`, which requires a
C array, not a pointer.

## Naming and layout

- `test3/tests/isa/<group>_test.cpp` -- per-instruction family (loads,
  stores, branches, ...). One `TEST()` per (opcode, addressing-mode,
  corner) tuple.
- Test name: `<mnemonic>_<mode>[_<corner>]` (e.g. `lda_imm`,
  `lda_absx_page_cross`, `brk_stacks_pc_plus_two`).
- `test3/tests/isa/nmos_only/` -- undocumented NMOS opcodes that don't
  exist on other variants.
- `test3/tests/corners/` -- cross-cutting cases (page-cross timing,
  RMW bus pattern, IO-port shadowing, etc.).
- `test3/tests/corners/interrupts/` -- IRQ/NMI/BRK/reset timing
  corners.

`Cpu::load(prog)` resets per-run config (terminators, max_cycles,
counters, logger), so reusing the same `cpu` across tests is safe.
`max_cycles` defaults to 9999, which matches the cycle-column padding;
tests that legitimately need more call `cpu.set_max_cycles(...)` after
`cpu.load(prog)`.

## Writing style

- ASCII only. No em-dashes, en-dashes, or other Unicode in source or
  comments. Use `--` for em-dash, `-` for en-dash.
- No stacked semicolons on one line.
- Present-tense active voice. Short sentences. No filler ("simply",
  "just", "of course", "obviously").
- Comment only what needs clarifying.
