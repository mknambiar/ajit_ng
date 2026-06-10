#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
LOG_DIR="${SITAR_DIR}/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"
BUILD_LOG="${LOG_DIR}/compile_${STAMP}.log"
SITAR_LOGGING_ARGS=()
if [[ "${AJIT_SITAR_ENABLE_LOGGING:-0}" != "0" ]]; then
  SITAR_LOGGING_ARGS+=(--logging)
else
  SITAR_LOGGING_ARGS+=(--no-logging)
fi

cd "${SITAR_DIR}"

rm -rf Output/*

sitar translate cop.sitar 2>&1 | tee "${LOG_DIR}/translate_cop_${STAMP}.log"
sitar translate memorytop.sitar 2>&1 | tee "${LOG_DIR}/translate_memorytop_${STAMP}.log"

sitar compile \
  -d ./Output -d ../shim -d ../memory -d ../aes_block/src -d ../swizzler/src -d ../ajit_thread/src -d ../ajit_thread_deps/src -d . \
  -m ../sitar_default_main.cpp \
  --openmp "${SITAR_LOGGING_ARGS[@]}" \
  --cflags "-B /opt/rh/gcc-toolset-11/root/usr/bin -D SW -D SITAR_NUMT=8 -DUSE_NEW_TLB -std=c++20 -O2 -Wall -Wextra -I../shim -I../ajit_thread/include -I../aes_block/include -I../swizzler/include -I../../C_multi_core_multi_thread/common/include -I../../C_multi_core_multi_thread/cpu/include -I../../C_multi_core_multi_thread/cpu_interface/include -I../../C_multi_core_multi_thread/mmu/include -I../../C_multi_core_multi_thread/cache/include -I../../C_multi_core_multi_thread/tlbs/include -I../../C_multi_core_multi_thread/monitorLogger/include -I../../C_multi_core_multi_thread/debugger/hwServer/include -I../../C_multi_core_multi_thread/bridge/include -I../../C_multi_core_multi_thread/rlut/include -I../../C_multi_core_multi_thread/half_precision_float/include -I${ROOT_DIR}/../../../../ahir_release/include -I${ROOT_DIR}/../../../../ahir_release/functionLibrary/include" \
  2>&1 | tee "${BUILD_LOG}"

echo "Build complete. Log: ${BUILD_LOG}"
