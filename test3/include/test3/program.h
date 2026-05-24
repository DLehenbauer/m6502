// test3/include/test3/program.h
//
// A tiny 6502 byte-emitter for assembling test programs inline in C++.
// The Program owns a flat 64 KiB image and a cursor; fluent opcode
// methods (lda/sta/jmp/brk/nop) append bytes at the current cursor and
// return *this so calls can be chained.
//
// Addressing modes are first-class operand types (Imm, ZP, ZPX, ZPY,
// Abs, AbsX, AbsY, Ind, IndX, IndY). One overloaded method per
// mnemonic (lda(Imm), lda(ZP), ...) keeps the API uniform as ISA
// coverage grows, instead of an LDA_IMM/LDA_ZP/LDA_ABS method
// explosion. Operand wrappers are intentionally not implicitly
// convertible -- callers brace-construct the mode at the call site so
// the addressing mode is always explicit and self-documenting.
//
// Labels capture a cursor and feed back into jump and vector setters.
// Backward references only (label() captures the *current* cursor for
// use in any *subsequent* call); forward references would require
// backpatching and are out of scope today.
//
// Example:
//     Program prog = Program::start(0x0400)   // entry: $0400
//         .lda(Imm{0x42})
//         .sta(ZP{0x80})
//         .brk();
//
//     // BRK exit lands at a pass terminator; reaching it ends the run
//     // cleanly. Returns a Label so it doubles as the vector target.
//     auto done = prog.pass("BRK terminated cleanly");
//     prog.brk_handler(done);
//
// `Program::start(addr)` is the canonical entry point: a static
// factory that constructs a Program with the reset vector patched
// to `addr` and the cursor placed there. The default constructor is
// private, and Program is move-only (copy deleted) so accidental
// O(64KiB) copies are a compile error. A second `reset_handler()`
// call (or a second `start()`-derived assignment) trips an EXPECT
// pointing at the prior set, so accidental double-entry fails loudly.
//
// Every chainable method has dual ref-qualified overloads: `&`
// returns Program& (lvalue chains continue by reference), `&&`
// returns Program by value via std::move (so chains off the
// temporary returned by start() compose without deep copies).
//
// Pseudo-ops `pass(msg)` / `fail(msg)` emit a 3-byte JMP-self at the
// cursor and register the address as a run terminator. When Cpu::run()
// sees an opcode fetch at that address it ends the run, surfacing
// `msg` in RunResult::message so tests can detect which acceptable
// path fired (or which invariant was violated).
//
// `Program::at(addr)` returns a [[nodiscard]] RAII scope that
// temporarily moves the cursor and restores it on destruction, useful
// for emitting a helper routine without losing your place:
//
//     prog.org(0x0400).lda(Imm{0x42}).sta(ZP{0x80});
//     {
//         auto _ = prog.at(0x0500);
//         prog.brk();                  // emit trap at $0500
//     }                                // cursor restored to $0404
//     prog.jmp(Abs{0x0500});
//
// Source-line annotations. Every public emit method captures its
// call-site __FILE__/__LINE__ via GCC/Clang __builtin_FILE() /
// __builtin_LINE() default arguments and tags the starting cursor in
// a side map. On trace-diff failure, compare_and_report(prog, ...)
// annotates the diff row with the source line that emitted the byte
// the CPU was fetching, making it trivial to jump from a failing
// half-cycle to the line of the test that produced it.

#pragma once

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <map>
#include <string_view>

#include "test3/expect.h"

namespace test3 {

// ---------------------------------------------------------------------------
// Addressing-mode operand types.
//
// Brace-constructed at the call site (lda(Imm{0x42})) so the mode is
// always visible. No implicit conversions: lda(0x42) is rejected and
// the author must say lda(Imm{0x42}) or lda(ZP{0x42}).
//
// 8-bit fields for zero-page and immediate modes; 16-bit for absolute
// and indirect.
// ---------------------------------------------------------------------------
struct Imm  { uint8_t  v; };   // #$nn       -- LDA #$42
struct ZP   { uint8_t  v; };   // $nn        -- STA $80
struct ZPX  { uint8_t  v; };   // $nn,X
struct ZPY  { uint8_t  v; };   // $nn,Y
struct Abs  { uint16_t v; };   // $nnnn      -- JMP $1234
struct AbsX { uint16_t v; };   // $nnnn,X
struct AbsY { uint16_t v; };   // $nnnn,Y
struct Ind  { uint16_t v; };   // ($nnnn)    -- JMP indirect
struct IndX { uint8_t  v; };   // ($nn,X)
struct IndY { uint8_t  v; };   // ($nn),Y

// A captured cursor location, returned by Program::label() for use in
// subsequent jumps and vector setters. Backward references only.
struct Label { uint16_t addr; };

// LabelOrProgramRef is what Program returns from methods that name a location
// in the program (org(), pass(), fail(), etc.). It inherits Label so
// it can be passed directly anywhere a Label is expected (e.g.
// brk_handler / nmi_handler / reset_handler). It also
// forwards the chainable emit ops back to the underlying Program so
// the chain can continue. Examples:
//
//     prog.brk_handler(prog.org(0x0500));            // org   -> Label conv
//     prog.brk_handler(prog.org(0x0500).pass("...")); // chained pass()
//     prog.brk().fail("BRK fell through").nop();        // chain after fail()
//
// Forwarders preserve __builtin_FILE() / __builtin_LINE() defaults so
// the source-line tag of each chained call matches the test's call
// site (not this header).
class Program;
class LabelOrProgramRef : public Label {
public:
    // Implicit Program& conversion lets a LabelOrProgramRef stand in for the
    // underlying Program in any context expecting one. (Method calls
    // still go through the explicit forwarders below: C++ does not
    // apply implicit conversion on the LHS of `.`.)
    operator Program&() & { return prog_; }

    // ---- forwarded chainable ops (the public emit + relocation set).
    // Vector setters and at()/label() stay on Program; reach them via
    // the implicit Program& conversion if needed. ---------------------

    Program& byte(uint8_t b,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE());
    Program& word(uint16_t w,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE());
    Program& bytes(std::initializer_list<uint8_t> bs,
                   const char* file = __builtin_FILE(),
                   int         line = __builtin_LINE());
    Program& string(std::string_view s,
                    const char* file = __builtin_FILE(),
                    int         line = __builtin_LINE());
    Program& fill(size_t count, uint8_t value,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE());

    // Addressing-mode opcode forwarders. Mirrors the macro-driven set
    // in Program; each just delegates to prog_.name(op, file, line).
#define TEST3_FWD_DECL_IMM(name)  Program& name(Imm  op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ZP(name)   Program& name(ZP   op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ZPX(name)  Program& name(ZPX  op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ZPY(name)  Program& name(ZPY  op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ABS(name)  Program& name(Abs  op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ABSX(name) Program& name(AbsX op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_ABSY(name) Program& name(AbsY op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_IND(name)  Program& name(Ind  op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_INDX(name) Program& name(IndX op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_INDY(name) Program& name(IndY op, const char* file = __builtin_FILE(), int line = __builtin_LINE());
#define TEST3_FWD_DECL_BR(name)   Program& name(Label l, const char* file = __builtin_FILE(), int line = __builtin_LINE());

