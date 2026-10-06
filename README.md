README
=============
Note: This repository has historically been tested on Ubuntu 16.04. Current
local development also uses Rocky Linux / RHEL style hosts with an external
SPARC GNU cross-toolchain.

The documentation of the Ajit toolchain can be found in the `./docs`
directory. This README explains the local build and setup process.

We use `$AJIT_HOME` to refer to the directory containing this `README.md` file.


Some Notes On The Build Process
-------------------------------

Ajit is a collection of many different projects which might use their own build
process (for example scripts, scons, cmake, or make). The build system is
therefore hierarchical: the top-level build script invokes sub-scripts that may
go to deeper levels.

The build output is aggregated at the top level, but each level may also keep a
copy of its output log. This helps when working on a sub-project.

To clean a sub-project build, go to the project directory and invoke its cleanup
script or command. The main public-resource build and clean entry points are
under `AjitPublicResources/`.


Building And Installation
-------------------------

Do not invoke the following commands as root or with `sudo`, except for the
package-manager commands used to install host prerequisites.

If setup fails, inspect the generated `*.log` files. One important log file is
`./build.log`.

Set `AJIT_HOME` from this directory:

    source ./set_ajit_home

From now on, `$AJIT_HOME` refers to this repository root.


Prerequisites
-------------

Some host-side Ajit tools and the Sitar build flow require:

- a recent host `gcc`/`g++`
- the SPARC GNU cross-toolchain commands on `PATH`, in particular
  `sparc64-linux-gnu-gcc`, `sparc64-linux-gnu-g++`, `sparc64-linux-gnu-as`,
  `sparc64-linux-gnu-ld`, `sparc64-linux-gnu-readelf`, and
  `sparc64-linux-gnu-objdump`
- a SPARC sysroot for headers and libraries

On Rocky Linux / RHEL / CentOS style systems:

    sudo dnf install gcc-toolset-11
    scl enable gcc-toolset-11 bash

The `scl` command comes from the Software Collections tooling on these
distributions. Use an installed external `sparc64-linux-gnu` toolchain and
sysroot, then point the Ajit environment at that sysroot before sourcing
`ajit_env`, for example:

    export AJIT_SPARC_SYSROOT_BASE=/path/to/sparc64-linux-gnu/sysroot
    source ./set_ajit_home
    source ./ajit_env

On Ubuntu or Debian, install the cross-toolchain packages directly:

    sudo apt update
    sudo apt install binutils-sparc64-linux-gnu gcc-sparc64-linux-gnu g++-sparc64-linux-gnu

On Ubuntu releases that provide the multilib package, install it as well if you
need the packaged 32-bit SPARC sysroot:

    sudo apt install gcc-multilib-sparc64-linux-gnu


Local Setup
-----------

To build the public Ajit tools and processor resources on the local system:

    source ./set_ajit_home
    source ./ajit_env
    cd $AJIT_HOME/AjitPublicResources
    ./build.sh

To clean those public-resource build outputs:

    source ./set_ajit_home
    source ./ajit_env
    cd $AJIT_HOME/AjitPublicResources
    ./clean.sh

To set up the environment for development and use:

    source ./set_ajit_home
    source ./ajit_env

`./ajit_env` contains the global environment variables needed for Ajit
development and use.


Run A Test Program
------------------

To run a sample test:

    source ./set_ajit_home
    source ./ajit_env
    cd $AJIT_HOME/tests/examples/misc/sin-model-test
    ./build.sh
    ./run_cmodel.sh
    ./clean.sh

The output of `./run_cmodel.sh` should include `Tests Successful`. If so, the
system is probably ready to use.

To run the broader automated tests:

    source ./set_ajit_home
    source ./ajit_env
    cd $AJIT_HOME/tests
    ./test.sh
    cd $AJIT_HOME/tests/verification
    ./verify.sh


Simulator versions
------------------

The repository contains three implementations, with two modes in the modular one:

| Version | Directory under `AjitPublicResources/processor/64bit/` | Mode |
| --- | --- | --- |
| Legacy C model | `C_multi_core_multi_thread` | pthread-based reference model |
| First SiTAR implementation | `C_mc_mt_sitar` | original SiTAR port |
| Modular SiTAR sync | `C_mc_mt_sitar_modular` | `AJIT_MODULAR_ASYNC=0` |
| Modular SiTAR async | `C_mc_mt_sitar_modular` | `AJIT_MODULAR_ASYNC=1` |

