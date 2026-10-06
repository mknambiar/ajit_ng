# C_mc_mt_sitar

This directory contains the Sitar-based Ajit simulator port that is being
brought up alongside the legacy `C_multi_core_multi_thread` implementation.
The current flow uses the real `ajit_thread` code path, the Sitar memory model,
and a small compatibility layer in `shim/` for the remaining interfaces.

The main entry points are:

- `build_sitar.sh`: translate Sitar sources and build `sitar/sitar_sim`
- `run_sitar_flow_real_thread.sh`: build, run, and check one testcase
- `run_regression_testcases_v2.sh`: run the registry-driven regression suite
- `check_sitar_results.sh`: compare simulator output against expected results

## Prerequisites

Run all commands from the repository root after setting up the Ajit environment:

```bash
source ./set_ajit_home
source ./ajit_env
```

Install the [pinned SiTAR toolchain](../../../../docs/sitar-toolchain.md) first.
The Sitar toolchain must be available on `PATH`
(`sitar translate`, `sitar compile`).

The host compiler used for the Sitar build should be a recent `gcc`/`g++`.
The active Ajit flow also expects the SPARC GNU cross-toolchain commands on
`PATH`, especially:

- `sparc64-linux-gnu-gcc`
- `sparc64-linux-gnu-g++`
- `sparc64-linux-gnu-as`
- `sparc64-linux-gnu-ld`
- `sparc64-linux-gnu-readelf`
- `sparc64-linux-gnu-objdump`

You also need a SPARC sysroot for headers and libraries.

On Rocky Linux / RHEL / CentOS style systems, if the default host compiler is
too old, start a newer toolset shell first:

```bash
sudo dnf install gcc-toolset-11
scl enable gcc-toolset-11 bash
```

Then use an installed external `sparc64-linux-gnu` toolchain/sysroot and point
Ajit at that sysroot before sourcing `ajit_env`:

```bash
export AJIT_SPARC_SYSROOT_BASE=/path/to/sparc64-linux-gnu/sysroot
source ./set_ajit_home
source ./ajit_env
```

On Ubuntu or Debian, install the cross-toolchain packages directly:

```bash
sudo apt update
sudo apt install binutils-sparc64-linux-gnu gcc-sparc64-linux-gnu g++-sparc64-linux-gnu
```

On Ubuntu releases that provide the multilib package, install it as well if you
want the packaged 32-bit SPARC sysroot:

```bash
sudo apt install gcc-multilib-sparc64-linux-gnu
```

## Directory layout

- `sitar/`: Sitar sources, generated output, simulator binary, and logs
- `ajit_thread/`: ported thread wrapper and step-task bridge
- `ajit_thread_deps/`: ported Ajit CPU/MMU/cache dependencies
- `memory/`: Ajit memory implementation used by the Sitar memory module
- `shim/`: compatibility shims and pipeless bridge stubs
- `testcase_registry_v2.txt`: regression testcase registry

## Build the simulator

From this directory:

```bash
cd AjitPublicResources/processor/64bit/C_mc_mt_sitar
./build_sitar.sh
```

Optional:

```bash
AJIT_NUM_CORES=2 ./build_sitar.sh
```

Notes:

- `AJIT_NUM_CORES` is clamped to `1..4`.
- The build cleans `sitar/Output/*`, removes stale `*.o`, runs
  `sitar translate` on `cop.sitar` and `memorytop.sitar`, and then compiles
  `sitar/sitar_sim`.
- Build logs are written under `sitar/logs/`:
  `translate_cop_<timestamp>.log`,
  `translate_memorytop_<timestamp>.log`,
  `compile_<timestamp>.log`.

## Configure cores and threads

The SITAR flow currently supports up to 4 cores and up to 2 thread slots per
core.

There are three related knobs:

- `AJIT_NUM_CORES`: number of model cores, allowed range `1..4`
- `AJIT_THREADS_PER_CORE`: runtime thread topology per core, allowed values `1`
  or `2`
