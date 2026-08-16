#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
REPO_ROOT="${ROOT_DIR}/../../../.."
RUN_CYCLES="${1:-40000}"
THREAD_PROFILE="${AJIT_THREAD_PROFILE:-}"
THREAD_CONFIG_FILE="${AJIT_THREAD_CONFIG_FILE:-}"
EXPECT_RESULTS_FILE=""
EXPECT_MEM_ADDRS=""

load_thread_profile_config() {
  local cfg="$1"
  local line key value
  [[ -f "${cfg}" ]] || return 0
  while IFS= read -r line || [[ -n "${line}" ]]; do
    line="${line%%#*}"
    line="${line#"${line%%[![:space:]]*}"}"
    line="${line%"${line##*[![:space:]]}"}"
    [[ -z "${line}" ]] && continue
    [[ "${line}" != *"="* ]] && continue
    key="${line%%=*}"
    value="${line#*=}"
    key="${key#"${key%%[![:space:]]*}"}"
    key="${key%"${key##*[![:space:]]}"}"
    value="${value#"${value%%[![:space:]]*}"}"
    value="${value%"${value##*[![:space:]]}"}"
    [[ "${key}" == AJIT_* ]] || continue
    current_value="${!key-}"
    if [[ -z "${current_value}" ]]; then
      printf -v "${key}" '%s' "${value}"
      export "${key}"
    fi
  done < "${cfg}"
}

if [[ ! -d "${SITAR_DIR}" ]]; then
  echo "ERROR: sitar directory not found at ${SITAR_DIR}" >&2
  exit 1
fi

if [[ "${THREAD_PROFILE}" == "krishna" && -z "${THREAD_CONFIG_FILE}" ]]; then
  THREAD_CONFIG_FILE="${SITAR_DIR}/krishna.config"
fi
if [[ -n "${THREAD_CONFIG_FILE}" ]]; then
  if [[ -f "${THREAD_CONFIG_FILE}" ]]; then
    load_thread_profile_config "${THREAD_CONFIG_FILE}"
    export AJIT_THREAD_PROFILE="${THREAD_PROFILE}"
    export AJIT_THREAD_CONFIG_FILE="${THREAD_CONFIG_FILE}"
  else
    echo "WARN: thread config file not found at ${THREAD_CONFIG_FILE}" >&2
  fi
fi

TA0_STOP_MODE="${AJIT_STOP_ON_TA0:-off}"
TA0_DUMP_REGS="${AJIT_DUMP_REGS_ON_TA0:-1}"
END_DUMP_REGS="${AJIT_DUMP_REGS_ON_SUMMARY:-1}"
ACTIVE_THREADS_REQ="${AJIT_ACTIVE_THREADS:-auto}"
SKIP_BUILD="${AJIT_SKIP_BUILD:-0}"
SKIP_CHECK="${AJIT_SKIP_CHECK:-0}"
FORCE_REBUILD="${AJIT_FORCE_REBUILD:-0}"
TRACE_W="${AJIT_TRACE_W:-}"
TRACE_E="${AJIT_TRACE_E:-}"
TRACE_F="${AJIT_TRACE_F:-}"
L2_CACHE_LINES="${AJIT_L2_CACHE_LINES:-}"
L2_ASSOC="${AJIT_L2_ASSOC:-}"
TRACE_L2="${AJIT_TRACE_L2:-}"
MEMORY_DELAY="${AJIT_MEMORY_DELAY:-}"
NUM_CORES_REQ="${AJIT_NUM_CORES:-4}"
NUM_CORES_RESOLVED=4
MAX_MODEL_THREADS=8
THREADS_PER_CORE_REQ="${AJIT_THREADS_PER_CORE:-2}"
THREADS_PER_CORE_RESOLVED=2
THREAD_DESCRIPTOR_WORD="${AJIT_THREAD_DESCRIPTOR_WORD:-}"
THREAD_ISA_MODE="${AJIT_THREAD_ISA_MODE:-}"
THREAD_BP_TABLE_SIZE="${AJIT_THREAD_BP_TABLE_SIZE:-}"
THREAD_ICACHE_LINES="${AJIT_THREAD_ICACHE_NUMBER_OF_LINES:-}"
THREAD_ICACHE_ASSOC="${AJIT_THREAD_ICACHE_ASSOCIATIVITY:-}"
THREAD_DCACHE_LINES="${AJIT_THREAD_DCACHE_NUMBER_OF_LINES:-}"
THREAD_DCACHE_ASSOC="${AJIT_THREAD_DCACHE_ASSOCIATIVITY:-}"
THREAD_TLB0_LOG_MEM_SIZE="${AJIT_THREAD_TLB0_LOG_MEM_SIZE:-}"
THREAD_TLB0_LOG_SET_SIZE="${AJIT_THREAD_TLB0_LOG_SET_SIZE:-}"
THREAD_TLB1_LOG_MEM_SIZE="${AJIT_THREAD_TLB1_LOG_MEM_SIZE:-}"
THREAD_TLB1_LOG_SET_SIZE="${AJIT_THREAD_TLB1_LOG_SET_SIZE:-}"
THREAD_TLB2_LOG_MEM_SIZE="${AJIT_THREAD_TLB2_LOG_MEM_SIZE:-}"
THREAD_TLB2_LOG_SET_SIZE="${AJIT_THREAD_TLB2_LOG_SET_SIZE:-}"
THREAD_TLB3_LOG_MEM_SIZE="${AJIT_THREAD_TLB3_LOG_MEM_SIZE:-}"
THREAD_TLB3_LOG_SET_SIZE="${AJIT_THREAD_TLB3_LOG_SET_SIZE:-}"
CONSOLE_INPUT_FILE="${AJIT_CONSOLE_INPUT_FILE:-}"
CONSOLE_OUTPUT_FILE="${AJIT_CONSOLE_OUTPUT_FILE:-}"
INIT_PC="${AJIT_INIT_PC:-}"

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

cd "${SITAR_DIR}"
LOG_DIR="${SITAR_DIR}/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"

SIN_MODEL_DIR="${REPO_ROOT}/tests/examples/misc/sin-model-test"
SIN_MODEL_MEMMAP_REMAP="${SIN_MODEL_DIR}/main.mmap.remapped"
SIN_MODEL_MEMMAP="${SIN_MODEL_DIR}/main.mmap"
FUNC_CALL_DIR="${REPO_ROOT}/tests/examples/func_call"
FUNC_CALL_MEMMAP_REMAP="${FUNC_CALL_DIR}/main.mmap.remapped"
FUNC_CALL_MEMMAP="${FUNC_CALL_DIR}/main.mmap"
TARGET_MEMMAP="${ROOT_DIR}/test_memmap_input_0.txt"
MMAP_PROFILE="${AJIT_TEST_PROFILE:-sin_model}"
MMAP_OVERRIDE="${AJIT_TEST_MEMMAP:-}"
MMAP_ACQ_REMAP="${REPO_ROOT}/tests/examples/misc/others/acq/mem_bw.mmap.remapped"
MMAP_ACQ="${REPO_ROOT}/tests/examples/misc/others/acq/mem_bw.mmap"
MMAP_ACOSL_REMAP="${REPO_ROOT}/tests/examples/uclibc/math/acosl/main.mmap.remapped"
MMAP_ACOSL="${REPO_ROOT}/tests/examples/uclibc/math/acosl/main.mmap"
SELECTED_MEMMAP=""

if [[ -n "${MMAP_OVERRIDE}" ]]; then
  # Accept override as absolute, C_mc_mt_sitar-relative, or repo-root-relative path.
  if [[ "${MMAP_OVERRIDE}" = /* ]]; then
    SELECTED_MEMMAP="${MMAP_OVERRIDE}"
  elif [[ -f "${ROOT_DIR}/${MMAP_OVERRIDE}" ]]; then
    SELECTED_MEMMAP="${ROOT_DIR}/${MMAP_OVERRIDE}"
  elif [[ -f "${REPO_ROOT}/${MMAP_OVERRIDE}" ]]; then
    SELECTED_MEMMAP="${REPO_ROOT}/${MMAP_OVERRIDE}"
  else
    SELECTED_MEMMAP="${MMAP_OVERRIDE}"
  fi
elif [[ "${MMAP_PROFILE}" == "mem_bw_acq" ]]; then
  if [[ -f "${MMAP_ACQ_REMAP}" ]]; then
    SELECTED_MEMMAP="${MMAP_ACQ_REMAP}"
  else
    SELECTED_MEMMAP="${MMAP_ACQ}"
  fi
elif [[ "${MMAP_PROFILE}" == "acosl" ]]; then
  if [[ -f "${MMAP_ACOSL_REMAP}" ]]; then
    SELECTED_MEMMAP="${MMAP_ACOSL_REMAP}"
  else
    SELECTED_MEMMAP="${MMAP_ACOSL}"
  fi
elif [[ "${MMAP_PROFILE}" == "func_call" ]]; then
  if [[ -f "${FUNC_CALL_MEMMAP_REMAP}" ]]; then
    SELECTED_MEMMAP="${FUNC_CALL_MEMMAP_REMAP}"
  else
    SELECTED_MEMMAP="${FUNC_CALL_MEMMAP}"
  fi
else
  # Keep SITAR default aligned with current parity/debug workflow:
  # prefer the remapped sin-model mmap when available.
  if [[ -f "${SIN_MODEL_MEMMAP_REMAP}" ]]; then
    SELECTED_MEMMAP="${SIN_MODEL_MEMMAP_REMAP}"
  else
    SELECTED_MEMMAP="${SIN_MODEL_MEMMAP}"
  fi
fi

if [[ -n "${SELECTED_MEMMAP}" && -f "${SELECTED_MEMMAP}" ]]; then
  cp "${SELECTED_MEMMAP}" "${TARGET_MEMMAP}"
  echo "[prep] Loaded profile=${MMAP_PROFILE} mmap=${SELECTED_MEMMAP} -> ${TARGET_MEMMAP}"
else
  echo "[prep] WARNING: selected mmap not found (profile=${MMAP_PROFILE}, override=${MMAP_OVERRIDE}); using existing ${TARGET_MEMMAP}"
fi

# Resolve active thread count:
# - manual override: AJIT_ACTIVE_THREADS=<1..8>
# - auto mode: if mmap belongs to a test-case folder (Ajit.TestCase), default to 1
#   unless an explicit thread_count.txt exists in that folder.
if [[ "${ACTIVE_THREADS_REQ}" != "auto" ]]; then
  ACTIVE_THREADS_RESOLVED="${ACTIVE_THREADS_REQ}"
else
  MEMMAP_DIR="$(dirname "${SELECTED_MEMMAP:-${TARGET_MEMMAP}}")"
  if [[ -f "${MEMMAP_DIR}/thread_count.txt" ]]; then
    ACTIVE_THREADS_RESOLVED="$(tr -d '[:space:]' < "${MEMMAP_DIR}/thread_count.txt")"
  elif [[ -f "${MEMMAP_DIR}/Ajit.TestCase" ]]; then
    ACTIVE_THREADS_RESOLVED="1"
  else
    ACTIVE_THREADS_RESOLVED="${MAX_MODEL_THREADS}"
  fi
fi

# Safety clamp
if ! [[ "${ACTIVE_THREADS_RESOLVED}" =~ ^[0-9]+$ ]]; then
  ACTIVE_THREADS_RESOLVED="${MAX_MODEL_THREADS}"
fi
if (( ACTIVE_THREADS_RESOLVED < 1 )); then
  ACTIVE_THREADS_RESOLVED=1
fi
if (( ACTIVE_THREADS_RESOLVED > MAX_MODEL_THREADS )); then
  ACTIVE_THREADS_RESOLVED="${MAX_MODEL_THREADS}"
fi

RESULT_DIR="$(dirname "${SELECTED_MEMMAP:-${TARGET_MEMMAP}}")"
EXPECT_RESULTS_FILE="${AJIT_EXPECT_RESULTS_FILE:-${RESULT_DIR}/main.results}"
if [[ "${EXPECT_RESULTS_FILE}" != /* ]]; then
  if [[ -f "${ROOT_DIR}/${EXPECT_RESULTS_FILE}" ]]; then
    EXPECT_RESULTS_FILE="${ROOT_DIR}/${EXPECT_RESULTS_FILE}"
  elif [[ -f "${REPO_ROOT}/${EXPECT_RESULTS_FILE}" ]]; then
    EXPECT_RESULTS_FILE="${REPO_ROOT}/${EXPECT_RESULTS_FILE}"
  fi
fi

if [[ -f "${EXPECT_RESULTS_FILE}" ]]; then
  EXPECT_MEM_ADDRS="$(
    awk '
      match($0, /^[[:space:]]*m\[[[:space:]]*0x[0-9a-fA-F]+[[:space:]]*\]/) {
        s = substr($0, RSTART, RLENGTH);
        gsub(/^[[:space:]]*m\[[[:space:]]*0x/, "", s);
        gsub(/[[:space:]]*\]$/, "", s);
        key = tolower(s);
        if (!(key in seen)) {
          seen[key] = 1;
          if (out != "") out = out ",";
          out = out "0x" key;
        }
      }
      END { print out; }
    ' "${EXPECT_RESULTS_FILE}"
  )"
fi

echo "[flow 1/3] build"
echo "[cfg] AJIT_NUM_CORES=${NUM_CORES_RESOLVED} MAX_MODEL_THREADS=${MAX_MODEL_THREADS}"
if [[ "${FORCE_REBUILD}" == "1" ]]; then
  echo "[build] forced rebuild (AJIT_FORCE_REBUILD=1)"
  AJIT_NUM_CORES="${NUM_CORES_RESOLVED}" "${ROOT_DIR}/build_sitar.sh"
elif [[ "${SKIP_BUILD}" == "1" ]]; then
  if [[ ! -x "${SITAR_DIR}/sitar_sim" ]]; then
    echo "ERROR: AJIT_SKIP_BUILD=1 but ${SITAR_DIR}/sitar_sim is missing or not executable." >&2
    exit 1
  fi
  echo "[build] skipped (AJIT_SKIP_BUILD=1)"
else
  if [[ -x "${SITAR_DIR}/sitar_sim" ]]; then
    echo "[build] skipped (existing sitar_sim; set AJIT_FORCE_REBUILD=1 to rebuild)"
  else
    echo "[build] sitar_sim missing; building once"
    AJIT_NUM_CORES="${NUM_CORES_RESOLVED}" "${ROOT_DIR}/build_sitar.sh"
  fi
fi

echo "[flow 2/3] run"
echo "[cfg] AJIT_ACTIVE_THREADS=${ACTIVE_THREADS_RESOLVED} (requested=${ACTIVE_THREADS_REQ})"
echo "[cfg] AJIT_STOP_ON_TA0=${TA0_STOP_MODE} AJIT_DUMP_REGS_ON_TA0=${TA0_DUMP_REGS} AJIT_DUMP_REGS_ON_SUMMARY=${END_DUMP_REGS}"
if [[ -n "${EXPECT_MEM_ADDRS}" ]]; then
  echo "[cfg] AJIT_DUMP_MEMORY_ON_TA0=1 AJIT_EXPECT_MEM_ADDRS=${EXPECT_MEM_ADDRS}"
fi
if [[ -n "${TRACE_W}" || -n "${TRACE_E}" || -n "${TRACE_F}" ]]; then
  echo "[cfg] AJIT_TRACE_W=${TRACE_W:-<off>} AJIT_TRACE_E=${TRACE_E:-<off>} AJIT_TRACE_F=${TRACE_F:-<off>}"
fi
if [[ -n "${L2_CACHE_LINES}" || -n "${L2_ASSOC}" || -n "${TRACE_L2}" ]]; then
  echo "[cfg] AJIT_L2_CACHE_LINES=${L2_CACHE_LINES:-<default>} AJIT_L2_ASSOC=${L2_ASSOC:-<default>} AJIT_TRACE_L2=${TRACE_L2:-<off>}"
fi
if [[ -n "${MEMORY_DELAY}" ]]; then
  echo "[cfg] AJIT_MEMORY_DELAY=${MEMORY_DELAY}"
fi
if [[ -n "${THREAD_DESCRIPTOR_WORD}" || -n "${THREAD_ISA_MODE}" || -n "${THREAD_BP_TABLE_SIZE}" || -n "${THREAD_ICACHE_LINES}" || -n "${THREAD_ICACHE_ASSOC}" || -n "${THREAD_DCACHE_LINES}" || -n "${THREAD_DCACHE_ASSOC}" || -n "${THREAD_TLB0_LOG_MEM_SIZE}" || -n "${THREAD_TLB0_LOG_SET_SIZE}" || -n "${THREAD_TLB1_LOG_MEM_SIZE}" || -n "${THREAD_TLB1_LOG_SET_SIZE}" || -n "${THREAD_TLB2_LOG_MEM_SIZE}" || -n "${THREAD_TLB2_LOG_SET_SIZE}" || -n "${THREAD_TLB3_LOG_MEM_SIZE}" || -n "${THREAD_TLB3_LOG_SET_SIZE}" ]]; then
  echo "[cfg] AJIT_THREAD_DESCRIPTOR_WORD=${THREAD_DESCRIPTOR_WORD:-<default>} AJIT_THREAD_ISA_MODE=${THREAD_ISA_MODE:-<default>} AJIT_THREAD_BP_TABLE_SIZE=${THREAD_BP_TABLE_SIZE:-<default>} AJIT_THREAD_ICACHE_NUMBER_OF_LINES=${THREAD_ICACHE_LINES:-<default>} AJIT_THREAD_ICACHE_ASSOCIATIVITY=${THREAD_ICACHE_ASSOC:-<default>} AJIT_THREAD_DCACHE_NUMBER_OF_LINES=${THREAD_DCACHE_LINES:-<default>} AJIT_THREAD_DCACHE_ASSOCIATIVITY=${THREAD_DCACHE_ASSOC:-<default>} AJIT_THREAD_TLB0_LOG_MEM_SIZE=${THREAD_TLB0_LOG_MEM_SIZE:-<default>} AJIT_THREAD_TLB0_LOG_SET_SIZE=${THREAD_TLB0_LOG_SET_SIZE:-<default>} AJIT_THREAD_TLB1_LOG_MEM_SIZE=${THREAD_TLB1_LOG_MEM_SIZE:-<default>} AJIT_THREAD_TLB1_LOG_SET_SIZE=${THREAD_TLB1_LOG_SET_SIZE:-<default>} AJIT_THREAD_TLB2_LOG_MEM_SIZE=${THREAD_TLB2_LOG_MEM_SIZE:-<default>} AJIT_THREAD_TLB2_LOG_SET_SIZE=${THREAD_TLB2_LOG_SET_SIZE:-<default>} AJIT_THREAD_TLB3_LOG_MEM_SIZE=${THREAD_TLB3_LOG_MEM_SIZE:-<default>} AJIT_THREAD_TLB3_LOG_SET_SIZE=${THREAD_TLB3_LOG_SET_SIZE:-<default>}"
fi
if [[ -n "${CONSOLE_INPUT_FILE}" ]]; then
  echo "[cfg] AJIT_CONSOLE_INPUT_FILE=${CONSOLE_INPUT_FILE}"
fi
if [[ -n "${CONSOLE_OUTPUT_FILE}" ]]; then
  echo "[cfg] AJIT_CONSOLE_OUTPUT_FILE=${CONSOLE_OUTPUT_FILE}"
fi
if [[ -n "${INIT_PC}" ]]; then
  echo "[cfg] AJIT_INIT_PC=${INIT_PC}"
fi
AJIT_ACTIVE_THREADS="${ACTIVE_THREADS_RESOLVED}" \
AJIT_NUM_CORES="${NUM_CORES_RESOLVED}" \
AJIT_INIT_PC="${INIT_PC}" \
AJIT_STOP_ON_TA0="${TA0_STOP_MODE}" \
AJIT_DUMP_REGS_ON_TA0="${TA0_DUMP_REGS}" \
AJIT_DUMP_REGS_ON_SUMMARY="${END_DUMP_REGS}" \
AJIT_DUMP_MEMORY_ON_TA0="$([ -n "${EXPECT_MEM_ADDRS}" ] && echo 1 || echo 0)" \
AJIT_EXPECT_MEM_ADDRS="${EXPECT_MEM_ADDRS}" \
AJIT_CONSOLE_INPUT_FILE="${CONSOLE_INPUT_FILE}" \
AJIT_CONSOLE_OUTPUT_FILE="${CONSOLE_OUTPUT_FILE}" \
AJIT_RDASR_USES_SITAR_TIME=1 \
AJIT_TRACE_W="${TRACE_W}" \
AJIT_TRACE_E="${TRACE_E}" \
AJIT_TRACE_F="${TRACE_F}" \
AJIT_L2_CACHE_LINES="${L2_CACHE_LINES}" \
AJIT_L2_ASSOC="${L2_ASSOC}" \
AJIT_TRACE_L2="${TRACE_L2}" \
AJIT_MEMORY_DELAY="${MEMORY_DELAY}" \
AJIT_THREAD_DESCRIPTOR_WORD="${THREAD_DESCRIPTOR_WORD}" \
AJIT_THREAD_ISA_MODE="${THREAD_ISA_MODE}" \
AJIT_THREAD_BP_TABLE_SIZE="${THREAD_BP_TABLE_SIZE}" \
AJIT_THREAD_ICACHE_NUMBER_OF_LINES="${THREAD_ICACHE_LINES}" \
AJIT_THREAD_ICACHE_ASSOCIATIVITY="${THREAD_ICACHE_ASSOC}" \
AJIT_THREAD_DCACHE_NUMBER_OF_LINES="${THREAD_DCACHE_LINES}" \
AJIT_THREAD_DCACHE_ASSOCIATIVITY="${THREAD_DCACHE_ASSOC}" \
AJIT_THREAD_TLB0_LOG_MEM_SIZE="${THREAD_TLB0_LOG_MEM_SIZE}" \
AJIT_THREAD_TLB0_LOG_SET_SIZE="${THREAD_TLB0_LOG_SET_SIZE}" \
AJIT_THREAD_TLB1_LOG_MEM_SIZE="${THREAD_TLB1_LOG_MEM_SIZE}" \
AJIT_THREAD_TLB1_LOG_SET_SIZE="${THREAD_TLB1_LOG_SET_SIZE}" \
AJIT_THREAD_TLB2_LOG_MEM_SIZE="${THREAD_TLB2_LOG_MEM_SIZE}" \
AJIT_THREAD_TLB2_LOG_SET_SIZE="${THREAD_TLB2_LOG_SET_SIZE}" \
AJIT_THREAD_TLB3_LOG_MEM_SIZE="${THREAD_TLB3_LOG_MEM_SIZE}" \
AJIT_THREAD_TLB3_LOG_SET_SIZE="${THREAD_TLB3_LOG_SET_SIZE}" \
"${ROOT_DIR}/run_sitar.sh" "${RUN_CYCLES}"

echo "=== Bridge mode messages (run.err) ==="
grep -n "BRIDGE\\[" run.err || true
grep -n "fallback path active\\|real ajit_thread coroutine active" run.err || true
grep -n "BRIDGE-RUNTIME" run.err || true

echo "[flow 3/3] check"
if [[ "${SKIP_CHECK}" == "1" ]]; then
  echo "[check] skipped (AJIT_SKIP_CHECK=1)"
else
  AJIT_EXPECT_RESULTS_FILE="${EXPECT_RESULTS_FILE}" \
  "${ROOT_DIR}/check_sitar_results.sh"
fi

if [[ "${AJIT_DUMP_TXT_HEADS:-0}" == "1" ]]; then
  echo "=== Log heads (*.txt, excluding sitar_scons_config.txt) ==="
  for f in *.txt; do
    [[ "${f}" == "sitar_scons_config.txt" ]] && continue
    echo "==== ${f}"
    sed -n '1,120p' "${f}"
  done
fi

echo "Done. Outputs are in ${SITAR_DIR}: run.out, run.err, TOP*.txt"
