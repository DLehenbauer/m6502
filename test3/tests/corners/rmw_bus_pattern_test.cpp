// test3/tests/corners/rmw_bus_pattern_test.cpp
//
// NMOS read-modify-write instructions write the original memory value back
// before they write the modified value. The trace exposes that bus pattern.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(inc_zp_rmw_pattern) {
    Program prog = Program::start()
        .inc(ZP{0x80})
        .pass("INC zero page RMW pattern completed");

    prog.org(0x0080).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- INC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: INC
        { /*    0.5 */ 0x0200, 0xE6,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $E6 = INC
        { /*    1.0 */ 0x0201, 0xE6,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x41,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x41,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $0080
        { /*    3.5 */ 0x0080, 0x41,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $41 -> $0080
        { /*    4.0 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080
        { /*    4.5 */ 0x0080, 0x42,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x41,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(dec_abs_rmw_pattern) {
    Program prog = Program::start()
        .dec(Abs{0x1234})
        .pass("DEC absolute RMW pattern completed");

    prog.org(0x1234).bytes({0x43});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- DEC at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: DEC
        { /*    0.5 */ 0x0200, 0xCE,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $CE = DEC
        { /*    1.0 */ 0x0201, 0xCE,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x34,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x34,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x12,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x1234, 0x12,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x1234, 0x43,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x1234, 0x43,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $1234
        { /*    4.5 */ 0x1234, 0x43,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $43 -> $1234
        { /*    5.0 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234
        { /*    5.5 */ 0x1234, 0x42,  W,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1234

        // --- PASS at $0203: NOP flushes pipelined registers, JMP-self at $0204 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    6.0 */ 0x0203, 0x43,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    6.5 */ 0x0203, 0xEA,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*    7.0 */ 0x0204, 0xEA,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    7.5 */ 0x0204, 0x4C,  R,    0, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },
        { /*    8.0 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*    8.5 */ 0x0204, 0x4C,  R,    1, 0x0204, 0x66, 0x00, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(asl_zp_rmw_pattern) {
    Program prog = Program::start()
        .asl(ZP{0x80})
        .pass("ASL zero page RMW pattern completed");

    prog.org(0x0080).bytes({0x66});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- ASL at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: ASL
        { /*    0.5 */ 0x0200, 0x06,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $06 = ASL
        { /*    1.0 */ 0x0201, 0x06,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x80,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0080, 0x80,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0080, 0x66,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0080, 0x66,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0080
        { /*    3.5 */ 0x0080, 0x66,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // write $66 -> $0080
        { /*    4.0 */ 0x0080, 0xCC,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $CC -> $0080
        { /*    4.5 */ 0x0080, 0xCC,  W,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // write $CC -> $0080

        // --- PASS at $0202: NOP flushes pipelined registers, JMP-self at $0203 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0202, 0x66,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    5.5 */ 0x0202, 0xEA,  R,    1, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    6.0 */ 0x0203, 0xEA,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    6.5 */ 0x0203, 0x4C,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },
        { /*    7.0 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*    7.5 */ 0x0203, 0x4C,  R,    1, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(rol_zpx_rmw_pattern) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .rol(ZPX{0x80})
        .pass("ROL zero page X RMW pattern completed");

    prog.org(0x0084).bytes({0x66});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- ROL at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: ROL
        { /*    2.5 */ 0x0202, 0x36,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $36 = ROL
        { /*    3.0 */ 0x0203, 0x36,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x80,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0080, 0x80,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0080, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x0084, 0x00,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x0084, 0x66,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x0084, 0x66,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0084
        { /*    6.5 */ 0x0084, 0x66,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $66 -> $0084
        { /*    7.0 */ 0x0084, 0xCC,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // write $CC -> $0084
        { /*    7.5 */ 0x0084, 0xCC,  W,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // write $CC -> $0084

        // --- PASS at $0204: NOP flushes pipelined registers, JMP-self at $0205 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    8.0 */ 0x0204, 0x66,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // SYNC: NOP
        { /*    8.5 */ 0x0204, 0xEA,  R,    1, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // $EA = NOP
        { /*    9.0 */ 0x0205, 0xEA,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // dummy / operand
        { /*    9.5 */ 0x0205, 0x4C,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },
        { /*   10.0 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // SYNC: JMP
        { /*   10.5 */ 0x0205, 0x4C,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b10100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(inc_absx_rmw_pattern) {
    Program prog = Program::start()
        .ldx(Imm{0x04})
        .inc(AbsX{0x1234})
        .pass("INC absolute X RMW pattern completed");

    prog.org(0x1238).bytes({0x41});

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- LDX at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: LDX
        { /*    0.5 */ 0x0200, 0xA2,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $A2 = LDX
        { /*    1.0 */ 0x0201, 0xA2,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x04,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- INC at $0202 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    2.0 */ 0x0202, 0x04,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: INC
        { /*    2.5 */ 0x0202, 0xFE,  R,    1, 0x0202, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $FE = INC
        { /*    3.0 */ 0x0203, 0xFE,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    3.5 */ 0x0203, 0x34,  R,    0, 0x0203, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    4.0 */ 0x0204, 0x34,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    4.5 */ 0x0204, 0x12,  R,    0, 0x0204, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    5.0 */ 0x1238, 0x12,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    5.5 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    6.0 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*    6.5 */ 0x1238, 0x41,  R,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*    7.0 */ 0x1238, 0x41,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $1238
        { /*    7.5 */ 0x1238, 0x41,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $41 -> $1238
        { /*    8.0 */ 0x1238, 0x42,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1238
        { /*    8.5 */ 0x1238, 0x42,  W,    0, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // write $42 -> $1238

        // --- PASS at $0205: NOP flushes pipelined registers, JMP-self at $0206 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    9.0 */ 0x0205, 0x41,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: NOP
        { /*    9.5 */ 0x0205, 0xEA,  R,    1, 0x0205, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $EA = NOP
        { /*   10.0 */ 0x0206, 0xEA,  R,    0, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // dummy / operand
        { /*   10.5 */ 0x0206, 0x4C,  R,    0, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },
        { /*   11.0 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // SYNC: JMP
        { /*   11.5 */ 0x0206, 0x4C,  R,    1, 0x0206, 0x66, 0x04, 0x00, 0xFD, 0b00100100 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
