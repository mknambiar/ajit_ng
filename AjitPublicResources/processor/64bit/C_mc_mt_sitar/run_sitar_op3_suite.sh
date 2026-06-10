#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${ROOT_DIR}/../../../.." && pwd)"
CYCLES="${1:-8000}"
MIN_HITS="${AJIT_OP3_MIN_HITS:-1}"
CASA_MMAP_OVERRIDE="${AJIT_CASA_MMAP:-}"

LOG_DIR="${ROOT_DIR}/sitar/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"
SUITE_LOG="${LOG_DIR}/op3_suite_${STAMP}.log"

ensure_casa_mmap() {
  local default_rel="tests/verification/ajit64/New_instn_tests/cswap/compiled/cswap/cswap.mmap"
  local default_abs="${REPO_ROOT}/${default_rel}"
  if [[ -f "${default_abs}" ]]; then
    echo "${default_abs}"
    return 0
  fi

  local gen_dir="${REPO_ROOT}/.codex-tmp/casa_probe"
  local gen_mmap="${gen_dir}/cswap.mmap"
  if [[ -f "${gen_mmap}" ]]; then
    echo "${gen_mmap}"
    return 0
  fi

  local tool_py="${REPO_ROOT}/AjitPublicResources/tools/scripts/compileToSparc.ForValidation.py"
  local lnk="${REPO_ROOT}/AjitPublicResources/tools/linker/validationLinkerScript.lnk"
  local src="${REPO_ROOT}/tests/verification/ajit64/New_instn_tests/cswap/cswap.s"
  if [[ ! -f "${tool_py}" || ! -f "${lnk}" || ! -f "${src}" ]]; then
    return 1
  fi

  mkdir -p "${gen_dir}"
  (
    cd "${gen_dir}"
    AJIT_PROJECT_HOME="${REPO_ROOT}" \
      python3 "${tool_py}" \
        -L "${lnk}" \
        -E cswap.elf -V cswap.vars -H cswap.hex -M cswap.mmap -O cswap.objdump \
        -s "${src}" >/tmp/run_sitar_op3_casa_build.out 2>/tmp/run_sitar_op3_casa_build.err
  ) || return 1

  [[ -f "${gen_mmap}" ]] || return 1
  echo "${gen_mmap}"
  return 0
}

audit_case() {
  local op3="$1"
  local label="$2"
  local mmap_in="$3"

  local mmap_path="$mmap_in"
  if [[ ! -f "${mmap_path}" ]]; then
    if [[ -f "${ROOT_DIR}/${mmap_in}" ]]; then
      mmap_path="${ROOT_DIR}/${mmap_in}"
    elif [[ -f "${REPO_ROOT}/${mmap_in}" ]]; then
      mmap_path="${REPO_ROOT}/${mmap_in}"
    fi
  fi

  echo "---- case op3=${op3} label=${label}" | tee -a "${SUITE_LOG}"
  echo "mmap=${mmap_in}" | tee -a "${SUITE_LOG}"

  if [[ ! -f "${mmap_path}" ]]; then
    echo "status: SKIP (mmap not found)" | tee -a "${SUITE_LOG}"
    return 10
  fi

  local out="/tmp/run_sitar_op3_suite_${op3}_${STAMP}.out"
  local err="/tmp/run_sitar_op3_suite_${op3}_${STAMP}.err"

  if AJIT_TEST_MEMMAP="${mmap_path}" \
     AJIT_OP3_ONLY="${op3}" \
     AJIT_OP3_MIN_HITS="${MIN_HITS}" \
     AJIT_OP3_STRICT_MISSING=1 \
     "${ROOT_DIR}/run_sitar_mem_op3_probe.sh" "${CYCLES}" >"${out}" 2>"${err}"; then
    local child_log
    child_log="$(sed -n 's/^log: //p' "${out}" | tail -n 1)"
    [[ -z "${child_log}" ]] && child_log="(not reported)"
    echo "status: PASS" | tee -a "${SUITE_LOG}"
    echo "child-log: ${child_log}" | tee -a "${SUITE_LOG}"
    return 0
  else
    local child_log
    child_log="$(sed -n 's/^log: //p' "${out}" | tail -n 1)"
    [[ -z "${child_log}" ]] && child_log="(not reported)"
    echo "status: FAIL" | tee -a "${SUITE_LOG}"
    echo "child-log: ${child_log}" | tee -a "${SUITE_LOG}"
    echo "---- child tail ----" | tee -a "${SUITE_LOG}"
    tail -n 40 "${out}" | tee -a "${SUITE_LOG}" || true
    tail -n 40 "${err}" | tee -a "${SUITE_LOG}" || true
    return 1
  fi
}

