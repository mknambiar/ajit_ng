#!/usr/bin/env bash
set -euo pipefail

# Regression runner for Ajit testcase folders listed in testcase registry.
# Supports optional per-test topology fields in registry:
#   id|path|expect|num_cores|threads_per_core|active_threads|active_thread_mask
# and rebuilds sitar_sim whenever core/thread topology changes between cases.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
FLOW_SCRIPT="${SCRIPT_DIR}/run_sitar_flow_real_thread.sh"
STAMP="$(date +%Y%m%d_%H%M%S)"
REG_DIR="${SCRIPT_DIR}/sitar/logs/regression/${STAMP}"
SUMMARY="${REG_DIR}/summary.txt"

REGISTRY_FILE="${SCRIPT_DIR}/testcase_registry.txt"
TEST_IDS_CSV="${1:-all}"  # all | id1,id2,id3
RUN_CYCLES="${2:-400000}"
RUN_LEGACY="${RUN_LEGACY:-0}"   # 1 => run run_cmodel.sh when present
RUN_CLEAN="${RUN_CLEAN:-0}"     # 1 => run clean.sh when present
EXPECT_NAME_DEFAULT="${AJIT_EXPECT_RESULTS_NAME:-main.results}"  # default expected results filename
EXPECT_AUTO_PICK="${AJIT_EXPECT_RESULTS_AUTO_PICK:-1}"           # if default missing, auto-pick single *.results

mkdir -p "${REG_DIR}"

if [[ ! -x "${FLOW_SCRIPT}" ]]; then
  echo "ERROR: missing flow script: ${FLOW_SCRIPT}" >&2
  exit 1
fi

if [[ ! -f "${REGISTRY_FILE}" ]]; then
  echo "ERROR: testcase registry missing: ${REGISTRY_FILE}" >&2
  exit 1
fi

cd "${REPO_ROOT}"
if [[ ! -f "./set_ajit_home" ]]; then
  echo "ERROR: set_ajit_home not found in ${REPO_ROOT}" >&2
  exit 1
fi

source ./set_ajit_home
# ajit_env expects some vars to exist; avoid nounset failures here.
set +u
source "${AJIT_HOME}/ajit_env"
set -u

run_in_dir() {
  local dir="$1"
  shift
  pushd "${dir}" > /dev/null
  "$@"
  popd > /dev/null
}

