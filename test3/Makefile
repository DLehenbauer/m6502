# test3/Makefile
#
# Build the test3 framework binary. Verilates two 6502 cores into the
# same executable so each test runs against both:
#   - aholme: transistor-level chip_6502 netlist (test2/aholme/)
#   - m6502:  our own RTL core (rtl/cpu_6502.sv)
#
# Verilator only produces one top-module per invocation, so we run it
# twice into separate --Mdir directories. The aholme pass uses
# --exe --build to drive the final compile + link (and pulls in our
# framework + tests + every m6502 generated .cpp as user inputs).
#
# Usage:
#   make            -> build
#   make run        -> build + run all tests
#   make run ARGS="--filter=smoke"
#   make clean

VERILATOR = verilator
AHOLME_TOP  = chip_6502
M6502_TOP   = cpu_6502
BUILD_DIR   = build
AHOLME_DIR  = $(BUILD_DIR)/aholme
M6502_DIR   = $(BUILD_DIR)/m6502
AHOLME_CORE = ../test2/aholme
M6502_RTL   = ../rtl

AHOLME_VERILOG = \
	$(AHOLME_CORE)/chip_6502.v \
	$(AHOLME_CORE)/MUX.v

# Verilator parses files in order; package must precede modules that
# import its types. Header files (.vh) are included by .sv sources via
# `include and are not on the verilator command line.
M6502_PKG_SOURCES = $(wildcard $(M6502_RTL)/cpu_6502*_pkg.sv)
M6502_VERILOG = \
	$(M6502_PKG_SOURCES) \
	$(filter-out $(M6502_PKG_SOURCES),$(wildcard $(M6502_RTL)/cpu_6502*.sv))

# Find every C++ source (framework + tests) so adding a new test file
# just requires dropping it under tests/ (any depth).
CPP_SOURCES := \
	$(wildcard src/*.cpp) \
	$(shell find tests -name '*.cpp' 2>/dev/null)

# Common verilator flags shared by both invocations.
COMMON_VFLAGS = -Wno-fatal -O3

# m6502 pass: --cc only (no compile/link). The aholme pass below picks
# up the generated .cpp/.h files. --public-flat-rw exposes internal
# regs (register_acc, register_x, ..., status_*) so the shim can read
# architectural state without a debug port.
M6502_STAMP = $(M6502_DIR)/.stamp
M6502_VFLAGS = \
	$(COMMON_VFLAGS) --cc --public-flat-rw \
	-I$(M6502_RTL) \
	--Mdir $(M6502_DIR) \
	--top-module $(M6502_TOP) \
	-Wno-WIDTHTRUNC -Wno-WIDTHEXPAND -Wno-UNUSED -Wno-UNOPTFLAT -Wno-CASEINCOMPLETE

# aholme pass: --exe --build to compile + link everything. The
# generated m6502 .cpp files are appended to the CPP inputs after the
# m6502 pass finishes (Make evaluates $(shell ...) lazily for the
# recipe line, after $(M6502_STAMP) is satisfied).
AHOLME_VFLAGS = \
	$(COMMON_VFLAGS) --cc --exe --build \
	-I$(AHOLME_CORE) \
	-Iinclude \
	-I$(M6502_DIR) \
	--Mdir $(AHOLME_DIR) \
	--top-module $(AHOLME_TOP) \
	-CFLAGS "-O3 -std=c++17 -I../../include -I../../build/m6502"

BIN = $(AHOLME_DIR)/V$(AHOLME_TOP)

.PHONY: all build run clean

all: build

build: $(BIN)

# Stage 1: verilate m6502 into its own Mdir. Touch a stamp file so
# the aholme rule can declare a dependency without globbing for files
# that don't exist on the first build.
$(M6502_STAMP): $(M6502_VERILOG)
	@mkdir -p $(M6502_DIR)
	$(VERILATOR) $(M6502_VFLAGS) $(M6502_VERILOG)
	@touch $@

# Stage 2: verilate aholme, compile the framework + every test + every
# m6502 generated .cpp, link into one executable.
$(BIN): $(AHOLME_VERILOG) $(CPP_SOURCES) $(wildcard include/test3/*.h) $(M6502_STAMP)
	$(VERILATOR) $(AHOLME_VFLAGS) $(AHOLME_VERILOG) $(CPP_SOURCES) \
		$(shell find $(M6502_DIR) -name '*.cpp' 2>/dev/null)

run: build
	$(BIN) $(ARGS)

clean:
	rm -rf $(BUILD_DIR)