    TEST3_FWD_DECL_IMM(lda)  TEST3_FWD_DECL_ZP(lda)  TEST3_FWD_DECL_ZPX(lda) TEST3_FWD_DECL_ABS(lda)
    TEST3_FWD_DECL_ABSX(lda) TEST3_FWD_DECL_ABSY(lda) TEST3_FWD_DECL_INDX(lda) TEST3_FWD_DECL_INDY(lda)
    TEST3_FWD_DECL_IMM(ldx)  TEST3_FWD_DECL_ZP(ldx)  TEST3_FWD_DECL_ZPY(ldx) TEST3_FWD_DECL_ABS(ldx) TEST3_FWD_DECL_ABSY(ldx)
    TEST3_FWD_DECL_IMM(ldy)  TEST3_FWD_DECL_ZP(ldy)  TEST3_FWD_DECL_ZPX(ldy) TEST3_FWD_DECL_ABS(ldy) TEST3_FWD_DECL_ABSX(ldy)
    TEST3_FWD_DECL_ZP(sta)   TEST3_FWD_DECL_ZPX(sta) TEST3_FWD_DECL_ABS(sta) TEST3_FWD_DECL_ABSX(sta)
    TEST3_FWD_DECL_ABSY(sta) TEST3_FWD_DECL_INDX(sta) TEST3_FWD_DECL_INDY(sta)
    TEST3_FWD_DECL_ZP(stx)   TEST3_FWD_DECL_ZPY(stx) TEST3_FWD_DECL_ABS(stx)
    TEST3_FWD_DECL_ZP(sty)   TEST3_FWD_DECL_ZPX(sty) TEST3_FWD_DECL_ABS(sty)
    TEST3_FWD_DECL_IMM(adc)  TEST3_FWD_DECL_ZP(adc)  TEST3_FWD_DECL_ZPX(adc) TEST3_FWD_DECL_ABS(adc)
    TEST3_FWD_DECL_ABSX(adc) TEST3_FWD_DECL_ABSY(adc) TEST3_FWD_DECL_INDX(adc) TEST3_FWD_DECL_INDY(adc)
    TEST3_FWD_DECL_IMM(sbc)  TEST3_FWD_DECL_ZP(sbc)  TEST3_FWD_DECL_ZPX(sbc) TEST3_FWD_DECL_ABS(sbc)
    TEST3_FWD_DECL_ABSX(sbc) TEST3_FWD_DECL_ABSY(sbc) TEST3_FWD_DECL_INDX(sbc) TEST3_FWD_DECL_INDY(sbc)
    TEST3_FWD_DECL_IMM(and_) TEST3_FWD_DECL_ZP(and_) TEST3_FWD_DECL_ZPX(and_) TEST3_FWD_DECL_ABS(and_)
    TEST3_FWD_DECL_ABSX(and_) TEST3_FWD_DECL_ABSY(and_) TEST3_FWD_DECL_INDX(and_) TEST3_FWD_DECL_INDY(and_)
    TEST3_FWD_DECL_IMM(ora)  TEST3_FWD_DECL_ZP(ora)  TEST3_FWD_DECL_ZPX(ora) TEST3_FWD_DECL_ABS(ora)
    TEST3_FWD_DECL_ABSX(ora) TEST3_FWD_DECL_ABSY(ora) TEST3_FWD_DECL_INDX(ora) TEST3_FWD_DECL_INDY(ora)
    TEST3_FWD_DECL_IMM(eor)  TEST3_FWD_DECL_ZP(eor)  TEST3_FWD_DECL_ZPX(eor) TEST3_FWD_DECL_ABS(eor)
    TEST3_FWD_DECL_ABSX(eor) TEST3_FWD_DECL_ABSY(eor) TEST3_FWD_DECL_INDX(eor) TEST3_FWD_DECL_INDY(eor)
    TEST3_FWD_DECL_ZP(bit)   TEST3_FWD_DECL_ABS(bit)
    TEST3_FWD_DECL_IMM(cmp)  TEST3_FWD_DECL_ZP(cmp)  TEST3_FWD_DECL_ZPX(cmp) TEST3_FWD_DECL_ABS(cmp)
    TEST3_FWD_DECL_ABSX(cmp) TEST3_FWD_DECL_ABSY(cmp) TEST3_FWD_DECL_INDX(cmp) TEST3_FWD_DECL_INDY(cmp)
    TEST3_FWD_DECL_IMM(cpx)  TEST3_FWD_DECL_ZP(cpx)  TEST3_FWD_DECL_ABS(cpx)
    TEST3_FWD_DECL_IMM(cpy)  TEST3_FWD_DECL_ZP(cpy)  TEST3_FWD_DECL_ABS(cpy)
    TEST3_FWD_DECL_ZP(asl)   TEST3_FWD_DECL_ZPX(asl) TEST3_FWD_DECL_ABS(asl) TEST3_FWD_DECL_ABSX(asl)
    TEST3_FWD_DECL_ZP(lsr)   TEST3_FWD_DECL_ZPX(lsr) TEST3_FWD_DECL_ABS(lsr) TEST3_FWD_DECL_ABSX(lsr)
    TEST3_FWD_DECL_ZP(rol)   TEST3_FWD_DECL_ZPX(rol) TEST3_FWD_DECL_ABS(rol) TEST3_FWD_DECL_ABSX(rol)
    TEST3_FWD_DECL_ZP(ror)   TEST3_FWD_DECL_ZPX(ror) TEST3_FWD_DECL_ABS(ror) TEST3_FWD_DECL_ABSX(ror)
    TEST3_FWD_DECL_ZP(inc)   TEST3_FWD_DECL_ZPX(inc) TEST3_FWD_DECL_ABS(inc) TEST3_FWD_DECL_ABSX(inc)
    TEST3_FWD_DECL_ZP(dec)   TEST3_FWD_DECL_ZPX(dec) TEST3_FWD_DECL_ABS(dec) TEST3_FWD_DECL_ABSX(dec)
    TEST3_FWD_DECL_ABS(jmp)  TEST3_FWD_DECL_IND(jmp) TEST3_FWD_DECL_BR(jmp)
    TEST3_FWD_DECL_ABS(jsr)  TEST3_FWD_DECL_BR(jsr)
    TEST3_FWD_DECL_BR(bpl)   TEST3_FWD_DECL_BR(bmi)  TEST3_FWD_DECL_BR(bvc)  TEST3_FWD_DECL_BR(bvs)
    TEST3_FWD_DECL_BR(bcc)   TEST3_FWD_DECL_BR(bcs)  TEST3_FWD_DECL_BR(bne)  TEST3_FWD_DECL_BR(beq)

#undef TEST3_FWD_DECL_IMM
#undef TEST3_FWD_DECL_ZP
#undef TEST3_FWD_DECL_ZPX
#undef TEST3_FWD_DECL_ZPY
#undef TEST3_FWD_DECL_ABS
#undef TEST3_FWD_DECL_ABSX
#undef TEST3_FWD_DECL_ABSY
#undef TEST3_FWD_DECL_IND
#undef TEST3_FWD_DECL_INDX
#undef TEST3_FWD_DECL_INDY
#undef TEST3_FWD_DECL_BR

    Program& brk(Imm sig,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE());
    Program& brk(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE());
    Program& nop(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE());
    Program& rti(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE());

