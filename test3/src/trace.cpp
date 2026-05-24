// test3/src/trace.cpp

#include "test3/trace.h"
#include "test3/program.h"

#include <cstdio>
#include <cstring>

namespace test3 {

// Column header line that precedes a table (or table section). Public
// so tests can place it under their section delimiters when hand-
// authoring multi-section traces. Indent matches the table's `    `.
static const char* k_table_header_regs =
    "        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC";
static const char* k_table_header_no_regs =
    "        //    cycle      addr, data, rw, sync,     (no registers from this shim)";

static const char* rw_str(uint8_t rw) { return rw == R ? "R" : "W"; }

// 6502 opcode mnemonic table for trace annotation. Indexed by opcode
// byte; entries are 3-char ASCII mnemonics for documented opcodes and
// nullptr for undocumented/illegal NMOS opcodes (the printer falls
// back to "$XX" in that case).
static const char* const k_mnemonic[256] = {
    /* 00 */ "BRK", "ORA", nullptr, nullptr, nullptr, "ORA", "ASL", nullptr,
    /* 08 */ "PHP", "ORA", "ASL", nullptr, nullptr, "ORA", "ASL", nullptr,
    /* 10 */ "BPL", "ORA", nullptr, nullptr, nullptr, "ORA", "ASL", nullptr,
    /* 18 */ "CLC", "ORA", nullptr, nullptr, nullptr, "ORA", "ASL", nullptr,
    /* 20 */ "JSR", "AND", nullptr, nullptr, "BIT", "AND", "ROL", nullptr,
    /* 28 */ "PLP", "AND", "ROL", nullptr, "BIT", "AND", "ROL", nullptr,
    /* 30 */ "BMI", "AND", nullptr, nullptr, nullptr, "AND", "ROL", nullptr,
    /* 38 */ "SEC", "AND", nullptr, nullptr, nullptr, "AND", "ROL", nullptr,
    /* 40 */ "RTI", "EOR", nullptr, nullptr, nullptr, "EOR", "LSR", nullptr,
    /* 48 */ "PHA", "EOR", "LSR", nullptr, "JMP", "EOR", "LSR", nullptr,
    /* 50 */ "BVC", "EOR", nullptr, nullptr, nullptr, "EOR", "LSR", nullptr,
    /* 58 */ "CLI", "EOR", nullptr, nullptr, nullptr, "EOR", "LSR", nullptr,
    /* 60 */ "RTS", "ADC", nullptr, nullptr, nullptr, "ADC", "ROR", nullptr,
    /* 68 */ "PLA", "ADC", "ROR", nullptr, "JMP", "ADC", "ROR", nullptr,
    /* 70 */ "BVS", "ADC", nullptr, nullptr, nullptr, "ADC", "ROR", nullptr,
    /* 78 */ "SEI", "ADC", nullptr, nullptr, nullptr, "ADC", "ROR", nullptr,
    /* 80 */ nullptr, "STA", nullptr, nullptr, "STY", "STA", "STX", nullptr,
    /* 88 */ "DEY", nullptr, "TXA", nullptr, "STY", "STA", "STX", nullptr,
    /* 90 */ "BCC", "STA", nullptr, nullptr, "STY", "STA", "STX", nullptr,
    /* 98 */ "TYA", "STA", "TXS", nullptr, nullptr, "STA", nullptr, nullptr,
    /* A0 */ "LDY", "LDA", "LDX", nullptr, "LDY", "LDA", "LDX", nullptr,
    /* A8 */ "TAY", "LDA", "TAX", nullptr, "LDY", "LDA", "LDX", nullptr,
    /* B0 */ "BCS", "LDA", nullptr, nullptr, "LDY", "LDA", "LDX", nullptr,
    /* B8 */ "CLV", "LDA", "TSX", nullptr, "LDY", "LDA", "LDX", nullptr,
    /* C0 */ "CPY", "CMP", nullptr, nullptr, "CPY", "CMP", "DEC", nullptr,
    /* C8 */ "INY", "CMP", "DEX", nullptr, "CPY", "CMP", "DEC", nullptr,
    /* D0 */ "BNE", "CMP", nullptr, nullptr, nullptr, "CMP", "DEC", nullptr,
    /* D8 */ "CLD", "CMP", nullptr, nullptr, nullptr, "CMP", "DEC", nullptr,
    /* E0 */ "CPX", "SBC", nullptr, nullptr, "CPX", "SBC", "INC", nullptr,
    /* E8 */ "INX", "SBC", "NOP", nullptr, "CPX", "SBC", "INC", nullptr,
    /* F0 */ "BEQ", "SBC", nullptr, nullptr, nullptr, "SBC", "INC", nullptr,
    /* F8 */ "SED", "SBC", nullptr, nullptr, nullptr, "SBC", "INC", nullptr,
};

// Print one row. `index` is the row's half-cycle index (its position in
// the trace array); the printed `/* N.X */` prefix decodes it as
// cycle N, phase 0 (=phi1) or 5 (=phi2). Column widths are padded so
// rows align under the table header above (cycle %4d.%c for cycles up
// to 9999.5; rw %2s; sync %4u). `as_table_row` controls the leading
// "    " indent for paste-block output vs inline diff lines.
// `fields_mask` determines whether to print real register values or
// placeholders.
static void print_row(FILE* out, const TraceRow& r, size_t index,
                      uint32_t fields_mask, bool as_table_row,
                      const char* trailing_comment = nullptr) {
    const unsigned long long cyc   = (unsigned long long)(index / 2);
    const char               phase = (index & 1) ? '5' : '0';
    const char* indent = as_table_row ? "        " : "";
    const char* tail_sep = trailing_comment ? "  // " : "";
    const char* tail     = trailing_comment ? trailing_comment : "";
    if (fields_mask & TF_REGS) {
        std::fprintf(out,
            "%s{ /* %4llu.%c */ 0x%04X, 0x%02X, %2s, %4u, "
            "0x%04X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, "
            "0b%c%c%c%c%c%c%c%c },%s%s\n",
            indent, cyc, phase, r.addr, r.data,
            rw_str(r.rw), (unsigned)r.sync,
            r.pc, r.a, r.x, r.y, r.s,
            (r.p & 0x80) ? '1' : '0',
            (r.p & 0x40) ? '1' : '0',
            (r.p & 0x20) ? '1' : '0',
            (r.p & 0x10) ? '1' : '0',
            (r.p & 0x08) ? '1' : '0',
            (r.p & 0x04) ? '1' : '0',
            (r.p & 0x02) ? '1' : '0',
            (r.p & 0x01) ? '1' : '0',
            tail_sep, tail);
    } else {
        std::fprintf(out,
            "%s{ /* %4llu.%c */ 0x%04X, 0x%02X, %2s, %4u, "
            "______, _,    _,    _,    _,    __________ },%s%s\n",
            indent, cyc, phase, r.addr, r.data,
            rw_str(r.rw), (unsigned)r.sync,
            tail_sep, tail);
    }
}

// Per-row trailing comment naming what the cycle is doing:
// SYNC fetch, dummy/operand, write, push, stack read, vector fetch.
// `next_data` is the next row's data byte (used to name the opcode on
// SYNC-phase-0 rows; the opcode byte appears on the trailing phase-5).
static void row_comment(char* buf, size_t buflen,
                        const TraceRow& r, size_t index, uint8_t next_data) {
    const bool phase0 = (index & 1) == 0;
    auto mnem = [](uint8_t op) -> const char* {
        return k_mnemonic[op] ? k_mnemonic[op] : nullptr;
    };
    if (r.sync && phase0) {
        const char* m = mnem(next_data);
        if (m) std::snprintf(buf, buflen, "SYNC: %s", m);
        else   std::snprintf(buf, buflen, "SYNC: $%02X", next_data);
    } else if (r.sync && !phase0) {
        const char* m = mnem(r.data);
        if (m) std::snprintf(buf, buflen, "$%02X = %s", r.data, m);
        else   std::snprintf(buf, buflen, "$%02X", r.data);
    } else if (r.rw == W) {
        if (r.addr >= 0x0100 && r.addr <= 0x01FF) {
            std::snprintf(buf, buflen, "push $%02X -> stack", r.data);
        } else {
            std::snprintf(buf, buflen, "write $%02X -> $%04X",
                          r.data, r.addr);
        }
    } else {
        if (r.addr >= 0xFFFA) {
            std::snprintf(buf, buflen, "vector fetch");
        } else if (r.addr >= 0x0100 && r.addr <= 0x01FF) {
            std::snprintf(buf, buflen, "stack read");
        } else if (phase0) {
            std::snprintf(buf, buflen, "dummy / operand");
        } else {
            buf[0] = '\0';
        }
    }
}

// Emit the paste-ready trace block with section headers at every SYNC
// boundary and a per-row trailing comment. Trap pairs (the framework's
// auto-inserted landing-pad NOP at $N immediately followed by a
// JMP-self at $N+1) collapse into one combined header. The trap kind
// (PASS / FAIL) comes from prog->terminator_for(jmp_addr); falls back
// to a generic instruction header when `prog` is null.
void print_actual_paste_block(uint32_t fields_mask,
                              const std::vector<TraceRow>& actual,
                              const Program* prog) {
    const char* tbl_hdr = (fields_mask & TF_REGS)
                              ? k_table_header_regs
                              : k_table_header_no_regs;
    std::fprintf(stderr,
        "    // --- BEGIN actual trace (paste this into the test) ---\n");
    std::fprintf(stderr,
        "    // The /* N.X */ prefix decodes each row's array index as\n"
        "    // cycle N, phase 0 (end-of-phi1) or 5 (end-of-phi2).\n");
    std::fprintf(stderr,
        "    static const ::test3::TraceRow expected[] = {\n");

    const size_t n = actual.size();
    // Pre-compute which row indexes start a SYNC (sync=1 and phase=0)
    // and pair adjacent SYNCs that form a trap (NOP at $N + JMP at $N+1).
    // `skip_header[i]` is true when the JMP-self SYNC at index i has
    // already been covered by a combined trap header at the preceding
    // NOP SYNC.
    std::vector<bool> skip_header(n, false);
    for (size_t i = 0; i < n; ++i) {
        if (!(actual[i].sync && (i & 1) == 0)) continue;
        if (i + 1 >= n) continue;
        const uint8_t opcode = actual[i + 1].data;
        if (opcode != 0xEA) continue;            // not a NOP
        // Find the next SYNC after this one.
        size_t j = i + 2;
        while (j < n && !(actual[j].sync && (j & 1) == 0)) ++j;
        if (j + 1 >= n) continue;
        const uint8_t next_opcode = actual[j + 1].data;
        if (next_opcode != 0x4C) continue;       // not a JMP
        if (actual[j].addr != actual[i].addr + 1) continue;
        skip_header[j] = true;                   // mark JMP SYNC absorbed
    }

    for (size_t i = 0; i < n; ++i) {
        // Section header at SYNC boundary.
        if (actual[i].sync && (i & 1) == 0 && !skip_header[i]) {
            const uint16_t addr = actual[i].addr;
            const uint8_t  op   = (i + 1 < n) ? actual[i + 1].data : 0;
            // Trap pair?
            bool is_trap = false;
            if (op == 0xEA) {
                // Find the paired JMP-self addr (i's NOP, then JMP at +1)
                size_t j = i + 2;
                while (j < n && !(actual[j].sync && (j & 1) == 0)) ++j;
                if (j + 1 < n && actual[j + 1].data == 0x4C
                    && actual[j].addr == addr + 1) {
                    const uint16_t jmp_addr = actual[j].addr;
                    const char* kind = "PASS";
                    if (prog) {
                        const Terminator* t = prog->terminator_for(jmp_addr);
                        if (t) kind = (t->kind == TermKind::Pass) ? "PASS"
                                                                  : "FAIL";
                    }
                    std::fprintf(stderr,
                        "\n        // --- %s at $%04X: NOP flushes pipelined "
                        "registers, JMP-self at $%04X traps ---\n%s\n",
                        kind, addr, jmp_addr, tbl_hdr);
                    is_trap = true;
                }
            }
            if (!is_trap) {
                const char* m = k_mnemonic[op];
                if (m) {
                    std::fprintf(stderr,
                        "\n        // --- %s at $%04X ---\n%s\n",
                        m, addr, tbl_hdr);
                } else {
                    std::fprintf(stderr,
                        "\n        // --- $%02X at $%04X ---\n%s\n",
                        op, addr, tbl_hdr);
                }
            }
        }

        // Trailing comment.
        char comment[64];
        const uint8_t next_data = (i + 1 < n) ? actual[i + 1].data : 0;
        row_comment(comment, sizeof(comment), actual[i], i, next_data);
        print_row(stderr, actual[i], i, fields_mask, /*as_table_row=*/true,
                  comment[0] ? comment : nullptr);
    }

    std::fprintf(stderr, "    };\n");
    std::fprintf(stderr,
        "    // --- END actual trace ---\n");
}

void print_actual_paste_block(uint32_t fields_mask,
                              const std::vector<TraceRow>& actual) {
    print_actual_paste_block(fields_mask, actual, nullptr);
}

bool compare_and_report(const char* test_name,
                        const char* core_name,
                        uint32_t fields_mask,
                        const TraceRow* expected,
                        size_t expected_count,
                        const std::vector<TraceRow>& actual,
                        const Program* prog) {
    const size_t n = expected_count < actual.size() ? expected_count : actual.size();
    size_t first_diff = n;
    for (size_t i = 0; i < n; ++i) {
        if (!rows_equal(expected[i], actual[i], fields_mask)) { first_diff = i; break; }
    }

    const bool size_match = (expected_count == actual.size());
    const bool match = (first_diff == n) && size_match;
    if (match) return true;

    std::fprintf(stderr, "FAIL: %s [core=%s]\n", test_name, core_name);
    std::fprintf(stderr,
        "  expected half-cycles: %zu, actual half-cycles: %zu\n",
        expected_count, actual.size());

    if (first_diff < n) {
        std::fprintf(stderr,
            "  first mismatch at half-cycle index %zu:\n", first_diff);
        std::fprintf(stderr, "    expected: ");
        print_row(stderr, expected[first_diff], first_diff,
                  fields_mask, /*as_table_row=*/false);
        std::fprintf(stderr, "    actual:   ");
        print_row(stderr, actual[first_diff], first_diff,
                  fields_mask, /*as_table_row=*/false);
        if (prog) {
            SrcLoc loc = prog->src_for(expected[first_diff].addr);
            if (loc.file) {
                std::fprintf(stderr,
                    "    source:   $%04X was emitted at %s:%d\n",
                    expected[first_diff].addr, loc.file, loc.line);
            }
        }
    } else if (!size_match) {
        std::fprintf(stderr,
            "  expected and actual agree on the first %zu half-cycles; "
            "lengths differ.\n", n);
        if (actual.size() > expected_count) {
            std::fprintf(stderr,
                "  first extra actual row [%zu]: ", expected_count);
            print_row(stderr, actual[expected_count], expected_count,
                      fields_mask, /*as_table_row=*/false);
        } else {
            std::fprintf(stderr,
                "  first missing expected row [%zu]: ", actual.size());
            print_row(stderr, expected[actual.size()], actual.size(),
                      fields_mask, /*as_table_row=*/false);
        }
    }

    print_actual_paste_block(fields_mask, actual, prog);
    return false;
}

bool compare_and_report(const char* test_name,
                        const char* core_name,
                        uint32_t fields_mask,
                        const TraceRow* expected,
                        size_t expected_count,
                        const std::vector<TraceRow>& actual) {
    return compare_and_report(test_name, core_name, fields_mask,
                              expected, expected_count, actual,
                              /*prog=*/nullptr);
}

} // namespace test3
