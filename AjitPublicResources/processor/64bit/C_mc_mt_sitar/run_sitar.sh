#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
RUN_CYCLES="${1:-40000}"
TA0_STOP_MODE="${AJIT_STOP_ON_TA0:-off}"
TA0_DUMP_REGS="${AJIT_DUMP_REGS_ON_TA0:-1}"
END_DUMP_REGS="${AJIT_DUMP_REGS_ON_SUMMARY:-1}"
ACTIVE_THREADS_RESOLVED="${AJIT_ACTIVE_THREADS:-8}"
NUM_CORES_REQ="${AJIT_NUM_CORES:-4}"
NUM_CORES_RESOLVED=4
MAX_MODEL_THREADS=8
TRACE_W="${AJIT_TRACE_W:-}"
TRACE_E="${AJIT_TRACE_E:-}"
TRACE_F="${AJIT_TRACE_F:-}"
L2_CACHE_LINES="${AJIT_L2_CACHE_LINES:-}"
L2_ASSOC="${AJIT_L2_ASSOC:-}"
TRACE_L2="${AJIT_TRACE_L2:-}"
DUMP_MEMORY_ON_TA0="${AJIT_DUMP_MEMORY_ON_TA0:-0}"
EXPECT_MEM_ADDRS="${AJIT_EXPECT_MEM_ADDRS:-}"
CONSOLE_INPUT_FILE="${AJIT_CONSOLE_INPUT_FILE:-}"
CONSOLE_OUTPUT_FILE="${AJIT_CONSOLE_OUTPUT_FILE:-}"
INIT_PC="${AJIT_INIT_PC:-}"

if [[ ! -d "${SITAR_DIR}" ]]; then
  echo "ERROR: sitar directory not found at ${SITAR_DIR}" >&2
  exit 1
fi

if ! [[ "${NUM_CORES_REQ}" =~ ^[0-9]+$ ]]; then
  NUM_CORES_REQ=4
fi
if (( NUM_CORES_REQ < 1 )); then
  NUM_CORES_REQ=1
fi
if (( NUM_CORES_REQ > 4 )); then
  NUM_CORES_REQ=4
fi
NUM_CORES_RESOLVED="${NUM_CORES_REQ}"

MAX_MODEL_THREADS="$((NUM_CORES_RESOLVED * 2))"

if ! [[ "${ACTIVE_THREADS_RESOLVED}" =~ ^[0-9]+$ ]]; then
  ACTIVE_THREADS_RESOLVED="${MAX_MODEL_THREADS}"
fi
if (( ACTIVE_THREADS_RESOLVED < 1 )); then
  ACTIVE_THREADS_RESOLVED=1
fi
if (( ACTIVE_THREADS_RESOLVED > MAX_MODEL_THREADS )); then
  ACTIVE_THREADS_RESOLVED="${MAX_MODEL_THREADS}"
fi

cd "${SITAR_DIR}"
rm -f run.out run.err

if [[ ! -x "./sitar_sim" ]]; then
  echo "ERROR: ./sitar_sim not found. Run ./build_sitar.sh first." >&2
  exit 1
fi

echo "[run] cycles=${RUN_CYCLES}"
echo "[run] AJIT_NUM_CORES=${NUM_CORES_RESOLVED}"
echo "[run] AJIT_ACTIVE_THREADS=${ACTIVE_THREADS_RESOLVED}"
echo "[run] AJIT_STOP_ON_TA0=${TA0_STOP_MODE} AJIT_DUMP_REGS_ON_TA0=${TA0_DUMP_REGS} AJIT_DUMP_REGS_ON_SUMMARY=${END_DUMP_REGS}"
if [[ "${DUMP_MEMORY_ON_TA0}" == "1" && -n "${EXPECT_MEM_ADDRS}" ]]; then
  echo "[run] AJIT_DUMP_MEMORY_ON_TA0=1 AJIT_EXPECT_MEM_ADDRS=${EXPECT_MEM_ADDRS}"
fi
if [[ -n "${TRACE_W}" || -n "${TRACE_E}" || -n "${TRACE_F}" ]]; then
  echo "[run] AJIT_TRACE_W=${TRACE_W:-<off>} AJIT_TRACE_E=${TRACE_E:-<off>} AJIT_TRACE_F=${TRACE_F:-<off>}"
fi
if [[ -n "${L2_CACHE_LINES}" || -n "${L2_ASSOC}" || -n "${TRACE_L2}" ]]; then
  echo "[run] AJIT_L2_CACHE_LINES=${L2_CACHE_LINES:-<default>} AJIT_L2_ASSOC=${L2_ASSOC:-<default>} AJIT_TRACE_L2=${TRACE_L2:-<off>}"
fi
if [[ -n "${CONSOLE_INPUT_FILE}" ]]; then
  echo "[run] AJIT_CONSOLE_INPUT_FILE=${CONSOLE_INPUT_FILE}"
fi
if [[ -n "${CONSOLE_OUTPUT_FILE}" ]]; then
  echo "[run] AJIT_CONSOLE_OUTPUT_FILE=${CONSOLE_OUTPUT_FILE}"
fi
if [[ -n "${INIT_PC}" ]]; then
  echo "[run] AJIT_INIT_PC=${INIT_PC}"
fi

if ! AJIT_ACTIVE_THREADS="${ACTIVE_THREADS_RESOLVED}" \
       AJIT_NUM_CORES="${NUM_CORES_RESOLVED}" \
       AJIT_INIT_PC="${INIT_PC}" \
       AJIT_STOP_ON_TA0="${TA0_STOP_MODE}" \
       AJIT_DUMP_REGS_ON_TA0="${TA0_DUMP_REGS}" \
       AJIT_DUMP_REGS_ON_SUMMARY="${END_DUMP_REGS}" \
       AJIT_DUMP_MEMORY_ON_TA0="${DUMP_MEMORY_ON_TA0}" \
       AJIT_EXPECT_MEM_ADDRS="${EXPECT_MEM_ADDRS}" \
       AJIT_CONSOLE_INPUT_FILE="${CONSOLE_INPUT_FILE}" \
       AJIT_CONSOLE_OUTPUT_FILE="${CONSOLE_OUTPUT_FILE}" \
       AJIT_TRACE_W="${TRACE_W}" \
       AJIT_TRACE_E="${TRACE_E}" \
       AJIT_TRACE_F="${TRACE_F}" \
       AJIT_L2_CACHE_LINES="${L2_CACHE_LINES}" \
       AJIT_L2_ASSOC="${L2_ASSOC}" \
       AJIT_TRACE_L2="${TRACE_L2}" \
       ./sitar_sim "${RUN_CYCLES}" > run.out 2> run.err; then
  echo "ERROR: sitar_sim failed. See ${SITAR_DIR}/run.err" >&2
  tail -n 120 run.err || true
  exit 1
fi

echo "Run complete."
echo "  ${SITAR_DIR}/run.out"
echo "  ${SITAR_DIR}/run.err"
