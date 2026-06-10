#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
AJIT_SRC_ROOT="${ROOT_DIR}/../C_multi_core_multi_thread"
REPO_ROOT="${ROOT_DIR}/../../../.."

if [[ ! -d "${SITAR_DIR}" ]]; then
  echo "ERROR: sitar directory not found at ${SITAR_DIR}" >&2
  exit 1
fi

RUN_CYCLES="${1:-2000}"
LOG_DIR="${SITAR_DIR}/logs"
STAMP="$(date +%Y%m%d_%H%M%S)"
GDB_LOG="${LOG_DIR}/gdb_bt_${STAMP}.log"
GDB_CMDS="${LOG_DIR}/gdb_cmds_${STAMP}.txt"
COMPILE_LOG="${LOG_DIR}/compile_gdb_${STAMP}.log"

mkdir -p "${LOG_DIR}"
cd "${SITAR_DIR}"

AJIT_CORE_INCLUDE_FLAGS="\
-I${AJIT_SRC_ROOT}/common/include \
-I${AJIT_SRC_ROOT}/cpu/include \
-I${AJIT_SRC_ROOT}/cpu_interface/include \
-I${AJIT_SRC_ROOT}/mmu/include \
-I${AJIT_SRC_ROOT}/cache/include \
-I${AJIT_SRC_ROOT}/monitorLogger/include \
-I${AJIT_SRC_ROOT}/tlbs/include \
-I${AJIT_SRC_ROOT}/rlut/include \
-I${AJIT_SRC_ROOT}/bridge/include \
-I${AJIT_SRC_ROOT}/debugger/hwServer/include \
-I${AJIT_SRC_ROOT}/half_precision_float/include \
-I${REPO_ROOT}/ahir_release/include \
-I${REPO_ROOT}/ahir_release/functionLibrary/include \
-I${AJIT_SRC_ROOT}/half_precision_float/aa2clib/include"

echo "[1/5] Cleaning generated artifacts"
rm -rf Output/*
rm -f sitar_sim run.out run.err
find "${ROOT_DIR}" -type f -name '*.o' -delete

echo "[2/5] Translating sitar sources"
sitar translate cop.sitar > "${LOG_DIR}/translate_cop.log" 2>&1
sitar translate memorytop.sitar > "${LOG_DIR}/translate_memorytop.log" 2>&1

echo "[3/5] Compiling debug binary"
sitar compile \
  -d ./Output \
  -d ../shim \
  -d ../ajit_thread/src \
  -d ../ajit_thread_deps/src \
  -d ../memory \
  -d ../aes_block/src \
  -d ../swizzler/src \
  -d . \
  -m ../sitar_default_main.cpp \
  --openmp --logging \
  --cflags "-B /opt/rh/gcc-toolset-11/root/usr/bin -D SW -D SITAR_NUMT=8 -DUSE_NEW_TLB -std=c++20 -O0 -g3 -fno-omit-frame-pointer -Wall -Wextra -I../shim -I../ajit_thread/include -I../memory -I../aes_block/include -I../swizzler/include ${AJIT_CORE_INCLUDE_FLAGS}" \
  > "${COMPILE_LOG}" 2>&1

if [[ ! -f "./sitar_sim" ]]; then
  echo "ERROR: sitar_sim was not generated. See ${COMPILE_LOG}" >&2
  exit 1
fi

echo "[4/5] Preparing gdb command script"
cat > "${GDB_CMDS}" <<'EOF'
set pagination off
set confirm off
set print thread-events off
set breakpoint pending on
handle SIGPIPE nostop noprint pass
handle SIGSEGV stop print pass
handle SIGABRT stop print pass
catch signal SIGSEGV
catch signal SIGABRT
run
echo \n===== gdb stop reason =====\n
info program
echo \n===== gdb where =====\n
where
echo \n===== gdb bt =====\n
bt
echo \n===== gdb bt full =====\n
bt full
echo \n===== gdb registers =====\n
info registers
echo \n===== gdb pc =====\n
x/i $pc
echo \n===== gdb info threads =====\n
info threads
echo \n===== gdb all thread bt full =====\n
thread apply all bt full
quit
EOF

echo "[5/5] Running under gdb (cycles=${RUN_CYCLES})"
gdb -q -x "${GDB_CMDS}" --args ./sitar_sim "${RUN_CYCLES}" > "${GDB_LOG}" 2>&1 || true

echo "Done."
echo "Compile log: ${COMPILE_LOG}"
echo "GDB log: ${GDB_LOG}"
echo "Run artifacts: ${SITAR_DIR}/TOP*.txt ${SITAR_DIR}/run.out ${SITAR_DIR}/run.err"
