#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
AJIT_SRC_ROOT="${ROOT_DIR}/../C_multi_core_multi_thread"
REPO_ROOT="${ROOT_DIR}/../../../.."
SITAR_BIN="${SITAR_BIN:-$(command -v sitar || true)}"
if [[ -z "${SITAR_BIN}" || ! -x "${SITAR_BIN}" ]]; then
  echo "ERROR: install SiTAR and put sitar on PATH, or set SITAR_BIN to its executable." >&2
  exit 1
fi
# Resolve compiler helpers alongside the selected GCC, including toolset symlinks.
HOST_COMPILER_DIR="$(dirname "$(readlink -f "$(command -v g++)")")"
NUM_CORES_REQ="${AJIT_NUM_CORES:-4}"

if [[ ! -d "${SITAR_DIR}" ]]; then
  echo "ERROR: sitar directory not found at ${SITAR_DIR}" >&2
  exit 1
fi

LOG_DIR="${SITAR_DIR}/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"

if ! [[ "${NUM_CORES_REQ}" =~ ^[0-9]+$ ]]; then
  NUM_CORES_REQ=4
fi
if (( NUM_CORES_REQ < 1 )); then
  NUM_CORES_REQ=1
fi
if (( NUM_CORES_REQ > 4 )); then
  NUM_CORES_REQ=4
fi
SITAR_NUM_CORES_RESOLVED="${NUM_CORES_REQ}"
SITAR_NUMT_RESOLVED="$((SITAR_NUM_CORES_RESOLVED * 2))"
SITAR_LOGGING_ARGS=()
if [[ "${AJIT_SITAR_ENABLE_LOGGING:-0}" != "0" ]]; then
  SITAR_LOGGING_ARGS+=(--logging)
else
  SITAR_LOGGING_ARGS+=(--no-logging)
fi

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

cd "${SITAR_DIR}"



echo "[build 1/4] Cleaning generated Output/*"
rm -rf Output/*
rm -f sitar_sim

echo "[build 2/4] Cleaning stale objects in C_mc_mt_sitar"
find "${ROOT_DIR}" -type f -name '*.o' -delete

echo "[build 3/4] Translating sitar sources"
 if ! "${SITAR_BIN}" translate cop.sitar > "${LOG_DIR}/translate_cop_${STAMP}.log" 2>&1; then
  echo "ERROR: sitar translate cop.sitar failed. See ${LOG_DIR}/translate_cop_${STAMP}.log" >&2
  tail -n 120 "${LOG_DIR}/translate_cop_${STAMP}.log" || true
  exit 1
fi
 if ! "${SITAR_BIN}" translate memorytop.sitar > "${LOG_DIR}/translate_memorytop_${STAMP}.log" 2>&1; then
  echo "ERROR: sitar translate memorytop.sitar failed. See ${LOG_DIR}/translate_memorytop_${STAMP}.log" >&2
  tail -n 120 "${LOG_DIR}/translate_memorytop_${STAMP}.log" || true
  exit 1
fi

echo "[build 4/4] Compiling sitar_sim"
echo "[build cfg] AJIT_NUM_CORES=${SITAR_NUM_CORES_RESOLVED} SITAR_NUMT=${SITAR_NUMT_RESOLVED}"
echo "[build cfg] AJIT_SITAR_ENABLE_LOGGING=${AJIT_SITAR_ENABLE_LOGGING:-0}"
if ! "${SITAR_BIN}" compile \
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
  --cflags "-B ${HOST_COMPILER_DIR}/ ${AJIT_HOST_CFLAGS:-} -D SW -D SITAR_NUM_CORES=${SITAR_NUM_CORES_RESOLVED} -D SITAR_NUMT=${SITAR_NUMT_RESOLVED} -DUSE_NEW_TLB -std=c++20 -O2 -Wall -Wextra -I../shim -I../ajit_thread/include -I../memory -I../aes_block/include -I../swizzler/include ${AJIT_CORE_INCLUDE_FLAGS}" \
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

echo "Build complete."
echo "  ${LOG_DIR}/translate_cop_${STAMP}.log"
echo "  ${LOG_DIR}/translate_memorytop_${STAMP}.log"
echo "  ${LOG_DIR}/compile_${STAMP}.log"
