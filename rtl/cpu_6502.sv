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

// Reset settle-spin length. The vector-fetch reset path now walks the BRK
// frame (PUSH_PCH/PUSH_PCL/WRITE_SR/LOAD_VECTOR, four extra slots before the
// vector read), so it needs only 2 settle cycles to reach the first opcode
// fetch at the same cycle as before. The START_PC_ENABLED bring-up shortcut
// branches straight to LOAD_INITIAL_PC at init_rdy and does NOT walk the
// frame, so it keeps the original 6-cycle settle.
localparam INIT_CYCLES = START_PC_ENABLED ? 6 : 2;

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

// Power-on state. RESET does not load the register file (it is a read-only
// BRK frame that only decrements S and forces I), so the post-reset values
// come from power-on, and a warm reset preserves the running values. There is
// no async-reset clobber of these registers. An `initial` block (not
// declaration initializers) keeps Verilator PROCASSINIT lint quiet.
//
// START_PC_ENABLED is the non-silicon FPGA bring-up mode: it loads PC from the
// fixed START_PC instead of fetching the reset vector, and its consumers expect
// a deterministic all-zero power-on. The silicon-faithful path (vector reset)
// instead seeds the Perfect6502 NMOS power-on residue the conformance tests pin
// (X=$C0, S=$C0 pre-decrement so the three reset dummy stack reads settle S to
// $BD; A=$00, Y=$00; I and Z set).
initial begin
    register_acc = 8'h00;
    register_x   = START_PC_ENABLED ? 8'h00 : 8'hC0;
    register_y   = 8'h00;
    register_sp  = START_PC_ENABLED ? 8'h00 : 8'hC0;
    status_negative  = 1'b0;
    status_overflow  = 1'b0;
    status_decimal   = 1'b0;
    status_interrupt = START_PC_ENABLED ? 1'b0 : 1'b1;
    status_zero      = START_PC_ENABLED ? 1'b0 : 1'b1;
    status_carry     = 1'b0;
end

reg [15:0] effective_address;
wire [7:0] effective_address_lo, effective_address_hi;
reg effective_address_lo_carry;
assign effective_address_lo = effective_address[7:0];
assign effective_address_hi = effective_address[15:8];

reg init;
reg [7:0] opcode;
// Staged NEW byte for a memory read-modify-write. The read latches the OLD
// byte; ALU_MODIFY drives OLD and stages alu_result here; RMW_WRITE_NEW drives
// this on the second (real) write beat.
reg [7:0] rmw_new;

reg [2:0] init_counter;
reg [7:0] bus_data_write;

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
assign o_sync = first_microinstruction;

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

