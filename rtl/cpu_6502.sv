`include "cpu_6502_instructions.vh"

module cpu_6502 import cpu_6502_pkg::*; #(
    START_PC_ENABLED = 0,
    START_PC = 0
) (
    input i_clk,
    output o_phi1,
    output o_phi2,

    input i_reset_n,

    input i_rdy,
    input i_nmi_n,
    input i_irq_n,
    input i_so_n,

    output o_sync,

    // bus
    input [7:0] i_bus_data,
    output reg [7:0] o_bus_data,
    output reg [15:0] o_bus_addr,
    output reg o_rw,

    // debug port
    input [2:0] i_debug_sel,
    output reg [7:0] o_debug_data
);

assign o_phi2 = i_clk;
assign o_phi1 = ~i_clk;

localparam RESET_VECTOR = 16'hfffc;
localparam NMI_VECTOR = 16'hfffa;
localparam IRQ_VECTOR = 16'hfffe;

localparam INIT_CYCLES = 6;

typedef enum logic [3:0] {
    INIT = 0,
    OP_ZERO = 1,
    OP_LOAD_ZP = 2,
    OP_LOAD_ZP_INDEXED = 3,
    OP_ABSOLUTE_HI = 4,
    OP_LOAD_INDIRECT_LO = 5,
    OP_CALCULATE_BRANCH_OFFSET = 6,
    OP_BRANCH_PAGE_CROSS = 7,
    OP_ABSOLUTE_LO = 8,
    OP_ABSOLUTE_PAGE_CROSS = 9
} operation_t;
operation_t operation;

reg [15:0] program_counter;
reg status_negative, status_overflow,
    status_decimal, status_interrupt, status_zero,
    status_carry;

reg [7:0] register_x, register_y, register_acc, register_sp;
reg [15:0] effective_address;
wire [7:0] effective_address_lo, effective_address_hi;
reg effective_address_lo_carry;
assign effective_address_lo = effective_address[7:0];
assign effective_address_hi = effective_address[15:8];

reg init;
reg [7:0] opcode;

reg [2:0] init_counter;
reg [7:0] bus_data_write;
reg [7:0] rmw_new;

alu_op_t alu_operation;
reg [7:0] alu_result, alu_lhs, alu_rhs;
reg alu_carry_out, alu_carry_in, alu_overflow, alu_decimal;
reg alu_negative, alu_zero;

wire init_rdy;
assign init_rdy = operation == INIT && init_counter == INIT_CYCLES;

operand_type_t addressing_mode;

reg handle_irq, handle_nmi;

reg first_microinstruction;

// In a real 6502, o_sync is asserted shortly after the negedge of i_clk, which
// allows adequate setup time before the i_rdy signal is sampled after the
// rising edge of i_clk.
//
//                v-- i_rdy must be stable by this point
//  ___           :____
//     \__________/
//       :
//       ^-- o_sync must be valid shortly after negedge
//
// Driving o_sync combinationally from first_microinstruction (which is set on
// the negedge i_clk that starts each opcode-fetch cycle) meets this timing. We
// require combinational logic here because we need the post-edge values of
// handle_irq/handle_nmi.
assign o_sync = first_microinstruction && !handle_irq && !handle_nmi;

microinstruction_t current_microinstruction, prev_mi;
reg [7:0] current_instruction;
always_comb begin
    current_instruction = first_microinstruction ? i_bus_data : opcode;
end

cpu_6502_ir_decoder cpu_6502_ir_decoder (
    .i_opcode(current_instruction),
    .o_operand_type(addressing_mode)
);

microinstruction_t next_microinstruction, next2_microinstruction;
microinstruction_t active_microinstruction, next_active_microinstruction;

cpu_6502_microcode microcode_next (
    .i_current_instruction(current_instruction),
    .i_current_microinstruction(current_microinstruction),
    .i_handle_irq(handle_irq),
    .i_init(init),
    .o_next_microinstruction(next_microinstruction)
);

cpu_6502_microcode microcode_next2 (
    .i_current_instruction(current_instruction),
    .i_current_microinstruction(next_microinstruction),
    .i_handle_irq(handle_irq),
    .i_init(init),
    .o_next_microinstruction(next2_microinstruction)
);

always_comb begin
    if (current_microinstruction == START) begin
        active_microinstruction = next_microinstruction;
        next_active_microinstruction = next2_microinstruction;
    end else begin
        active_microinstruction = current_microinstruction;
        next_active_microinstruction = next_microinstruction;
    end
end

reg branch_taken;
always_comb begin
    case (current_instruction)
    OPCODE_BCC: branch_taken = !status_carry;
    OPCODE_BCS: branch_taken = status_carry;
    OPCODE_BEQ: branch_taken = status_zero;
    OPCODE_BNE: branch_taken = !status_zero;
    OPCODE_BPL: branch_taken = !status_negative;
    OPCODE_BMI: branch_taken = status_negative;
    OPCODE_BVS: branch_taken = status_overflow;
    OPCODE_BVC: branch_taken = !status_overflow;
    default:
        branch_taken = 0;
    endcase
end

always @(posedge i_clk) begin
    o_bus_data <= bus_data_write;
end

reg prev_so_n;
reg trigger_overflow;
always @(posedge o_phi2 or negedge i_reset_n) begin
    if (!i_reset_n) begin
        prev_so_n <= 0;
        trigger_overflow <= 0;
    end else begin
        trigger_overflow <= 0;
        prev_so_n <= i_so_n;
        if (prev_so_n && !i_so_n)
            trigger_overflow <= 1;
    end
end

reg nmi_n_sync, nmi_n_sync2, prev_nmi_n, pending_nmi;
always @(negedge i_clk or negedge i_reset_n) begin
    if (!i_reset_n) begin
        nmi_n_sync <= 1;
        nmi_n_sync2 <= 1;
    end else begin
        nmi_n_sync2 <= nmi_n_sync;
        nmi_n_sync <= i_nmi_n;
    end
end


always @(negedge i_clk or negedge i_reset_n) begin
    if (!i_reset_n) begin
        first_microinstruction <= 0;
        o_rw <= 1;
        operation <= INIT;
        init_counter <= 0;
        program_counter <= 0;

        current_microinstruction <= MICRO_INIT;

        o_bus_addr <= 0;
        bus_data_write <= 0;
        effective_address <= 0;
        effective_address_lo_carry <= 0;
        handle_irq <= 0;
        handle_nmi <= 0;
        init <= 0;
        prev_nmi_n <= 1;
        pending_nmi <= 0;

        // Initialize `opcode` to NOP so the vector-load MICRO_EXECUTE at the end
        // of the init sequence preserves the just-reset flags and registers.
        opcode <= OPCODE_NOP;
    end
    else begin
        prev_nmi_n <= nmi_n_sync2;
        if (prev_nmi_n && !nmi_n_sync2)
            pending_nmi <= 1;

        if (i_rdy) begin
            first_microinstruction <= 0;
            prev_mi <= active_microinstruction;
            o_rw <= 0;

            if (handle_irq || handle_nmi) begin
                if (active_microinstruction == WRITE_SR)
                    // Push SR with B=0 (bit 4 clear) to indicate hardware source (IRQ/NMI).
                    bus_data_write <= {status_negative, status_overflow, /* U: */ 1'b1, /* B: */ 1'b0, status_decimal,
                                        status_interrupt, status_zero, status_carry};
                else
                    bus_data_write <= active_microinstruction == PUSH_PCL ? program_counter[7:0] : program_counter[15:8];
            end
            else begin
                priority casez (opcode)
                // SH-family: store reg AND (high(base)+1). The base high byte
                // is in effective_address[15:8] (page-cross high fixup skipped).
                OPCODE_SHY: bus_data_write <= register_y & (effective_address[15:8] + 8'b1);
                OPCODE_SHX: bus_data_write <= register_x & (effective_address[15:8] + 8'b1);
                OPCODE_SHA, OPCODE_SHA2, OPCODE_TAS:
                    bus_data_write <= register_acc & register_x & (effective_address[15:8] + 8'b1);
                OPCODE_TYPE_STA, OPCODE_PHA: bus_data_write <= register_acc;
                OPCODE_PHP: begin
                    // Push SR with B=1 (bit 4 set) to indicate software source (PHP).
                    bus_data_write <= {status_negative, status_overflow, /* U: */ 1'b1, /* B: */ 1'b1, status_decimal,
                                        status_interrupt, status_zero, status_carry};
                end
                OPCODE_TYPE_STX: bus_data_write <= register_x;
                OPCODE_TYPE_STY: bus_data_write <= register_y;
                OPCODE_TYPE_SAX: bus_data_write <= register_acc & register_x;
                OPCODE_BRK: begin
                    if (active_microinstruction == WRITE_SR)
                        // Push SR with B=1 (bit 4 set) to indicate software source (BRK).
                        bus_data_write <= {status_negative, status_overflow, /* U: */ 1'b1, /* B: */ 1'b1, status_decimal,
                                            status_interrupt, status_zero, status_carry};
                    else
                        bus_data_write <= active_microinstruction == PUSH_PCH ? program_counter[15:8] : program_counter[7:0];
                end
                OPCODE_JSR: begin
                    bus_data_write <= active_microinstruction == PUSH_PCL ? program_counter[7:0] : program_counter[15:8];
                end
                default: bus_data_write <= 0;
                endcase
            end

            case (active_microinstruction)
            PUSH_PCH, PUSH_PCL, WRITE_SR, ALU_MODIFY, WRITE: o_rw <= 0;
            default: o_rw <= 1;
            endcase

            if (current_microinstruction == LOAD_INITIAL_PC) begin
                program_counter <= program_counter;
                current_microinstruction <= START;
                o_bus_addr <= program_counter;
                first_microinstruction <= 1;
            end
            else if (next_active_microinstruction == START) begin
                first_microinstruction <= 1;
                o_bus_addr <= program_counter;
                program_counter <= program_counter;
                init <= 0;
            end

            if (first_microinstruction) begin
                if (handle_irq || handle_nmi) begin
                    // Set `opcode` to NOP so the vector-load MICRO_EXECUTE at the end of the interrupt
                    // entry sequence preserves the interrupted instruction's flags and registers.
                    opcode <= OPCODE_NOP;
                end else begin
                    // Normal instruction sequence: latch the opcode from the bus and increment PC.
                    opcode <= current_instruction;
                    program_counter <= program_counter + 1;
                    o_bus_addr <= program_counter + 1;
                end
            end

            if (current_microinstruction == MICRO_INIT) begin
                init_counter <= init_counter + 1;
                if (init_rdy) begin
                    if (START_PC_ENABLED) begin
                        program_counter <= START_PC;
                        operation <= OP_ZERO;
                        current_microinstruction <= LOAD_INITIAL_PC;
                        o_bus_addr <= START_PC;
                    end else begin
                        o_bus_addr <= RESET_VECTOR;
                        current_microinstruction <= READ_VECTOR_HI;
                        init <= 1;
                    end
                end
            end
            else if (active_microinstruction == NOP) begin
                program_counter <= program_counter + 1;
                o_bus_addr <= o_bus_addr + 1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == STALL) begin
                current_microinstruction <= next_active_microinstruction;
                // PLA/PLP: present the value address (0x100 + old_SP + 1)
                // for the final pull read. POP_STACK already incremented SP,
                // so register_sp == old_SP + 1 here.
                if (opcode == OPCODE_PLA || opcode == OPCODE_PLP)
                    o_bus_addr <= {8'b1, register_sp};
                // RTI: present the PCH stack address (0x100 + old_SP + 3) for
                // the final pull read. SP has been incremented three times by
                // POP_STACK/PULL_PCL/PULL_PCH, so register_sp == old_SP + 3.
                if (opcode == OPCODE_RTI)
                    o_bus_addr <= {8'b1, register_sp};
                // RTS C5: read the pulled PCH, compose the return-1 PC, and
                // present it for the final dummy read before the increment.
                if (opcode == OPCODE_RTS) begin
                    program_counter <= {i_bus_data, effective_address_lo};
                    o_bus_addr <= {i_bus_data, effective_address_lo};
                end
                // RMW: the STALL cycle is the dummy write of the old value.
                // Drive the modified value and keep the bus in write so the
                // following MICRO_EXECUTE cycle commits it to the same address.
                priority casez (opcode)
                OPCODE_TYPE_INC, OPCODE_TYPE_DEC, OPCODE_TYPE_ASL,
                OPCODE_TYPE_LSR, OPCODE_TYPE_ROL, OPCODE_TYPE_ROR,
                OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE,
                OPCODE_TYPE_RRA, OPCODE_TYPE_DCP, OPCODE_TYPE_ISB: begin
                    o_rw           <= 0;
                    bus_data_write <= rmw_new;
                end
                default: ;
                endcase
            end
            else if (active_microinstruction == PULL_REGISTER) begin
                current_microinstruction <= next_active_microinstruction;
                // PLA/PLP/RTI: this is the cycle after the dummy read at PC.
                // Present the first stack read address (0x100 + old_SP).
                if (opcode == OPCODE_PLA || opcode == OPCODE_PLP || opcode == OPCODE_RTI)
                    o_bus_addr <= {8'b1, register_sp - 8'b1};
            end
            else if (active_microinstruction == WRITE) begin
                current_microinstruction <= next_active_microinstruction;
                // PHA/PHP: NMOS reads the dummy byte at PC on the cycle
                // after fetch, then writes to the stack. PUSH_STACK leaves
                // PC on the bus for that dummy read; present the stack
                // write target (old SP = current SP + 1, since PUSH_STACK
                // already predecremented) for the write cycle that follows.
                o_bus_addr <= {8'b1, register_sp + 8'b1};
            end
            else if (active_microinstruction == READ_ADL) begin
                program_counter <= program_counter + 2;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == BUFFER_ADL) begin
                effective_address <= {8'b0, i_bus_data};
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= {8'b1, register_sp};
            end
            else if (active_microinstruction == WRITE_SR) begin
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == PC_INC) begin
                // RTS C4: latch the pulled PCL and present the PCH pull address
                // (0x100+old_SP+2). register_sp == old_SP+2 here.
                effective_address[7:0] <= i_bus_data;
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == READ_PCL) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= o_bus_addr + 1;
                program_counter <= program_counter + 1;
            end
            else if (active_microinstruction == PULL_PCL || active_microinstruction == PULL_PCH) begin
                current_microinstruction <= next_active_microinstruction;
                // RTI (only user of PULL_PCL/PULL_PCH): present the next stack
                // read address. register_sp holds old_SP+1 at PULL_PCL and
                // old_SP+2 at PULL_PCH (POP_STACK/PULL_PCL already
                // incremented), so {1, register_sp} steps the read pointer to
                // P+1 (PCL) then P+2 (PCH).
                o_bus_addr <= {8'h1, register_sp};
            end
            else if (active_microinstruction == READ_PCH) begin
                effective_address <= {8'b0, i_bus_data};
                o_bus_addr <= o_bus_addr + 1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == PUSH_PCH || active_microinstruction == PUSH_PCL) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= {8'b1, register_sp};
            end
            else if (active_microinstruction == READ_ADH) begin
                o_bus_addr <= program_counter;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == READ_EFFECTIVE_LO) begin
                o_bus_addr <= program_counter + 1;
                program_counter <= program_counter + 1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == READ_EFFECTIVE_HI) begin
                effective_address <= {8'b0, i_bus_data};
                o_bus_addr <= program_counter + 1;
                program_counter <= program_counter + 1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == LOAD_PC_EFFECTIVE_LO) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= {i_bus_data, effective_address_lo};
            end
            else if (active_microinstruction == LOAD_PC_EFFECTIVE_HI || active_microinstruction == READ_VECTOR_HI) begin
                current_microinstruction <= next_active_microinstruction;
                program_counter <= {8'b0, i_bus_data};
                // NMOS JMP () page bug: the pointer high byte is read from the
                // same page, so the low byte wraps $FF -> $00 with no carry
                // into the high byte. Interrupt-vector reads increment normally.
                if (opcode == OPCODE_JMP_IND)
                    o_bus_addr <= {o_bus_addr[15:8], o_bus_addr[7:0] + 8'b1};
                else
                    o_bus_addr <= o_bus_addr + 1;
            end
            else if (active_microinstruction == LOAD_VECTOR) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= handle_nmi ? NMI_VECTOR : IRQ_VECTOR;
            end
            else if (active_microinstruction == MAYBE_BRANCH) begin
                if (first_microinstruction) begin
                    program_counter <= program_counter + 2;
                    if (branch_taken) begin
                        operation <= OP_CALCULATE_BRANCH_OFFSET;
                    end else begin
                        current_microinstruction <= next_active_microinstruction;
                    end
                end
                else if (operation == OP_CALCULATE_BRANCH_OFFSET) begin
                    if (branch_taken) begin
                        if ((alu_carry_out && !i_bus_data[7]) || (!alu_carry_out && i_bus_data[7])) begin
                            // Page cross. NMOS spends an extra internal cycle to
                            // fix PCH. Present the post-operand PC here, then the
                            // OP_BRANCH_PAGE_CROSS cycle presents the target with
                            // the not-yet-fixed PCH before the corrected fetch.
                            // The branch offset is only on i_bus_data this cycle,
                            // so compute both PC forms now: stash the wrong-PCH
                            // target in effective_address and the fixed target in
                            // program_counter for the SYNC fetch two cycles later.
                            operation <= OP_BRANCH_PAGE_CROSS;
                            o_bus_addr <= program_counter;
                            effective_address <= {program_counter[15:8], alu_result};
                            program_counter <= {program_counter[15:8] + (i_bus_data[7] ? 8'hff : 8'h01), alu_result};
                        end
                        else begin
                            // No page cross. NMOS presents the post-operand PC
                            // for one internal cycle (offset add), then fetches
                            // the target as the next opcode. Hold the current PC
                            // on the bus here; the MICRO_EXECUTE->START transition
                            // presents the branch target for the SYNC fetch.
                            program_counter <= {program_counter[15:8], alu_result};
                            o_bus_addr <= program_counter;
                            current_microinstruction <= next_active_microinstruction;
                        end
                    end
                    else begin
                        current_microinstruction <= next_active_microinstruction;
                    end
                end
                else if (operation == OP_BRANCH_PAGE_CROSS) begin
                        // Present the wrong-PCH target stashed last cycle; the
                        // fixed PC is already in program_counter for the SYNC
                        // opcode fetch that the MICRO_EXECUTE->START step drives.
                        o_bus_addr <= effective_address;
                        current_microinstruction <= next_active_microinstruction;
                end
            end
            else if (active_microinstruction == MICRO_EXECUTE) begin
                if (handle_nmi) handle_nmi <= 0;
                if (handle_irq) handle_irq <= 0;

                if (handle_nmi || handle_irq || init) begin
                    o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    program_counter <= {i_bus_data, program_counter[7:0]};
                end
                else begin
                    priority casez (opcode)
                    OPCODE_JMP_ABS: begin
                        program_counter <= {i_bus_data, effective_address_lo};
                        o_bus_addr <= {i_bus_data, effective_address_lo};
                    end
                    OPCODE_JMP_IND: begin
                        program_counter <= {i_bus_data, program_counter[7:0]};
                        o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    end
                    OPCODE_RTS: begin
                        // RTS C6: present the dummy read at return-1 (already on
                        // the bus), then increment PC so the next opcode fetches
                        // at the return address.
                        program_counter <= program_counter + 1;
                        o_bus_addr <= program_counter + 1;
                    end
                    OPCODE_BRK: begin
                        program_counter <= {i_bus_data, program_counter[7:0]};
                        o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    end
                    OPCODE_JSR: begin
                        program_counter <= {i_bus_data, effective_address_lo};
                        o_bus_addr <= {i_bus_data, effective_address_lo};
                    end
                    OPCODE_RTI: begin
                        // Final stack read (PCH). Compose the restored PC from
                        // the PCL latched at STALL and present it for the next
                        // opcode fetch.
                        program_counter <= {i_bus_data, program_counter[7:0]};
                        o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    end
                    default: ;
                    endcase
                end

                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == LOAD || active_microinstruction == STORE) begin
                if (first_microinstruction) begin
                    program_counter <= program_counter + 2;
                    o_bus_addr <= program_counter + 1;
                    case (addressing_mode)
                        IMMEDIATE: current_microinstruction <= next_active_microinstruction;
                        ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT: operation <= OP_ABSOLUTE_LO;
                        INDEX_X_INDIRECT, INDEX_Y_INDIRECT, ZP, ZP_X, ZP_Y: operation <= OP_LOAD_ZP;

                        // invalid opcode, continue
                        default: begin
                            current_microinstruction <= next_active_microinstruction;
                        end
                    endcase
                end
                else begin
                    if (operation == OP_LOAD_ZP) begin
                        case (addressing_mode)
                            ZP: begin
                                current_microinstruction <= next_active_microinstruction;
                                effective_address <= {8'b0, i_bus_data};
                                o_bus_addr <= {8'b0, i_bus_data};
                                if (active_microinstruction == STORE)
                                    o_rw <= 0;
                            end
                            INDEX_Y_INDIRECT: begin
                                o_bus_addr <= {8'b0, i_bus_data};
                                operation <= OP_ABSOLUTE_LO;
                            end
                            INDEX_X_INDIRECT, ZP_X, ZP_Y: begin
                                // NMOS reads the un-indexed zp base for one
                                // dummy cycle, then the indexed address. The
                                // base is on i_bus_data only now, so add the
                                // index here (zp wraps) and stash it; present
                                // the base for the dummy read.
                                effective_address <= {8'b0,
                                    i_bus_data + (addressing_mode == ZP_Y ? register_y : register_x)};
                                o_bus_addr <= {8'b0, i_bus_data};
                                operation <= OP_LOAD_ZP_INDEXED;
                            end
                            // invalid opcode, continue
                            default: begin
                                current_microinstruction <= next_active_microinstruction;
                            end
                        endcase
                    end
                    else if (operation == OP_LOAD_ZP_INDEXED) begin
                        // Present the indexed address stashed during the base
                        // dummy read. INDEX_X_INDIRECT continues to read the
                        // pointer there; ZP_X/ZP_Y read the operand directly.
                        o_bus_addr <= effective_address;
                        if (addressing_mode == INDEX_X_INDIRECT) begin
                            operation <= OP_ABSOLUTE_LO;
                        end
                        else begin
                            current_microinstruction <= next_active_microinstruction;
                            if (active_microinstruction == STORE)
                                o_rw <= 0;
                        end
                    end
                    else if (operation == OP_ABSOLUTE_LO) begin
                        if (addressing_mode == ABSOLUTE_X || addressing_mode == ABSOLUTE_Y || addressing_mode == ABSOLUTE)
                            program_counter <= program_counter + 1;

                        effective_address <= {8'b0, i_bus_data};
                        // Zero-page indirect pointers wrap within page zero; the
                        // high byte is read from (base+1) & $FF, not base+1.
                        if (addressing_mode == INDEX_X_INDIRECT || addressing_mode == INDEX_Y_INDIRECT)
                            o_bus_addr <= {8'b0, o_bus_addr[7:0] + 8'b1};
                        else
                            o_bus_addr <= o_bus_addr + 1;
                        operation <= OP_ABSOLUTE_HI;

                        if (opcode == OPCODE_JMP_ABS || opcode == OPCODE_JSR)
                            current_microinstruction <= next_active_microinstruction;
                    end
                    else if (operation == OP_ABSOLUTE_HI) begin
                        effective_address <= {i_bus_data, alu_result};
                        o_bus_addr <= {i_bus_data, alu_result};
                        effective_address_lo_carry <= alu_carry_out;

                        if (opcode == OPCODE_JMP_IND) begin
                            operation <= OP_LOAD_INDIRECT_LO;
                        end
                        else if (alu_carry_out || (active_microinstruction == STORE
                                 && addressing_mode != ABSOLUTE
                                 && addressing_mode != INDEX_X_INDIRECT))
                            operation <= OP_ABSOLUTE_PAGE_CROSS;
                        else begin
                            priority casez (opcode)
                            OPCODE_TYPE_INC, OPCODE_TYPE_DEC, OPCODE_TYPE_ROR, OPCODE_TYPE_ROL, OPCODE_TYPE_ASL,
                            OPCODE_TYPE_LSR,
                            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE,
                            OPCODE_TYPE_RRA, OPCODE_TYPE_DCP, OPCODE_TYPE_ISB: begin
                                // Post-indexed RMW always spends the mandatory
                                // extra read cycle at the effective address.
                                // Plain absolute and (zp,X) have no post-index,
                                // so they read once and proceed to the modify.
                                if (addressing_mode == ABSOLUTE_X
                                    || addressing_mode == ABSOLUTE_Y
                                    || addressing_mode == INDEX_Y_INDIRECT)
                                    operation <= OP_ABSOLUTE_PAGE_CROSS;
                                else
                                    current_microinstruction <= next_active_microinstruction;
                            end
                            default: begin
                                current_microinstruction <= next_active_microinstruction;
                                if (active_microinstruction == STORE)
                                    o_rw <= 0;
                            end
                            endcase
                        end
                    end
                    else if (operation == OP_LOAD_INDIRECT_LO) begin
                        // NMOS behavior, low byte wraps around (high byte not incremented)
                        o_bus_addr <= o_bus_addr + 1;
                        effective_address <= {8'b0, i_bus_data};
                        current_microinstruction <= next_active_microinstruction;
                    end
                    else if (operation == OP_ABSOLUTE_PAGE_CROSS) begin
                        // SH-family stores skip the page-cross high-byte fixup:
                        // the target keeps high(base) and the value already
                        // masked it. Other ops apply the carried high byte.
                        priority casez (opcode)
                        OPCODE_SHY, OPCODE_SHX, OPCODE_SHA, OPCODE_SHA2, OPCODE_TAS: begin
                            o_bus_addr <= effective_address;
                            o_rw <= 0;
                            // TAS also copies A AND X into the stack pointer.
                            if (opcode == OPCODE_TAS)
                                register_sp <= register_acc & register_x;
                        end
                        default: begin
                            effective_address <= {alu_result, effective_address[7:0]};
                            o_bus_addr <= {alu_result, effective_address[7:0]};
                            if (active_microinstruction == STORE)
                                o_rw <= 0;
                        end
                        endcase
                        current_microinstruction <= next_active_microinstruction;
                    end
                end
            end
            else if (active_microinstruction == POP_STACK) begin
                current_microinstruction <= next_active_microinstruction;
                // PLA/PLP/RTI: leave PC on the bus for the NMOS dummy read
                // after fetch; the following microinstructions present the
                // stack addresses. RTS keeps the immediate stack address here.
                // POP_STACK runs in the collapsed fetch cycle, where `opcode`
                // is not yet latched, so gate on current_instruction.
                if (current_instruction != OPCODE_PLA && current_instruction != OPCODE_PLP
                    && current_instruction != OPCODE_RTI && current_instruction != OPCODE_RTS)
                    o_bus_addr <= {8'b1, register_sp + 8'b1};
            end
            else if (active_microinstruction == RESTORE_STACK2) begin
                // RTS C3: the dummy stack read at 0x100+old_SP. Present the
                // PCL pull address (old_SP+1) next. register_sp == old_SP+1
                // here (POP_STACK incremented once); RESTORE_STACK2 increments
                // again so it becomes old_SP+2 at PC_INC.
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == RESTORE_STACK) begin
                // RTS C2: the dummy read at PC+1 (left on the bus by the fetch).
                // Present the dummy stack read address 0x100+old_SP next.
                // register_sp == old_SP+1 here (POP_STACK already incremented).
                o_bus_addr <= {8'b1, register_sp - 8'b1};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == PUSH_STACK) begin
                // Leave PC on the bus (set by the opcode-fetch block) so the
                // cycle after fetch is the NMOS dummy read at PC. The WRITE
                // microinstruction presents the stack write target next.
                current_microinstruction <= next_active_microinstruction;

                if (opcode == OPCODE_JSR) begin
                    effective_address <= {i_bus_data, effective_address_lo};
                end
            end
            else if (active_microinstruction == ALU_MODIFY) begin
                current_microinstruction <= next_active_microinstruction;
                // NMOS read-modify-write writes the unmodified value back first
                // (the dummy write), then the modified value. Drive the OLD byte
                // for the upcoming write cycle and latch the NEW byte for the one
                // after, presented from the STALL cycle below.
                bus_data_write <= i_bus_data;
                rmw_new        <= alu_result;
            end

            // RTI reconstructs PC explicitly: PCL is read at the STALL cycle
            // (bus address 0x100 + old_SP + 2) and PCH at MICRO_EXECUTE. The
            // old prev_mi-based PULL_PCL/PULL_PCH latches are superseded.
            if (opcode == OPCODE_RTI && active_microinstruction == STALL)
                program_counter[7:0] <= i_bus_data;

            if (next_active_microinstruction == START && !i_irq_n && !status_interrupt && !handle_irq && !handle_nmi) begin
                handle_irq <= 1;
                o_bus_addr <= {8'b1, register_sp};
            end
            else if (next_active_microinstruction == START && pending_nmi && !handle_irq && !handle_nmi && !init) begin
                handle_irq <= 1;
                handle_nmi <= 1;
                pending_nmi <= 0;
                o_bus_addr <= {8'b1, register_sp};
            end
        end
    end
end

always @(negedge i_clk or negedge i_reset_n) begin
    if (!i_reset_n) begin
        register_acc <= 0;
        register_y <= 0;
        register_x <= 0;
        register_sp <= 0;
    end
    else begin
        if (i_rdy) begin
            case (active_microinstruction)
                POP_STACK: register_sp <= register_sp + 8'b1;
                PUSH_STACK, PUSH_PCL, PUSH_PCH, WRITE_SR: register_sp <= register_sp - 8'b1;
                PULL_PCL, PULL_PCH: register_sp <= register_sp + 8'b1;
                RESTORE_STACK2: register_sp <= register_sp + 8'b1;
                PULL_REGISTER: begin
                    // PLA/PLP latch the pulled value at MICRO_EXECUTE (the
                    // final stack read), not here: this cycle is the dummy
                    // stack read at 0x100 + old_SP.
                end
                MICRO_EXECUTE: begin
                    priority casez (opcode)
                    OPCODE_PLA: register_acc <= i_bus_data;
                    OPCODE_PLP: begin
                        // no register file update (status handled below)
                    end
                    OPCODE_TYPE_BRANCH: begin
                        // noop
                    end
                    OPCODE_TAX: register_x <= register_acc;
                    OPCODE_TXA: register_acc <= register_x;
                    OPCODE_TYA: register_acc <= register_y;
                    OPCODE_TAY: register_y <= register_acc;
                    OPCODE_TSX: register_x <= register_sp;
                    OPCODE_TXS: register_sp <= register_x;
                    OPCODE_INX, OPCODE_DEX: register_x <= alu_result;
                    OPCODE_INY, OPCODE_DEY: register_y <= alu_result;
                    OPCODE_ASL_ACC, OPCODE_LSR_ACC, OPCODE_ROL_ACC, OPCODE_ROR_ACC: register_acc <= alu_result;
                    OPCODE_TYPE_LDA: register_acc <= i_bus_data;
                    OPCODE_TYPE_LDX: register_x <= i_bus_data;
                    OPCODE_TYPE_LDY: register_y <= i_bus_data;
                    OPCODE_LAS: begin
                        register_acc <= i_bus_data & register_sp;
                        register_x   <= i_bus_data & register_sp;
                        register_sp  <= i_bus_data & register_sp;
                    end
                    OPCODE_TYPE_LAX: begin
                        register_acc <= i_bus_data;
                        register_x   <= i_bus_data;
                    end
                    // Undocumented immediate ALU ops (before the cc=11 combos).
                    OPCODE_ANC, OPCODE_ANC2, OPCODE_USBC:
                        register_acc <= alu_result;
                    OPCODE_XAA:
                        register_acc <= register_x & i_bus_data;
                    OPCODE_ALR:
                        register_acc <= {1'b0, alu_result[7:1]};
                    OPCODE_ARR:
                        register_acc <= status_decimal
                            ? arr_dec_result
                            : {status_carry, alu_result[7:1]};
                    OPCODE_AXS:
                        register_x <= alu_result;
                    OPCODE_TYPE_ADC, OPCODE_TYPE_AND, OPCODE_TYPE_ORA,
                    OPCODE_TYPE_EOR, OPCODE_TYPE_SBC:
                        register_acc <= alu_result;
                    // RMW+ALU combos write the accumulator op result back to A
                    // (DCP only compares, so it leaves A unchanged).
                    OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE,
                    OPCODE_TYPE_RRA, OPCODE_TYPE_ISB:
                        register_acc <= alu_result;
                    default: ;
                    endcase
                end
                default: ;
            endcase
        end
    end
end

always @(negedge i_clk or negedge i_reset_n) begin
    if (!i_reset_n) begin
        status_negative <= 0;
        status_decimal <= 0;
        status_overflow <= 0;
        status_carry <= 0;
        status_zero <= 0;
        status_interrupt <= 0;
    end else begin
        if (init_rdy) begin
            status_negative <= 0;
            status_overflow <= 0;
            status_decimal <= 0;
            status_interrupt <= 1;
            status_zero <= 0;
            status_carry <= 0;
        end
        if (active_microinstruction == PULL_PCH && opcode == OPCODE_RTI && i_rdy) begin
            // RTI pulls P at the first stack read (bus address 0x100+old_SP+1),
            // which is the PULL_PCH cycle under the dummy-read reordering.
            status_negative <= i_bus_data[7];
            status_overflow <= i_bus_data[6];
            status_decimal <= i_bus_data[3];
            status_interrupt <= i_bus_data[2];
            status_zero <= i_bus_data[1];
            status_carry <= i_bus_data[0];
        end
        if (active_microinstruction == MICRO_EXECUTE && i_rdy) begin
            if (handle_irq)
                status_interrupt <= 1;

            priority casez (opcode)
            // Undocumented multi-byte NOPs read and discard: no flag effects.
            // Listed first so the ones that share a pattern with BIT/CPY/CPX
            // do not commit those flags.
            8'h80, 8'h82, 8'h89, 8'hC2, 8'hE2,
            8'h04, 8'h44, 8'h64,
            8'h14, 8'h34, 8'h54, 8'h74, 8'hD4, 8'hF4,
            8'h0C,
            8'h1C, 8'h3C, 8'h5C, 8'h7C, 8'hDC, 8'hFC: begin
            end
            OPCODE_TYPE_BRANCH: begin
            end
            OPCODE_JSR: begin
                // no updates
            end
            OPCODE_PLP: begin
                // PLP pulls P at the final stack read (this cycle).
                status_negative <= i_bus_data[7];
                status_overflow <= i_bus_data[6];
                status_decimal <= i_bus_data[3];
                status_interrupt <= i_bus_data[2];
                status_zero <= i_bus_data[1];
                status_carry <= i_bus_data[0];
            end
            OPCODE_PLA: begin
                status_negative <= i_bus_data[7];
                status_zero <= i_bus_data == 0;
            end
            OPCODE_SEC: status_carry <= 1;
            OPCODE_CLC: status_carry <= 0;
            OPCODE_SED: status_decimal <= 1;
            OPCODE_CLD: status_decimal <= 0;
            OPCODE_SEI: status_interrupt <= 1;
            OPCODE_CLI: status_interrupt <= 0;
            OPCODE_CLV: status_overflow <= 0;
            OPCODE_BRK: status_interrupt <= 1;
            OPCODE_TSX: begin
                status_negative <= register_sp[7];
                status_zero <= register_sp == 0;
            end
            OPCODE_TAX, OPCODE_TAY: begin
                status_negative <= register_acc[7];
                status_zero <= register_acc == 0;
            end
            OPCODE_TYA: begin
                status_negative <= register_y[7];
                status_zero <= register_y == 0;
            end
            OPCODE_TXA: begin
                status_negative <= register_x[7];
                status_zero <= register_x == 0;
            end
            OPCODE_INX, OPCODE_INY, OPCODE_DEX, OPCODE_DEY: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
            end
            OPCODE_TYPE_BIT: begin
                status_negative <= i_bus_data[7];
                status_overflow <= i_bus_data[6];
                status_zero <= alu_result == 0;
            end
            OPCODE_TYPE_CMP, OPCODE_TYPE_CPX, OPCODE_TYPE_CPY: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_ROL_ACC, OPCODE_ASL_ACC, OPCODE_LSR_ACC, OPCODE_ROR_ACC: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_TYPE_AND, OPCODE_TYPE_EOR, OPCODE_TYPE_ORA: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
            end
            OPCODE_LAS: begin
                status_negative <= i_bus_data[7] & register_sp[7];
                status_zero <= (i_bus_data & register_sp) == 0;
            end
            OPCODE_TYPE_LDA, OPCODE_TYPE_LDX, OPCODE_TYPE_LDY, OPCODE_TYPE_LAX: begin
                status_negative <= i_bus_data[7];
                status_zero <= i_bus_data == 0;
            end
            OPCODE_TYPE_ADC, OPCODE_TYPE_SBC: begin
                status_negative <= alu_negative;
                status_zero <= alu_zero;
                status_carry <= alu_carry_out;
                status_overflow <= alu_overflow;
            end
            // Undocumented immediate ALU ops (before the cc=11 combos).
            OPCODE_ANC, OPCODE_ANC2: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_result[7];
            end
            OPCODE_XAA: begin
                status_negative <= (register_x & i_bus_data) >> 7;
                status_zero <= (register_x & i_bus_data) == 0;
            end
            OPCODE_ALR: begin
                status_negative <= 1'b0;
                status_zero <= alu_result[7:1] == 0;
                status_carry <= alu_result[0];
            end
            OPCODE_ARR: begin
                // AND then ROR: result = {oldC, (A&imm)[7:1]}. NMOS sets C from
                // result bit6 (= (A&imm)[7]) and V from result bit6 ^ bit5. In
                // decimal mode N/Z/V stay binary but C comes from the high-nibble
                // BCD adjust.
                status_negative <= status_carry;
                status_zero <= {status_carry, alu_result[7:1]} == 0;
                status_carry <= status_decimal ? arr_high_fix : alu_result[7];
                status_overflow <= alu_result[7] ^ alu_result[6];
            end
            OPCODE_AXS: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_USBC: begin
                status_negative <= alu_negative;
                status_zero <= alu_zero;
                status_carry <= alu_carry_out;
                status_overflow <= alu_overflow;
            end
            // RMW+ALU combos commit the accumulator op's flags. SLO/RLA/SRE
            // leave C from the shift (set at ALU_MODIFY); RRA/DCP/ISB set C
            // here, and RRA/ISB also set V.
            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
            end
            OPCODE_TYPE_DCP: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_TYPE_RRA, OPCODE_TYPE_ISB: begin
                status_negative <= alu_negative;
                status_zero <= alu_zero;
                status_carry <= alu_carry_out;
                status_overflow <= alu_overflow;
            end
            default: begin
            end
            endcase
        end
        else if (active_microinstruction == ALU_MODIFY && i_rdy) begin
            priority casez (opcode)
            OPCODE_TYPE_ASL, OPCODE_TYPE_LSR, OPCODE_TYPE_ROL, OPCODE_TYPE_ROR: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_TYPE_INC, OPCODE_TYPE_DEC: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
            end
            // RMW+ALU shift combos: publish the shift carry now. For
            // SLO/RLA/SRE it is the final C (the ORA/AND/EOR leave C alone);
            // for RRA it is the ROR carry-out that feeds the ADC carry-in.
            // N/Z come from the accumulator op at MICRO_EXECUTE.
            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE, OPCODE_TYPE_RRA:
                status_carry <= alu_carry_out;
            default: ;
            endcase
        end

        if (trigger_overflow)
            status_overflow <= 1;
    end
end

reg load_or_store;
always_comb begin
    alu_operation = ALU_ADC;
    alu_carry_in = 0;
    alu_lhs = 0;
    alu_rhs = 0;
    alu_decimal = 0;
    load_or_store = active_microinstruction == LOAD || active_microinstruction == STORE; 

    if (load_or_store && operation == OP_LOAD_ZP_INDEXED) begin
        alu_lhs = i_bus_data;
        alu_rhs = (addressing_mode == ZP_X || addressing_mode == INDEX_X_INDIRECT) ? register_x : register_y;
    end
    else if (load_or_store && operation == OP_ABSOLUTE_PAGE_CROSS) begin
        alu_lhs = effective_address_hi;
        alu_rhs = {7'b0, effective_address_lo_carry};
    end
    else if (load_or_store && operation == OP_ABSOLUTE_HI) begin
        alu_lhs = effective_address_lo;
        if (addressing_mode == ABSOLUTE_X)
            alu_rhs = register_x;
        else if(addressing_mode == ABSOLUTE_Y || addressing_mode == INDEX_Y_INDIRECT)
            alu_rhs = register_y;
    end
    else if (active_microinstruction == MAYBE_BRANCH) begin
        alu_lhs = program_counter[7:0];
        alu_rhs = i_bus_data;
    end
    else if (active_microinstruction == STORE) begin
        alu_lhs = i_bus_data;
        priority casez (opcode)
        OPCODE_TYPE_INC: alu_rhs = 8'h01;
        OPCODE_TYPE_DEC: alu_rhs = 8'hff;
        default: ;
        endcase
    end
    else if (active_microinstruction == MICRO_EXECUTE) begin
        alu_lhs = register_acc;
        alu_rhs = i_bus_data;
        alu_carry_in = status_carry;
        priority casez (opcode)
        OPCODE_DEY: begin
            alu_lhs = register_y;
            alu_rhs = 8'hff;
            alu_carry_in = 0;
        end
        OPCODE_DEX: begin
            alu_lhs = register_x;
            alu_rhs = 8'hff;
            alu_carry_in = 0;
        end
        OPCODE_INY: begin
            alu_lhs = register_y;
            alu_rhs = 1;
            alu_carry_in = 0;
        end
        OPCODE_INX: begin
            alu_lhs = register_x;
            alu_rhs = 1;
            alu_carry_in = 0;
        end
        OPCODE_TYPE_BRANCH: begin
            alu_lhs = program_counter[7:0];
            alu_carry_in = 0;
            if (i_bus_data[7]) begin
                alu_carry_in = 0;
            end else begin
                alu_rhs = i_bus_data;
            end
        end
        OPCODE_TYPE_BIT: begin
            alu_carry_in = 0;
            alu_lhs = register_acc;
            alu_rhs = i_bus_data;
            alu_operation = ALU_AND;
        end
        OPCODE_TYPE_CPY: begin
            alu_lhs = register_y;
            alu_rhs = ~i_bus_data;
            alu_carry_in = 1;
        end
        OPCODE_TYPE_CPX: begin
            alu_lhs = register_x;
            alu_rhs = ~i_bus_data;
            alu_carry_in = 1;
        end
        OPCODE_TYPE_CMP: begin
            alu_lhs = register_acc;
            alu_rhs = ~i_bus_data;
            alu_carry_in = 1;
        end
        OPCODE_TYPE_AND: alu_operation = ALU_AND;
        OPCODE_TYPE_ORA: alu_operation = ALU_ORA;
        OPCODE_TYPE_EOR: alu_operation = ALU_EOR;
        OPCODE_TYPE_ADC: begin
            alu_rhs = i_bus_data;
            alu_decimal = status_decimal;
        end
        OPCODE_TYPE_SBC: begin
            alu_rhs = ~i_bus_data;
            alu_carry_in = status_carry;
            alu_decimal = status_decimal;
            alu_operation = ALU_SBC;
        end
        // Undocumented immediate ALU ops (placed before the cc=11 combos).
        OPCODE_USBC: begin
            alu_rhs = ~i_bus_data;
            alu_carry_in = status_carry;
            alu_decimal = status_decimal;
            alu_operation = ALU_SBC;
        end
        OPCODE_ANC, OPCODE_ANC2, OPCODE_ALR: alu_operation = ALU_AND;
        OPCODE_ARR: alu_operation = ALU_AND;
        OPCODE_AXS: begin
            alu_lhs = register_acc & register_x;
            alu_rhs = ~i_bus_data;
            alu_carry_in = 1;
        end
        // RMW+ALU combos: the accumulator op runs against the modified byte
        // latched in rmw_new (the bus is mid-write this cycle, so i_bus_data
        // is not the operand).
        OPCODE_TYPE_SLO: begin
            alu_rhs = rmw_new;
            alu_operation = ALU_ORA;
        end
        OPCODE_TYPE_RLA: begin
            alu_rhs = rmw_new;
            alu_operation = ALU_AND;
        end
        OPCODE_TYPE_SRE: begin
            alu_rhs = rmw_new;
            alu_operation = ALU_EOR;
        end
        OPCODE_TYPE_RRA: begin
            alu_rhs = rmw_new;
            alu_carry_in = status_carry;
            alu_decimal = status_decimal;
            alu_operation = ALU_ADC;
        end
        OPCODE_TYPE_DCP: begin
            alu_rhs = ~rmw_new;
            alu_carry_in = 1;
            alu_operation = ALU_ADC;
        end
        OPCODE_TYPE_ISB: begin
            alu_rhs = ~rmw_new;
            alu_carry_in = status_carry;
            alu_decimal = status_decimal;
            alu_operation = ALU_SBC;
        end
        OPCODE_TYPE_ROR: begin
            if (opcode == OPCODE_ROR_ACC)
                alu_lhs = register_acc;
            else
                alu_lhs = i_bus_data;

            alu_rhs = 0;
            alu_operation = ALU_ROR;
        end
        OPCODE_TYPE_ROL: begin
            if (opcode == OPCODE_ROL_ACC)
                alu_lhs = register_acc;
            else
                alu_lhs = i_bus_data;

            alu_rhs = 0;
            alu_operation = ALU_ROL;
        end
        OPCODE_TYPE_ASL: begin
            if (opcode == OPCODE_ASL_ACC)
                alu_lhs = register_acc;
            else
                alu_lhs = i_bus_data;

            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_ASL;
        end
        OPCODE_TYPE_LSR: begin
            if (opcode == OPCODE_LSR_ACC)
                alu_lhs = register_acc;
            else
                alu_lhs = i_bus_data;

            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_LSR;
        end
        default: ;
        endcase
    end
    else if (active_microinstruction == ALU_MODIFY) begin
        alu_lhs = i_bus_data;
        priority casez (opcode)
        OPCODE_TYPE_INC: alu_rhs = 1;
        OPCODE_TYPE_DEC: begin
            alu_rhs = 8'hff;
        end
        OPCODE_TYPE_ROR: begin
            alu_rhs = 0;
            alu_operation = ALU_ROR;
            alu_carry_in = status_carry;
        end
        OPCODE_TYPE_ROL: begin
            alu_rhs = 0;
            alu_operation = ALU_ROL;
            alu_carry_in = status_carry;
        end
        OPCODE_TYPE_ASL: begin
            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_ASL;
        end
        OPCODE_TYPE_LSR: begin
            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_LSR;
        end
        OPCODE_TYPE_ISB: alu_rhs = 1;
        OPCODE_TYPE_DCP: alu_rhs = 8'hff;
        OPCODE_TYPE_RRA: begin
            alu_rhs = 0;
            alu_operation = ALU_ROR;
            alu_carry_in = status_carry;
        end
        OPCODE_TYPE_RLA: begin
            alu_rhs = 0;
            alu_operation = ALU_ROL;
            alu_carry_in = status_carry;
        end
        OPCODE_TYPE_SLO: begin
            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_ASL;
        end
        OPCODE_TYPE_SRE: begin
            alu_carry_in = 0;
            alu_rhs = 0;
            alu_operation = ALU_LSR;
        end
        default: ;
        endcase
    end
end

cpu_6502_alu alu (
    .i_operation(alu_operation),
    .i_lhs(alu_lhs),
    .i_rhs(alu_rhs),
    .i_carry(alu_carry_in),
    .i_bcd(alu_decimal),
    .o_result(alu_result),
    .o_carry(alu_carry_out),
    .o_overflow(alu_overflow),
    .o_negative(alu_negative),
    .o_zero(alu_zero)
);

// NMOS decimal-mode ARR fixup. ARR ANDs A with the immediate (alu_result),
// rotates right through carry to form the binary result arr_bin, then applies a
// BCD correction to the stored byte. N/Z/V still come from arr_bin; C comes from
// the high-nibble adjust. t is the pre-rotate AND value (alu_result).
wire [7:0] arr_bin = {status_carry, alu_result[7:1]};
wire arr_low_fix  = (alu_result[3:0] + {4'b0, alu_result[0]}) > 5'd5;
wire arr_high_fix = (alu_result[7:4] + {4'b0, alu_result[4]}) > 5'd5;
wire [7:0] arr_after_low = arr_low_fix
    ? {arr_bin[7:4], arr_bin[3:0] + 4'h6}
    : arr_bin;
wire [7:0] arr_dec_result = arr_high_fix
    ? arr_after_low + 8'h60
    : arr_after_low;

always_comb begin
    case (i_debug_sel)
        3'd0: o_debug_data = {2'b00, active_microinstruction};
        3'd1: o_debug_data = {2'b00, next_active_microinstruction};
        3'd2: o_debug_data = opcode;
        3'd3: o_debug_data = {4'b0000, operation};
        3'd4: o_debug_data = program_counter[15:8];
        3'd5: o_debug_data = program_counter[7:0];
        3'd6: o_debug_data = register_acc;
        3'd7: o_debug_data = {status_negative, status_overflow, 1'b1, 1'b1,
                              status_decimal, status_interrupt, status_zero, status_carry};
        default: o_debug_data = 8'h00;
    endcase
end

endmodule