declare -A REG_PATHS=()
declare -A REG_EXPECTS=()
declare -A REG_NUM_CORES=()
declare -A REG_THREADS_PER_CORE=()
declare -A REG_ACTIVE_THREADS=()
declare -A REG_ACTIVE_MASK=()
while IFS='|' read -r id relpath expect_name num_cores threads_per_core active_threads active_mask; do
  id="$(echo "${id}" | tr -d '[:space:]')"
  relpath="$(echo "${relpath}" | tr -d '[:space:]')"
  expect_name="$(echo "${expect_name}" | tr -d '[:space:]')"
  num_cores="$(echo "${num_cores}" | tr -d '[:space:]')"
  threads_per_core="$(echo "${threads_per_core}" | tr -d '[:space:]')"
  active_threads="$(echo "${active_threads}" | tr -d '[:space:]')"
  active_mask="$(echo "${active_mask}" | tr -d '[:space:]')"
  [[ -z "${id}" ]] && continue
  [[ "${id}" =~ ^# ]] && continue
  [[ -z "${relpath}" ]] && continue
  REG_PATHS["${id}"]="${relpath}"
  REG_EXPECTS["${id}"]="${expect_name}"
  REG_NUM_CORES["${id}"]="${num_cores}"
  REG_THREADS_PER_CORE["${id}"]="${threads_per_core}"
  REG_ACTIVE_THREADS["${id}"]="${active_threads}"
  REG_ACTIVE_MASK["${id}"]="${active_mask}"
done < "${REGISTRY_FILE}"

if [[ ${#REG_PATHS[@]} -eq 0 ]]; then
  echo "ERROR: empty testcase registry: ${REGISTRY_FILE}" >&2
  exit 1
fi

declare -a SELECTED_IDS=()
if [[ "${TEST_IDS_CSV}" == "all" ]]; then
  mapfile -t SELECTED_IDS < <(printf '%s\n' "${!REG_PATHS[@]}" | sort)
else
  IFS=',' read -r -a req_ids <<< "${TEST_IDS_CSV}"
  for rid in "${req_ids[@]}"; do
    rid="$(echo "${rid}" | tr -d '[:space:]')"
    [[ -z "${rid}" ]] && continue
    if [[ -z "${REG_PATHS[${rid}]:-}" ]]; then
      echo "ERROR: testcase id not in registry: ${rid}" >&2
      exit 1
    fi
    SELECTED_IDS+=("${rid}")
  done
fi

if [[ ${#SELECTED_IDS[@]} -eq 0 ]]; then
  echo "ERROR: no testcase ids selected" >&2
  exit 1
fi

echo "Registry        : ${REGISTRY_FILE}" | tee -a "${SUMMARY}"
echo "Selected IDs    : ${TEST_IDS_CSV}" | tee -a "${SUMMARY}"
echo "Cycles          : ${RUN_CYCLES}" | tee -a "${SUMMARY}"
echo "Run legacy      : ${RUN_LEGACY}" | tee -a "${SUMMARY}"
echo "Run clean       : ${RUN_CLEAN}" | tee -a "${SUMMARY}"
echo "Report dir      : ${REG_DIR}" | tee -a "${SUMMARY}"
echo | tee -a "${SUMMARY}"

pass=0
fail=0
last_build_key=""

for id in "${SELECTED_IDS[@]}"; do
  rel="${REG_PATHS[${id}]}"
  tc="${REPO_ROOT}/${rel}"
  name="$(echo "${id}" | tr '/' '_')"
  log="${REG_DIR}/${name}.log"

  echo "===== ${id} -> ${rel} =====" | tee -a "${SUMMARY}"
  set +e
  {
    echo "[case-id] ${id}"
    echo "[case-path] ${rel}"
    if [[ ! -d "${tc}" ]]; then
      echo "[error] testcase directory missing: ${tc}"
      exit 12
    fi
    build_script=""
    if [[ -f "${tc}/build.sh" ]]; then
      build_script="build.sh"
    elif [[ -f "${tc}/compile_for_ajit.sh" ]]; then
      build_script="compile_for_ajit.sh"
    else
      echo "[error] missing build script in ${tc} (expected build.sh or compile_for_ajit.sh)"
      exit 14
    fi

    expect_name="${REG_EXPECTS[${id}]}"
    if [[ -z "${expect_name}" ]]; then
      expect_name="${EXPECT_NAME_DEFAULT}"
    fi
    expect_file="${tc}/${expect_name}"
    if [[ ! -f "${expect_file}" && "${EXPECT_AUTO_PICK}" == "1" ]]; then
      mapfile -t alt_results < <(find "${tc}" -maxdepth 1 -type f -name "*.results" | sort)
      if [[ ${#alt_results[@]} -eq 1 ]]; then
        expect_file="${alt_results[0]}"
        echo "[warn] missing ${tc}/${expect_name}; auto-picked ${expect_file}"
      fi
    fi
    if [[ ! -f "${expect_file}" ]]; then
      echo "[error] expected results file missing: ${expect_file}"
      exit 15
    fi

    if [[ ! -f "${tc}/Ajit.TestCase" ]]; then
      echo "[warn] Ajit.TestCase not found in ${tc}; proceeding"
    fi

    echo "[step] ${build_script}"
    if [[ -x "${tc}/${build_script}" ]]; then
      run_in_dir "${tc}" "./${build_script}"
    else
      echo "[warn] ${build_script} is not executable; invoking via bash"
      run_in_dir "${tc}" bash "./${build_script}"
    fi

    cfg_num_cores="${REG_NUM_CORES[${id}]}"
    cfg_threads_per_core="${REG_THREADS_PER_CORE[${id}]}"
    cfg_active_threads="${REG_ACTIVE_THREADS[${id}]}"
    cfg_active_mask="${REG_ACTIVE_MASK[${id}]}"

    if [[ -z "${cfg_num_cores}" ]]; then
      cfg_num_cores=4
    fi
    if [[ -z "${cfg_threads_per_core}" ]]; then
      cfg_threads_per_core=2
    fi

    if ! [[ "${cfg_num_cores}" =~ ^[0-9]+$ ]]; then
      echo "[error] invalid num_cores='${cfg_num_cores}' for ${id}"
      exit 16
    fi
    if ! [[ "${cfg_threads_per_core}" =~ ^[0-9]+$ ]]; then
      echo "[error] invalid threads_per_core='${cfg_threads_per_core}' for ${id}"
      exit 17
    fi

    build_key="${cfg_num_cores}:${cfg_threads_per_core}"
    if [[ "${build_key}" != "${last_build_key}" ]]; then
      cfg_skip_build=0
      last_build_key="${build_key}"
    else
      cfg_skip_build=1
    fi

    echo "[cfg] AJIT_NUM_CORES=${cfg_num_cores} AJIT_THREADS_PER_CORE=${cfg_threads_per_core} AJIT_ACTIVE_THREADS=${cfg_active_threads:-auto} AJIT_ACTIVE_THREAD_MASK=${cfg_active_mask:-<none>}"
    echo "[cfg] AJIT_SKIP_BUILD=${cfg_skip_build} (topology-key=${build_key})"

    if [[ "${RUN_LEGACY}" == "1" && -x "${tc}/run_cmodel.sh" ]]; then
      echo "[step] run_cmodel.sh"
      run_in_dir "${tc}" ./run_cmodel.sh
    fi

    mmap="${tc}/main.mmap.remapped"
    if [[ ! -f "${mmap}" ]]; then
      mmap="${tc}/main.mmap"
    fi
    if [[ ! -f "${mmap}" ]]; then
      mapfile -t alt_mmaps < <(find "${tc}" -maxdepth 1 -type f -name "*.mmap.remapped" | sort)
      if [[ ${#alt_mmaps[@]} -eq 1 ]]; then
        mmap="${alt_mmaps[0]}"
        echo "[warn] missing ${tc}/main.mmap.remapped; auto-picked ${mmap}"
      fi
    fi
    if [[ ! -f "${mmap}" ]]; then
      mapfile -t alt_mmaps < <(find "${tc}" -maxdepth 1 -type f -name "*.mmap" | sort)
      if [[ ${#alt_mmaps[@]} -eq 1 ]]; then
        mmap="${alt_mmaps[0]}"
        echo "[warn] missing ${tc}/main.mmap; auto-picked ${mmap}"
      fi
    fi
    if [[ ! -f "${mmap}" ]]; then
      echo "[error] no mmap found (expected main.mmap.remapped/main.mmap or single *.mmap[.remapped])"
      exit 11
    fi

    echo "[step] sitar flow"
    pushd "${SCRIPT_DIR}" > /dev/null
    AJIT_TEST_MEMMAP="${mmap}" \
    AJIT_EXPECT_RESULTS_FILE="${expect_file}" \
    AJIT_NUM_CORES="${cfg_num_cores}" \
    AJIT_THREADS_PER_CORE="${cfg_threads_per_core}" \
    AJIT_ACTIVE_THREADS="${cfg_active_threads}" \
    AJIT_ACTIVE_THREAD_MASK="${cfg_active_mask}" \
    AJIT_SKIP_BUILD="${cfg_skip_build}" \
    "${FLOW_SCRIPT}" "${RUN_CYCLES}"
    popd > /dev/null

    if [[ "${RUN_CLEAN}" == "1" && -x "${tc}/clean.sh" ]]; then
      echo "[step] clean.sh"
      run_in_dir "${tc}" ./clean.sh
    fi
  } > "${log}" 2>&1
  rc=$?
  set -e

  if [[ ${rc} -eq 0 ]]; then
    echo "PASS ${id}" | tee -a "${SUMMARY}"
    pass=$((pass + 1))
  else
    echo "FAIL ${id} (rc=${rc}) log=${log}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
  fi
done

echo | tee -a "${SUMMARY}"
echo "TOTAL: ${#SELECTED_IDS[@]} PASS: ${pass} FAIL: ${fail}" | tee -a "${SUMMARY}"
echo "Summary: ${SUMMARY}"

if [[ ${fail} -ne 0 ]]; then
  exit 1
fi
