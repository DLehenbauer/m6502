// test3/include/test3/expect.h
//
// Single-macro assertion facility for the test3 framework.
//
// Usage:
//
//     EXPECT(cond, "spec-style invariant statement");
//     EXPECT(cond, "spec-style invariant with %s context", str);
//
// EXPECT is also an expression that evaluates to `cond`, so callers
// can short-circuit on failure without restating the condition:
//
//     if (!EXPECT(ptr != nullptr, "ptr must be non-null")) return;
//
// Messages MUST be written as positive, present-tense statements of
// the invariant that holds when `cond` is true. The same string then
// reads sensibly with both an `OK:` prefix (in --verbose mode, on
// success) and a `FAIL:` prefix (on failure).
//
// Good:
//   EXPECT(ok, "canonical reset trace must match shim '%s'", shim_.name());
// Bad:
//   EXPECT(ok, "trace mismatch");          // describes the failure
//   EXPECT(ok, "ok must be true");         // restates the expression
//
// Runtime flags (parsed in main.cpp):
//   --fail-fast        abort on first failure (else mark_failed + continue)
//   -v / --verbose     print "OK: <message>" for each successful EXPECT
//
// Semantics:
//   - `cond` is evaluated exactly once.
//   - On success: nothing, unless --verbose, in which case "OK: <msg>".
//   - On failure: prints a FAIL block, then either std::abort()
//     (--fail-fast) or mark_failed(<formatted-msg>).
//   - EXPECT is an expression of type bool equal to `cond`. Callers
//     that must bail out after a failed precondition can write
//     `if (!EXPECT(...)) return;` instead of repeating the condition.

#pragma once

namespace test3 {
namespace detail {

// Read the global verbose flag (set by main.cpp via --verbose / -v).
bool verbose();

// Called on every successful EXPECT *only when verbose()* is true.
// Formats once into a fixed buffer and prints "OK:   <message>".
void report_pass(const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

// Called on every failed EXPECT. Formats the message once into a
// fixed buffer; prints:
//     FAIL: <formatted message>
//       cond: <cond_text>
//       in:   <func> (<file>:<line>)
// then dispatches to handle_failure(<formatted-msg>), which either
// std::abort()s (--fail-fast) or marks the current test as failed.
void report_fail(const char* file, int line, const char* func,
                 const char* cond_text,
                 const char* fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 5, 6)))
#endif
    ;

} // namespace detail
} // namespace test3

// Always-void single-severity assertion. The message is mandatory;
// printf-style varargs after the format string are optional.
//
// Implemented as a GCC statement-expression so the whole thing has a
// value equal to `cond` (after a single evaluation). This lets callers
// short-circuit without restating the condition:
//
//     if (!EXPECT(ptr != nullptr, "ptr must be non-null")) return;
//
// The discarded-value form is also fine:
//
//     EXPECT(cond, "...");
//
// `__func__` inside the block expands to the enclosing function's
// name (a lambda-based implementation would expand it to "operator()").
//
// The format-string + args are only touched on failure or in verbose
// mode, so cheap argument expressions stay cheap on the hot path.
#define EXPECT(cond, ...)                                                  \
    __extension__ ({                                                       \
        const bool _t3_ok = (cond);                                        \
        if (!_t3_ok) {                                                     \
            ::test3::detail::report_fail(                                  \
                __FILE__, __LINE__, __func__, #cond, __VA_ARGS__);         \
        } else if (::test3::detail::verbose()) {                           \
            ::test3::detail::report_pass(__VA_ARGS__);                     \
        }                                                                  \
        _t3_ok;                                                            \
    })
