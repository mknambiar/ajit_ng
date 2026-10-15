#!/usr/bin/env python3
"""Focused report/behavior comparison against a pre-change simulator binary.

Build one core first. Uses existing func_call image; never rebuilds guest code.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
repo = root.parents[3]


def run(binary, folder, mode, threads, budget, stop):
    work = folder / "sitar"
    work.mkdir(parents=True)
    (folder / "test_memmap_input_0.txt").symlink_to(
        repo / "tests/examples/func_call/main.mmap.remapped")
    env = {k: v for k, v in os.environ.items()
           if not k.startswith(("AJIT_", "OMP_"))}
    env.update(AJIT_MODULAR_ASYNC=str(mode), AJIT_MODULAR_REAL_THREAD="1",
               AJIT_NUM_CORES="1", AJIT_THREADS_PER_CORE=str(threads),
               AJIT_ACTIVE_THREADS=str(threads), AJIT_STOP_ON_TA0=stop,
               AJIT_INIT_PC="0", AJIT_MEMORY_DELAY="0", OMP_NUM_THREADS="1",
               AJIT_DUMP_REGS_ON_SUMMARY="1", AJIT_DUMP_REGS_ON_TA0="1")
    with (folder / "run.err").open("w") as err, (folder / "run.out").open("w") as out:
        subprocess.run([str(binary), str(budget)], cwd=str(work), env=env,
                       stdout=out, stderr=err, check=True, timeout=60)
    return (folder / "run.err").read_text()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--baseline", type=Path, required=True)
    args = ap.parse_args()
    baseline = args.baseline.resolve()
    current = root / "sitar/sitar_sim"
    output = Path(tempfile.mkdtemp(prefix="interface_timing_", dir=str(root / "sitar/logs")))
    rows = []
    for mode in (0, 1):
        for name, threads, budget, stop in (
                ("completed", 1, 400000, "all"),
                ("budget_stop", 1, 20, "all"),
                ("continued_after_ta0", 1, 10000, "off"),
                ("multiple_threads", 2, 200, "all")):
            case = output / (str(mode) + "_" + name)
            before = run(baseline, case / "before", mode, threads, budget, stop)
            after = run(current, case / "after", mode, threads, budget, stop)
            # Compare actual architectural outputs, original runtime and cache
            # summaries. A reporting-only edit must not change any of these.
            def observed(text):
                prefixes = ("BRIDGE-REGS-", "BRIDGE-RUNTIME ", "MOD-CACHE-SUMMARY ", "MOD-ASYNC c")
                return sorted(line for line in text.splitlines() if line.startswith(prefixes))
            assert observed(before) == observed(after), (mode, name, "behavior changed")
            reports = [line for line in after.splitlines() if line.startswith("BRIDGE-TIMING ")]
            assert len(reports) == 1, (mode, name, reports)
            report = dict(re.findall(r"([a-z-]+)=([^ ]+)", reports[0]))
            if mode == 1 and name == "completed":
                assert report["status"] == "estimate"
                i = int(re.search(r"MOD-CACHE-SUMMARY c0 IF\(a=(\d+)", after)[1])
                d = int(re.search(r"MOD-CACHE-SUMMARY c0 DF\(a=(\d+)", after)[1])
                raw = int(re.search(r"BRIDGE-RUNTIME total cycles=(\d+)", after)[1])
                assert int(report["icache-accesses"]) == i
                assert int(report["dcache-accesses"]) == d
                assert int(report["estimated-interface-overhead"]) == 2 * (i + d)
                assert int(report["interface-adjusted-cycles"]) == raw - 2 * (i + d)
            else:
                expected = ("overlapping-threads" if threads > 1 else
                            "sync-handover-not-fixed" if name == "completed" else
                            "requires-completed-ta0-stop")
                assert report["reason"] == expected, report
                assert report["interface-adjusted-cycles"] == "unavailable"
            rows.append(dict(mode=mode, case=name, report=reports[0], passed=True))
            print("PASS", mode, name, flush=True)
    (output / "summary.json").write_text(json.dumps(rows, indent=2) + "\n")
    print(output)


if __name__ == "__main__":
    main()
