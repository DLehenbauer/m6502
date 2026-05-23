# aholme transistor-level 6502 core

Files vendored from Andrew Holme's transistor-level 6502 reference core:

  http://www.aholme.co.uk/6502/SRC/Core/

Downloaded: 2026-05-23

Only the CPU netlist files are vendored here:

- `chip_6502.v`            — top-level module wrapping the netlist
- `chip_6502_nodes.inc`    — `define`s mapping signal names to node indices
- `logic.inc`              — auto-generated combinational/sequential logic
- `MUX.v`                  — small parameterized N-input mux primitive used
                             throughout `logic.inc`

These three files together implement the `chip_6502` module — a cycle- and
half-cycle accurate model of the MOS 6502 derived from the Visual 6502
project's transistor-level netlist. This serves as our gold-standard
reference for building an exhaustive 6502 test suite.

The other files in the upstream directory (`Pool.v`, `Pool.ucf`,
`Pool_tb.v`, `DAC.v`, `MUX.v`, `ram.hex`, `rom.hex`) are FPGA-board glue
and demo program for Andrew's Spartan-3E "pool" target. We do not need them
here, but `Pool.v` was inspected during testbench development for the
clk:phi ratio (8 `clk` ticks per `phi` half-period) and the memory-access
edge convention (service memory on posedge `phi`).

The files in this directory are unmodified upstream and retain the
original author's copyright.
