from cocotb.clock import Clock
from cocotb.triggers import ClockCycles, RisingEdge, FallingEdge
import cocotb

# Reset vector location
RESET_VECTOR_LO = 0xFFFC
RESET_VECTOR_HI = 0xFFFD

# Status register bits
SR_C = 0  # Carry
SR_Z = 1  # Zero
SR_I = 2  # Interrupt disable
SR_D = 3  # Decimal
SR_B = 4  # Break
SR_V = 6  # Overflow
SR_N = 7  # Negative

# Opcodes
LDA_IMM = 0xA9
LDX_IMM = 0xA2
LDY_IMM = 0xA0
STA_ABS = 0x8D
NOP = 0xEA
JMP_ABS = 0x4C
INX = 0xE8
TXS = 0x9A
SEC = 0x38
SED = 0xF8
SEI = 0x78
CLI = 0x58
CLV = 0xB8
ADC_IMM = 0x69

def lo(addr):
    return addr & 0xFF

def hi(addr):
    return (addr >> 8) & 0xFF

def get_acc(dut):
    return int(dut.cpu_6502.register_acc.value)

def get_x(dut):
    return int(dut.cpu_6502.register_x.value)

def get_y(dut):
    return int(dut.cpu_6502.register_y.value)

def get_sp(dut):
    return int(dut.cpu_6502.register_sp.value)

def get_pc(dut):
    return int(dut.cpu_6502.program_counter.value)

def get_sr(dut):
    n = int(dut.cpu_6502.status_negative.value)
    v = int(dut.cpu_6502.status_overflow.value)
    d = int(dut.cpu_6502.status_decimal.value)
    i = int(dut.cpu_6502.status_interrupt.value)
    z = int(dut.cpu_6502.status_zero.value)
    c = int(dut.cpu_6502.status_carry.value)
    return (n << 7) | (v << 6) | (1 << 5) | (0 << 4) | (d << 3) | (i << 2) | (z << 1) | c

def assert_acc(dut, expected):
    actual = get_acc(dut)
    assert actual == expected, f"ACC: expected {expected:#04x}, got {actual:#04x}"

def assert_x(dut, expected):
    actual = get_x(dut)
    assert actual == expected, f"X: expected {expected:#04x}, got {actual:#04x}"

def assert_y(dut, expected):
    actual = get_y(dut)
    assert actual == expected, f"Y: expected {expected:#04x}, got {actual:#04x}"

def assert_sp(dut, expected):
    actual = get_sp(dut)
    assert actual == expected, f"SP: expected {expected:#04x}, got {actual:#04x}"

def assert_pc(dut, expected):
    actual = get_pc(dut)
    assert actual == expected, f"PC: expected {expected:#06x}, got {actual:#06x}"

def assert_flag(dut, bit, expected, name=""):
    sr = get_sr(dut)
    actual = (sr >> bit) & 1
    assert actual == expected, \
        f"Flag {name}(bit {bit}): expected {expected}, got {actual} (SR={sr:#04x})"


async def setup_reset_test(dut, reset_vector, program, data=None, cycles=50):
    """
    Setup test with reset vector pointing to program location.

    Args:
        dut: Device under test
        reset_vector: 16-bit address to store in reset vector (0xFFFC/0xFFFD)
        program: List of bytes to write at reset_vector address
        data: Optional dict of addr->value for additional memory setup
        cycles: Number of cycles to run after init
    """
    Clock(dut.i_clk, 100, "ns").start()
    dut.i_reset_n.value = 0
    dut.i_rdy.value = 1
    dut.i_nmi_n.value = 1
    dut.i_irq_n.value = 1
    dut.i_so_n.value = 1

    # Hold reset low for 2 cycles
    await ClockCycles(dut.i_clk, 2)

    # Write reset vector
    dut.ram.mem[RESET_VECTOR_LO].value = lo(reset_vector)
    dut.ram.mem[RESET_VECTOR_HI].value = hi(reset_vector)

    # Write program at reset vector address
    for i, b in enumerate(program):
        dut.ram.mem[reset_vector + i].value = b

    # Write additional data if provided
    if data:
        for addr, val in data.items():
            dut.ram.mem[addr].value = val

    # Release reset
    dut.i_reset_n.value = 1

    # Wait for init sequence (6 cycles) + reset vector read (2 cycles)
    await ClockCycles(dut.i_clk, 8)

    # Run program
    await ClockCycles(dut.i_clk, cycles)