- `AJIT_ACTIVE_THREADS`: how many thread slots are active for a run

Important behavior:

- `AJIT_NUM_CORES` affects both build and run.
- `build_sitar.sh` compiles the model with `SITAR_NUMT = AJIT_NUM_CORES * 2`.
  So the compiled model always has capacity for 2 slots per configured core.
- `AJIT_THREADS_PER_CORE` is interpreted by the runtime bridge, not by the
  compiler. Use `1` for one active thread slot per core, or `2` for the normal
  two-slot topology.
- `AJIT_ACTIVE_THREADS` is then clamped to the available runtime slots:
  `AJIT_NUM_CORES * AJIT_THREADS_PER_CORE`.

Runtime slot layout used by the bridge:

- with `AJIT_THREADS_PER_CORE=2`, slot `0..7` maps as:
  - `0 -> core0 thread0`
  - `1 -> core0 thread1`
  - `2 -> core1 thread0`
  - `3 -> core1 thread1`
  - `4 -> core2 thread0`
  - `5 -> core2 thread1`
  - `6 -> core3 thread0`
  - `7 -> core3 thread1`
- with `AJIT_THREADS_PER_CORE=1`, slots are contiguous by core:
  - `0 -> core0 thread0`
  - `1 -> core1 thread0`
  - `2 -> core2 thread0`
  - `3 -> core3 thread0`

Examples:

Build a 2-core model:

```bash
AJIT_NUM_CORES=2 ./build_sitar.sh
```

Run as 2 cores x 2 threads per core, with all 4 slots active:

```bash
AJIT_NUM_CORES=2 AJIT_THREADS_PER_CORE=2 AJIT_ACTIVE_THREADS=4 ./run_sitar_flow_real_thread.sh 40000
```

Run as 4 cores x 1 thread per core:

```bash
AJIT_NUM_CORES=4 AJIT_THREADS_PER_CORE=1 AJIT_ACTIVE_THREADS=4 ./run_sitar_flow_real_thread.sh 40000
```

Run only one active thread in a 4-core build:

```bash
AJIT_NUM_CORES=4 AJIT_THREADS_PER_CORE=2 AJIT_ACTIVE_THREADS=1 ./run_sitar_flow_real_thread.sh 40000
```

`AJIT_ACTIVE_THREAD_MASK` is also supported by the bridge.
It can be specified as:

- an 8-bit mask string, for example `10101010`
- a comma-separated slot list, for example `0,2,4,6`

When a mask is present, it selects active slot ids directly. If the mask is
missing or invalid, the bridge falls back to the first
`AJIT_ACTIVE_THREADS` valid slots.

Current caveats:

- the bridge supports `AJIT_THREADS_PER_CORE=1` or `2`; other values fall back
  to `2`
- `AJIT_ACTIVE_THREAD_MASK` uses the 8-slot bridge numbering above
- registry entries can override `num_cores`, `threads_per_core`,
  `active_threads`, and `active_thread_mask` per testcase

## Run one testcase

The recommended single-test flow is:

```bash
cd AjitPublicResources/processor/64bit/C_mc_mt_sitar
./run_sitar_flow_real_thread.sh 40000
```

This script performs three steps:

1. build the simulator unless `AJIT_SKIP_BUILD=1`
2. run `run_sitar.sh`
3. compare the observed results against the expected `.results` file

By default it uses the `sin-model-test` memory map. To run a different testcase,
point it to a specific mmap or choose a known profile:

```bash
AJIT_TEST_MEMMAP=tests/examples/func_call/main.mmap.remapped \
AJIT_EXPECT_RESULTS_FILE=tests/examples/func_call/main.results \
./run_sitar_flow_real_thread.sh 40000
```

Useful environment variables:

