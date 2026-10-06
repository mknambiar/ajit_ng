# Modular SiTAR simulator

This implementation separates CPU threads, I-cache, D-cache, and MMU into
SiTAR modules. Sync and async cache modes share one executable. The legacy C
model and first SiTAR port remain in sibling directories.

Follow the [repository setup and four-mode guide](../../../../README.md) first,
including [SiTAR installation](../../../../docs/sitar-toolchain.md).

## Build

From this directory:

```bash
AJIT_NUM_CORES=1 bash build_sitar.sh
```

`AJIT_NUM_CORES` selects 1–4 cores. Each compiled core has two native CPU thread
slots; runtime configuration selects one or two active threads per core.
Rebuild when changing the core count. Use a separate checkout for concurrent
builds with different topologies. `sitar/sitar_sim`, `sitar/Output/`, and
`sitar/logs/` are generated. `AJIT_SITAR_ENABLE_LOGGING=1` enables SiTAR logging.
`SITAR_BIN` may name the absolute path to the SiTAR command; otherwise `PATH`
is used. Optional `AJIT_HOST_CFLAGS` supplies additional host compiler flags.

## Run and check images

Build the selected benchmark image first. For example, from any directory
with the Ajit environment sourced:

```bash
(cd "$AJIT_HOME/tests/examples/func_call" && sh build.sh)
```

Then, from this modular directory:

```bash
python3 tests_async/run_images.py --ids func_call --cores 1 --workers 8 --modes 0,1
```

`--modes 0` means sync; `--modes 1` means async. `--cycles` defaults to 400000;
registry entries can require more. The runner snapshots the executable and
records configuration, image hashes, console output, and results under
`sitar/logs/async_images_<timestamp>/`. A nonzero exit status indicates failure.

For direct shell runners, set `AJIT_MODULAR_REAL_THREAD=1` as well as
`AJIT_MODULAR_ASYNC=0` or `1`. The image runner sets both automatically.
`AJIT_THREADS_PER_CORE=1|2` and `AJIT_ACTIVE_THREADS` select runtime topology;
`--workers` is the number of OpenMP host workers, not simulated CPU threads.

## Protocol and benchmark checks

After building the simulator:

```bash
python3 tests_async/run_protocol_tests.py
python3 tests_async/run_images.py --ids short-existing --cores 1 --workers 8 --modes 0,1
```

Protocol checks cover phase-1 sends, phase-0 receives, exact hit/miss timing,
both branch orders, native scheduling, refill, forwarding, faults, and queues.
`short-existing` selects only images already built on the current checkout;
it is not a command that builds every benchmark. Its count depends on available
images. The registry lists testcase source paths, topology, and expectations.
The registry runner can build source testcases, for example:

```bash
AJIT_MODULAR_REAL_THREAD=1 AJIT_MODULAR_ASYNC=0 \
  bash run_regression_testcases_v2.sh func_call,strcmp 400000
```

Dhrystone uses two threads on one core:

```bash
(cd "$AJIT_HOME/validation_ladder/benchmarks/dhrystone/1x2" && bash build.sh)
python3 tests_async/run_images.py --ids dhrystone_1x2 --cores 1 --workers 8 --modes 0,1
```

For multicore tests, build their sources and rebuild the simulator with the
matching `AJIT_NUM_CORES`, then pass that number as `--cores`. The registry
contains `multi_core_hello_world_2x2_simple` and `multi_core_hello_world_4x1`,
among others. Do not run an image with a topology different from its registry entry.

## Linux boot

Obtain the image using the [Linux image instructions](../../../../validation_ladder/linux/22_0ct_21/c-model/README),
then stage it with:

```bash
bash "$AJIT_HOME/validation_ladder/linux/22_0ct_21/serial_boot_smoke/build.sh"
AJIT_NUM_CORES=1 bash build_sitar.sh
python3 tests_async/run_images.py --ids linux_22_0ct_21_serial_boot_smoke --cores 1 --workers 8 --modes 1 --cycles 500000000
python3 tests_async/run_images.py --ids linux_22_0ct_21_serial_boot_smoke --cores 1 --workers 8 --modes 0 --cycles 1000000000
```

The runner configures init PC `0xf0000000`, timer divider 10000, and memory
delay 0. Both `Welcome to Buildroot` and `buildroot login:` are required.
Linux budgets are at least 500M cycles; explicit larger budgets are honored.

The validated implementation passed 598 image executions across sync and async:
576 short tests, 2 Dhrystone, 6 two-core, 8 four-core, 4 smoke, and 2 Linux runs.
Linux used the different budgets above; these results are not a matched-budget
performance comparison. Async idle misses take one additional cycle before MMU
dispatch after the phase correction; immediate read-hit latency is unchanged.
