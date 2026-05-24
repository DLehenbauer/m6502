// test3/src/expect.cpp
//
// Runtime support for the EXPECT() macro: pass/fail reporters,
// failure dispatcher, and the verbose/fail-fast accessors.

#include "test3/expect.h"
#include "test3/test.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace test3 {

// --- runtime mode storage (set by main.cpp via --fail-fast / -v) -------

static bool g_fail_fast = false;
static bool g_verbose   = false;

void set_fail_fast(bool v) { g_fail_fast = v; }
bool fail_fast()           { return g_fail_fast; }

void set_verbose(bool v) { g_verbose = v; }
bool verbose()           { return g_verbose; }

namespace detail {

bool verbose() { return ::test3::verbose(); }

// Fixed buffer size for one formatted message. Plenty for the
// spec-style invariant strings we use; on overflow we leave a
// trailing "..." marker so authors notice.
constexpr int kMsgBufSize = 512;

// Format `fmt`+args into `buf`. Returns nothing; on overflow the
// last three printable bytes are replaced with "...".
static void format_msg(char* buf, int buf_size, const char* fmt, va_list ap) {
    int n = std::vsnprintf(buf, static_cast<size_t>(buf_size), fmt, ap);
    if (n < 0) {
        // Encoding failure -- leave an empty string rather than UB.
        buf[0] = '\0';
        return;
    }
    if (n >= buf_size) {
        // Truncated. vsnprintf guarantees a NUL at buf[buf_size-1].
        if (buf_size >= 4) {
            buf[buf_size - 4] = '.';
            buf[buf_size - 3] = '.';
            buf[buf_size - 2] = '.';
            buf[buf_size - 1] = '\0';
        }
    }
}

// Single place that decides what "failure" means: either abort the
// process (--fail-fast) or mark the current test failed and let it
// continue.
static void handle_failure(const char* formatted_msg) {
    if (fail_fast()) {
        std::abort();
    }
    mark_failed(formatted_msg);
}

void report_pass(const char* fmt, ...) {
    char buf[kMsgBufSize];
    va_list ap;
    va_start(ap, fmt);
    format_msg(buf, kMsgBufSize, fmt, ap);
    va_end(ap);
    std::fprintf(stderr, "OK:   %s\n", buf);
}

void report_fail(const char* file, int line, const char* func,
                 const char* cond_text,
                 const char* fmt, ...) {
    char buf[kMsgBufSize];
    va_list ap;
    va_start(ap, fmt);
    format_msg(buf, kMsgBufSize, fmt, ap);
    va_end(ap);

    std::fprintf(stderr,
                 "FAIL: %s\n"
                 "  cond: %s\n"
                 "  in:   %s (%s:%d)\n",
                 buf, cond_text, func, file, line);

    handle_failure(buf);
}

} // namespace detail
} // namespace test3