    Program& clc(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& sec(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& cli(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& sei(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& clv(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& cld(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& sed(const char* file = __builtin_FILE(), int line = __builtin_LINE());

    Program& tax(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& tay(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& txa(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& tya(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& tsx(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& txs(const char* file = __builtin_FILE(), int line = __builtin_LINE());

    Program& inx(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& iny(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& dex(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& dey(const char* file = __builtin_FILE(), int line = __builtin_LINE());

    Program& pha(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& pla(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& php(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& plp(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& rts(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& asl(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& lsr(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& rol(const char* file = __builtin_FILE(), int line = __builtin_LINE());
    Program& ror(const char* file = __builtin_FILE(), int line = __builtin_LINE());

    LabelOrProgramRef pass(const char* msg  = "normal success",
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE());
    LabelOrProgramRef fail(const char* msg,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE());

    LabelOrProgramRef org(uint16_t addr);
    Program& write(uint16_t addr, uint8_t v,
                   const char* file = __builtin_FILE(),
                   int         line = __builtin_LINE());

    LabelOrProgramRef brk_handler(uint16_t pc,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef brk_handler(Label l,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef brk_handler(const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef nmi_handler(uint16_t pc,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef nmi_handler(Label l,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef nmi_handler(const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE());
    LabelOrProgramRef reset_handler(uint16_t pc,
                                    const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE());
    LabelOrProgramRef reset_handler(Label l,
                                    const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE());
    LabelOrProgramRef reset_handler(const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE());

private:
    friend class Program;
    LabelOrProgramRef(Label l, Program& p) : Label(l), prog_(p) {}
    Program& prog_;
};

// Source location of an emit, captured automatically via GCC/Clang
// __builtin_FILE() / __builtin_LINE() default arguments (which
// evaluate at the call site, not the declaration site). Used by the
// trace-diff dumper to annotate failure rows with the source line
// that emitted the offending byte. See compare_and_report (trace.h).
struct SrcLoc { const char* file; int line; };

// Run-terminator pseudo-op declared by the program at an address.
// When Cpu::run() sees an opcode fetch (SYNC=1) at that address it
// ends the run with the recorded kind/message and reports the
// declaring file:line for diagnostics.
enum class TermKind { Pass, Fail };
struct Terminator {
    TermKind    kind;
    const char* msg;   // must outlive the run (string literals are typical)
    SrcLoc      src;   // where pass()/fail() was called in the test source
};

class Program;

// RAII cursor scope returned by Program::at(addr). Saves the current
// cursor on construction, sets it to `addr`, restores on destruction.
// [[nodiscard]] so dropping the binding warns -- a bare `prog.at(addr);`
// would destruct immediately and have no effect.
class [[nodiscard]] CursorScope {
public:
    ~CursorScope();
    CursorScope(const CursorScope&) = delete;
    CursorScope& operator=(const CursorScope&) = delete;
    CursorScope(CursorScope&&) = delete;
    CursorScope& operator=(CursorScope&&) = delete;
private:
    friend class Program;
    CursorScope(Program* p, uint16_t addr);
    Program* p_;
    uint16_t saved_;
};

class Program {
public:
    static constexpr size_t MEM_SIZE = 0x10000;

    // Default entry point used when Program::start() is called with no
    // address. $0200 sits immediately above the zero page ($0000-$00FF)
    // and the stack page ($0100-$01FF), so it's the first available RAM
    // page that doesn't collide with the canonical reset's zero-page
    // reads or BRK's stack pushes.
    static constexpr uint16_t DEFAULT_ENTRY = 0x0200;

    // Canonical entry point: returns a freshly constructed Program
    // with the reset vector patched to `addr` and the cursor placed
    // at `addr` so subsequent emit calls write the entry instruction
    // stream. One declaration site for both the program and its
    // entry point, with no duplicate literal:
    //
    //     Program prog = Program::start(0x0400);
    //     prog.lda(Imm{0x42}).sta(ZP{0x80}).brk();
    //
    // `addr` defaults to DEFAULT_ENTRY ($0400) for tests that don't
    // care where the program lives.
    //
    // The default constructor is private, so Program::start() (or
    // future sibling factories) is the only supported way to make a
    // Program. Source location of the call site is captured for the
    // duplicate-entry diagnostic on any later reset_handler().
    //
    // BRK/IRQ and NMI vectors are pre-wired to fail() pseudo-op traps
    // at $FFF0 and $FFF3 respectively, so any unexpected interrupt
    // ends the run with a clear FAIL ("unexpected BRK or IRQ" /
    // "unexpected NMI") instead of silently jumping into garbage at
    // $0000. Tests that exercise BRK/IRQ/NMI override the trap by
    // calling brk_handler(handler) or nmi_handler(handler).
    static Program start(uint16_t    addr = DEFAULT_ENTRY,
                         const char* file = __builtin_FILE(),
                         int         line = __builtin_LINE()) {
        Program p;
        // Default reset vector: writes $FFFC/$FFFD without flipping
        // `reset_vector_set_` so a later explicit reset_handler() call
        // is treated as the first user-driven entry-point declaration
        // and tags it at the test's source line. The cursor is set to
        // `addr` so emits land at the program entry by default.
        p.image_[0xFFFC] = static_cast<uint8_t>(addr & 0xFF);
        p.image_[0xFFFD] = static_cast<uint8_t>(addr >> 8);

        // Default fail-traps. Placed at high RAM so they don't collide
        // with typical program addresses; the user can still override
        // either vector via brk_handler / nmi_handler. Direct image_
        // writes (no tag()) so the user's later override call gets to
        // own the source-line tag at $FFFE/$FFFA (tag() is first-wins).
        p.cursor_ = 0xFFF0;
        Label brk_trap = p.fail("unexpected BRK or IRQ "
                                "(set brk_handler(handler) to override)",
                                file, line);
        p.cursor_ = 0xFFF3;
        Label nmi_trap = p.fail("unexpected NMI "
                                "(set nmi_handler(handler) to override)",
                                file, line);
        p.image_[0xFFFE] = static_cast<uint8_t>(brk_trap.addr & 0xFF);
        p.image_[0xFFFF] = static_cast<uint8_t>(brk_trap.addr >> 8);
        p.image_[0xFFFA] = static_cast<uint8_t>(nmi_trap.addr & 0xFF);
        p.image_[0xFFFB] = static_cast<uint8_t>(nmi_trap.addr >> 8);

        p.cursor_ = addr;
        return p;
    }

    // Movable but not copyable. Copying a 64 KiB image_ + two maps
    // is silently expensive and never what tests want; deleting the
    // copy turns any accidental copy into a compile error.
    Program(Program&&) = default;
    Program& operator=(Program&&) = default;
    Program(const Program&) = delete;
    Program& operator=(const Program&) = delete;

    // ---- cursor / raw emit -------------------------------------------------
    //
    // Each chainable method has two ref-qualified overloads:
    //   - `&` returns Program& so chains on an lvalue Program continue
    //     by reference (no copies).
    //   - `&&` returns Program by value (via std::move) so chains off
    //     a temporary (typical: `Program::start(...).lda(...).sta(...)`)
    //     compose without copy-constructing into the binding variable.
    // The && overload always delegates to the & overload (because
    // `*this` is an lvalue inside any method body, so the unqualified
    // call resolves to the & version) and then moves out, so the
    // emission logic lives in exactly one place.

    // Set the cursor to `addr`. Subsequent emits write at and after
    // it. The lvalue overload returns a LabelOrProgramRef whose Label is the
    // new cursor, so org() can be used directly as a vector target:
    //
    //     prog.brk_handler(prog.org(0x0500));
    //     prog.brk_handler(prog.org(0x0500).pass("..."));
    LabelOrProgramRef org(uint16_t addr) & { cursor_ = addr; return LabelOrProgramRef{Label{addr}, *this}; }
    Program  org(uint16_t addr) && { org(addr); return std::move(*this); }

    // Append a single byte at the cursor and advance. Captures the
    // call-site source location and tags the starting cursor so
    // trace-diff failures can be traced back to this emit.
    Program& byte(uint8_t b,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        return byte_raw(b);
    }
    Program byte(uint8_t b,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) && {
        byte(b, file, line); return std::move(*this);
    }

    // Append a little-endian 16-bit word.
    Program& word(uint16_t w,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        byte_raw(static_cast<uint8_t>(w & 0xFF));
        byte_raw(static_cast<uint8_t>(w >> 8));
        return *this;
    }
    Program word(uint16_t w,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) && {
        word(w, file, line); return std::move(*this);
    }

    // ---- data helpers ------------------------------------------------------

    // Emit a sequence of literal bytes.
    Program& bytes(std::initializer_list<uint8_t> bs,
                   const char* file = __builtin_FILE(),
                   int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        for (uint8_t b : bs) byte_raw(b);
        return *this;
    }
    Program bytes(std::initializer_list<uint8_t> bs,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE()) && {
        bytes(bs, file, line); return std::move(*this);
    }

    // Emit an ASCII string (no implicit NUL terminator -- add one with
    // .byte(0) if needed).
    Program& string(std::string_view s,
                    const char* file = __builtin_FILE(),
                    int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        for (char c : s) byte_raw(static_cast<uint8_t>(c));
        return *this;
    }
    Program string(std::string_view s,
                   const char* file = __builtin_FILE(),
                   int         line = __builtin_LINE()) && {
        string(s, file, line); return std::move(*this);
    }

    // Emit `count` copies of `value` (e.g. fill(16, 0xEA) for 16 NOPs
    // of padding).
    Program& fill(size_t count, uint8_t value,
                  const char* file = __builtin_FILE(),
                  int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        for (size_t i = 0; i < count; ++i) byte_raw(value);
        return *this;
    }
    Program fill(size_t count, uint8_t value,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) && {
        fill(count, value, file, line); return std::move(*this);
    }

    // ---- opcodes -----------------------------------------------------------
    //
    // The 6502 instruction set is defined via the macros below. Each
    // (instruction, addressing-mode) pair declares one method with the
    // standard & / && ref-qualified pair. The macros expand to the
    // same boilerplate the manual lda/sta/jmp definitions used to use,
    // just spelled compactly.
    //
    // Addressing-mode operand types live in this file:
    //   Imm, ZP, ZPX, ZPY, Abs, AbsX, AbsY, Ind, IndX, IndY, Label
    //
    // Opcode mnemonic naming:
    //   <name>(operand)     - addressing mode is inferred from the
    //                         operand's type (Imm vs ZP vs Abs, etc.)
    //   <name>()            - implied / accumulator addressing (1 byte)
    //   bcs(Label l)        - branch instructions take a Label and
    //                         emit the 8-bit signed PC-relative offset.

#define TEST3_OP_IMM(name, opcode)                                          \
    Program& name(Imm op,                                                   \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(Imm op,                                                    \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ZP(name, opcode)                                           \
    Program& name(ZP op,                                                    \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(ZP op,                                                     \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ZPX(name, opcode)                                          \
    Program& name(ZPX op,                                                   \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(ZPX op,                                                    \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ZPY(name, opcode)                                          \
    Program& name(ZPY op,                                                   \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(ZPY op,                                                    \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ABS(name, opcode)                                          \
    Program& name(Abs op,                                                   \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode);                                                   \
        byte_raw(static_cast<uint8_t>(op.v & 0xFF));                        \
        byte_raw(static_cast<uint8_t>(op.v >> 8));                          \
        return *this;                                                       \
    }                                                                       \
    Program name(Abs op,                                                    \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ABSX(name, opcode)                                         \
    Program& name(AbsX op,                                                  \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode);                                                   \
        byte_raw(static_cast<uint8_t>(op.v & 0xFF));                        \
        byte_raw(static_cast<uint8_t>(op.v >> 8));                          \
        return *this;                                                       \
    }                                                                       \
    Program name(AbsX op,                                                   \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_ABSY(name, opcode)                                         \
    Program& name(AbsY op,                                                  \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode);                                                   \
        byte_raw(static_cast<uint8_t>(op.v & 0xFF));                        \
        byte_raw(static_cast<uint8_t>(op.v >> 8));                          \
        return *this;                                                       \
    }                                                                       \
    Program name(AbsY op,                                                   \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_IND(name, opcode)                                          \
    Program& name(Ind op,                                                   \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode);                                                   \
        byte_raw(static_cast<uint8_t>(op.v & 0xFF));                        \
        byte_raw(static_cast<uint8_t>(op.v >> 8));                          \
        return *this;                                                       \
    }                                                                       \
    Program name(Ind op,                                                    \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_INDX(name, opcode)                                         \
    Program& name(IndX op,                                                  \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(IndX op,                                                   \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
#define TEST3_OP_INDY(name, opcode)                                         \
    Program& name(IndY op,                                                  \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); byte_raw(op.v); return *this;                     \
    }                                                                       \
    Program name(IndY op,                                                   \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(op, file, line); return std::move(*this);                      \
    }
    // Branch: opcode + 8-bit signed offset such that (cursor_+2) + offset
    // == target. EXPECTs that the target is within +127/-128 of PC+2.
#define TEST3_OP_BRANCH(name, opcode)                                       \
    Program& name(Label l,                                                  \
                  const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        const int delta = static_cast<int>(l.addr)                          \
                        - static_cast<int>(cursor_ + 2);                    \
        EXPECT(delta >= -128 && delta <= 127,                               \
               "branch target out of range "                                \
               "(target=$%04X, pc+2=$%04X, delta=%d)",                      \
               (unsigned)l.addr, (unsigned)(cursor_ + 2), delta);           \
        byte_raw(opcode);                                                   \
        byte_raw(static_cast<uint8_t>(delta & 0xFF));                       \
        return *this;                                                       \
    }                                                                       \
    Program name(Label l,                                                   \
                 const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(l, file, line); return std::move(*this);                       \
    }

    // ---- loads -------------------------------------------------------------
    TEST3_OP_IMM (lda, 0xA9)
    TEST3_OP_ZP  (lda, 0xA5)
    TEST3_OP_ZPX (lda, 0xB5)
    TEST3_OP_ABS (lda, 0xAD)
    TEST3_OP_ABSX(lda, 0xBD)
    TEST3_OP_ABSY(lda, 0xB9)
    TEST3_OP_INDX(lda, 0xA1)
    TEST3_OP_INDY(lda, 0xB1)

    TEST3_OP_IMM (ldx, 0xA2)
    TEST3_OP_ZP  (ldx, 0xA6)
    TEST3_OP_ZPY (ldx, 0xB6)
    TEST3_OP_ABS (ldx, 0xAE)
    TEST3_OP_ABSY(ldx, 0xBE)

    TEST3_OP_IMM (ldy, 0xA0)
    TEST3_OP_ZP  (ldy, 0xA4)
    TEST3_OP_ZPX (ldy, 0xB4)
    TEST3_OP_ABS (ldy, 0xAC)
    TEST3_OP_ABSX(ldy, 0xBC)

    // ---- stores ------------------------------------------------------------
    TEST3_OP_ZP  (sta, 0x85)
    TEST3_OP_ZPX (sta, 0x95)
    TEST3_OP_ABS (sta, 0x8D)
    TEST3_OP_ABSX(sta, 0x9D)
    TEST3_OP_ABSY(sta, 0x99)
    TEST3_OP_INDX(sta, 0x81)
    TEST3_OP_INDY(sta, 0x91)

    TEST3_OP_ZP  (stx, 0x86)
    TEST3_OP_ZPY (stx, 0x96)
    TEST3_OP_ABS (stx, 0x8E)

    TEST3_OP_ZP  (sty, 0x84)
    TEST3_OP_ZPX (sty, 0x94)
    TEST3_OP_ABS (sty, 0x8C)

    // ---- arithmetic --------------------------------------------------------
    TEST3_OP_IMM (adc, 0x69)
    TEST3_OP_ZP  (adc, 0x65)
    TEST3_OP_ZPX (adc, 0x75)
    TEST3_OP_ABS (adc, 0x6D)
    TEST3_OP_ABSX(adc, 0x7D)
    TEST3_OP_ABSY(adc, 0x79)
    TEST3_OP_INDX(adc, 0x61)
    TEST3_OP_INDY(adc, 0x71)

    TEST3_OP_IMM (sbc, 0xE9)
    TEST3_OP_ZP  (sbc, 0xE5)
    TEST3_OP_ZPX (sbc, 0xF5)
    TEST3_OP_ABS (sbc, 0xED)
    TEST3_OP_ABSX(sbc, 0xFD)
    TEST3_OP_ABSY(sbc, 0xF9)
    TEST3_OP_INDX(sbc, 0xE1)
    TEST3_OP_INDY(sbc, 0xF1)

    // ---- logical -----------------------------------------------------------
    TEST3_OP_IMM (and_, 0x29)
    TEST3_OP_ZP  (and_, 0x25)
    TEST3_OP_ZPX (and_, 0x35)
    TEST3_OP_ABS (and_, 0x2D)
    TEST3_OP_ABSX(and_, 0x3D)
    TEST3_OP_ABSY(and_, 0x39)
    TEST3_OP_INDX(and_, 0x21)
    TEST3_OP_INDY(and_, 0x31)

    TEST3_OP_IMM (ora, 0x09)
    TEST3_OP_ZP  (ora, 0x05)
    TEST3_OP_ZPX (ora, 0x15)
    TEST3_OP_ABS (ora, 0x0D)
    TEST3_OP_ABSX(ora, 0x1D)
    TEST3_OP_ABSY(ora, 0x19)
    TEST3_OP_INDX(ora, 0x01)
    TEST3_OP_INDY(ora, 0x11)

    TEST3_OP_IMM (eor, 0x49)
    TEST3_OP_ZP  (eor, 0x45)
    TEST3_OP_ZPX (eor, 0x55)
    TEST3_OP_ABS (eor, 0x4D)
    TEST3_OP_ABSX(eor, 0x5D)
    TEST3_OP_ABSY(eor, 0x59)
    TEST3_OP_INDX(eor, 0x41)
    TEST3_OP_INDY(eor, 0x51)

    TEST3_OP_ZP  (bit, 0x24)
    TEST3_OP_ABS (bit, 0x2C)

    // ---- compares ----------------------------------------------------------
    TEST3_OP_IMM (cmp, 0xC9)
    TEST3_OP_ZP  (cmp, 0xC5)
    TEST3_OP_ZPX (cmp, 0xD5)
    TEST3_OP_ABS (cmp, 0xCD)
    TEST3_OP_ABSX(cmp, 0xDD)
    TEST3_OP_ABSY(cmp, 0xD9)
    TEST3_OP_INDX(cmp, 0xC1)
    TEST3_OP_INDY(cmp, 0xD1)

    TEST3_OP_IMM (cpx, 0xE0)
    TEST3_OP_ZP  (cpx, 0xE4)
    TEST3_OP_ABS (cpx, 0xEC)

    TEST3_OP_IMM (cpy, 0xC0)
    TEST3_OP_ZP  (cpy, 0xC4)
    TEST3_OP_ABS (cpy, 0xCC)

    // ---- shifts / rotates (no-arg form is accumulator mode) ----------------
    TEST3_OP_ZP  (asl, 0x06)
    TEST3_OP_ZPX (asl, 0x16)
    TEST3_OP_ABS (asl, 0x0E)
    TEST3_OP_ABSX(asl, 0x1E)

    TEST3_OP_ZP  (lsr, 0x46)
    TEST3_OP_ZPX (lsr, 0x56)
    TEST3_OP_ABS (lsr, 0x4E)
    TEST3_OP_ABSX(lsr, 0x5E)

    TEST3_OP_ZP  (rol, 0x26)
    TEST3_OP_ZPX (rol, 0x36)
    TEST3_OP_ABS (rol, 0x2E)
    TEST3_OP_ABSX(rol, 0x3E)

    TEST3_OP_ZP  (ror, 0x66)
    TEST3_OP_ZPX (ror, 0x76)
    TEST3_OP_ABS (ror, 0x6E)
    TEST3_OP_ABSX(ror, 0x7E)

    // ---- INC/DEC memory ----------------------------------------------------
    TEST3_OP_ZP  (inc, 0xE6)
    TEST3_OP_ZPX (inc, 0xF6)
    TEST3_OP_ABS (inc, 0xEE)
    TEST3_OP_ABSX(inc, 0xFE)

    TEST3_OP_ZP  (dec, 0xC6)
    TEST3_OP_ZPX (dec, 0xD6)
    TEST3_OP_ABS (dec, 0xCE)
    TEST3_OP_ABSX(dec, 0xDE)

    // ---- jumps / subroutine ------------------------------------------------
    TEST3_OP_ABS (jmp, 0x4C)
    TEST3_OP_IND (jmp, 0x6C)
    Program& jmp(Label l,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        return jmp(Abs{l.addr}, file, line);
    }
    Program jmp(Label l,
                const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        jmp(l, file, line); return std::move(*this);
    }

    TEST3_OP_ABS (jsr, 0x20)
    Program& jsr(Label l,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        return jsr(Abs{l.addr}, file, line);
    }
    Program jsr(Label l,
                const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        jsr(l, file, line); return std::move(*this);
    }

    // ---- branches (8-bit signed PC-relative) -------------------------------
    TEST3_OP_BRANCH(bpl, 0x10)
    TEST3_OP_BRANCH(bmi, 0x30)
    TEST3_OP_BRANCH(bvc, 0x50)
    TEST3_OP_BRANCH(bvs, 0x70)
    TEST3_OP_BRANCH(bcc, 0x90)
    TEST3_OP_BRANCH(bcs, 0xB0)
    TEST3_OP_BRANCH(bne, 0xD0)
    TEST3_OP_BRANCH(beq, 0xF0)

#undef TEST3_OP_IMM
#undef TEST3_OP_ZP
#undef TEST3_OP_ZPX
#undef TEST3_OP_ZPY
#undef TEST3_OP_ABS
#undef TEST3_OP_ABSX
#undef TEST3_OP_ABSY
#undef TEST3_OP_IND
#undef TEST3_OP_INDX
#undef TEST3_OP_INDY
#undef TEST3_OP_BRANCH

    // BRK is a 2-byte instruction on the 6502: the CPU pushes PC+2
    // (skipping the byte at PC+1), so that signature byte is observable
    // on the bus during the "dummy fetch" cycle but never executes.
    // Conventionally the signature identifies the trap class to the
    // BRK/IRQ handler (Apple/Atari/Commodore all used this); the
    // default here ($42) is a recognizable marker, callers override
    // with `brk(Imm{0xNN})` when they need a specific code.
    Program& brk(Imm sig,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        byte_raw(0x00); byte_raw(sig.v); return *this;
    }
    Program brk(Imm sig,
                const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        brk(sig, file, line); return std::move(*this);
    }
    Program& brk(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        return brk(Imm{0x42}, file, line);
    }
    Program brk(const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        brk(file, line); return std::move(*this);
    }

    Program& nop(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        byte_raw(0xEA); return *this;
    }
    Program nop(const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        nop(file, line); return std::move(*this);
    }

    Program& rti(const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) & {
        tag(cursor_, file, line);
        byte_raw(0x40); return *this;
    }
    Program rti(const char* file = __builtin_FILE(),
                int         line = __builtin_LINE()) && {
        rti(file, line); return std::move(*this);
    }

    // ---- implied flag instructions (1 byte, 2 cycles) ---------------------
    //
    // Each toggles exactly one bit in P. Defined via a macro that follows
    // the same & / && ref-qualified pattern as brk/nop/rti.
#define TEST3_IMPLIED_1BYTE(name, opcode)                                   \
    Program& name(const char* file = __builtin_FILE(),                      \
                  int         line = __builtin_LINE()) & {                  \
        tag(cursor_, file, line);                                           \
        byte_raw(opcode); return *this;                                     \
    }                                                                       \
    Program name(const char* file = __builtin_FILE(),                       \
                 int         line = __builtin_LINE()) && {                  \
        name(file, line); return std::move(*this);                          \
    }
    TEST3_IMPLIED_1BYTE(clc, 0x18)   // clear carry
    TEST3_IMPLIED_1BYTE(sec, 0x38)   // set carry
    TEST3_IMPLIED_1BYTE(cli, 0x58)   // clear interrupt-disable
    TEST3_IMPLIED_1BYTE(sei, 0x78)   // set interrupt-disable
    TEST3_IMPLIED_1BYTE(clv, 0xB8)   // clear overflow
    TEST3_IMPLIED_1BYTE(cld, 0xD8)   // clear decimal
    TEST3_IMPLIED_1BYTE(sed, 0xF8)   // set decimal

    TEST3_IMPLIED_1BYTE(tax, 0xAA)   // transfer A -> X
    TEST3_IMPLIED_1BYTE(tay, 0xA8)   // transfer A -> Y
    TEST3_IMPLIED_1BYTE(txa, 0x8A)   // transfer X -> A
    TEST3_IMPLIED_1BYTE(tya, 0x98)   // transfer Y -> A
    TEST3_IMPLIED_1BYTE(tsx, 0xBA)   // transfer S -> X
    TEST3_IMPLIED_1BYTE(txs, 0x9A)   // transfer X -> S (no flag effect)

    TEST3_IMPLIED_1BYTE(inx, 0xE8)   // increment X
    TEST3_IMPLIED_1BYTE(iny, 0xC8)   // increment Y
    TEST3_IMPLIED_1BYTE(dex, 0xCA)   // decrement X
    TEST3_IMPLIED_1BYTE(dey, 0x88)   // decrement Y

    // Stack push/pull.
    TEST3_IMPLIED_1BYTE(pha, 0x48)
    TEST3_IMPLIED_1BYTE(pla, 0x68)
    TEST3_IMPLIED_1BYTE(php, 0x08)
    TEST3_IMPLIED_1BYTE(plp, 0x28)

    // Subroutine return.
    TEST3_IMPLIED_1BYTE(rts, 0x60)

    // Shift/rotate on accumulator (no-arg). Memory variants are
    // declared above via TEST3_OP_ZP/ZPX/ABS/ABSX.
    TEST3_IMPLIED_1BYTE(asl, 0x0A)
    TEST3_IMPLIED_1BYTE(lsr, 0x4A)
    TEST3_IMPLIED_1BYTE(rol, 0x2A)
    TEST3_IMPLIED_1BYTE(ror, 0x6A)
#undef TEST3_IMPLIED_1BYTE

    // ---- pseudo-ops: run terminators --------------------------------------
    //
    // pass()/fail() emit a 4-byte landing pad at the cursor: a NOP
    // followed by a 3-byte JMP-self. The JMP-self address is
    // registered as the run terminator -- when Cpu::run() sees an
    // opcode fetch (SYNC=1) at it, the run ends:
    //   pass() with RunResult{ok=true, message=msg}
    //   fail() with RunResult{ok=false, message=msg} plus an
    //         EXPECT(false, ...) so the failure participates in
    //         fail-fast / pass-count accounting.
    //
    // The leading NOP is a "landing pad" that lets the visible-register
    // state from a preceding instruction propagate into the captured
    // trace before the run ends (the aholme shim updates A/X/Y storage
    // one cycle later than the status flags, so a register write
    // immediately before pass() would otherwise be invisible). Tests
    // never have to add their own .nop() before pass() / fail().
    //
    // The lvalue (&) overloads return a LabelOrProgramRef -- a Label
    // pointing at the landing pad ENTRY (the NOP), so the terminator
    // remains a valid branch / vector target. Control reaches the NOP,
    // executes it harmlessly, then hits the JMP-self which fires the
    // terminator. The ref also forwards chainable emit ops back to *this:
    //
    //   prog.brk_handler(prog.org(0x0500).pass("..."));
    //   prog.brk().fail("BRK fell through").nop();
    //
    // The rvalue (&&) overloads return Program by value so a chain
    // off a temporary terminates cleanly:
    //
    //   Program prog = Program::start().brk().fail("BRK fell through");
    //
    // msg should be a string literal (or otherwise outlive the run);
    // it is not copied.
    LabelOrProgramRef pass(const char* msg  = "normal success",
                       const char* file = __builtin_FILE(),
                       int         line = __builtin_LINE()) & {
        Label l = emit_terminator(TermKind::Pass, msg, file, line);
        return LabelOrProgramRef{l, *this};
    }
    LabelOrProgramRef fail(const char* msg,
                       const char* file = __builtin_FILE(),
                       int         line = __builtin_LINE()) & {
        Label l = emit_terminator(TermKind::Fail, msg, file, line);
        return LabelOrProgramRef{l, *this};
    }
    // && overloads: return Program by value so a chain off a temporary
    // (e.g. `Program prog = Program::start().brk().fail(...);`) ends
    // with a Program suitable for move-assignment to `prog`. Inside
    // these bodies *this is an lvalue, so the unqualified pass()/fail()
    // resolves to the & overload above and emits exactly once.
    Program pass(const char* msg  = "normal success",
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) && {
        pass(msg, file, line); return std::move(*this);
    }
    Program fail(const char* msg,
                 const char* file = __builtin_FILE(),
                 int         line = __builtin_LINE()) && {
        fail(msg, file, line); return std::move(*this);
    }

    // ---- vectors / handlers ------------------------------------------------
    //
    // reset_handler, brk_handler, nmi_handler declare the post-reset
    // entry ($FFFC/$FFFD), the BRK/IRQ handler ($FFFE/$FFFF), and the
    // NMI handler ($FFFA/$FFFB) respectively.
    //
    // reset_handler: Program::start(addr) writes a DEFAULT entry of
    // `addr` without flagging it as user-set; a later explicit
    // reset_handler(...) is treated as the test's first declaration
    // and tags it at the test's source line. A second explicit
    // reset_handler() fires the must-set-exactly-once diagnostic.
    //
    // brk_handler / nmi_handler: Program::start() pre-wires both to
    // fail() traps in high RAM so unexpected interrupts surface as
    // FAIL with a clear message; tests override by calling
    // brk_handler / nmi_handler.
    //
    // Each handler has six overloads:
    //
    //   handler(uint16_t pc)  raw address; ALSO moves the cursor to pc
    //                         so the next emit defines the handler body:
    //                             prog.brk_handler(0x0500).rti();
    //   handler(Label l)      existing Label; does NOT move the cursor
    //                         (the Label typically points at code
    //                         already emitted, e.g.
    //                             prog.brk_handler(prog.org(0x0500).pass("..."))
    //                         where the cursor sits past the trap).
    //   handler()             no arg: vector -> current cursor. Lets
    //                         tests declare the handler inline at the
    //                         current position:
    //                             prog.org(0x0500).brk_handler().rti();
    //   ... and `&` / `&&` ref-qualified variants of each.
    //
    // Each form returns a LabelOrProgramRef so the call can chain on
    // either the handler address (implicit Label conversion) or
    // further emit ops on the Program.

    LabelOrProgramRef reset_handler(uint16_t pc,
                                    const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE()) & {
        patch_reset_vector(pc, file, line);
        cursor_ = pc;
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program reset_handler(uint16_t pc,
                          const char* file = __builtin_FILE(),
                          int         line = __builtin_LINE()) && {
        reset_handler(pc, file, line); return std::move(*this);
    }

    LabelOrProgramRef reset_handler(Label l,
                                    const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE()) & {
        patch_reset_vector(l.addr, file, line);
        return LabelOrProgramRef{l, *this};
    }
    Program reset_handler(Label l,
                          const char* file = __builtin_FILE(),
                          int         line = __builtin_LINE()) && {
        reset_handler(l, file, line); return std::move(*this);
    }

    // No-arg form: declare the reset entry at the current cursor.
    LabelOrProgramRef reset_handler(const char* file = __builtin_FILE(),
                                    int         line = __builtin_LINE()) & {
        const uint16_t pc = cursor_;
        patch_reset_vector(pc, file, line);
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program reset_handler(const char* file = __builtin_FILE(),
                          int         line = __builtin_LINE()) && {
        reset_handler(file, line); return std::move(*this);
    }

    LabelOrProgramRef brk_handler(uint16_t pc,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        patch_vector(0xFFFE, pc, file, line);
        cursor_ = pc;
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program brk_handler(uint16_t pc,
                        const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        brk_handler(pc, file, line); return std::move(*this);
    }

    LabelOrProgramRef brk_handler(Label l,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        patch_vector(0xFFFE, l.addr, file, line);
        return LabelOrProgramRef{l, *this};
    }
    Program brk_handler(Label l,
                        const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        brk_handler(l, file, line); return std::move(*this);
    }

    // No-arg form: declare the BRK handler at the current cursor.
    // Patches $FFFE/$FFFF to point at `cursor_` and returns a
    // LabelOrProgramRef so the handler body can chain directly:
    //
    //     prog.org(0x0500).brk_handler().rti();
    //
    // Cursor is unchanged (it already points at the handler entry).
    LabelOrProgramRef brk_handler(const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        const uint16_t pc = cursor_;
        patch_vector(0xFFFE, pc, file, line);
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program brk_handler(const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        brk_handler(file, line); return std::move(*this);
    }

    LabelOrProgramRef nmi_handler(uint16_t pc,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        patch_vector(0xFFFA, pc, file, line);
        cursor_ = pc;
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program nmi_handler(uint16_t pc,
                        const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        nmi_handler(pc, file, line); return std::move(*this);
    }

    LabelOrProgramRef nmi_handler(Label l,
                                  const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        patch_vector(0xFFFA, l.addr, file, line);
        return LabelOrProgramRef{l, *this};
    }
    Program nmi_handler(Label l,
                        const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        nmi_handler(l, file, line); return std::move(*this);
    }

    // No-arg form: declare the NMI handler at the current cursor.
    LabelOrProgramRef nmi_handler(const char* file = __builtin_FILE(),
                                  int         line = __builtin_LINE()) & {
        const uint16_t pc = cursor_;
        patch_vector(0xFFFA, pc, file, line);
        return LabelOrProgramRef{Label{pc}, *this};
    }
    Program nmi_handler(const char* file = __builtin_FILE(),
                        int         line = __builtin_LINE()) && {
        nmi_handler(file, line); return std::move(*this);
    }

    // ---- labels / scopes ---------------------------------------------------

    // Capture the current cursor for backward references.
    Label label() const { return Label{cursor_}; }

    // RAII cursor scope: moves to `addr`, restores on scope exit.
    CursorScope at(uint16_t addr) { return CursorScope{this, addr}; }

    // ---- accessors (non-fluent) --------------------------------------------
    uint8_t        read(uint16_t addr) const { return image_[addr]; }
    Program&       write(uint16_t addr, uint8_t v,
                         const char* file = __builtin_FILE(),
                         int         line = __builtin_LINE()) & {
        tag(addr, file, line);
        image_[addr] = v;
        return *this;
    }
    Program        write(uint16_t addr, uint8_t v,
                         const char* file = __builtin_FILE(),
                         int         line = __builtin_LINE()) && {
        write(addr, v, file, line); return std::move(*this);
    }
    uint16_t       cursor() const { return cursor_; }
    const uint8_t* data() const { return image_; }
    uint8_t*       data()       { return image_; }

    // Look up the source location of the emit that wrote at or before
    // `addr`. Returns {nullptr, 0} if no emit has tagged at or below
    // `addr`. Lookup is upper_bound + --it, so the byte at $0401
    // (operand of a two-byte instruction at $0400) inherits the
    // tag from $0400.
    SrcLoc src_for(uint16_t addr) const {
        if (src_map_.empty()) return {nullptr, 0};
        auto it = src_map_.upper_bound(addr);
        if (it == src_map_.begin()) return {nullptr, 0};
        --it;
        return it->second;
    }

    // Look up the run terminator registered at `addr` by pass()/fail().
    // Returns nullptr if no terminator was declared at that exact addr.
    // Cpu::run() calls this on every opcode fetch (SYNC=1).
    const Terminator* terminator_for(uint16_t addr) const {
        auto it = term_map_.find(addr);
        return it == term_map_.end() ? nullptr : &it->second;
    }

private:
    friend class CursorScope;

    // Default ctor is private; use Program::start(addr) (or future
    // sibling factories) to make a Program. This makes "where does
    // execution begin?" an explicit, single-call decision.
    Program() { std::memset(image_, 0, MEM_SIZE); }

    // Internal byte writer: advances the cursor without tagging. Used
    // by every public emit method after a single tag() call, so each
    // call-site contributes exactly one src_map_ entry.
    Program& byte_raw(uint8_t b) {
        if (!EXPECT(cursor_ < MEM_SIZE,
                    "Program::byte: cursor must be within MEM_SIZE "
                    "(cursor=$%05X, MEM_SIZE=$%05X)",
                    (unsigned)cursor_, (unsigned)MEM_SIZE)) return *this;
        image_[cursor_++] = b;
        return *this;
    }

    // Record `(addr -> {file, line})` in src_map_. Idempotent at the
    // same address (first write wins) so re-emitting at the same
    // address from a different call site doesn't shadow the original.
    void tag(uint16_t addr, const char* file, int line) {
        src_map_.emplace(addr, SrcLoc{file, line});
    }

    // Emit a landing-pad NOP followed by a 3-byte JMP-self, and register
    // the JMP-self address as a run terminator. The landing-pad NOP
    // exists so the visible-register state from a preceding instruction
    // propagates into the captured trace before the run ends: the
    // aholme shim updates A/X/Y storage one cycle later than the status
    // flags, so without the NOP a test that writes a register and then
    // calls pass() would never see the register update in its trace.
    //
    // The returned Label points at the NOP entry (not the JMP-self), so
    // it remains a valid vector target: control reaches the NOP first,
    // executes it harmlessly, then hits the JMP-self which triggers the
    // terminator. Used by pass() and fail().
    Label emit_terminator(TermKind kind, const char* msg,
                          const char* file, int line) {
        const uint16_t entry = cursor_;
        tag(entry, file, line);
        byte_raw(0xEA);                            // landing-pad NOP
        const uint16_t trap_addr = cursor_;
        byte_raw(0x4C);                            // JMP-self
        byte_raw(static_cast<uint8_t>(trap_addr & 0xFF));
        byte_raw(static_cast<uint8_t>(trap_addr >> 8));
        // first declaration wins if the same address is reused
        term_map_.emplace(trap_addr, Terminator{kind, msg, {file, line}});
        return Label{entry};
    }

    // Shared vector-patch helper. `vec_lo` is the low byte address of
    // the 2-byte big-endian-stored vector (e.g. $FFFE for BRK/IRQ).
    // Does NOT touch the cursor; the public set_*_vector(uint16_t)
    // overloads do that separately.
    void patch_vector(uint16_t vec_lo, uint16_t pc,
                      const char* file, int line) {
        tag(vec_lo, file, line);
        image_[vec_lo    ] = static_cast<uint8_t>(pc & 0xFF);
        image_[vec_lo + 1] = static_cast<uint8_t>(pc >> 8);
    }

    // Reset vector has its own helper because of the must-set-exactly-once
    // guard; this used to live in reset_handler(uint16_t) but the
    // Label-taking overload now needs the same check.
    void patch_reset_vector(uint16_t pc, const char* file, int line) {
        if (reset_vector_set_) {
            SrcLoc prior = src_for(0xFFFC);
            EXPECT(false,
                   "reset vector must be set exactly once "
                   "(already set at %s:%d)",
                   prior.file ? prior.file : "?", prior.line);
        }
        patch_vector(0xFFFC, pc, file, line);
        reset_vector_set_ = true;
    }

    uint8_t                          image_[MEM_SIZE];
    uint16_t                         cursor_ = 0;
    bool                             reset_vector_set_ = false;
    std::map<uint16_t, SrcLoc>       src_map_;
    std::map<uint16_t, Terminator>   term_map_;
};

// CursorScope ctor/dtor defined out-of-line because they touch
// Program internals.
inline CursorScope::CursorScope(Program* p, uint16_t addr)
    : p_(p), saved_(p->cursor_) {
    p_->cursor_ = addr;
}

inline CursorScope::~CursorScope() {
    p_->cursor_ = saved_;
}

// ---- LabelOrProgramRef forwarders (defined out-of-line because Program
// must be complete). Each just delegates to the underlying Program&
// with the explicit file/line so the source-line tag matches the
// chained call site rather than this header. -------------------------
inline Program& LabelOrProgramRef::byte(uint8_t b, const char* file, int line) {
    return prog_.byte(b, file, line);
}
inline Program& LabelOrProgramRef::word(uint16_t w, const char* file, int line) {
    return prog_.word(w, file, line);
}
inline Program& LabelOrProgramRef::bytes(std::initializer_list<uint8_t> bs,
                                     const char* file, int line) {
    return prog_.bytes(bs, file, line);
}
inline Program& LabelOrProgramRef::string(std::string_view s,
                                      const char* file, int line) {
    return prog_.string(s, file, line);
}
inline Program& LabelOrProgramRef::fill(size_t count, uint8_t value,
                                    const char* file, int line) {
    return prog_.fill(count, value, file, line);
}
inline Program& LabelOrProgramRef::lda(Imm op, const char* file, int line) {
    return prog_.lda(op, file, line);
}
inline Program& LabelOrProgramRef::sta(ZP op, const char* file, int line) {
    return prog_.sta(op, file, line);
}
inline Program& LabelOrProgramRef::jmp(Abs op, const char* file, int line) {
    return prog_.jmp(op, file, line);
}
inline Program& LabelOrProgramRef::jmp(Label l, const char* file, int line) {
    return prog_.jmp(l, file, line);
}

#define TEST3_FWD_DEF_OP(name, OP)                                                 \
    inline Program& LabelOrProgramRef::name(OP op, const char* file, int line) {   \
        return prog_.name(op, file, line);                                         \
    }
#define TEST3_FWD_DEF_BR(name)                                                     \
    inline Program& LabelOrProgramRef::name(Label l, const char* file, int line) { \
        return prog_.name(l, file, line);                                          \
    }

TEST3_FWD_DEF_OP(lda, ZP)   TEST3_FWD_DEF_OP(lda, ZPX)  TEST3_FWD_DEF_OP(lda, Abs)
TEST3_FWD_DEF_OP(lda, AbsX) TEST3_FWD_DEF_OP(lda, AbsY) TEST3_FWD_DEF_OP(lda, IndX) TEST3_FWD_DEF_OP(lda, IndY)
TEST3_FWD_DEF_OP(ldx, Imm)  TEST3_FWD_DEF_OP(ldx, ZP)   TEST3_FWD_DEF_OP(ldx, ZPY)  TEST3_FWD_DEF_OP(ldx, Abs) TEST3_FWD_DEF_OP(ldx, AbsY)
TEST3_FWD_DEF_OP(ldy, Imm)  TEST3_FWD_DEF_OP(ldy, ZP)   TEST3_FWD_DEF_OP(ldy, ZPX)  TEST3_FWD_DEF_OP(ldy, Abs) TEST3_FWD_DEF_OP(ldy, AbsX)
TEST3_FWD_DEF_OP(sta, ZPX)  TEST3_FWD_DEF_OP(sta, Abs)  TEST3_FWD_DEF_OP(sta, AbsX)
TEST3_FWD_DEF_OP(sta, AbsY) TEST3_FWD_DEF_OP(sta, IndX) TEST3_FWD_DEF_OP(sta, IndY)
TEST3_FWD_DEF_OP(stx, ZP)   TEST3_FWD_DEF_OP(stx, ZPY)  TEST3_FWD_DEF_OP(stx, Abs)
TEST3_FWD_DEF_OP(sty, ZP)   TEST3_FWD_DEF_OP(sty, ZPX)  TEST3_FWD_DEF_OP(sty, Abs)
TEST3_FWD_DEF_OP(adc, Imm)  TEST3_FWD_DEF_OP(adc, ZP)   TEST3_FWD_DEF_OP(adc, ZPX)  TEST3_FWD_DEF_OP(adc, Abs)
TEST3_FWD_DEF_OP(adc, AbsX) TEST3_FWD_DEF_OP(adc, AbsY) TEST3_FWD_DEF_OP(adc, IndX) TEST3_FWD_DEF_OP(adc, IndY)
TEST3_FWD_DEF_OP(sbc, Imm)  TEST3_FWD_DEF_OP(sbc, ZP)   TEST3_FWD_DEF_OP(sbc, ZPX)  TEST3_FWD_DEF_OP(sbc, Abs)
TEST3_FWD_DEF_OP(sbc, AbsX) TEST3_FWD_DEF_OP(sbc, AbsY) TEST3_FWD_DEF_OP(sbc, IndX) TEST3_FWD_DEF_OP(sbc, IndY)
TEST3_FWD_DEF_OP(and_, Imm)  TEST3_FWD_DEF_OP(and_, ZP)   TEST3_FWD_DEF_OP(and_, ZPX)  TEST3_FWD_DEF_OP(and_, Abs)
TEST3_FWD_DEF_OP(and_, AbsX) TEST3_FWD_DEF_OP(and_, AbsY) TEST3_FWD_DEF_OP(and_, IndX) TEST3_FWD_DEF_OP(and_, IndY)
TEST3_FWD_DEF_OP(ora, Imm)  TEST3_FWD_DEF_OP(ora, ZP)   TEST3_FWD_DEF_OP(ora, ZPX)  TEST3_FWD_DEF_OP(ora, Abs)
TEST3_FWD_DEF_OP(ora, AbsX) TEST3_FWD_DEF_OP(ora, AbsY) TEST3_FWD_DEF_OP(ora, IndX) TEST3_FWD_DEF_OP(ora, IndY)
TEST3_FWD_DEF_OP(eor, Imm)  TEST3_FWD_DEF_OP(eor, ZP)   TEST3_FWD_DEF_OP(eor, ZPX)  TEST3_FWD_DEF_OP(eor, Abs)
TEST3_FWD_DEF_OP(eor, AbsX) TEST3_FWD_DEF_OP(eor, AbsY) TEST3_FWD_DEF_OP(eor, IndX) TEST3_FWD_DEF_OP(eor, IndY)
TEST3_FWD_DEF_OP(bit, ZP)   TEST3_FWD_DEF_OP(bit, Abs)
TEST3_FWD_DEF_OP(cmp, Imm)  TEST3_FWD_DEF_OP(cmp, ZP)   TEST3_FWD_DEF_OP(cmp, ZPX)  TEST3_FWD_DEF_OP(cmp, Abs)
TEST3_FWD_DEF_OP(cmp, AbsX) TEST3_FWD_DEF_OP(cmp, AbsY) TEST3_FWD_DEF_OP(cmp, IndX) TEST3_FWD_DEF_OP(cmp, IndY)
TEST3_FWD_DEF_OP(cpx, Imm)  TEST3_FWD_DEF_OP(cpx, ZP)   TEST3_FWD_DEF_OP(cpx, Abs)
TEST3_FWD_DEF_OP(cpy, Imm)  TEST3_FWD_DEF_OP(cpy, ZP)   TEST3_FWD_DEF_OP(cpy, Abs)
TEST3_FWD_DEF_OP(asl, ZP)   TEST3_FWD_DEF_OP(asl, ZPX)  TEST3_FWD_DEF_OP(asl, Abs)  TEST3_FWD_DEF_OP(asl, AbsX)
TEST3_FWD_DEF_OP(lsr, ZP)   TEST3_FWD_DEF_OP(lsr, ZPX)  TEST3_FWD_DEF_OP(lsr, Abs)  TEST3_FWD_DEF_OP(lsr, AbsX)
TEST3_FWD_DEF_OP(rol, ZP)   TEST3_FWD_DEF_OP(rol, ZPX)  TEST3_FWD_DEF_OP(rol, Abs)  TEST3_FWD_DEF_OP(rol, AbsX)
TEST3_FWD_DEF_OP(ror, ZP)   TEST3_FWD_DEF_OP(ror, ZPX)  TEST3_FWD_DEF_OP(ror, Abs)  TEST3_FWD_DEF_OP(ror, AbsX)
TEST3_FWD_DEF_OP(inc, ZP)   TEST3_FWD_DEF_OP(inc, ZPX)  TEST3_FWD_DEF_OP(inc, Abs)  TEST3_FWD_DEF_OP(inc, AbsX)
TEST3_FWD_DEF_OP(dec, ZP)   TEST3_FWD_DEF_OP(dec, ZPX)  TEST3_FWD_DEF_OP(dec, Abs)  TEST3_FWD_DEF_OP(dec, AbsX)
TEST3_FWD_DEF_OP(jmp, Ind)
TEST3_FWD_DEF_OP(jsr, Abs)  TEST3_FWD_DEF_BR(jsr)
TEST3_FWD_DEF_BR(bpl) TEST3_FWD_DEF_BR(bmi) TEST3_FWD_DEF_BR(bvc) TEST3_FWD_DEF_BR(bvs)
TEST3_FWD_DEF_BR(bcc) TEST3_FWD_DEF_BR(bcs) TEST3_FWD_DEF_BR(bne) TEST3_FWD_DEF_BR(beq)

#undef TEST3_FWD_DEF_OP
#undef TEST3_FWD_DEF_BR
inline Program& LabelOrProgramRef::brk(Imm sig, const char* file, int line) {
    return prog_.brk(sig, file, line);
}
inline Program& LabelOrProgramRef::brk(const char* file, int line) {
    return prog_.brk(file, line);
}
inline Program& LabelOrProgramRef::nop(const char* file, int line) {
    return prog_.nop(file, line);
}
inline Program& LabelOrProgramRef::rti(const char* file, int line) {
    return prog_.rti(file, line);
}
#define TEST3_FWD_IMPLIED(name)                                            \
    inline Program& LabelOrProgramRef::name(const char* file, int line) {  \
        return prog_.name(file, line);                                     \
    }
TEST3_FWD_IMPLIED(clc)
TEST3_FWD_IMPLIED(sec)
TEST3_FWD_IMPLIED(cli)
TEST3_FWD_IMPLIED(sei)
TEST3_FWD_IMPLIED(clv)
TEST3_FWD_IMPLIED(cld)
TEST3_FWD_IMPLIED(sed)
TEST3_FWD_IMPLIED(tax)
TEST3_FWD_IMPLIED(tay)
TEST3_FWD_IMPLIED(txa)
TEST3_FWD_IMPLIED(tya)
TEST3_FWD_IMPLIED(tsx)
TEST3_FWD_IMPLIED(txs)
TEST3_FWD_IMPLIED(inx)
TEST3_FWD_IMPLIED(iny)
TEST3_FWD_IMPLIED(dex)
TEST3_FWD_IMPLIED(dey)
TEST3_FWD_IMPLIED(pha)
TEST3_FWD_IMPLIED(pla)
TEST3_FWD_IMPLIED(php)
TEST3_FWD_IMPLIED(plp)
TEST3_FWD_IMPLIED(rts)
TEST3_FWD_IMPLIED(asl)
TEST3_FWD_IMPLIED(lsr)
TEST3_FWD_IMPLIED(rol)
TEST3_FWD_IMPLIED(ror)
#undef TEST3_FWD_IMPLIED
inline LabelOrProgramRef LabelOrProgramRef::pass(const char* msg,
                                         const char* file, int line) {
    return prog_.pass(msg, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::fail(const char* msg,
                                         const char* file, int line) {
    return prog_.fail(msg, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::org(uint16_t addr) {
    return prog_.org(addr);
}
inline Program& LabelOrProgramRef::write(uint16_t addr, uint8_t v,
                                     const char* file, int line) {
    return prog_.write(addr, v, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::brk_handler(uint16_t pc,
                                                       const char* file, int line) {
    return prog_.brk_handler(pc, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::brk_handler(Label l,
                                                       const char* file, int line) {
    return prog_.brk_handler(l, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::brk_handler(const char* file, int line) {
    return prog_.brk_handler(file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::nmi_handler(uint16_t pc,
                                                       const char* file, int line) {
    return prog_.nmi_handler(pc, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::nmi_handler(Label l,
                                                       const char* file, int line) {
    return prog_.nmi_handler(l, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::nmi_handler(const char* file, int line) {
    return prog_.nmi_handler(file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::reset_handler(uint16_t pc,
                                                          const char* file, int line) {
    return prog_.reset_handler(pc, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::reset_handler(Label l,
                                                          const char* file, int line) {
    return prog_.reset_handler(l, file, line);
}
inline LabelOrProgramRef LabelOrProgramRef::reset_handler(const char* file, int line) {
    return prog_.reset_handler(file, line);
}

} // namespace test3