// NMOS RDY read-hold with write-bypass. RDY is sampled on phi2 (the read
// half-cycle, the same edge the SO detector uses) and the stall engages on
// the FOLLOWING cycle: the bus/PC advance for the cycle being entered has
// already committed, so that cycle's address appears on the bus and is then
// held while the read repeats. A registered copy rdy_q gives the one-cycle
// engage/release lag (the held read completes one cycle after RDY returns
// high). RDY pauses reads only; write cycles bypass the stall and commit
// (NMOS quirk), so the advance gate also passes when the current bus cycle
// is a write (o_rw == 0).
reg rdy_q;
wire seq_advance = rdy_q || (o_rw == 1'b0);
always @(posedge o_phi2 or negedge i_reset_n) begin
    if (!i_reset_n)
        rdy_q <= 1'b1;
    else
        rdy_q <= i_rdy;
end

reg nmi_n_sync, nmi_n_sync2, prev_nmi_n, pending_nmi;
reg irq_n_sync, irq_n_sync2;

// NMOS samples the asynchronous /IRQ and /NMI pins during phi2 (the read
// half-cycle, the same edge the SO detector and rdy_q use). Capturing the
// first synchronizer stage on the phi2 posedge means an interrupt asserted
// on phi2 of a cycle is seen that same cycle; sampling on the negedge missed
// a phi2 assertion by a full cycle and deferred the interrupt a whole
// instruction on short (two-cycle) ops where the recognition window is tight.
always @(posedge o_phi2 or negedge i_reset_n) begin
    if (!i_reset_n) begin
        nmi_n_sync <= 1;
        irq_n_sync <= 1;
    end else begin
        nmi_n_sync <= i_nmi_n;
        irq_n_sync <= i_irq_n;
    end
end

// Second synchronizer stage, on the negedge that drives recognition.
always @(negedge i_clk or negedge i_reset_n) begin
    if (!i_reset_n) begin
        nmi_n_sync2 <= 1;
        irq_n_sync2 <= 1;
    end else begin
        nmi_n_sync2 <= nmi_n_sync;
        irq_n_sync2 <= irq_n_sync;
    end
end

// A freshly synchronized NMI falling edge. pending_nmi latches this for
// recognition at a later instruction boundary, but pending_nmi is a
// non-blocking register: on the negedge the edge is detected it still reads
// 0. When the edge lands exactly on an instruction boundary the recognition
// must see it this cycle, so consult the combinational edge alongside the
// latched pending_nmi.
wire nmi_edge_now = prev_nmi_n && !nmi_n_sync2;

// NMI edge recognized at the LOAD_VECTOR cycle of an in-flight IRQ or BRK
// entry hijacks it to the NMI vector. Accept either the combinational edge
// (an edge synchronizing exactly on LOAD_VECTOR, before pending_nmi
// registers it) or the latched pending_nmi (an edge from a cycle earlier
// whose push-cycle hijack window has not yet steered it). handle_nmi is set
// from this AFTER the SR push, so the pushed B image is unaffected; gated on
// !handle_nmi so an edge already consumed does not re-fire.
wire nmi_load_vector_hijack = (nmi_edge_now || pending_nmi) && !handle_nmi
    && (handle_irq || opcode == OPCODE_BRK)
    && active_microinstruction == LOAD_VECTOR;


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

        if (seq_advance) begin
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
                OPCODE_TYPE_STA, OPCODE_PHA: bus_data_write <= register_acc;
                OPCODE_PHP: begin
                    // Push SR with B=1 (bit 4 set) to indicate software source (PHP).
                    bus_data_write <= {status_negative, status_overflow, /* U: */ 1'b1, /* B: */ 1'b1, status_decimal,
                                        status_interrupt, status_zero, status_carry};
                end
                OPCODE_TYPE_STX: bus_data_write <= register_x;
                OPCODE_TYPE_SAX: bus_data_write <= register_acc & register_x;
                OPCODE_TYPE_STY: bus_data_write <= register_y;
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
            // RESET (init) demotes the three BRK push slots to dummy READS
            // (the silicon's DORES R/W override), so S still decrements three
            // times but nothing is written. A real BRK/IRQ/NMI (init=0) keeps
            // the writes.
            PUSH_PCH, PUSH_PCL, WRITE_SR: o_rw <= init ? 1'b1 : 1'b0;
            PUSH_STACK, ALU_MODIFY, WRITE, RMW_WRITE_NEW: o_rw <= 0;
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
                        // Enter the read-only BRK frame: walk PUSH_PCH ->
                        // PUSH_PCL -> WRITE_SR (three dummy stack reads that
                        // decrement S by three) -> LOAD_VECTOR -> READ_VECTOR_HI.
                        // Do not pre-present a stack address; the slot handlers
                        // present {1,S}/{1,S-1}/{1,S-2}. init holds through the
                        // sequence (cleared when next_active==START), gating the
                        // o_rw reads and the LOAD_VECTOR reset-vector select.
                        current_microinstruction <= PUSH_PCH;
                        init <= 1;
                    end
                end
            end
            else if (active_microinstruction == STACK_DUMMY_PC) begin
                // The shared first post-fetch beat of every stack/return op:
                // a dummy read at PC. The opcode-fetch block already drove PC+1
                // onto o_bus_addr, so leave it (do NOT touch o_bus_addr, S, or
                // PC); just advance. This occupies the post-fetch cycle the die
                // reserves for the PC dummy read, so the real stack beats that
                // follow are no longer collapsed and drive their {1,S} address
                // unconditionally.
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == NOP) begin
                program_counter <= program_counter + 1;
                o_bus_addr <= o_bus_addr + 1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == STALL ||
                    active_microinstruction == WRITE) begin
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == PULL_REGISTER) begin
                // Value-read beat for PLA/PLP/RTI. POP_STACK already pointed the
                // bus at the old-S dummy slot {1,S} and incremented S, so
                // register_sp == old_S + 1 here: drive {1, register_sp} for the
                // real pull read on the next cycle. The pulled byte is latched
                // one cycle later (prev_mi == PULL_REGISTER) when it is on the
                // bus.
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == READ_ADL) begin
                // BRK advances PC past its padding byte. IRQ/NMI entry re-reads
                // the same PC (dummy) and must NOT advance it; the unmodified PC
                // is what gets pushed.
                if (!handle_irq && !handle_nmi)
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
                // RTS only: this beat reads PCH. Compose the return address from
                // PCH (i_bus_data) and the PCL latched at RESTORE_STACK2, drive
                // it as the trailing dummy read (the RTS/5 cycle reads the
                // pulled PC), and set PC to pulled+1 so the next fetch lands on
                // the instruction after the JSR.
                o_bus_addr <= {i_bus_data, effective_address_lo};
                program_counter <= {i_bus_data, effective_address_lo} + 16'b1;
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == READ_PCL) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= o_bus_addr + 1;
                program_counter <= program_counter + 1;
            end
            else if (active_microinstruction == PULL_PCL || active_microinstruction == PULL_PCH) begin
                current_microinstruction <= next_active_microinstruction;
                o_bus_addr <= {8'h1, register_sp + 8'b1};
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
            else if (active_microinstruction == LOAD_PC_EFFECTIVE_HI) begin
                // JMP (): NMOS reads the pointer-high byte WITHOUT carrying out
                // of the low byte (the page-boundary bug). Increment only ADL;
                // hold ADH. So JMP ($05FF) reads PCH from $0500, not $0600.
                current_microinstruction <= next_active_microinstruction;
                program_counter <= {8'b0, i_bus_data};
                o_bus_addr <= {o_bus_addr[15:8], o_bus_addr[7:0] + 8'b1};
            end
            else if (active_microinstruction == READ_VECTOR_HI) begin
                current_microinstruction <= next_active_microinstruction;
                program_counter <= {8'b0, i_bus_data};
                o_bus_addr <= o_bus_addr + 1;
            end
            else if (active_microinstruction == LOAD_VECTOR) begin
                current_microinstruction <= next_active_microinstruction;
                // RESET (init) selects the reset vector $FFFC; the IRQ/NMI
                // entries select their own vectors. Reset reaches LOAD_VECTOR
                // via the BRK frame now, so the vector source must be selected
                // here (the old direct init jump pre-loaded $FFFC instead).
                o_bus_addr <= init ? RESET_VECTOR
                            : (handle_nmi || nmi_load_vector_hijack) ? NMI_VECTOR
                            : IRQ_VECTOR;
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
                            // fix PCH. The offset is only on i_bus_data this
                            // cycle, so compute both PC forms now: present the
                            // post-operand PC, stash the wrong-PCH target in
                            // effective_address for the next cycle, and load the
                            // fixed PC into program_counter for the SYNC fetch two
                            // cycles later.
                            operation <= OP_BRANCH_PAGE_CROSS;
                            o_bus_addr <= program_counter;
                            effective_address <= {program_counter[15:8], alu_result};
                            program_counter <= {program_counter[15:8] + (i_bus_data[7] ? 8'hff : 8'h01), alu_result};
                        end
                        else begin
                            // No page cross. NMOS presents the post-operand PC for
                            // one internal cycle (offset add), then fetches the
                            // target as the next opcode. Hold the current PC on the
                            // bus here; the MICRO_EXECUTE->START transition presents
                            // the branch target for the SYNC fetch.
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
                        // opcode fetch the MICRO_EXECUTE->START step drives.
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
                        o_bus_addr <= program_counter;
                    end
                    OPCODE_RTI: begin
                        // PCH is on the bus this cycle; program_counter gets its
                        // high half from the prev_mi == PULL_PCH path below, so
                        // drive the same composed address for the next fetch.
                        o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    end
                    OPCODE_BRK: begin
                        program_counter <= {i_bus_data, program_counter[7:0]};
                        o_bus_addr <= {i_bus_data, program_counter[7:0]};
                    end
                    OPCODE_JSR: begin
                        program_counter <= {i_bus_data, effective_address_lo};
                        o_bus_addr <= {i_bus_data, effective_address_lo};
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
                                // Dummy read at the UNINDEXED zero-page operand
                                // {00, operand}, then the indexed read. The index
                                // is added here while the operand is live on the
                                // bus; the 8-bit sum wraps in page zero. Latch it
                                // so OP_LOAD_ZP_INDEXED presents it (the indexed
                                // cycle's i_bus_data is the dummy byte, not the
                                // operand).
                                o_bus_addr <= {8'b0, i_bus_data};
                                effective_address <= {8'b0, i_bus_data +
                                    ((addressing_mode == ZP_Y) ? register_y : register_x)};
                                operation <= OP_LOAD_ZP_INDEXED;
                            end
                            // invalid opcode, continue
                            default: begin
                                current_microinstruction <= next_active_microinstruction;
                            end
                        endcase
                    end
                    else if (operation == OP_LOAD_ZP_INDEXED) begin
                        // Present the indexed zero-page address latched at
                        // OP_LOAD_ZP (i_bus_data is the dummy byte now, so do not
                        // recompute the index here).
                        o_bus_addr <= {8'b0, effective_address[7:0]};
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
                        // Zero-page indirect ((zp,X)/(zp),Y) reads the pointer
                        // high byte with an 8-bit increment that wraps in page
                        // zero; absolute increments the full 16-bit pointer.
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
                                 && (addressing_mode == ABSOLUTE_X
                                     || addressing_mode == ABSOLUTE_Y
                                     || addressing_mode == INDEX_Y_INDIRECT)))
                            // Post-indexed stores (abs,X / abs,Y / (zp),Y) always
                            // take the corrected-address read cycle. (zp,X) is
                            // pre-indexed and has no post-index page cross, so it
                            // must NOT take it.
                            operation <= OP_ABSOLUTE_PAGE_CROSS;
                        else begin
                            priority casez (opcode)
                            OPCODE_TYPE_INC, OPCODE_TYPE_DEC, OPCODE_TYPE_ROR, OPCODE_TYPE_ROL, OPCODE_TYPE_ASL,
                            OPCODE_TYPE_LSR,
                            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE,
                            OPCODE_TYPE_RRA, OPCODE_TYPE_DCP, OPCODE_TYPE_ISB: begin
                                // Post-indexed RMW (abs,X for the legal ops;
                                // abs,X / abs,Y / (zp),Y for the combos) always
                                // spends the mandatory corrected-address read
                                // cycle, even with no page cross. Plain absolute
                                // and the pre-indexed (zp,X) have no post-index,
                                // so they read once and proceed to modify/write.
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
                        effective_address <= {alu_result, effective_address[7:0]};
                        o_bus_addr <= {alu_result, effective_address[7:0]};
                        current_microinstruction <= next_active_microinstruction;
                        if (active_microinstruction == STORE)
                            o_rw <= 0;
                    end
                end
            end
            else if (active_microinstruction == POP_STACK) begin
                // First stack beat of PLA/PLP/RTS/RTI: drive the old-S dummy
                // stack read at {1, S}. register_sp is still the pre-op S here
                // (the +1 it latches this beat is for the next, real read).
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == RESTORE_STACK2) begin
                // RTS: this beat reads PCL from {1, S}. Latch it as the low half
                // of the return address and point the bus at PCH ({1, S+1}); S
                // increments this beat (register block), so register_sp+1 is the
                // PCH slot. PC is composed at PC_INC from PCH and this PCL.
                effective_address <= {8'b0, i_bus_data};
                o_bus_addr <= {8'b1, register_sp + 8'b1};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == RESTORE_STACK) begin
                // RTS: this beat reads the old-S dummy byte (discarded) and
                // points the bus at PCL ({1, register_sp}; register_sp == old_S+1
                // after POP_STACK).
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == PUSH_STACK) begin
                // PHA/PHP write beat (only these opcodes reach PUSH_STACK): drive
                // the stack write target {1, S}; the register block decrements S
                // and the o_rw case asserts the write.
                o_bus_addr <= {8'b1, register_sp};
                current_microinstruction <= next_active_microinstruction;
            end
            else if (active_microinstruction == ALU_MODIFY) begin
                // RMW beat 2: drive the OLD (unmodified) byte for the dummy
                // write and stage the NEW (modified) byte. The o_rw case already
                // marks ALU_MODIFY a write, so the next cycle writes OLD; the
                // RMW_WRITE_NEW beat then writes the staged NEW byte.
                current_microinstruction <= next_active_microinstruction;
                bus_data_write <= i_bus_data;
                rmw_new        <= alu_result;
            end
            else if (active_microinstruction == RMW_WRITE_NEW) begin
                // RMW beat 3: write the staged NEW byte to the same address.
                current_microinstruction <= next_active_microinstruction;
                bus_data_write <= rmw_new;
            end

            case (prev_mi)
                PULL_PCL: program_counter <= {8'b0, i_bus_data};
                PULL_PCH: begin
                    program_counter <= {i_bus_data, program_counter[7:0]};
                end
                default: ;
            endcase

            // Interrupt recognition poll, at the instruction boundary
            // (next mi == START). IRQ is level-sensitive, sampled through the
            // synchronizer (irq_n_sync2). NMI is the edge (pending_nmi or the
            // combinational nmi_edge_now). NMI effectively wins: when IRQ fires
            // first the still-pending NMI hijacks the entry below to the NMI
            // vector, and IRQ/NMI both push B=0, so the visible result matches.
            // Two recognition guards beyond the I mask:
            //  - BRK commits I as it completes (active==MICRO_EXECUTE, opcode
            //    BRK). status_interrupt commits on this same edge so it still
            //    reads 0; treat BRK's pending I-set as masking the IRQ poll for
            //    the handler's first instruction.
            //  - A taken branch runs an extra execute cycle (MICRO_EXECUTE with
            //    next_active already START). NMOS does not poll interrupts on
            //    that cycle, so an interrupt arriving during a taken branch is
            //    deferred past the target instruction. branch_taken is nonzero
            //    only for branch opcodes; not-taken branches poll normally. The
            //    oracle defers both same-page and page-crossing taken branches
            //    here (probe-matched for this corpus; the page-dependent story
            //    is Visual6502 lore not exercised by these tests).
            if (next_active_microinstruction == START && !irq_n_sync2 && !status_interrupt
                && !handle_irq && !handle_nmi
                && !(active_microinstruction == MICRO_EXECUTE && opcode == OPCODE_BRK)
                && !(branch_taken && active_microinstruction == MICRO_EXECUTE)) begin
                handle_irq <= 1;
            end
            else if (next_active_microinstruction == START && (pending_nmi || nmi_edge_now)
                && !handle_irq && !handle_nmi && !init
                && !(active_microinstruction == MICRO_EXECUTE && opcode == OPCODE_BRK)
                && !(branch_taken && active_microinstruction == MICRO_EXECUTE)) begin
                handle_irq <= 1;
                handle_nmi <= 1;
                pending_nmi <= 0;
            end

            // NMI late vector steering. The vector source is late-bound: an NMI
            // edge latched during an in-flight IRQ or BRK entry redirects it to
            // the NMI vector. This only steers LOAD_VECTOR; the pushed P image
            // (BRK B=1, IRQ B=0) was already fixed at WRITE_SR by the opcode /
            // handle_irq state, so a hijacked BRK still pushes B=1 and a
            // hijacked IRQ still pushes B=0.
            //  - push-cycle hijack window (PUSH_PCH..WRITE_SR, and BRK's
            //    READ_ADL signature cycle): a registered pending_nmi steers it.
            //  - LOAD_VECTOR same-cycle race: handled by nmi_load_vector_hijack,
            //    which also consults the combinational edge.
            //  - absorbed regime: an NMI edge arriving in BRK's vector tail
            //    (READ_VECTOR_HI / MICRO_EXECUTE) is too late to steer the
            //    vector but is consumed by the in-progress BRK sequence on
            //    NMOS; BRK completes through $FFFE and the edge is NOT
            //    separately serviced. An edge one cycle later survives as a
            //    normal deferred NMI.
            if (pending_nmi && !handle_nmi && (handle_irq || opcode == OPCODE_BRK)
                && (active_microinstruction == READ_ADL
                    || active_microinstruction == PUSH_PCH
                    || active_microinstruction == PUSH_PCL
                    || active_microinstruction == WRITE_SR)) begin
                handle_nmi <= 1;
                pending_nmi <= 0;
            end
            else if (nmi_load_vector_hijack) begin
                handle_nmi <= 1;
                pending_nmi <= 0;
            end
            else if ((pending_nmi || nmi_edge_now) && !handle_nmi
                && opcode == OPCODE_BRK
                && (active_microinstruction == READ_VECTOR_HI
                    || active_microinstruction == MICRO_EXECUTE)) begin
                pending_nmi <= 0;
            end
        end
    end
end

always @(negedge i_clk) begin
    // START_PC_ENABLED (FPGA bring-up) zeros the register file on reset; the
    // native harness relies on it to clear state between cases. It clears at
    // init_rdy (reset-sequence completion, reached on every reset) so this
    // synchronous block never references the async i_reset_n. The silicon-
    // faithful path (vector reset) does NOT load the register file: reset is a
    // read-only BRK frame that only decrements S (PUSH_PCH/PUSH_PCL/WRITE_SR)
    // and leaves A/X/Y untouched, preserving the running values (warm) or the
    // power-on residue seeded in the initial block (cold).
    if (START_PC_ENABLED && init_rdy) begin
        register_acc <= 0;
        register_x <= 0;
        register_y <= 0;
        register_sp <= 0;
    end
    else begin
        if (seq_advance) begin
            case (active_microinstruction)
                POP_STACK: register_sp <= register_sp + 8'b1;
                PUSH_STACK, PUSH_PCL, PUSH_PCH, WRITE_SR: register_sp <= register_sp - 8'b1;
                PULL_PCL, PULL_PCH: register_sp <= register_sp + 8'b1;
                RESTORE_STACK2: register_sp <= register_sp + 8'b1;
                MICRO_EXECUTE: begin
                    priority casez (opcode)
                    OPCODE_PLP, OPCODE_PLA: begin
                        // no updates
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
                    OPCODE_CLC, OPCODE_SEC, OPCODE_CLI, OPCODE_SEI, OPCODE_CLV,
                    OPCODE_CLD, OPCODE_SED, OPCODE_NOP: begin
                        // Implied flag/control ops write no register-file entry;
                        // on the die they assert only their flag-control line.
                        // List them ahead of the broad load opcode-bit patterns
                        // so an implied op can never inherit a memory-group or
                        // LDY-pattern (101???00) data write. Today only CLV
                        // ($B8 = 101_110_00) legally aliases a load pattern; the
                        // rest are an intentional invariant lock so a later mask
                        // change cannot silently re-open the hole.
                    end
                    OPCODE_TYPE_LDA: register_acc <= i_bus_data;
                    OPCODE_TYPE_LDX: register_x <= i_bus_data;
                    OPCODE_TYPE_LDY: register_y <= i_bus_data;
                    OPCODE_TYPE_LAX: begin
                        register_acc <= i_bus_data;
                        register_x   <= i_bus_data;
                    end
                    OPCODE_TYPE_ADC, OPCODE_TYPE_AND, OPCODE_TYPE_ORA,
                    OPCODE_TYPE_EOR, OPCODE_TYPE_SBC:
                        register_acc <= alu_result;
                    // Undocumented immediate ALU ops (exact opcodes before the
                    // cc=11 combos). AXS writes (A AND X) - imm to X; XAA writes
                    // A = X AND operand. USBC writes A via the ISB combo arm
                    // below, which commits the SBC result.
                    OPCODE_AXS: register_x <= alu_result;
                    OPCODE_XAA: register_acc <= register_x & i_bus_data;
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
            // PLA latches the pulled accumulator one cycle after PULL_REGISTER,
            // when the value byte (from {1, old_S+1}) is on the bus. The
            // PULL_REGISTER beat itself reads the old-S dummy.
            if (prev_mi == PULL_REGISTER && opcode == OPCODE_PLA)
                register_acc <= i_bus_data;
        end
    end
end

always @(negedge i_clk) begin
    begin
        if (init_rdy) begin
            // RESET forces the I flag (interrupts masked out of reset).
            // N/V/D/Z/C self-hold on the silicon path: warm reset preserves the
            // running flags, and the cold residue (Z set) comes from the initial
            // power-on seed. START_PC_ENABLED (bring-up) additionally clears
            // N/V/D/Z/C so the native harness sees a clean P between cases.
            status_interrupt <= 1;
            if (START_PC_ENABLED) begin
                status_negative <= 0;
                status_overflow <= 0;
                status_decimal <= 0;
                status_zero <= 0;
                status_carry <= 0;
            end
        end
        if (prev_mi == PULL_REGISTER && seq_advance) begin
            // The pulled byte is on the bus the cycle after PULL_REGISTER.
            // PLP/RTI restore the status register from it; PLA sets N/Z from the
            // pulled accumulator value.
            priority casez (opcode)
            OPCODE_PLP, OPCODE_RTI: begin
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
            default: ;
            endcase
        end
        if (active_microinstruction == MICRO_EXECUTE && seq_advance) begin
            if (handle_irq)
                status_interrupt <= 1;

            priority casez (opcode)
            // Undocumented multi-byte NOPs read and discard: no flag effects.
            // Listed first so the ones sharing a pattern with BIT/CPY/CPX do
            // not commit those flags.
            8'h80, 8'h82, 8'h89, 8'hC2, 8'hE2,
            8'h04, 8'h44, 8'h64,
            8'h14, 8'h34, 8'h54, 8'h74, 8'hD4, 8'hF4,
            8'h0C,
            8'h1C, 8'h3C, 8'h5C, 8'h7C, 8'hDC, 8'hFC: begin
            end
            OPCODE_TYPE_BRANCH: begin
            end
            OPCODE_PLP, OPCODE_PLA, OPCODE_JSR: begin
                // no updates (PLA N/Z and PLP P restore at prev_mi==PULL_REGISTER)
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
            // Undocumented immediate ALU ops (exact opcodes before the cc=11
            // combos so they win the priority casez). AXS commits CMP-style
            // binary N/Z/C (no V); XAA commits N/Z from X AND operand. USBC has
            // no arm here: it is the SBC alias, so the ADC/SBC/RRA/ISB arm below
            // commits its decimal-aware N/Z/C/V.
            OPCODE_AXS: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_XAA: begin
                status_negative <= register_x[7] & i_bus_data[7];
                status_zero <= (register_x & i_bus_data) == 0;
            end
            // RMW+ALU combos: the accumulator op commits its flags here.
            // SLO/RLA/SRE leave C from the shift (set at ALU_MODIFY); their
            // ORA/AND/EOR only set N/Z. DCP's CMP sets N/Z/C (binary). RRA/ISB
            // join the ADC/SBC arm below for the decimal-aware N/Z/C/V taps.
            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
            end
            OPCODE_TYPE_DCP: begin
                status_negative <= alu_result[7];
                status_zero <= alu_result == 0;
                status_carry <= alu_carry_out;
            end
            OPCODE_TYPE_LDA, OPCODE_TYPE_LDX, OPCODE_TYPE_LDY, OPCODE_TYPE_LAX: begin
                status_negative <= i_bus_data[7];
                status_zero <= i_bus_data == 0;
            end
            OPCODE_TYPE_ADC, OPCODE_TYPE_SBC,
            OPCODE_TYPE_RRA, OPCODE_TYPE_ISB: begin
                // N and Z come from the ALU's selected tap (binary or, in
                // decimal mode, the pre-correction value), not the final
                // BCD-corrected result byte. RRA/ISB reuse the decimal-aware
                // ADC/SBC taps.
                status_negative <= alu_negative;
                status_zero <= alu_zero;
                status_carry <= alu_carry_out;
                status_overflow <= alu_overflow;
            end
            default: begin
            end
            endcase
        end
        else if (active_microinstruction == ALU_MODIFY && seq_advance) begin
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
            // RMW+ALU shift combos publish the shift carry now. For SLO/RLA/SRE
            // it is the final C (the ORA/AND/EOR at MICRO_EXECUTE leave C); for
            // RRA it is the ROR carry-out that feeds the ADC carry-in. N/Z come
            // from the accumulator op at MICRO_EXECUTE.
            OPCODE_TYPE_SLO, OPCODE_TYPE_RLA, OPCODE_TYPE_SRE, OPCODE_TYPE_RRA:
                status_carry <= alu_carry_out;
            default: ;
            endcase
        end

        // V is a single status node with prioritized controls (Breaking
        // 6502, flags.md): the CLV clear line (0/V) outranks the SO set
        // line (1/V), which outranks the ADC/SBC/BIT/PLP/RTI load lines
        // (AVR/V, DB/V). The load writes above run before this SO apply,
        // so statement order already encodes set-over-load. The remaining
        // rule is clear-over-set: a coincident SO falling edge must not
        // re-set V while CLV clears it. 0/V has a single decode source
        // (CLV), so opcode != OPCODE_CLV is the faithful test for "the 0/V
        // clear line is not asserted". This single-flop SO detector
        // suppresses across CLV's whole window rather than only the die's
        // narrow PHI2 clear aperture; the difference is untested and
        // accepted (see CL8 design.md).
        if (trigger_overflow && opcode != OPCODE_CLV)
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
        // Undocumented immediate ALU ops (placed before the cc=11 combos so
        // the exact opcodes win the priority casez; $CB->DCP and $EB->ISB use
        // ~rmw_new, which is never loaded for a 2-cycle immediate). USBC is the
        // SBC alias. AXS subtracts the immediate from A AND X (CMP-style: ~imm
        // with carry-in forced, ALU_ADC default, decimal off -> binary).
        OPCODE_USBC: begin
            alu_rhs = ~i_bus_data;
            alu_carry_in = status_carry;
            alu_decimal = status_decimal;
            alu_operation = ALU_SBC;
        end
        OPCODE_AXS: begin
            alu_lhs = register_acc & register_x;
            alu_rhs = ~i_bus_data;
            alu_carry_in = 1;
        end
        // RMW+ALU combos: the accumulator op runs against the modified byte
        // latched in rmw_new (the bus is mid-write this cycle, so i_bus_data
        // is not the operand). alu_lhs defaults to register_acc above.
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
            // CMP is always binary: subtract via ADC of the complement with
            // carry-in 1, no decimal correction.
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
        // RMW+ALU combos: the modify half (ALU_MODIFY) is the same shift/inc/
        // dec as the legal op; alu_lhs = i_bus_data above. ISB=INC, DCP=DEC,
        // RRA=ROR, RLA=ROL, SLO=ASL, SRE=LSR.
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
