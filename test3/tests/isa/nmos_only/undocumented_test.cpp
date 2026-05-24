// test3/tests/isa/nmos_only/undocumented_test.cpp
//
// NMOS undocumented opcode coverage scaffold.
//
// Canonical families worth covering:
//   LAX: load A and X.
//   SAX: store A & X.
//   DCP: decrement memory, then compare with A.
//   ISC: increment memory, then subtract with carry.
//   SLO: arithmetic shift left, then ORA.
//   RLA: rotate left, then AND.
//   SRE: logical shift right, then EOR.
//   RRA: rotate right, then ADC.
//   ALR: AND immediate, then logical shift right.
//   ANC: AND immediate, then copy bit 7 to C.
//   AXS/SBX: subtract immediate from A & X into X.
//   ARR: AND immediate, then rotate right with decimal quirks.
//   JAM/KIL: $02/$12/$22/$32/$42/$52/$62/$72/$92/$B2/$D2/$F2 lock the CPU.
//   SHA/SHX/SHY/SHS: stable store variants for completeness.
//
// TODO(test3): add Program emitters for these undocumented opcodes.
// TODO(test3): add trace coverage for all legal addressing modes per family.
// TODO(test3): keep JAM/KIL skipped until the framework can detect CPU halt
// within max_cycles instead of treating it as a timeout.

#include "test3/test.h"
#include "test3/cpu.h"

using namespace test3;

TEST(lax_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "LAX is undocumented NMOS-only");
    // TODO(test3): exercise LAX once Program emitter exists.
}

TEST(sax_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "SAX is undocumented NMOS-only");
    // TODO(test3): exercise SAX once Program emitter exists.
}

TEST(dcp_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "DCP is undocumented NMOS-only");
    // TODO(test3): exercise DCP once Program emitter exists.
}

TEST(isc_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "ISC is undocumented NMOS-only");
    // TODO(test3): exercise ISC once Program emitter exists.
}

TEST(slo_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "SLO is undocumented NMOS-only");
    // TODO(test3): exercise SLO once Program emitter exists.
}

TEST(rla_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "RLA is undocumented NMOS-only");
    // TODO(test3): exercise RLA once Program emitter exists.
}

TEST(sre_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "SRE is undocumented NMOS-only");
    // TODO(test3): exercise SRE once Program emitter exists.
}

TEST(rra_zp_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "RRA is undocumented NMOS-only");
    // TODO(test3): exercise RRA once Program emitter exists.
}

TEST(anc_imm_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "ANC is undocumented NMOS-only");
    // TODO(test3): exercise ANC once Program emitter exists.
}

TEST(alr_imm_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "ALR is undocumented NMOS-only");
    // TODO(test3): exercise ALR once Program emitter exists.
}

TEST(arr_imm_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "ARR is undocumented NMOS-only");
    // TODO(test3): exercise ARR once Program emitter exists.
}

TEST(axs_imm_skip) {
    SKIP_UNLESS(CV_NMOS_6502, "AXS is undocumented NMOS-only");
    // TODO(test3): exercise AXS once Program emitter exists.
}
