#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
AJIT_SRC_ROOT="${ROOT_DIR}/../C_multi_core_multi_thread"
REPO_ROOT="${ROOT_DIR}/../../../.."
RUN_CYCLES="${1:-40000}"
SITAR_LOGGING_ARGS=()
if [[ "${AJIT_SITAR_ENABLE_LOGGING:-0}" != "0" ]]; then
  SITAR_LOGGING_ARGS+=(--logging)
else
  SITAR_LOGGING_ARGS+=(--no-logging)
fi

if [[ ! -d "${SITAR_DIR}" ]]; then
  echo "ERROR: sitar directory not found at ${SITAR_DIR}" >&2
  exit 1
fi

cd "${SITAR_DIR}"
LOG_DIR="${SITAR_DIR}/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"

AJIT_CORE_INCLUDE_FLAGS="\
-I${AJIT_SRC_ROOT}/common/include \
-I${AJIT_SRC_ROOT}/cpu/include \
-I${AJIT_SRC_ROOT}/cpu_interface/include \
-I${AJIT_SRC_ROOT}/mmu/include \
-I${AJIT_SRC_ROOT}/cache/include \
-I${AJIT_SRC_ROOT}/monitorLogger/include \
-I${AJIT_SRC_ROOT}/tlbs/include \
-I${AJIT_SRC_ROOT}/rlut/include \
-I${AJIT_SRC_ROOT}/debugger/hwServer/include \
-I${REPO_ROOT}/ahir_release/include"

echo "[1/4] Cleaning generated Output/*"
rm -rf Output/*
rm -f run.out run.err sitar_sim

echo "[1b/4] Cleaning stale objects in C_mc_mt_sitar"
find "${ROOT_DIR}" -type f -name '*.o' -delete

echo "[2/4] Translating sitar sources"
if ! sitar translate cop.sitar > "${LOG_DIR}/translate_cop_${STAMP}.log" 2>&1; then
  echo "ERROR: sitar translate cop.sitar failed. See ${LOG_DIR}/translate_cop_${STAMP}.log" >&2
  tail -n 120 "${LOG_DIR}/translate_cop_${STAMP}.log" || true
  exit 1
fi
if ! sitar translate memorytop.sitar > "${LOG_DIR}/translate_memorytop_${STAMP}.log" 2>&1; then
  echo "ERROR: sitar translate memorytop.sitar failed. See ${LOG_DIR}/translate_memorytop_${STAMP}.log" >&2
  tail -n 120 "${LOG_DIR}/translate_memorytop_${STAMP}.log" || true
  exit 1
fi

echo "[3/4] Compiling sitar_sim"
echo "[cfg] AJIT_SITAR_ENABLE_LOGGING=${AJIT_SITAR_ENABLE_LOGGING:-0}"
if ! sitar compile \
  -d ./Output \
  -d ../shim \
  -d ../ajit_thread/src \
  -d ../ajit_thread_deps/src \
  -d ../memory \
  -d ../aes_block/src \
  -d ../swizzler/src \
  -d . \
  -m ../sitar_default_main.cpp \
  --openmp "${SITAR_LOGGING_ARGS[@]}" \
  --cflags "-B /opt/rh/gcc-toolset-11/root/usr/bin -D SW -D SITAR_NUMT=8 -DUSE_NEW_TLB -std=c++20 -O2 -Wall -Wextra -I../shim -I../ajit_thread/include -I../memory -I../aes_block/include -I../swizzler/include ${AJIT_CORE_INCLUDE_FLAGS} -I${REPO_ROOT}/ahir_release/functionLibrary/include" \
  > "${LOG_DIR}/compile_${STAMP}.log" 2>&1; then
  echo "ERROR: sitar compile failed. See ${LOG_DIR}/compile_${STAMP}.log" >&2
  tail -n 200 "${LOG_DIR}/compile_${STAMP}.log" || true
  exit 1
fi

if ! grep -q "SUCCESS: Generated Simulation Executable" "${LOG_DIR}/compile_${STAMP}.log"; then
  echo "ERROR: sitar compile did not report SUCCESS. See ${LOG_DIR}/compile_${STAMP}.log" >&2
  tail -n 200 "${LOG_DIR}/compile_${STAMP}.log" || true
  exit 1
fi

if [[ ! -f "./sitar_sim" ]]; then
  echo "ERROR: sitar_sim was not generated. See ${LOG_DIR}/compile_${STAMP}.log" >&2
  tail -n 200 "${LOG_DIR}/compile_${STAMP}.log" || true
  exit 1
fi

echo "[4/4] Running sitar_sim (${RUN_CYCLES} cycles)"
if ! ./sitar_sim "${RUN_CYCLES}" > run.out 2> run.err; then
  echo "ERROR: sitar_sim failed. See run.out and run.err" >&2
  tail -n 120 run.err || true
  exit 1
fi

echo "=== Bridge mode messages (run.err) ==="
grep -n "BRIDGE\\[" run.err || true
grep -n "fallback path active\\|real ajit_thread coroutine active" run.err || true
grep -n "BRIDGE-RUNTIME" run.err || true

echo "=== Log heads (*.txt, excluding sitar_scons_config.txt) ==="
for f in *.txt; do
  [[ "${f}" == "sitar_scons_config.txt" ]] && continue
  echo "==== ${f}"
  sed -n '1,120p' "${f}"
done

echo "Done. Outputs are in ${SITAR_DIR}: run.out, run.err, TOP*.txt"
echo "Build logs:"
echo "  ${LOG_DIR}/translate_cop_${STAMP}.log"
echo "  ${LOG_DIR}/translate_memorytop_${STAMP}.log"
echo "  ${LOG_DIR}/compile_${STAMP}.log"