- `AJIT_SKIP_BUILD=1`: reuse an existing `sitar/sitar_sim`
- `AJIT_NUM_CORES=<1..4>`: simulator core count used at build/run time
- `AJIT_THREADS_PER_CORE=1|2`: runtime bridge topology, and used by regression when selecting a topology
- `AJIT_ACTIVE_THREADS=<n|auto>`: active simulator threads for the run
- `AJIT_THREAD_PROFILE=krishna`: load the Krishna/FPGA-like thread profile from `sitar/krishna.config`
- `AJIT_THREAD_CONFIG_FILE=<path>`: load an explicit thread profile config file; this overrides the profile default path
- `AJIT_TEST_MEMMAP=<path>`: explicit mmap file to load
- `AJIT_EXPECT_RESULTS_FILE=<path>`: expected results file to compare against
- `AJIT_TIMER_TICK_DIV=<n>`: cycle divider used by `memorytop.sitar` for timer pacing
- `AJIT_MEMORY_DELAY=<n>`: main-memory wait-state count used by `memorytop.sitar`
- `AJIT_STOP_ON_TA0=off|all`: stop policy for trap instruction `ta 0`
- `AJIT_DUMP_REGS_ON_TA0=1`: dump registers when TA0 is hit
- `AJIT_DUMP_REGS_ON_SUMMARY=1`: dump registers at end of run
- `AJIT_TRACE_W`, `AJIT_TRACE_E`, `AJIT_TRACE_F`: enable extra tracing

Thread profile config files only set simulator configuration knobs such as
core/thread count, descriptor word, cache/TLB parameters, and memory delay.
They do not select the benchmark program; the program still comes from
`AJIT_TEST_MEMMAP` or the selected registry testcase.

Outputs from a single run:

- `sitar/run.out`
- `sitar/run.err`
- `sitar/main.results.sitar`
- `sitar/TOP*.txt`

`check_sitar_results.sh` extracts `BRIDGE-REGS-*` and `BRIDGE-MEM-*` dumps from
`run.err`, writes `main.results.sitar`, and reports mismatches if the expected
results file is present.

## Run the regression suite

The recommended regression driver is `run_regression_testcases_v2.sh`.
It reads testcase IDs from `testcase_registry_v2.txt`, rebuilds `sitar_sim`
when topology changes, and writes one consolidated report per run.

Run the full registered suite:

```bash
cd AjitPublicResources/processor/64bit/C_mc_mt_sitar
OMP_NUM_THREADS=4 \
OMP_PROC_BIND=close \
OMP_PLACES=cores \
./run_regression_testcases_v2.sh all 4000000
```

Do not set `AJIT_STOP_ON_TA0=all` for a full registry run. Some legacy
validation tests execute `ta 0` as part of their normal trap/pass flow; stopping
the simulator globally at that instruction can halt those tests before their
own trap handlers write the expected pass markers. Use `AJIT_STOP_ON_TA0=all`
only for individual tests or registry entries where that termination behavior
is known to be correct.

Run selected testcase IDs:

```bash
./run_regression_testcases_v2.sh sin_model_test,func_call,strcmp 100000
```

Run a single validation `.vprj` testcase:

```bash
./run_regression_testcases_v2.sh v32apr_trivial_ta0 100000
```

Optional regression controls:

- `RUN_LEGACY=1`: also run legacy `run_cmodel.sh` when a testcase provides it
- `RUN_CLEAN=1`: run testcase `clean.sh` when available
- `AJIT_EXPECT_RESULTS_NAME=<file>`: default expected results filename
- `AJIT_EXPECT_RESULTS_AUTO_PICK=1`: auto-pick a single `*.results` file
- `AJIT_STOP_ON_TA0=off|all`: trap-stop policy for single-test diagnosis; keep
  it unset/off for the full registry unless the selected test explicitly needs
  TA0 stop behavior
- `AJIT_INIT_PC=<addr>`: override the initial PC for the run

The registry format is:

```text
id|path|expect|num_cores|threads_per_core|active_threads|active_thread_mask|run_cycles|stdin_file|stdout_expected_file|init_pc|timer_tick_div|memory_delay|stop_on_ta0
```

Only `id` and `path` are mandatory. The remaining fields override the default
topology, I/O, and run configuration on a per-test basis.

Two timer modes are intentional and both are now represented in the registry:

- Linux boot / serial login smoke:
  - use `AJIT_TIMER_TICK_DIV=10000`
  - use `AJIT_MEMORY_DELAY=0` for faster host runtime during long boots
- Bare-metal timer/IRC regressions:
  - use `AJIT_TIMER_TICK_DIV=1`
  - this preserves the old “tick every modeled cycle” expectation used by the
    short validation tests and keeps them within their legacy cycle budgets

Do not mix these blindly. The Linux boot image needs paced timer interrupts,
while the small timer regressions are checking functional interrupt behavior
with short run budgets.

Console output checking is part of the registry flow. If `stdout_expected_file`
is set, the runner captures simulator console output and checks it. Plain
stdout files are compared exactly. Files ending in `.markers` are marker
oracles: each non-comment line must appear in the console output, and an
optional `|count=N` suffix requires an exact occurrence count. Marker lines are
expanded through the runner environment, so benchmark knobs such as
`DHRYSTONE_ITERS` and `WHETSTONE_PASSES_X100` can be used in stdout checks.

If both `stdin_file` and `stdout_expected_file` are set, the runner also enables
paced console input. CoRTOS log macros such as `CORTOS_DEBUG` and
`CORTOS_TRACE` print through the same console path, so console-producing
CoRTOS/CoRTOS2 cases should keep a stable stdout oracle in
`test_io/stdout_expected/` or `test_io/stdout_markers/`.

The registered CoRTOS2 examples use explicit topology from their `config.yaml`
and set `init_pc=0x40000000`. Do not add a CoRTOS2 example to the registry
until it has a deterministic pass condition; if it emits console output, add
the corresponding `stdout_expected_file` at the same time.

### Registered Benchmarks

The registry includes stdout-marker smoke runs for Dhrystone, Whetstone, and
CoreMark. Their registry entries carry the per-test cycle budget and TA0 stop
policy now, so the second argument to `run_regression_testcases_v2.sh` is only
used as a fallback for ad-hoc runs.

Dhrystone uses `DHRYSTONE_ITERS` to control the compiled iteration count. The
default is `100`:

```bash
./run_regression_testcases_v2.sh dhrystone_1x2 60000000
DHRYSTONE_ITERS=10000 ./run_regression_testcases_v2.sh dhrystone_1x2 600000000
```

Whetstone uses `WHETSTONE_PASSES_X100` to control the fixed pass count printed
as `Use N passes (x 100)`. The default is `1`. Setting
`WHETSTONE_PASSES_X100=0` restores the original auto-calibration path, but
fixed values are preferred for regression:

```bash
./run_regression_testcases_v2.sh whetstone_bigmem 80000000
WHETSTONE_PASSES_X100=10 ./run_regression_testcases_v2.sh whetstone_bigmem 400000000
```

CoreMark uses `COREMARK_ITERS` to control the compiled iteration count. The
default is `1`, which is intentionally a smoke configuration rather than a
reportable score:

```bash
./run_regression_testcases_v2.sh coremark_1x2 20000000
COREMARK_ITERS=10 ./run_regression_testcases_v2.sh coremark_1x2 200000000
```

All three benchmark cases rely on console marker validation rather than a
`main.results` file.

### Common registry entries

The following rows are the ones most often used while bring-up and regression
debugging:

