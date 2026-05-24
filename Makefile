.PHONY: test test-klaus test-klaus-aholme test-klaus-aholme-smoke test-test3 clean-test

test:
	uv run pytest test/test_runner.py -s -x
ifndef TESTCASE
	$(MAKE) test-klaus
endif

test-klaus:
	cd test && make -f Makefile.mcu_klaus run

# Klaus tests run against Andrew Holme's transistor-level 6502 reference
# core (see test2/). Slower than the m6502 RTL (the netlist has ~1700
# transistor nodes), so allow ~40-45 min for the full functional +
# decimal + interrupt sweep.
test-klaus-aholme:
	cd test2 && make -f Makefile.aholme_klaus run

# Fast sanity check for the aholme harness (a tiny hand-assembled
# program, ~140 cycles). Useful before paying for the long Klaus run.
test-klaus-aholme-smoke:
	cd test2 && make -f Makefile.aholme_smoke run

# Half-cycle-accurate Verilator test framework (test3/). Phase 1: runs
# the one smoke test against the aholme transistor-level core.
test-test3:
	cd test3 && make run

clean-test:
	cd test  && make -f Makefile.mcu_klaus    clean
	cd test2 && make -f Makefile.aholme_klaus clean
	cd test2 && make -f Makefile.aholme_smoke clean
	cd test3 && make                          clean
	rm -rf sim_build
