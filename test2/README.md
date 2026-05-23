# test2/ — aholme transistor-level 6502 reference + Klaus tests

This directory holds an **isolated** test harness that runs Klaus
Dormann's 6502 functional / decimal / interrupt test suites against
Andrew Holme's transistor-level 6502 reference core. The reference core
is a Verilog netlist auto-generated from the Visual 6502 project's
transistor-level dump of a real NMOS 6502 die. It is half-cycle accurate
and serves as the gold-standard model for building a future exhaustive
test suite that can be cross-checked against the in-house m6502 RTL.

This work intentionally lives outside of `rtl/` and `test/` and shares no
SystemVerilog with the rest of the project.

## Layout

```
test2/
├── README.md                  (this file)
├── aholme/                    (vendored upstream — see SOURCE.md)
│   ├── chip_6502.v
│   ├── chip_6502_nodes.inc
│   ├── logic.inc
│   ├── MUX.v
│   └── SOURCE.md              (provenance + URL + date)
├── Makefile.aholme_klaus      (build + run the full Klaus suite)
├── tb_chip_6502_klaus.cpp     (C++ Verilator testbench, all 3 tests)
├── Makefile.aholme_smoke      (build + run the smoke test)
└── tb_smoke.cpp               (fast sanity check, ~140 cycles)
```

## Architecture notes

- **Memory in C++ (not Verilog).** The 64 KiB main memory lives as a
  `uint8_t[65536]` inside the C++ harness. The bus is serviced once per
  CPU cycle around the `phi` clock edges: read data is presented on
  `dbi` at posedge `phi`; write data is captured from `dbo` at negedge
  `phi`. This keeps the Verilated module to just the CPU netlist (no
  BRAM RTL) and gives us a clean hook for future half-cycle trace
  generation against the m6502 RTL.
- **Clocking.** The aholme netlist uses two clock inputs: a fast FPGA
  `clk` (whose posedges propagate signals through the netlist's
  registers) and a slower `phi` (the 6502 clock). We run 8 `clk` ticks
  per `phi` half-period (16 per full 6502 cycle), matching the `/16`
  divider in the upstream `Pool.v` demo. This gives the netlist enough
  iterations per half-cycle for all combinational paths to settle.
- **Instruction boundary detection.** The CPU's `SYNC` output goes high
  during opcode-fetch cycles. We sample `ab` at those moments to get
  the PC of the about-to-execute opcode — cleaner than peeking at
  internal microcode state.
- **Trap detection.** Klaus's tests use `JMP *` (or `BNE *`) self-loops
  to signal pass/fail. When the same PC appears on two consecutive
  instruction boundaries we declare the CPU "trapped" and let each
  test's check callback decide pass vs. fail vs. continue.
- **Interrupt feedback register at `$BFFC`.** Klaus's interrupt test
  drives IRQ and NMI from a memory-mapped feedback register
  (bit 0 → IRQ, bit 1 → NMI, both active-low at the CPU pin). We
  intercept reads / writes of `$BFFC` in the C++ bus model and update
  the CPU's `irq`/`nmi` inputs accordingly, on the negedge of `phi`
  (same edge as the existing `test/test_mcu_klaus.sv` harness).

## Build & run

Prereqs: Verilator 5.0+ in `$PATH` (already present in the devcontainer).

```bash
# Quick sanity check (~seconds wall-clock):
make -f Makefile.aholme_smoke run

# Full Klaus suite (functional + decimal + interrupt, ~40-45 min):
make -f Makefile.aholme_klaus run

# Just one of the three:
cd obj_dir_aholme_klaus && ./Vchip_6502 decimal
cd obj_dir_aholme_klaus && ./Vchip_6502 interrupt
cd obj_dir_aholme_klaus && ./Vchip_6502 functional

# Per-instruction PC tracing (interrupt-sized tests only, very noisy):
CPU_TRACE=1 ./Vchip_6502 interrupt

# Waveforms (huge for the full Klaus run):
make -f Makefile.aholme_klaus run WAVES=1
```

Top-level convenience targets:

```bash
make test-klaus-aholme         # full suite
make test-klaus-aholme-smoke   # sanity check
```

## Expected results

| Test       | Trap PC | Cycles      | Wall-clock (devcontainer) |
| ---------- | ------- | ----------- | ------------------------- |
| smoke      | $041F   | ~140        | < 1 s                     |
| decimal    | $024B   | 46,089,505  | ~13 min                   |
| interrupt  | $075C   | 2,724       | < 1 s                     |
| functional | $3469   | 96,241,370  | ~27 min                   |

### Note: interrupt test "$075C" pass

A real NMOS 6502 fails Klaus's BRK+NMI overlap test at `$075C` due to a
documented NMOS hardware quirk: when NMI fires concurrently with BRK,
the CPU hijacks BRK with the NMI vector but pushes status with B=1
(BRK's setup, not cleared). Klaus's NMI handler detects this and traps
at `$075C` with the comment "_this may fail on a real 6502 due to a
hardware bug on concurrent BRK & NMI_". Since the aholme core is a
faithful transistor-level NMOS 6502, it correctly trips this check —
the harness reports it as `SUCCESS … (NMOS BRK+NMI hardware quirk —
expected on a real 6502)`.

The m6502 RTL passes Klaus's interrupt test at `$06F5` instead, because
its microcode does not replicate the NMOS BRK+NMI hijack. That's a
deliberate behavior difference, not a bug — but it does mean any future
cross-validation that drives Klaus's overlap-test code through both
cores will diverge at this point.

## References

- Andrew Holme's transistor-level 6502 core:
  http://www.aholme.co.uk/6502/Main.htm
- Source download: http://www.aholme.co.uk/6502/SRC/Core/
- Visual 6502 project: http://www.visual6502.org/
- Klaus Dormann's tests:
  https://github.com/Klaus2m5/6502_65C02_functional_tests
