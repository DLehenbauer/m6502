// test3/tests/corners/io_port_test.cpp
//
// 6510 ($00/$01 I/O port) behavior. The current aholme shim reports
// CV_NMOS_6502, and Cpu services $0000/$0001 through flat RAM. These
// scaffold tests stay skip-gated until a CV_6510 shim models the port.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(io_port_ddr_default) {
    SKIP_UNLESS(CV_6510, "IO port is 6510-only");

    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200)
        .fail("$0000 DDR default must read as $2F");

    prog.org(0x0210)
        .lda(ZP{0x00})
        .cmp(Imm{0x2F})
        .bne(wrong)
        .pass("$0000 DDR default reads as $2F");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(io_port_data_default) {
    SKIP_UNLESS(CV_6510, "IO port is 6510-only");

    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200)
        .fail("$0001 PORT default must read as $37");

    prog.org(0x0210)
        .lda(ZP{0x01})
        .cmp(Imm{0x37})
        .bne(wrong)
        .pass("$0001 PORT default reads as $37");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(io_port_write_then_read_back) {
    SKIP_UNLESS(CV_6510, "IO port is 6510-only");

    Program prog = Program::start(0x0210);
    Label wrong = prog.org(0x0200)
        .fail("$0001 PORT reads external pullups when DDR is all input");

    prog.org(0x0210)
        .lda(Imm{0x00})
        .sta(ZP{0x00})
        .lda(ZP{0x01})
        .cmp(Imm{0xFF})
        .bne(wrong)
        .pass("$0001 PORT reads external pullups when DDR is all input");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}

TEST(io_port_overridden_by_test_shim) {
    SKIP_UNLESS(CV_6510, "IO port is 6510-only");

    Program prog = Program::start(0x0210);
    Label io_visible = prog.org(0x0200)
        .pass("$0001 PORT read is served by IO, not RAM");

    prog.org(0x0210)
        .lda(Imm{0x00})
        .sta(ZP{0x00})
        .lda(Imm{0x42})
        .sta(ZP{0x01})
        .lda(ZP{0x01})
        .cmp(Imm{0x42})
        .bne(io_visible)
        .fail("$0001 PORT read must not return the RAM byte last written");

    cpu.load(prog);

    static const TraceRow expected[] = {
    };

    EXPECT_PASS(expected);
}
