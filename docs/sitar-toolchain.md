# SiTAR toolchain for the AJIT simulators

Use GCC/G++ 11 or newer, GNU make, Python 3, SCons, OpenMP and Git on Linux.
The simulations require C++20 coroutines. The SPARC cross-compiler is for
building guest programs, not for building the simulator.

The validated runtime and translator sources match SiTAR commit
`2205536a0646c57515f90a9afda605d01c2ac4fb`. The upstream repository moved to
[https://github.com/sitar-sim/sitar](https://github.com/sitar-sim/sitar).
Use that commit and the supplied compiler patch, rather than assuming current
upstream has identical scheduling behavior.

From the AJIT repository, with `AJIT_HOME` set:

```bash
git clone https://github.com/sitar-sim/sitar.git "$HOME/sitar-ajit"
cd "$HOME/sitar-ajit"
git checkout 2205536a0646c57515f90a9afda605d01c2ac4fb
git apply "$AJIT_HOME/docs/sitar-ajit.patch"
python3 install.py
export PATH="$HOME/sitar-ajit/scripts:$PATH"
export LD_LIBRARY_PATH="$HOME/sitar-ajit/translator/antlr3Cruntime/build/lib:${LD_LIBRARY_PATH:-}"
export SITAR_BIN="$HOME/sitar-ajit/scripts/sitar"
```

Check that `translator/parser/sitar_translator` was built successfully.
The patch adds `.c` source discovery alongside `.cpp` and separates appended
compiler flags with a space. It also preserves the shell environment in SCons
so the selected compiler and assembler remain on PATH during installation and
simulation builds. It makes no scheduler/runtime changes. SiTAR is
MIT-licensed; copyright Neha V. Karanjkar and Madhav P. Desai (see upstream LICENSE).

Both simulator build scripts use `SITAR_BIN` when supplied, otherwise `sitar`
on PATH. Use an absolute path for `SITAR_BIN`. They discover GCC helper tools
beside the resolved `g++` executable, so Rocky/RHEL toolset installations do
not require a hardcoded `/opt/rh` path in the repository. Activate the selected
GCC toolset before installing SiTAR or building the simulators. Optional
`AJIT_HOST_CFLAGS` is appended to simulator compiler flags.

SiTAR compiles runtime object files in its own installation directory. The
installation must therefore be writable by the user. Avoid concurrent builds
sharing that installation. Modular protocol tests read the installation path
from the simulator's generated `sitar_scons_config.txt`; build first.