Complete the prerequisites and local setup above first. Set
`AJIT_SPARC_SYSROOT_BASE` to your installed SPARC sysroot before sourcing
`ajit_env`; `/usr/sparc64-linux-gnu` is the default. The simulator builds use
host GCC/G++ with C++20 support (GCC 11 or newer), OpenMP, Python 3.6 or newer,
and SCons. Compiling AJIT programs additionally needs the SPARC cross-compiler,
32-bit libraries, and the Ajit tools built by `AjitPublicResources/build.sh`.
SiTAR requires its own installation; see [SiTAR setup](docs/sitar-toolchain.md).

Start from the repository root and build a small test image:

```bash
source ./set_ajit_home
source ./ajit_env
(cd "$AJIT_HOME/tests/examples/func_call" && sh build.sh)
```

This generates `main.mmap`, `main.mmap.remapped`, and other build products;
these are not simulator source files and are not committed.

### Legacy C model

If not already built by the local setup:

```bash
(cd "$AJIT_HOME/AjitPublicResources" &&
 source ./reference_64bit_C_model_exports.sh &&
 cd processor/64bit/C_multi_core_multi_thread && scons)
(cd "$AJIT_HOME/tests/examples/func_call" && sh run_cmodel.sh)
```

The executable is `C_multi_core_multi_thread/testbench/bin/ajit_C_system_model`.
Use `-n 1..4` for cores and `-t 1|2` for threads per core; `-m` selects an image,
`-d -r` enables checking against expected results. See the
[legacy model guide](AjitPublicResources/processor/64bit/C_multi_core_multi_thread/README).

### First SiTAR implementation

```bash
cd "$AJIT_HOME/AjitPublicResources/processor/64bit/C_mc_mt_sitar"
AJIT_NUM_CORES=1 AJIT_THREADS_PER_CORE=1 AJIT_ACTIVE_THREADS=1 \
AJIT_TEST_MEMMAP="$AJIT_HOME/tests/examples/func_call/main.mmap.remapped" \
AJIT_EXPECT_RESULTS_FILE="$AJIT_HOME/tests/examples/func_call/main.results" \
OMP_NUM_THREADS=8 bash run_sitar_flow_real_thread.sh 400000
```

This builds, runs, and checks the test. For registry-driven tests, use
`bash run_regression_testcases_v2.sh func_call,strcmp 400000`.
The [first SiTAR guide](AjitPublicResources/processor/64bit/C_mc_mt_sitar/README.md)
explains its runtime settings.

### Modular SiTAR: sync and async

Both modes use the same executable:

```bash
cd "$AJIT_HOME/AjitPublicResources/processor/64bit/C_mc_mt_sitar_modular"
AJIT_NUM_CORES=1 bash build_sitar.sh
# Synchronous mode:
python3 tests_async/run_images.py --ids func_call --cores 1 --workers 8 --modes 0
# Asynchronous mode:
python3 tests_async/run_images.py --ids func_call --cores 1 --workers 8 --modes 1
```

Use `--modes 0,1` to check both. The runner selects the real CPU thread path,
sets `AJIT_MODULAR_ASYNC`, and checks register/memory or console expectations.
See the [modular guide](AjitPublicResources/processor/64bit/C_mc_mt_sitar_modular/README.md)
for multicore builds, protocol tests, Dhrystone, and Linux.


Important Files And Directories
-------------------------------

- `tests/examples/`: examples that demonstrate many of the tools in this repo
- `tests/verification/`: verification tests
- `validation_ladder/`: larger validation programs and benchmarks
- `AjitPublicResources/processor/64bit/C_mc_mt_sitar/`: Sitar simulator port
- `docs/`: toolchain and architecture documentation


General Information
-------------------

The local setup uses the directory identified by `AJIT_HOME` as the repository
root. Use the relevant sub-project `clean.sh` when a clean rebuild is needed;
for public-resource builds, use `AjitPublicResources/clean.sh`.