echo "OP3 suite: cycles=${CYCLES} min_hits=${MIN_HITS}" | tee -a "${SUITE_LOG}"
if [[ -n "${CASA_MMAP_OVERRIDE}" ]]; then
  echo "CASA override mmap: ${CASA_MMAP_OVERRIDE}" | tee -a "${SUITE_LOG}"
else
  echo "CASA override mmap: (not set; will auto-probe/generate for 0x2f)" | tee -a "${SUITE_LOG}"
fi

declare -a CASES=(
  "0f swap tests/verification/ajit32/instruction_tests/data_transfer/swap/compiled/swap/swap.mmap"
  "1f swapa tests/verification/ajit32/instruction_tests/data_transfer/swap/compiled/swapa/swapa.mmap"
  "0d ldstub tests/verification/ajit32/instruction_tests/data_transfer/ldstub/compiled/ldstub/ldstub.mmap"
  "1d ldstuba tests/verification/ajit32/instruction_tests/data_transfer/ldstub/compiled/ldstuba/ldstuba.mmap"
  "17 stda tests/verification/ajit32/instruction_tests/data_transfer/ld_st/iu_ld_st/store/stda/compiled/stda/stda.mmap"
  "2f casa __CASA_MMAP__"
  "3f casxa tests/examples/misc/others/acq/mem_bw.mmap"
)

pass_count=0
fail_count=0
skip_count=0

for c in "${CASES[@]}"; do
  op3="$(echo "$c" | awk '{print $1}')"
  label="$(echo "$c" | awk '{print $2}')"
  mmap="$(echo "$c" | awk '{print $3}')"

  if [[ "${mmap}" == "__CASA_MMAP__" ]]; then
    if [[ -n "${CASA_MMAP_OVERRIDE}" ]]; then
      mmap="${CASA_MMAP_OVERRIDE}"
    else
      if mmap_auto="$(ensure_casa_mmap)"; then
        mmap="${mmap_auto}"
        echo "auto-generated/located CASA mmap: ${mmap}" | tee -a "${SUITE_LOG}"
      else
        # Keep 0x2f visible if generate path is unavailable.
        mmap="tests/verification/ajit64/New_instn_tests/cswap/compiled/cswap/cswap.mmap"
        echo "CASA mmap unavailable; 0x2f may skip (see /tmp/run_sitar_op3_casa_build.err)" | tee -a "${SUITE_LOG}"
      fi
    fi
  fi

  set +e
  audit_case "${op3}" "${label}" "${mmap}"
  rc=$?
  set -e

  if [[ $rc -eq 0 ]]; then
    pass_count=$((pass_count + 1))
  elif [[ $rc -eq 10 ]]; then
    skip_count=$((skip_count + 1))
  else
    fail_count=$((fail_count + 1))
  fi

done

echo "==== suite summary: pass=${pass_count} fail=${fail_count} skip=${skip_count}" | tee -a "${SUITE_LOG}"
echo "log: ${SUITE_LOG}" | tee -a "${SUITE_LOG}"

if [[ ${fail_count} -ne 0 ]]; then
  exit 2
fi