@cocotb.test()
async def test_reset_vector_basic(dut):
    """Reset vector: PC loads from 0xFFFC/0xFFFD after reset."""
    reset_addr = 0x8000
    prog = [
        LDA_IMM, 0x42,  # LDA #$42
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    # Verify PC is at reset_addr + program length
    assert_pc(dut, reset_addr + len(prog))
    # Verify instruction executed
    assert_acc(dut, 0x42)


@cocotb.test()
async def test_reset_vector_low_address(dut):
    """Reset vector: PC loads correctly for low address (0x0200)."""
    reset_addr = 0x0200
    prog = [
        LDA_IMM, 0xAB,  # LDA #$AB
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    assert_pc(dut, reset_addr + len(prog))
    assert_acc(dut, 0xAB)


@cocotb.test()
async def test_reset_vector_high_address(dut):
    """Reset vector: PC loads correctly for high address (0xF000)."""
    reset_addr = 0xF000
    prog = [
        LDA_IMM, 0xCD,  # LDA #$CD
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    assert_pc(dut, reset_addr + len(prog))
    assert_acc(dut, 0xCD)


@cocotb.test()
async def test_reset_vector_interrupt_flag_set(dut):
    """Reset: Interrupt disable flag (I) is set after reset."""
    reset_addr = 0x8000
    prog = [NOP, NOP]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    # I flag should be set after reset
    assert_flag(dut, SR_I, 1, "I")


@cocotb.test()
async def test_reset_vector_flags_cleared(dut):
    """Reset: N, V, D, Z, C flags are cleared after reset."""
    reset_addr = 0x8000
    prog = [NOP, NOP]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    assert_flag(dut, SR_N, 0, "N")
    assert_flag(dut, SR_V, 0, "V")
    assert_flag(dut, SR_D, 0, "D")
    assert_flag(dut, SR_Z, 0, "Z")
    assert_flag(dut, SR_C, 0, "C")


@cocotb.test()
async def test_reset_vector_execution_continues(dut):
    """Reset vector: Execution continues normally after reset."""
    reset_addr = 0x8000
    prog = [
        LDA_IMM, 0x10,  # LDA #$10
        LDA_IMM, 0x20,  # LDA #$20 (overwrites)
        LDA_IMM, 0x30,  # LDA #$30 (overwrites)
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=8)

    assert_pc(dut, reset_addr + len(prog))
    assert_acc(dut, 0x30)


@cocotb.test()
async def test_reset_vector_with_store(dut):
    """Reset vector: Store operations work after reset."""
    reset_addr = 0x8000
    store_addr = 0x0300
    prog = [
        LDA_IMM, 0x55,           # LDA #$55
        STA_ABS, lo(store_addr), hi(store_addr),  # STA $0300
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=8)

    # Verify store worked
    stored = int(dut.ram.mem[store_addr].value)
    assert stored == 0x55, f"Memory at {store_addr:#06x}: expected 0x55, got {stored:#04x}"


@cocotb.test()
async def test_reset_vector_with_jump(dut):
    """Reset vector: JMP works correctly after reset."""
    reset_addr = 0x8000
    jump_target = 0x9000

    # Program at reset vector jumps to another location
    prog = [
        JMP_ABS, lo(jump_target), hi(jump_target),  # JMP $9000
    ]

    # Program at jump target
    target_prog = [
        LDA_IMM, 0x77,  # LDA #$77
        NOP,
    ]

    data = {}
    for i, b in enumerate(target_prog):
        data[jump_target + i] = b

    await setup_reset_test(dut, reset_addr, prog, data=data, cycles=8)

    # Verify we jumped and executed at new location
    assert_pc(dut, jump_target + len(target_prog))
    assert_acc(dut, 0x77)


@cocotb.test()
async def test_reset_vector_registers_zeroed(dut):
    """Reset: Registers A, X, Y are zeroed after reset."""
    reset_addr = 0x8000
    prog = [NOP, NOP]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    assert_acc(dut, 0x00)
    assert_x(dut, 0x00)
    assert get_y(dut) == 0x00, f"Y: expected 0x00, got {get_y(dut):#04x}"


@cocotb.test()
async def test_reset_vector_page_boundary(dut):
    """Reset vector: PC loads correctly when address crosses page boundary."""
    # Reset vector points to 0x10FF - instruction spans pages
    reset_addr = 0x10FF
    prog = [
        LDA_IMM, 0xEE,  # Opcode at 0x10FF, operand at 0x1100
        NOP,
    ]
    await setup_reset_test(dut, reset_addr, prog, cycles=4)

    assert_pc(dut, reset_addr + len(prog))
    assert_acc(dut, 0xEE)


# ============================================================
# Warm reset: registers and flags are re-initialized regardless
# of pre-reset state.
# ============================================================
# A warm reset (asserting reset after the CPU has executed code) must
# leave the CPU in the same state as a cold reset:
#
#   - A = X = Y = SP = 0
#   - I = 1 (interrupts masked)
#   - N = V = D = Z = C = 0
#   - PC loaded from $FFFC/$FFFD
#
@cocotb.test()
async def test_reset_warm(dut):
    """Warm reset: all registers and flags re-initialize from any prior state.

    Sequence:
      1. Cold reset, then run a setup program that mutates X, Y, SP, D, I
         and drives A and the ADC-touched flags (N, V, Z, C) into a known
         steady state, then loops on ADC so the internal `opcode` register
         holds OPCODE_TYPE_ADC when reset is asserted.
      2. Sanity-check the steady state.
      3. Assert reset (warm reset) and release.
      4. Verify A = X = Y = SP = 0, PC = reset vector, I = 1, and
         N = V = D = Z = C = 0.
    """
    reset_addr = 0x8000

    # Change registers and flags to known values that differ than reset state.
    prog = [
        LDY_IMM, 0xEF,          # Y = $EF  (ADC does not touch Y)
        LDX_IMM, 0xCC,          # X = $CC  (ADC does not touch X)
        TXS,                    # SP = $CC (ADC does not touch SP)
        CLI,                    # I = 0    (ADC does not touch I)
        LDA_IMM, 0x90,          # A = $90  (valid BCD, bit 7 set -> N=1)
        SEC,                    # C = 1    (carry-in for the fixed point)
        SED,                    # D = 1    (ADC runs in BCD mode)
    ]

    # ADC #$99 with carry-in = 1 in BCD mode is a fixed point of the
    # ALU for any valid BCD operand A:
    #     A_next = A + 99 + 1 = A + 100 = A (mod 100, BCD)
    #     C_out  = 1
    # So the very first ADC leaves the CPU in this steady state and
    # every subsequent ADC keeps it there for the rest of the loop:
    #
    #     A = $90   (non-zero, valid BCD, != $80 - a stale-opcode bug
    #                that writes the reset-vector high byte into A
    #                would be clearly visible)
    #     N = 1     (opposite of reset N=0)
    #     C = 1     (opposite of reset C=0)
    #     D = 1     (opposite of reset D=0; SED above, ADC does not
    #                touch D)
    #     I = 0     (opposite of reset I=1; CLI above, ADC does not
    #                touch I)
    #     Z = 0     (matches reset; ADC cannot land Z=1 here without
    #                zeroing A, which would defeat the A-reset check)
    #     V = 0     (matches reset; the 6502 V flag is meaningless in
    #                BCD mode, and no ADC fixed point sets V anyway)
    #
    # Whichever ADC the warm reset interrupts, the last latched opcode is
    # OPCODE_TYPE_ADC and the state is known to differ from the reset state
    # for A, N, C, D, and I.
    prog += [ADC_IMM, 0x99] * 64

    Clock(dut.i_clk, 100, "ns").start()
    dut.i_reset_n.value = 0
    dut.i_rdy.value = 1
    dut.i_nmi_n.value = 1
    dut.i_irq_n.value = 1
    dut.i_so_n.value = 1

    await ClockCycles(dut.i_clk, 2)

    dut.ram.mem[RESET_VECTOR_LO].value = lo(reset_addr)
    dut.ram.mem[RESET_VECTOR_HI].value = hi(reset_addr)
    for i, b in enumerate(prog):
        dut.ram.mem[reset_addr + i].value = b

    # Cold reset, then run long enough to execute the setup program
    # and reach the ADC #$FF steady state.
    dut.i_reset_n.value = 1
    await ClockCycles(dut.i_clk, 30)

    # Sanity: confirm the pre-reset steady state. Every value below is
    # the opposite of (or distinct from) the reset state, so the
    # post-reset assertions below cannot pass by coincidence.
    assert_acc(dut, 0x90)
    assert_x(dut, 0xCC)
    assert_y(dut, 0xEF)
    assert_sp(dut, 0xCC)
    assert_flag(dut, SR_N, 1, "N (pre-reset)")
    assert_flag(dut, SR_C, 1, "C (pre-reset)")
    assert_flag(dut, SR_D, 1, "D (pre-reset)")
    assert_flag(dut, SR_I, 0, "I (pre-reset)")

    # Warm reset: assert reset without changing memory or the opcode reg.
    dut.i_reset_n.value = 0
    await ClockCycles(dut.i_clk, 4)
    dut.i_reset_n.value = 1

    # Advance the CPU for exactly the init + vector-load sequence
    # (matches setup_reset_test) so we sample state right after PC
    # has been loaded from $FFFC/$FFFD but before the first user
    # instruction is fetched.
    await ClockCycles(dut.i_clk, 9)

    # PC must be loaded from the reset vector.
    assert_pc(dut, reset_addr)

    # Registers must be at their reset values. If A is $80 (the reset
    # vector's high byte), the stale opcode regression has returned.
    assert_acc(dut, 0x00)
    assert_x(dut, 0x00)
    assert_y(dut, 0x00)
    assert_sp(dut, 0x00)

    # Flags: I=1 (interrupts masked), all others cleared.
    assert_flag(dut, SR_N, 0, "N")
    assert_flag(dut, SR_V, 0, "V")
    assert_flag(dut, SR_D, 0, "D")
    assert_flag(dut, SR_I, 1, "I")
    assert_flag(dut, SR_Z, 0, "Z")
    assert_flag(dut, SR_C, 0, "C")


@cocotb.test()
async def test_so_pin_sets_overflow_flag(dut):
    """SO pin: Falling edge on SO sets the V (overflow) flag."""
    reset_addr = 0x8000
    prog = [
        NOP,  # NOP - wait for SO trigger
        NOP,  # NOP - V flag should be set now
        CLV,  # CLV - clear overflow flag
        NOP,  # NOP - V flag should be clear now
    ]

    # Start with SO high
    Clock(dut.i_clk, 100, "ns").start()
    dut.i_reset_n.value = 0
    dut.i_rdy.value = 1
    dut.i_nmi_n.value = 1
    dut.i_irq_n.value = 1
    dut.i_so_n.value = 1

    await ClockCycles(dut.i_clk, 2)

    # Write reset vector and program
    dut.ram.mem[RESET_VECTOR_LO].value = lo(reset_addr)
    dut.ram.mem[RESET_VECTOR_HI].value = hi(reset_addr)
    for i, b in enumerate(prog):
        dut.ram.mem[reset_addr + i].value = b

    # Release reset
    dut.i_reset_n.value = 1
    await ClockCycles(dut.i_clk, 8)  # Reset sequence

    # V flag should be clear initially
    assert_flag(dut, SR_V, 0, "V (before SO)")

    # Execute first NOP
    await ClockCycles(dut.i_clk, 2)

    # Trigger SO (falling edge: 1 -> 0)
    dut.i_so_n.value = 0
    await ClockCycles(dut.i_clk, 1)

    # Execute second NOP - V flag should now be set
    await ClockCycles(dut.i_clk, 2)
    assert_flag(dut, SR_V, 1, "V (after SO)")

    # Keep SO low - should not re-trigger (edge-triggered, not level)
    await ClockCycles(dut.i_clk, 1)
    assert_flag(dut, SR_V, 1, "V (SO held low)")

    # Execute CLV - clears V flag
    await ClockCycles(dut.i_clk, 2)
    assert_flag(dut, SR_V, 0, "V (after CLV)")

    # Execute final NOP - V should still be clear
    await ClockCycles(dut.i_clk, 2)
    assert_flag(dut, SR_V, 0, "V (final)")

    # Release SO back to high
    dut.i_so_n.value = 1
    await ClockCycles(dut.i_clk, 2)


@cocotb.test()
async def test_so_pin_edge_triggered(dut):
    """SO pin: Only falling edge triggers, not level."""
    reset_addr = 0x8000
    prog = [
        CLV,  # CLV - ensure V is clear
        NOP,  # NOP
        NOP,  # NOP
        NOP,  # NOP
    ]

    # Start with SO already low
    Clock(dut.i_clk, 100, "ns").start()
    dut.i_reset_n.value = 0
    dut.i_rdy.value = 1
    dut.i_nmi_n.value = 1
    dut.i_irq_n.value = 1
    dut.i_so_n.value = 0  # Start LOW (no edge yet)

    await ClockCycles(dut.i_clk, 2)

    # Write reset vector and program
    dut.ram.mem[RESET_VECTOR_LO].value = lo(reset_addr)
    dut.ram.mem[RESET_VECTOR_HI].value = hi(reset_addr)
    for i, b in enumerate(prog):
        dut.ram.mem[reset_addr + i].value = b

    # Release reset (SO still low)
    dut.i_reset_n.value = 1
    await ClockCycles(dut.i_clk, 8)  # Reset sequence

    # Execute CLV
    await ClockCycles(dut.i_clk, 2)

    # V should be clear - SO being low doesn't set it (no edge)
    assert_flag(dut, SR_V, 0, "V (SO held low, no edge)")

    # Execute NOPs - V should stay clear
    await ClockCycles(dut.i_clk, 4)
    assert_flag(dut, SR_V, 0, "V (still low)")

    # Now create a falling edge: high -> low
    dut.i_so_n.value = 1
    await ClockCycles(dut.i_clk, 2)
    dut.i_so_n.value = 0  # Falling edge
    await ClockCycles(dut.i_clk, 2)

    # NOW V should be set (edge detected)
    assert_flag(dut, SR_V, 1, "V (after falling edge)")
