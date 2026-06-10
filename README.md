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


Sitar Simulator Flow
--------------------

The Sitar-based Ajit simulator port lives in
`AjitPublicResources/processor/64bit/C_mc_mt_sitar`. Use the README in that
directory as the source of truth for build, single-test, and regression usage.

Typical commands are:

    source ./set_ajit_home
    source ./ajit_env
    cd $AJIT_HOME/AjitPublicResources/processor/64bit/C_mc_mt_sitar
    ./build_sitar.sh
    OMP_NUM_THREADS=4 OMP_PROC_BIND=close OMP_PLACES=cores ./run_regression_testcases_v2.sh all

See `AjitPublicResources/processor/64bit/C_mc_mt_sitar/README.md` for the
complete workflow and testcase selection details.


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
