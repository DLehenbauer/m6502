`include "cpu_6502_instructions.vh"

// Highly vertical microcode to save space. If the Microcode ROM was external it'd be better
// to make it more horizontal, but for space savings this is chosen for now.
module cpu_6502_microcode import cpu_6502_pkg::*; (
    input [7:0] i_current_instruction,
    input i_init,
    input i_handle_irq,
    input microinstruction_t i_current_microinstruction,
    output microinstruction_t o_next_microinstruction
);

always_comb begin
    o_next_microinstruction = NOP;

    if (i_handle_irq || i_init) begin
        priority casez (i_current_microinstruction)
            START: o_next_microinstruction = PUSH_PCH;
            PUSH_PCH: o_next_microinstruction = PUSH_PCL;
            PUSH_PCL: o_next_microinstruction = WRITE_SR;
            WRITE_SR: o_next_microinstruction = LOAD_VECTOR;
            LOAD_VECTOR: o_next_microinstruction = READ_VECTOR_HI;
            READ_VECTOR_HI: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: o_next_microinstruction = NOP;
        endcase
    end
    else begin
        priority casez (i_current_instruction)
        OPCODE_SEC, OPCODE_CLC, OPCODE_SEI, OPCODE_CLI, OPCODE_TAX,
        OPCODE_TAY, OPCODE_TXA, OPCODE_TYA, OPCODE_TSX, OPCODE_TXS,
        OPCODE_INX, OPCODE_INY, OPCODE_DEY, OPCODE_DEX, OPCODE_CLD,
        OPCODE_CLV, OPCODE_NOP, OPCODE_SED,
        // Undocumented single-byte implied NOPs.
        8'h1A, 8'h3A, 8'h5A, 8'h7A, 8'hDA, 8'hFA: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = NOP;
            NOP: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        // Undocumented multi-byte NOPs read like a load but discard the value.
        8'h80, 8'h82, 8'h89, 8'hC2, 8'hE2,
        8'h04, 8'h44, 8'h64,
        8'h14, 8'h34, 8'h54, 8'h74, 8'hD4, 8'hF4,
        8'h0C,
        8'h1C, 8'h3C, 8'h5C, 8'h7C, 8'hDC, 8'hFC: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = LOAD;
            LOAD: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_PLA, OPCODE_PLP: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = POP_STACK;
            POP_STACK: o_next_microinstruction = PULL_REGISTER;
            PULL_REGISTER: o_next_microinstruction = STALL;
            STALL: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_PHA, OPCODE_PHP: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = PUSH_STACK;
            PUSH_STACK: o_next_microinstruction = WRITE;
            WRITE: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_ROL_ACC, OPCODE_ROR_ACC, OPCODE_ASL_ACC, OPCODE_LSR_ACC: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = STALL;
            STALL: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_BRK: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = READ_ADL;
            READ_ADL: o_next_microinstruction = PUSH_PCH;
            PUSH_PCH: o_next_microinstruction = PUSH_PCL;
            PUSH_PCL: o_next_microinstruction = WRITE_SR;
            WRITE_SR: o_next_microinstruction = LOAD_VECTOR;
            LOAD_VECTOR: o_next_microinstruction = READ_VECTOR_HI;
            READ_VECTOR_HI: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_JSR: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = READ_ADL;
            READ_ADL: o_next_microinstruction = BUFFER_ADL;
            BUFFER_ADL: o_next_microinstruction = PUSH_PCH;
            PUSH_PCH: o_next_microinstruction = PUSH_PCL;
            PUSH_PCL: o_next_microinstruction = READ_ADH;
            READ_ADH: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_JMP_ABS: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = READ_PCL;
            READ_PCL: o_next_microinstruction = READ_PCH;
            READ_PCH: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_JMP_IND: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = READ_EFFECTIVE_LO;
            READ_EFFECTIVE_LO: o_next_microinstruction = READ_EFFECTIVE_HI;
            READ_EFFECTIVE_HI: o_next_microinstruction = LOAD_PC_EFFECTIVE_LO;
            LOAD_PC_EFFECTIVE_LO: o_next_microinstruction = LOAD_PC_EFFECTIVE_HI;
            LOAD_PC_EFFECTIVE_HI: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_RTS: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = POP_STACK;
            POP_STACK: o_next_microinstruction = RESTORE_STACK;
            RESTORE_STACK: o_next_microinstruction = RESTORE_STACK2;
            RESTORE_STACK2: o_next_microinstruction = PC_INC;
            PC_INC: o_next_microinstruction = STALL;
            STALL: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_RTI: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = POP_STACK;
            POP_STACK: o_next_microinstruction = PULL_REGISTER;
            PULL_REGISTER: o_next_microinstruction = PULL_PCL;
            PULL_PCL: o_next_microinstruction = PULL_PCH;
            PULL_PCH: o_next_microinstruction = STALL;
            STALL: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_TYPE_BRANCH: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = MAYBE_BRANCH;
            MAYBE_BRANCH: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_TYPE_LDA, OPCODE_TYPE_LDX, OPCODE_TYPE_LDY,
        OPCODE_TYPE_LAX,
        OPCODE_ANC, OPCODE_ANC2, OPCODE_ALR, OPCODE_ARR, OPCODE_XAA, OPCODE_AXS, OPCODE_USBC,
        OPCODE_TYPE_AND, OPCODE_TYPE_ORA, OPCODE_TYPE_EOR,
        OPCODE_TYPE_ADC, OPCODE_TYPE_SBC, OPCODE_TYPE_CMP,
        OPCODE_TYPE_CPX, OPCODE_TYPE_CPY, OPCODE_TYPE_BIT: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = LOAD;
            LOAD: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_TYPE_INC, OPCODE_TYPE_DEC, OPCODE_TYPE_ASL, OPCODE_TYPE_LSR, OPCODE_TYPE_ROR, OPCODE_TYPE_ROL,
        OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE, OPCODE_TYPE_RRA, OPCODE_TYPE_DCP, OPCODE_TYPE_ISB: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = LOAD;
            LOAD: o_next_microinstruction = ALU_MODIFY;
            ALU_MODIFY: o_next_microinstruction = STALL;
            STALL: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        OPCODE_TYPE_STA, OPCODE_TYPE_STX, OPCODE_TYPE_STY, OPCODE_TYPE_SAX: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = STORE;
            STORE: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        default: begin
            case (i_current_microinstruction)
            START: o_next_microinstruction = NOP;
            NOP: o_next_microinstruction = MICRO_EXECUTE;
            MICRO_EXECUTE: o_next_microinstruction = START;
            default: ;
            endcase
        end
        endcase
    end
end

endmodule