| Testcase | Validation style | Termination policy | Notes |
| --- | --- | --- | --- |
| `linux_22_0ct_21_serial_boot_smoke` | console markers | `AJIT_STOP_ON_TA0=off` | Uses the verified Linux mmap bundle, `AJIT_TIMER_TICK_DIV=10000`, and `AJIT_MEMORY_DELAY=0`. Pass milestone is `running exec /sbin/init`, `Welcome to Buildroot`, `buildroot login:`. |
| `v32_other_tests_timer_and_IRC_validation_Test_Timer` | results file | harness ends on expected completion | Bare-metal timer/IRC functional validation. Uses `AJIT_TIMER_TICK_DIV=1`. |
| `dhrystone_1x2` | console markers | `AJIT_STOP_ON_TA0=on` | Benchmark smoke run. Timing reads come from ASR30/31 via `__ajit_get_clock_time()`. |
| `whetstone_bigmem` | console markers | `AJIT_STOP_ON_TA0=off` | Benchmark smoke run with fixed `WHETSTONE_PASSES_X100`. |
| `coremark_1x2` | console markers | `AJIT_STOP_ON_TA0=on` | Benchmark smoke run with `COREMARK_ITERS` and a larger registry budget. |
| `cortos2_example_*` | stdout oracle or completion | usually `AJIT_STOP_ON_TA0=on` | Uses `init_pc=0x40000000` and explicit topology from `config.yaml`. Keep stdout oracles in sync with the source when logging is enabled. |

The timing helper `__ajit_get_clock_time()` reads the architectural ASR30/31
counter state. In SiTAR, those registers are mirrored from each thread's
simulated time (`sitar_sim_time`) rather than being derived from the legacy
`getCycleEstimate()` path. `getCycleEstimate()` is still used for diagnostics
and regression summaries, but it is no longer the source of truth for
benchmark timing reads.

## Regression outputs

Each regression run creates a timestamped directory:

```text
sitar/logs/regression/<timestamp>/
```

Important files there:

- `summary.txt`: overall pass/fail summary
- `<test-id>.log`: full log for each testcase

Each testcase log includes Sitar runtime lines from the bridge:

```text
BRIDGE-RUNTIME t0 c0 h0 active=1 seen=1 done=1 tick=37231 cycles=18616
BRIDGE-RUNTIME total cycles=18616
```

The raw `tick` value is the Sitar scheduler time. Sitar runs two scheduler
phases per modeled cycle, so the reported `cycles` value is rounded up as:

```text
cycles = (tick + 1) / 2
```

For each active CPU thread, runtime uses the first `ta 0` tick if that thread
reached `ta 0`; otherwise it uses the last observed scheduler tick for that
thread. `BRIDGE-RUNTIME total cycles` is the maximum reported cycle count across
active, observed threads. This runtime is independent of the legacy
`getCycleEstimate` accounting in `C_multi_core_multi_thread`.

Generate a regression cycle table after a run:

```bash
latest=$(ls -td sitar/logs/regression/* | head -n 1)
{
  printf "status\ttestcase\tcycles\n"
  awk '/^(PASS|FAIL) / {print $1, $2}' "$latest/summary.txt" |
  while read -r status id; do
    cycles=$(sed -n 's/.*BRIDGE-RUNTIME total cycles=\([0-9][0-9]*\).*/\1/p' "$latest/$id.log" | tail -n 1)
    printf "%s\t%s\t%s\n" "$status" "$id" "${cycles:-MISSING}"
  done
} > "$latest/cycles_by_testcase.tsv"
```

The script also prints the report directory at startup, for example:

```text
Report dir      : .../sitar/logs/regression/20260221_232308
```

## Current status

The command below completed with passing results across the registered v2 suite:

```bash
OMP_NUM_THREADS=4 OMP_PROC_BIND=close OMP_PLACES=cores \
  ./run_regression_testcases_v2.sh all 4000000
```

That run reported:

```text
TOTAL: 304 PASS: 304 FAIL: 0
ABORTED: no
```

Its output directory was:

```text
sitar/logs/regression/20260420_163608
```

## Notes

- The older scaffold description in this directory is obsolete; use the scripts
  above as the source of truth for the current flow.
