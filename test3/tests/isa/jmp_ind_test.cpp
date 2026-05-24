// test3/tests/isa/jmp_ind_test.cpp
//
// JMP-(ind). Tests here document the NMOS page-wrap behavior.

#include "test3/test.h"
#include "test3/program.h"
#include "test3/cpu.h"
#include "test3/trace.h"
#include "test3/expect.h"

using namespace test3;

TEST(jmp_ind_normal) {
    Program prog = Program::start();

    prog.org(0x0300).bytes({0x00, 0x05}).org(0x0200);
    prog.jmp(Ind{0x0300})
        .fail("JMP indirect must not fall through");

    prog.org(0x0500)
        .pass("JMP indirect arrived");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- JMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    0.5 */ 0x0200, 0x6C,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $6C = JMP
        { /*    1.0 */ 0x0201, 0x6C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x00,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x00,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x03,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0300, 0x03,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0300, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0301, 0x00,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0301, 0x05,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0500: NOP flushes pipelined registers, JMP-self at $0501 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0500, 0x05,  R,    1, 0x0500, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    5.5 */ 0x0500, 0xEA,  R,    1, 0x0500, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    6.0 */ 0x0501, 0xEA,  R,    0, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0501, 0x4C,  R,    0, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    7.5 */ 0x0501, 0x4C,  R,    1, 0x0501, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(jmp_ind_page_wrap_bug) {
    Program prog = Program::start(0x0400);

    prog.org(0x0200).bytes({0x06});
    prog.org(0x02FF).bytes({0x10, 0x07}).org(0x0400);
    prog.jmp(Ind{0x02FF})
        .fail("JMP indirect must not fall through");

    prog.org(0x0610)
        .pass("JMP indirect wraps high-byte fetch inside the page");

    prog.org(0x0710)
        .fail("JMP indirect must not fetch high byte from the next page");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- JMP at $0400 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0400, 0x04,  R,    1, 0x0400, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    0.5 */ 0x0400, 0x6C,  R,    1, 0x0400, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $6C = JMP
        { /*    1.0 */ 0x0401, 0x6C,  R,    0, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0401, 0xFF,  R,    0, 0x0401, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0402, 0xFF,  R,    0, 0x0402, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0402, 0x02,  R,    0, 0x0402, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x02FF, 0x02,  R,    0, 0x0403, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x02FF, 0x10,  R,    0, 0x0403, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0200, 0x10,  R,    0, 0x0403, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0200, 0x06,  R,    0, 0x0403, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0610: NOP flushes pipelined registers, JMP-self at $0611 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0610, 0x06,  R,    1, 0x0610, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    5.5 */ 0x0610, 0xEA,  R,    1, 0x0610, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    6.0 */ 0x0611, 0xEA,  R,    0, 0x0611, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0611, 0x4C,  R,    0, 0x0611, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0611, 0x4C,  R,    1, 0x0611, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    7.5 */ 0x0611, 0x4C,  R,    1, 0x0611, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}

TEST(jmp_ind_label) {
    constexpr uint16_t ptr = 0x0310;
    Label target{0x0520};
    uint8_t target_lo = static_cast<uint8_t>(target.addr & 0xFF);
    uint8_t target_hi = static_cast<uint8_t>(target.addr >> 8);

    Program prog = Program::start();

    prog.org(ptr).bytes({target_lo, target_hi}).org(0x0200);
    prog.jmp(Ind{ptr})
        .fail("JMP indirect label must not fall through");

    prog.org(target.addr)
        .pass("JMP indirect label arrived");

    cpu.load(prog);

    static const TraceRow expected[] = {

        // --- JMP at $0200 ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    0.0 */ 0x0200, 0x02,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    0.5 */ 0x0200, 0x6C,  R,    1, 0x0200, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $6C = JMP
        { /*    1.0 */ 0x0201, 0x6C,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    1.5 */ 0x0201, 0x10,  R,    0, 0x0201, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    2.0 */ 0x0202, 0x10,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    2.5 */ 0x0202, 0x03,  R,    0, 0x0202, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    3.0 */ 0x0310, 0x03,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    3.5 */ 0x0310, 0x20,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    4.0 */ 0x0311, 0x20,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    4.5 */ 0x0311, 0x05,  R,    0, 0x0203, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },

        // --- PASS at $0520: NOP flushes pipelined registers, JMP-self at $0521 traps ---
        //    cycle      addr, data, rw, sync,     PC,    A,    X,    Y,    S,   NV-BDIZC
        { /*    5.0 */ 0x0520, 0x05,  R,    1, 0x0520, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: NOP
        { /*    5.5 */ 0x0520, 0xEA,  R,    1, 0x0520, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $EA = NOP
        { /*    6.0 */ 0x0521, 0xEA,  R,    0, 0x0521, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // dummy / operand
        { /*    6.5 */ 0x0521, 0x4C,  R,    0, 0x0521, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },
        { /*    7.0 */ 0x0521, 0x4C,  R,    1, 0x0521, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // SYNC: JMP
        { /*    7.5 */ 0x0521, 0x4C,  R,    1, 0x0521, 0x66, 0x00, 0x00, 0xFD, 0b00100110 },  // $4C = JMP
    };

    EXPECT_PASS(expected);
}
