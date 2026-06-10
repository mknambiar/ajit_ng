#!/bin/bash
set -e
CWD=$(pwd)
# build the tools
echo "build tools";
cd tools;
PATH=/usr/bin:/bin:$PATH scons;
cd $CWD;
# build test-environments
echo "build processor/TestEnvironments"
cd processor/TestEnvironments;
scons;
cd $CWD;
# the C reference model testbench.
source reference_C_model_exports.sh;
cd processor/C_reference_model/;
echo "build processor/C_reference_model";
scons;
cd -;
# the 64-bit C reference model testbench.
source reference_64bit_C_model_exports.sh;
cd processor/64bit/C_multi_core_multi_thread/;
echo "build processor/64bit/C_multi_core_multi_thread/";
scons;
cd $CWD
# ajit_debug_monitor_mt.
echo "build tools/ajit_debug_monitor_mt";
cd tools/ajit_debug_monitor_mt;
scons;
cd $CWD;
# ajit_debug_monitor.
echo "build tools/ajit_debug_monitor";
cd tools/ajit_debug_monitor;
scons;
cd $CWD;
# ajit_access_routines_mt (libajit*.a).
echo "build tools/ajit_access_routines_mt";
cd tools/ajit_access_routines_mt;
export SPARC_SYSROOT="/home/Manoj/sparc64-sysroot/sparc64-multilib-linux-gnu/sysroot";
export CFLAGS="--sysroot=$SPARC_SYSROOT";
export CXXFLAGS="--sysroot=$SPARC_SYSROOT";
export CPPFLAGS="--sysroot=$SPARC_SYSROOT";
export LDFLAGS="--sysroot=$SPARC_SYSROOT";
./build_lib.sh alt
./build_lib.sh default
unset SPARC_SYSROOT CFLAGS CXXFLAGS CPPFLAGS LDFLAGS;
cd $CWD;
