# Repository conventions

## Writing style for comments, docs, and commit messages

Aim for the register of a senior engineer writing for peers, not an
AI assistant briefing a reader.

- ASCII only. No smart quotes, em/en-dashes, ellipsis chars, arrows.
- No em-dashes (`--` included). Use comma, colon, parens, or two sentences.
- At most one semicolon per sentence.
- No filler ("Note that", "essentially", "basically", "In summary").
- No editorializing ("elegant", "robust", "cleanly", "nicely").
- Present tense, active voice.
- Short sentences. Prefer two of 12 words over one of 30.

## Test assertions (test3 / `EXPECT`)

`test3/include/test3/expect.h` provides a single `EXPECT(cond, msg, ...)`
macro for all assertions in the half-cycle-accurate test framework.

### Spec-style invariant messages

All `EXPECT` messages **must** be written as positive, present-tense
statements of the invariant that holds when `cond` is true (the "must
hold" form). The same string then reads naturally with both an `OK:`
prefix (when running with `--verbose`, on success) and a `FAIL:`
prefix (on failure).

Good:

- `EXPECT(ok, "canonical reset trace must match shim '%s'", shim_.name());`
- `EXPECT(ci.addr == 0xFFFC && ci.rw, "reset-vector low fetch must address $FFFC with rw=R (got $%04X rw=%c)", ci.addr, ci.rw ? 'R' : 'W');`
- `EXPECT(res.ok, "run must terminate cleanly (got reason=%s)", res.fail_reason ? res.fail_reason : "?");`

Bad (do not write these):

- `EXPECT(ok, "trace mismatch")`        -- describes the failure, not the invariant
- `EXPECT(ci.rw, "rw was wrong")`       -- same
- `EXPECT(ok, "ok must be true")`       -- restates the expression; says nothing useful

Contextual values (actual vs expected, addresses, names) belong in
the printf-style arguments, not in the assertion text itself.

### Runtime flags

The framework supports two flags that change `EXPECT`'s behavior:

- `--fail-fast` -- `std::abort()` on the first failed `EXPECT`.
  Default off; the default runner records the failure via
  `mark_failed` and continues so a single run can surface multiple
  problems.
- `-v` / `--verbose` -- print `OK: <message>` for each successful
  `EXPECT`. Failures are always printed regardless of this flag.

### Semantics

- `cond` is evaluated exactly once.
- `EXPECT` is an **expression of type `bool` equal to `cond`**. Callers
  that must bail out after a failed precondition (e.g. to avoid
  undefined behavior on the next line) can use it as a guard without
  repeating the condition:

  ```cpp
  if (!EXPECT(cursor_ < MEM_SIZE,
              "Program::byte: cursor must be within MEM_SIZE "
              "(cursor=$%05X)", (unsigned)cursor_)) return;
  image_[cursor_++] = b;
  ```

  The discarded-value form is also fine for assertions that should not
  short-circuit:

  ```cpp
  EXPECT(captured_trace_ok, "captured trace must match expected[]");
  ```

### Trace comparisons: `EXPECT_TRACE`

For comparing a captured per-half-cycle trace against an expected
table, use `EXPECT_TRACE` (declared in `test3/include/test3/trace.h`)
instead of calling `compare_and_report(...)` directly and surfacing
the result through a separate `EXPECT`. The macro wraps both steps:
it prints the side-by-side diff + paste-ready actual block on
mismatch (via `compare_and_report`) and the standard `OK:` /
`FAIL:` block (via `EXPECT`).

```cpp
// Bail-out form (e.g. inside Cpu::reset(); skip the rest of the test
// once we know the trace doesn't match):
if (!EXPECT_TRACE("[reset]", shim_.name(),
                  expected.data(), expected.size(), logger_.rows(),
                  "canonical reset trace must match shim '%s'",
                  shim_.name())) return;

// Discarded-value form (e.g. final assertion in a test body):
EXPECT_TRACE("smoke", shim.name(),
             expected, expected_count, runner.logger().rows(),
             "captured trace must match expected[]");
```

`EXPECT_TRACE` is an expression of type `bool` equal to "did the
traces match?", and follows the same spec-style message convention
as plain `EXPECT`.

## Program assembly (test3 / `Program` builder)

`test3/include/test3/program.h` provides a fluent 6502 byte emitter
used to assemble test programs inline in C++. The macros that used to
live in `test3/include/test3/opcodes.h` (`LDA_IMM(...)`, `STA_ZP(...)`,
`JMP_ABS(...)`, ...) have been replaced by a fluent API; do not
reintroduce them.

### Construction: `Program::start(addr)` is the canonical entry

`Program`'s default constructor is **private**. Make a `Program` via
the static factory `Program::start(addr)`, which both patches the
reset vector and places the cursor at `addr` -- single declaration
site, no duplicate literal. `Program` is **move-only** (copy
deleted), and every chainable emit method has dual `&` / `&&`
overloads, so the canonical idiom is to chain off `start()`:

```cpp
Program prog = Program::start(0x0400)
    .lda(Imm{0x42})
    .sta(ZP{0x80})
    .brk();
```

The chain runs entirely on the prvalue returned by `start()` (each
`&&` overload moves out) and the final initialization move-constructs
into `prog` -- no deep copies of the 64 KiB image. Calling on an
lvalue (`prog.lda(...).sta(...)`) still works via the `&` overloads,
returning `Program&` so existing call sites are unaffected.

`Program::start(addr)` writes a default reset vector pointing at
`addr` but does not flag the entry as user-set. A later explicit
`prog.reset_handler(...)` cleanly overrides that default and tags the
override at the test's source line; a second explicit `reset_handler`
trips an `EXPECT` that surfaces the prior call's file:line. `org()`
and the handler setters remain available for secondary code blocks
and handler vectors.

`addr` defaults to `Program::DEFAULT_ENTRY` (`$0200`) -- the first RAM
page above the zero page and stack page. Tests that care about the
address pass it explicitly (especially when the `expected[]` trace
table references it); tests that don't can call `Program::start()`
bare.

`start()` also pre-wires the BRK/IRQ vector ($FFFE/$FFFF) and NMI
vector ($FFFA/$FFFB) to `fail()` traps at `$FFF0` / `$FFF3` so any
unexpected interrupt ends the run with a clear FAIL message instead
of silently jumping into garbage. Tests that exercise BRK/IRQ/NMI
override the traps with `prog.brk_handler(handler)` or
`prog.nmi_handler(handler)`; the user's call wins both the vector
contents and the source-line tag at the vector address.

### Operand types are first-class (one method per mnemonic)

Addressing modes are value structs (`Imm`, `ZP`, `ZPX`, `ZPY`, `Abs`,
`AbsX`, `AbsY`, `Ind`, `IndX`, `IndY`). Each opcode mnemonic has a
single lowercase method overloaded across modes. The caller
brace-constructs the operand at the call site so the addressing mode
is always explicit (`lda(0x42)` is intentionally rejected):

```cpp
prog.lda(Imm{0x42})       // LDA #$42
    .sta(ZP {0x80})       // STA $80
    .jmp(Abs{0x1234})     // JMP $1234
    .brk();
```

Do **not** add per-mode method names (`lda_imm`, `lda_zp`, ...): they
defeat the uniform overload-set pattern that keeps the API tractable
as ISA coverage expands.

### Labels and cursor scopes

`prog.label()` captures the current cursor as a `Label`. Labels are
**backward references only** -- capture *after* emitting the target.
Jump and vector-setter methods accept either a raw address or a
`Label`:

```cpp
Program prog = Program::start(0x0400);
prog.lda(Imm{0x42}).sta(ZP{0x80}).brk();

prog.org(0x0500);
auto trap = prog.label();        // captures $0500
prog.jmp(trap);                  // JMP $0500
prog.brk_handler(trap);       // reset vector already set by start()
```

`prog.at(addr)` returns a `[[nodiscard]]` RAII cursor scope that
temporarily moves the cursor and restores it on destruction, useful
for emitting a helper without losing your place:

```cpp
Program prog = Program::start(0x0400);
prog.lda(Imm{0x42});
{
    auto _ = prog.at(0x0500);
    prog.brk();                  // emit at $0500
}                                // cursor restored to $0402
```

### Test file boilerplate

Hoist `using namespace test3;` to file scope (right after the
`#include`s, above the first `TEST()`). Don't repeat it inside each
test body. The test files compile in isolation, so a file-scope
`using` directive is local enough; it keeps the test bodies focused
on the program/assertion logic.

```cpp
#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(lda_imm) {
    Program prog = Program::start().lda(Imm{0x42}).brk();
    ...
}
```

### Source-line annotations and `EXPECT_PASS`

Every public emit method captures `__FILE__` / `__LINE__` from its
call site (via `__builtin_FILE()` / `__builtin_LINE()` default args)
and records `address -> (file, line)` in a per-Program source map.
When a trace mismatch is reported, the diff dumper uses this to point
at the line of the test that emitted the offending byte.

The canonical one-liner for asserting a successful run inside a
`TEST()` body is `EXPECT_PASS(expected)`:

```cpp
cpu.load(prog);
EXPECT_PASS(expected);
```

It runs the CPU, asserts that the run terminated via `prog.pass()`
(not `prog.fail()` / max_cycles), and matches the captured trace
against the C-array `expected[]` (length deduced via `sizeof / sizeof`).
It threads `cpu.program()` (the Program installed by the preceding
`cpu.load()`) into the trace-diff dumper so source-line annotations
appear automatically. Test name is auto-captured via `__func__`.

For trace comparisons that don't correspond to a test-authored
Program (e.g. `Cpu::reset()`'s canonical reset-trace check),
`EXPECT_TRACE(label, core_name, fields_mask, expected, n, actual, ...)`
remains the right tool. `EXPECT_TRACE_FROM(prog_ptr, ...)` is the
underlying primitive `EXPECT_PASS` expands to; tests rarely need it
directly.

### Run-terminator pseudo-ops: `pass()` / `fail()`

Prefer `prog.pass(msg)` / `prog.fail(msg)` over the legacy
`set_terminator_pc_trap` / `set_terminator_write` predicates when the
end-of-run condition is a specific PC. Each pseudo-op emits a 3-byte
`JMP self` at the cursor and registers the address as a run
terminator; `Cpu::run()` ends the run when an opcode fetch (SYNC=1)
hits that address.

Both return a `Label` so the pseudo-op can be wired as a branch or
vector target directly:

```cpp
prog.org(0x0500);
auto done = prog.pass("BRK terminated cleanly");
prog.brk_handler(done);

// or, inline at a vector setter:
prog.brk_handler(prog.fail("unexpected BRK"));
```

The `msg` surfaces in `RunResult::message` so tests can distinguish
acceptable variant paths (e.g., NMOS vs 65C02 behavior) by inspecting
which `pass()` fired. `pass()`'s default message is `"normal success"`;
`fail()` requires a message. The message must outlive the run (string
literals are typical; it is not copied).

`fail()` also routes through `EXPECT(false, ...)` with the declaring
file:line, so failures participate in `--fail-fast` and pass/fail
accounting just like any other `EXPECT`.

### Suite organization and variant handling

Tests live under `test3/tests/`:

- `tests/isa/<group>_test.cpp` -- per-instruction family (loads,
  stores, branches, ...). One TEST() per (opcode, addressing-mode,
  corner) tuple. Naming: `<mnemonic>_<mode>[_<corner>]`
  (e.g. `lda_imm`, `lda_absx_page_cross`, `jmp_ind_at_page_boundary`).
- `tests/isa/nmos_only/` -- NMOS undocumented opcodes
  (LAX/SAX/DCP/...). Use `SKIP_UNLESS(::test3::CV_NMOS_LIKE, ...)`.
- `tests/corners/<corner>_test.cpp` -- cross-cutting behaviors
  (decimal mode, page crossing, RMW timing, interrupt timing).
- `tests/smoke_test.cpp` -- pre-existing; stays at the root as the
  tutorial example.

Today there's only one shim (`aholme`, NMOS 6502). The driver loops
over `shims` so adding a shim later is a one-line change. Variant
infrastructure (`CpuVariant`, `ICpuShim::variant()`, `SKIP_UNLESS`)
is already in place. When a CMOS shim or others are added:

- **Variant-only opcodes** that only exist on one variant (e.g. STZ
  on 65C02) get a new subdirectory (e.g. `tests/isa/cmos_only/`)
  and use `SKIP_UNLESS(variant_mask, "reason")` from `test3/test.h`.
- **Same opcode, divergent behavior** (JMP-ind page wrap, decimal-mode
  flags, RMW bus pattern, BRK+NMI hijack) uses a `cpu.variant()`
  if/else with two `static const TraceRow expected[]` tables -- each
  followed by its own `EXPECT_PASS(expected);`. A ternary doesn't
  work because `EXPECT_PASS` deduces length via `sizeof / sizeof`,
  which requires a C array, not a pointer.

`CpuVariant` (in `test3/shim.h`) values: `CV_NMOS_6502`, `CV_6510`,
`CV_6509`, `CV_CMOS_65C02`; convenience masks `CV_NMOS_LIKE`
(all three NMOS-family variants) and `CV_ALL`.
