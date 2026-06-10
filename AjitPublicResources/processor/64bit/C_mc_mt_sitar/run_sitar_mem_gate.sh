#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_CYCLES="${1:-40000}"

cd "${ROOT_DIR}"

# Memory-path regression gate:
# - memory-heavy mmap profile
# - init PC at known memory-op entry
export AJIT_TEST_PROFILE="${AJIT_TEST_PROFILE:-mem_bw_acq}"
export AJIT_INIT_PC="${AJIT_INIT_PC:-0x34}"

./run_sitar_flow_real_thread.sh "${RUN_CYCLES}"

RUN_ERR="${ROOT_DIR}/sitar/run.err"
if [[ ! -f "${RUN_ERR}" ]]; then
  echo "ERROR: ${RUN_ERR} not found"
  exit 1
fi

mem_total="$(sed -n 's/.*mem-class-total=\([0-9][0-9]*\).*/\1/p' "${RUN_ERR}" | tail -n 1)"
fmt3="$(sed -n 's/.*fmt3=\([0-9][0-9]*\).*/\1/p' "${RUN_ERR}" | tail -n 1)"
unhandled="$(rg -n "STEP-TASK-MEM-UNHANDLED" "${RUN_ERR}" | wc -l | tr -d ' ')"
rd_fail="$(rg -n "STEP-TASK-MEM-RD-FAIL" "${RUN_ERR}" | wc -l | tr -d ' ')"
wr_fail="$(rg -n "STEP-TASK-MEM-WR-FAIL" "${RUN_ERR}" | wc -l | tr -d ' ')"

echo "MEM-GATE summary: mem-class-total=${mem_total:-NA} fmt3=${fmt3:-NA} unhandled=${unhandled} rd_fail=${rd_fail} wr_fail=${wr_fail}"

if [[ -z "${mem_total}" || -z "${fmt3}" ]]; then
  echo "MEM-GATE FAIL: missing summary counters in run.err"
  exit 2
fi

if [[ "${mem_total}" -eq 0 || "${fmt3}" -eq 0 ]]; then
  echo "MEM-GATE FAIL: memory path not exercised (mem-class-total=${mem_total}, fmt3=${fmt3})"
  exit 3
fi

if [[ "${unhandled}" -ne 0 || "${rd_fail}" -ne 0 || "${wr_fail}" -ne 0 ]]; then
  echo "MEM-GATE FAIL: memory StepTask errors present"
  exit 4
fi

echo "MEM-GATE PASS"
