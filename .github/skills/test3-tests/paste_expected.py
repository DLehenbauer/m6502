#!/usr/bin/env python3
"""Extract paste blocks per test from a test_run.log and inject them into
each test's empty expected[] = {}; in the source file."""
import re, os, sys, pathlib

LOG = "/tmp/test_run.log"
ROOT = pathlib.Path("/workspaces/m6502/test3")

# Parse log into {test_name: [body_lines]}.
blocks = {}
with open(LOG) as f:
    lines = f.readlines()

i = 0
current = None
while i < len(lines):
    m = re.match(r"FAIL: test3_test_(\S+) \[core=", lines[i])
    if m:
        current = m.group(1)
    elif current and lines[i].strip().startswith("// --- BEGIN actual trace"):
        # Skip header lines until "static const ... expected[] = {"
        j = i + 1
        while j < len(lines) and "expected[] = {" not in lines[j]:
            j += 1
        # body starts after that line, ends before "    };"
        body_start = j + 1
        k = body_start
        while k < len(lines) and lines[k].rstrip() != "    };":
            k += 1
        body = lines[body_start:k]
        blocks[current] = body
        current = None
        i = k
    i += 1

print(f"Parsed {len(blocks)} blocks")

# Map test name -> source file by scanning all test files for TEST(name).
file_map = {}
for f in ROOT.rglob("tests/**/*.cpp"):
    txt = f.read_text()
    for m in re.finditer(r"TEST\((\w+)\)", txt):
        file_map[m.group(1)] = f

# For each file with failing tests, rewrite empty expected[]={} into the captured body.
edits_by_file = {}
for tname, body in blocks.items():
    if tname not in file_map:
        print(f"WARN: no source for {tname}", file=sys.stderr)
        continue
    edits_by_file.setdefault(file_map[tname], []).append((tname, body))

for fpath, edits in edits_by_file.items():
    txt = fpath.read_text()
    src_lines = txt.splitlines(keepends=True)
    out = []
    i = 0
    # Per-test, walk and find the corresponding TEST(name) block, then
    # find the next "static const TraceRow expected[] = {" through "};"
    # and replace contents.
    pattern_test = re.compile(r"TEST\((\w+)\)")
    pattern_arr_open = re.compile(r"^(\s*)static const TraceRow expected\[\]\s*=\s*\{(.*)$")

    target_tests = {t for t, _ in edits}
    bodies = {t: b for t, b in edits}
    new_lines = []
    i = 0
    while i < len(src_lines):
        line = src_lines[i]
        m = pattern_test.search(line)
        if m and m.group(1) in target_tests:
            tname = m.group(1)
            new_lines.append(line); i += 1
            # Find the static const TraceRow expected[] = { line
            while i < len(src_lines):
                aopen = pattern_arr_open.match(src_lines[i])
                if aopen:
                    indent = aopen.group(1)
                    rest = aopen.group(2)
                    # If the whole array is on one line ("= { };"), normalize
                    # to multi-line and inject body.
                    if "};" in rest:
                        new_lines.append(f"{indent}static const TraceRow expected[] = {{\n")
                        for bl in bodies[tname]:
                            new_lines.append(bl)
                        new_lines.append(f"{indent}}};\n")
                        i += 1
                        break
                    new_lines.append(src_lines[i])
                    i += 1
                    # consume everything up to closing };
                    while i < len(src_lines) and src_lines[i].lstrip().rstrip("\n") != "};":
                        i += 1
                    # inject body, then keep the };
                    for bl in bodies[tname]:
                        new_lines.append(bl)
                    if i < len(src_lines):
                        new_lines.append(src_lines[i])  # };
                        i += 1
                    break
                new_lines.append(src_lines[i])
                i += 1
        else:
            new_lines.append(line); i += 1

    fpath.write_text("".join(new_lines))
    print(f"Updated {fpath} ({len(edits)} tests)")
