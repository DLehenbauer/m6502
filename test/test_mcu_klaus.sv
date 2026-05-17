`timescale 1ps/1ps

// Top-level RTL used to run Klaus Dormann's 6502 tests.  The testbench
// (tb_mcu_klaus.cpp) loads the appropriate .hex into bram via --public-flat-rw
// and runs each test in turn.

module test_mcu_klaus (
    input i_clk
);

reg i_reset_n;

wire cpu_sync;
wire cpu_phi1;
wire cpu_phi2;
wire cpu_rw;
wire [15:0] bus_addr;
wire [7:0] bus_write_data;
wire [7:0] bus_read_data;
wire [7:0] bram_read_data;
wire [7:0] debug_data;

// The interrupt tests use a feedback register at $BFFC to drive IRQ / NMI from
// inside the test program:
//
//   Bit | Signal  | When 1                | When 0
//   ----+---------+-----------------------+------------------------
//    0  | i_irq_n | asserted (driven low) | deasserted (high)
//    1  | i_nmi_n | asserted (driven low) | deasserted (high)
//
// The register is reset to $00 so all interrupt lines come out of reset
// deasserted.

reg [7:0] feedback_reg;
wire cs_fb = (bus_addr == 16'hBFFC);

always @(negedge cpu_phi2 or negedge i_reset_n) begin
    if (!i_reset_n)
        feedback_reg <= 8'h00;
    else if (cs_fb && !cpu_rw)
        feedback_reg <= bus_write_data;
end

assign bus_read_data = cs_fb ? feedback_reg : bram_read_data;

wire i_irq_n = ~feedback_reg[0];
wire i_nmi_n = ~feedback_reg[1];

cpu_6502 #(
    .START_PC(16'h0400),
    .START_PC_ENABLED(1)
) cpu_6502 (
    .i_clk(i_clk),
    .o_phi1(cpu_phi1),
    .o_phi2(cpu_phi2),
    .i_reset_n(i_reset_n),
    .i_rdy(1'b1),
    .i_nmi_n(i_nmi_n),
    .i_irq_n(i_irq_n),
    .i_so_n(1'b1),
    .o_sync(cpu_sync),
    .i_bus_data(bus_read_data),
    .o_bus_data(bus_write_data),
    .o_bus_addr(bus_addr),
    .o_rw(cpu_rw),
    .i_debug_sel(3'b000),
    .o_debug_data(debug_data)
);

// bram is loaded at runtime from C++.
bram bram (
    .i_clk(i_clk),
    .i_phi2(cpu_phi2),
    .i_addr(bus_addr),
    .i_data(bus_write_data),
    .i_rw(cpu_rw),
    .i_en(!cs_fb),
    .o_data(bram_read_data)
);

endmodule
