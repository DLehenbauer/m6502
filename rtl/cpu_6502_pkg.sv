// Shared enum types for the cpu_6502 modules.
//
// For portability, types crossing module boundaries must live in a package. A
// named package has a single global scope and resolves to the same type
// everywhere it is imported.
//
// Types brought in via `include` may be treated as a distinct type per file,
// depending on the tool. This is because the SystemVerilog standard leaves the
// file-to-compilation-unit grouping tool-defined.

package cpu_6502_pkg;

    typedef enum logic [3:0] {
        IMMEDIATE, ZP, ZP_X, ZP_Y, IMPLIED,
        INDIRECT, INDEX_X_INDIRECT,
        INDEX_Y_INDIRECT, RELATIVE, ABSOLUTE,
        ABSOLUTE_X, ABSOLUTE_Y, ACCUMULATOR
    } operand_type_t;

    typedef enum logic [3:0] {
        ALU_ADC = 0, ALU_AND = 1, ALU_ORA = 2, ALU_EOR = 3,
        ALU_ASL = 4, ALU_LSR = 5, ALU_ROL = 6, ALU_ROR = 7, ALU_SBC = 8
    } alu_op_t;

    typedef enum logic [5:0] {
        NOP = 0, LOAD = 1, MICRO_INIT = 2, MICRO_EXECUTE = 3, WRITE = 4, STORE = 5,
        PUSH_STACK = 6, POP_STACK = 7, RESTORE_STACK = 8, START = 9, LOAD_INITIAL_PC = 10,
        MAYBE_BRANCH = 11, STALL = 12, RESTORE_STACK2 = 13, ALU_MODIFY = 14,
        READ_ADL = 15, BUFFER_ADL = 16, PUSH_PCH = 17, PUSH_PCL = 18, READ_ADH = 19, PC_INC = 20,
        READ_PCL = 21, READ_PCH = 22, LOAD_PC_EFFECTIVE_LO = 23, LOAD_PC_EFFECTIVE_HI = 24,
        READ_EFFECTIVE_LO = 25, READ_EFFECTIVE_HI = 26, PULL_REGISTER = 27, WRITE_SR = 28,
        READ_VECTOR_HI = 29, PULL_PCH = 30, PULL_PCL = 31, LOAD_VECTOR = 32,
        // Shared first post-fetch beat of every stack/return op (PHA/PHP/PLA/
        // PLP/RTS/RTI): a dummy read at PC. The NMOS die reserves the cycle
        // after the opcode fetch for this read (a predecode/PC side effect, not
        // stack logic), so it must never inherit the next beat's stack address.
        STACK_DUMMY_PC = 33
    } microinstruction_t;

endpackage
