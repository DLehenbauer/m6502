`include "cpu_6502_instructions.vh"

module cpu_6502_alu import cpu_6502_pkg::*; (
    input alu_op_t i_operation,
    input i_carry,
    input [7:0] i_lhs,
    input [7:0] i_rhs,
    input i_bcd,
    output [7:0] o_result,
    output o_carry,
    output o_overflow,
    output o_negative,
    output o_zero
);

reg [8:0] result;

wire b7, b0;
assign b7 = i_lhs[7];
assign b0 = i_lhs[0];


reg [4:0] result_hi, result_lo;
reg bcd_carry_lo, bcd_carry_hi;

// Binary add/subtract value (i_rhs is already inverted by the caller for SBC).
// NMOS decimal mode latches several flags from this pre-correction value.
wire [8:0] bin_result = {1'b0, i_lhs} + {1'b0, i_rhs} + {8'b0, i_carry};

// ADC decimal intermediate: the byte after the low-nibble BCD fixup but before
// the high-nibble +$60 fixup (the die's "step-1c" byte that DB7/AVR observe).
// result_hi already folded bcd_carry_lo into the high nibble; dec_lo's +6 is
// 4-bit truncated so the low carry is not re-injected.
wire [3:0] dec_lo = bcd_carry_lo ? (result_lo[3:0] + 4'd6) : result_lo[3:0];
wire [7:0] dec_step1c = {result_hi[3:0], dec_lo};
reg dec_negative, dec_zero, dec_overflow, dec_carry;

always_comb begin
    result_lo = {1'b0, i_lhs[3:0]} + {1'b0, i_rhs[3:0]} + {4'b0, i_carry};
    bcd_carry_lo = i_operation == ALU_ADC ? result_lo > 5'd9 : result_lo[4];
    result_hi = {1'b0, i_lhs[7:4]} + {1'b0, i_rhs[7:4]} + {4'b0, bcd_carry_lo};
    bcd_carry_hi = i_operation == ALU_ADC ? result_hi > 5'd9 : result_hi[4];

    if (i_operation == ALU_ADC || i_operation == ALU_SBC) begin
        if (i_bcd) begin
            if (i_operation == ALU_ADC) begin
                result = {bcd_carry_hi,
                    bcd_carry_hi ? result_hi[3:0] + 4'd6 : result_hi[3:0],
                    bcd_carry_lo ? result_lo[3:0] + 4'd6 : result_lo[3:0]
                };
            end else begin
                result = {1'b0,
                    bcd_carry_hi ? result_hi[3:0] : result_hi[3:0] - 4'd6,
                    bcd_carry_lo ? result_lo[3:0] : result_lo[3:0] - 4'd6
                };
            end
        end
        else begin
            result = {1'b0, i_lhs} + {1'b0, i_rhs} + {8'b0, i_carry};
        end
    end
    else if (i_operation == ALU_AND)
        result = {1'b0, i_lhs & i_rhs};
    else if (i_operation == ALU_ORA)
        result = {1'b0, i_lhs | i_rhs};
    else if (i_operation == ALU_EOR)
        result = {1'b0, i_lhs ^ i_rhs};
    else if (i_operation == ALU_ASL)
        result = {i_lhs, 1'b0};
    else if (i_operation == ALU_LSR)
        result = {b0, 1'b0, i_lhs[7:1]};
    else if (i_operation == ALU_ROL)
        result = {b7, i_lhs[6:0], i_carry};
    else if (i_operation == ALU_ROR)
        result = {b0, i_carry, i_lhs[7:1]};
    else begin
        result = 9'b0;
    end

    // NMOS decimal-mode flag values, latched from the pre-correction taps.
    if (i_operation == ALU_ADC) begin
        // ADC: Z from the raw binary sum, N/V from the step-1c byte (post
        // low-nibble fixup, pre high-nibble fixup), C from the decimal high
        // carry.
        dec_zero     = bin_result[7:0] == 8'b0;
        dec_negative = dec_step1c[7];
        dec_overflow = (~(i_lhs ^ i_rhs) & (i_lhs ^ dec_step1c) & 8'h80) != 8'b0;
        dec_carry    = bcd_carry_hi;
    end else begin
        // SBC: every flag follows the plain binary subtraction (i_rhs already
        // inverted); only the result byte is decimal-adjusted.
        dec_zero     = bin_result[7:0] == 8'b0;
        dec_negative = bin_result[7];
        dec_overflow = (i_lhs[7] == i_rhs[7]) && (bin_result[7] != i_lhs[7]);
        dec_carry    = bin_result[8];
    end
end

assign o_result = result[7:0];
assign o_carry = i_bcd ? dec_carry : result[8];
assign o_negative = i_bcd ? dec_negative : result[7];
assign o_zero = i_bcd ? dec_zero : (result[7:0] == 8'b0);

assign o_overflow = i_bcd ? dec_overflow
    : (i_lhs[7] == i_rhs[7] && result[7] != i_lhs[7]);

endmodule

