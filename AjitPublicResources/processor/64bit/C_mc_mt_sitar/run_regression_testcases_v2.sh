#!/usr/bin/env bash
set -euo pipefail

# Regression runner for Ajit testcase folders/.vprj files listed in testcase registry.
# Supports optional per-test fields in registry:
#   id|path|expect|num_cores|threads_per_core|active_threads|active_thread_mask|run_cycles|stdin_file|stdout_expected_file|init_pc|timer_tick_div|memory_delay|stop_on_ta0
# and rebuilds sitar_sim whenever core/thread topology changes between cases.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
FLOW_SCRIPT="${SCRIPT_DIR}/run_sitar_flow_real_thread.sh"
STAMP="$(date +%Y%m%d_%H%M%S)"
REG_DIR="${SCRIPT_DIR}/sitar/logs/regression/${STAMP}"
SUMMARY="${REG_DIR}/summary.txt"

REGISTRY_FILE="${SCRIPT_DIR}/testcase_registry_v2.txt"
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

resolve_console_sidecar() {
  local test_dir="$1"
  local test_base="$2"
  local kind="$3"
  local candidate=""

  case "${kind}" in
    stdin)
      for candidate in \
        "${test_dir}/${test_base}.stdin" \
        "${test_dir}/console.stdin"
      do
        [[ -f "${candidate}" ]] && { printf '%s\n' "${candidate}"; return 0; }
      done
      ;;
    stdout_expected)
      for candidate in \
        "${test_dir}/${test_base}.stdout.expected" \
        "${test_dir}/${test_base}.stdout" \
        "${test_dir}/console.stdout.expected" \
        "${test_dir}/console.stdout"
      do
        [[ -f "${candidate}" ]] && { printf '%s\n' "${candidate}"; return 0; }
      done
      ;;
  esac
  return 1
}

resolve_optional_registry_path() {
  local raw="${1:-}"
  if [[ -z "${raw}" ]]; then
    return 1
  fi
  if [[ "${raw}" = /* ]]; then
    printf '%s\n' "${raw}"
  else
    printf '%s\n' "${REPO_ROOT}/${raw}"
  fi
}

compare_console_output() {
  local expected_file="$1"
  local actual_file="$2"
  local marker=""
  local marker_count=""
  local marker_pattern=""
  local expected_count=""
  [[ -f "${expected_file}" ]] || return 0

  if [[ ! -f "${actual_file}" ]]; then
    echo "[console] expected output file present but actual output file missing: ${actual_file}"
    return 1
  fi

  if [[ "${expected_file}" == *.markers ]]; then
    while IFS= read -r marker || [[ -n "${marker}" ]]; do
      marker="${marker%$'\r'}"
      [[ -z "${marker}" ]] && continue
      [[ "${marker}" =~ ^# ]] && continue

      marker_pattern="${marker}"
      expected_count=""
      if [[ "${marker}" == *"|count="* ]]; then
        marker_pattern="${marker%%|count=*}"
        expected_count="${marker##*|count=}"
      fi
      marker_pattern="$(eval "printf '%s' \"${marker_pattern}\"")"

      if [[ -n "${expected_count}" ]]; then
        marker_count="$(rg -c -F "${marker_pattern}" "${actual_file}" || true)"
        if [[ "${marker_count}" != "${expected_count}" ]]; then
          echo "[console] marker count mismatch: '${marker_pattern}' expected=${expected_count} actual=${marker_count}"
          return 1
        fi
      elif ! rg -q -F "${marker_pattern}" "${actual_file}"; then
        echo "[console] marker missing: '${marker_pattern}'"
        return 1
      fi
    done < "${expected_file}"

    echo "[console] output markers matched ${expected_file}"
    return 0
  fi

  if ! cmp -s "${expected_file}" "${actual_file}"; then
    echo "[console] output mismatch"
    echo "[console] expected=${expected_file}"
    echo "[console] actual=${actual_file}"
    return 1
  fi

  echo "[console] output matched ${expected_file}"
  return 0
}

extract_vprj_results() {
  local vprj="$1"
  local out_file="$2"
  awk '
    BEGIN { in_results = 0 }
    /^[[:space:]]*RESULTS[[:space:]]*=/ { in_results = 1; next }
    {
      if (in_results) {
        print $0
      }
    }
  ' "${vprj}" \
  | sed 's/\r$//' \
  | sed '/^[[:space:]]*$/d' > "${out_file}"
}

extract_vprj_sources() {
  local vprj="$1"
  awk '
    BEGIN { in_sources = 0 }
    /^[[:space:]]*SOURCES[[:space:]]*=/ {
      in_sources = 1
      sub(/^[[:space:]]*SOURCES[[:space:]]*=[[:space:]]*/, "", $0)
      if (length($0) > 0) print $0
      next
    }
    /^[[:space:]]*(INCLUDES|DEFINES|RESULTS)[[:space:]]*=/ { in_sources = 0 }
    {
      if (in_sources && length($0) > 0) print $0
    }
  ' "${vprj}" \
  | tr ' ' '\n' | sed '/^[[:space:]]*$/d'
}

extract_vprj_includes() {
  local vprj="$1"
  awk '
    BEGIN { in_inc = 0 }
    /^[[:space:]]*INCLUDES[[:space:]]*=/ {
      in_inc = 1
      sub(/^[[:space:]]*INCLUDES[[:space:]]*=[[:space:]]*/, "", $0)
      if (length($0) > 0) print $0
      next
    }
    /^[[:space:]]*(SOURCES|DEFINES|RESULTS)[[:space:]]*=/ { in_inc = 0 }
    {
      if (in_inc && length($0) > 0) print $0
    }
  ' "${vprj}" \
  | tr ' ' '\n' | sed '/^[[:space:]]*$/d'
}

expand_vprj_path() {
  local raw="$1"
  local base_dir="$2"
  local expanded
  local legacy_validation_path
  local legacy_common_include_path

  # Expand env vars used in legacy .vprj files after ajit_env has been sourced.
  expanded="$(eval "printf '%s' \"$raw\"")"
  if [[ "${expanded}" != /* ]]; then
    expanded="${base_dir}/${expanded}"
  fi
  if [[ ! -e "${expanded}" ]]; then
    legacy_validation_path="${expanded/\/processor\/C\/validation\//\/processor\/validation\/}"
    if [[ "${legacy_validation_path}" != "${expanded}" && -e "${legacy_validation_path}" ]]; then
      expanded="${legacy_validation_path}"
    fi
  fi
  if [[ ! -e "${expanded}" ]]; then
    legacy_common_include_path="${expanded/\/processor\/C\/common\/include/\/processor\/64bit\/C_multi_core_multi_thread\/common\/include}"
    if [[ "${legacy_common_include_path}" != "${expanded}" && -e "${legacy_common_include_path}" ]]; then
      expanded="${legacy_common_include_path}"
    fi
  fi
  printf '%s\n' "${expanded}"
}

parse_vprj_include_args() {
  local vprj="$1"
  local base_dir="$2"
  local pending_flag=""
  local tok=""

  while IFS= read -r tok; do
    case "${tok}" in
      -C|-I)
        pending_flag="${tok}"
        ;;
      *)
        if [[ -z "${pending_flag}" ]]; then
          printf 'I\t%s\n' "$(expand_vprj_path "${tok}" "${base_dir}")"
        else
          printf '%s\t%s\n' "${pending_flag#-}" "$(expand_vprj_path "${tok}" "${base_dir}")"
          pending_flag=""
        fi
        ;;
    esac
  done < <(extract_vprj_includes "${vprj}")
}

extract_vprj_defines() {
  local vprj="$1"
  awk '
    BEGIN { in_def = 0 }
    /^[[:space:]]*DEFINES[[:space:]]*=/ {
      in_def = 1
      sub(/^[[:space:]]*DEFINES[[:space:]]*=[[:space:]]*/, "", $0)
      if (length($0) > 0) print $0
      next
    }
    /^[[:space:]]*(SOURCES|INCLUDES|RESULTS)[[:space:]]*=/ { in_def = 0 }
    {
      if (in_def && length($0) > 0) print $0
    }
  ' "${vprj}" \
  | tr ' ' '\n' | sed '/^[[:space:]]*$/d'
}

patch_validation_save_to_nop() {
  local mmap="$1"
  [[ -f "${mmap}" ]] || return 0
  sed -i \
    -e 's/f0004000[[:space:]]\+9d/f0004000\t01/' \
    -e 's/f0004001[[:space:]]\+e3/f0004001\t00/' \
    -e 's/f0004002[[:space:]]\+bf/f0004002\t00/' \
    -e 's/f0004003[[:space:]]\+a0/f0004003\t00/' \
    "${mmap}"
}

declare -A REG_PATHS=()
declare -A REG_EXPECTS=()
declare -A REG_NUM_CORES=()
declare -A REG_THREADS_PER_CORE=()
declare -A REG_ACTIVE_THREADS=()
declare -A REG_ACTIVE_MASK=()
declare -A REG_RUN_CYCLES=()
declare -A REG_STDIN_FILES=()
declare -A REG_STDOUT_EXPECTED=()
declare -A REG_INIT_PC=()
declare -A REG_TIMER_TICK_DIV=()
declare -A REG_MEMORY_DELAY=()
declare -A REG_STOP_ON_TA0=()
while IFS='|' read -r id relpath expect_name num_cores threads_per_core active_threads active_mask run_cycles stdin_file stdout_expected_file init_pc timer_tick_div memory_delay stop_on_ta0; do
  id="$(echo "${id}" | tr -d '[:space:]')"
  relpath="$(echo "${relpath}" | tr -d '[:space:]')"
  expect_name="$(echo "${expect_name}" | tr -d '[:space:]')"
  num_cores="$(echo "${num_cores}" | tr -d '[:space:]')"
  threads_per_core="$(echo "${threads_per_core}" | tr -d '[:space:]')"
  active_threads="$(echo "${active_threads}" | tr -d '[:space:]')"
  active_mask="$(echo "${active_mask}" | tr -d '[:space:]')"
  run_cycles="$(echo "${run_cycles}" | tr -d '[:space:]')"
  stdin_file="$(echo "${stdin_file}" | tr -d '[:space:]')"
  stdout_expected_file="$(echo "${stdout_expected_file}" | tr -d '[:space:]')"
  init_pc="$(echo "${init_pc}" | tr -d '[:space:]')"
  timer_tick_div="$(echo "${timer_tick_div}" | tr -d '[:space:]')"
  memory_delay="$(echo "${memory_delay}" | tr -d '[:space:]')"
  stop_on_ta0="$(echo "${stop_on_ta0}" | tr -d '[:space:]')"
  [[ -z "${id}" ]] && continue
  [[ "${id}" =~ ^# ]] && continue
  [[ -z "${relpath}" ]] && continue
  REG_PATHS["${id}"]="${relpath}"
  REG_EXPECTS["${id}"]="${expect_name}"
  REG_NUM_CORES["${id}"]="${num_cores}"
  REG_THREADS_PER_CORE["${id}"]="${threads_per_core}"
  REG_ACTIVE_THREADS["${id}"]="${active_threads}"
  REG_ACTIVE_MASK["${id}"]="${active_mask}"
  REG_RUN_CYCLES["${id}"]="${run_cycles}"
  REG_STDIN_FILES["${id}"]="${stdin_file}"
  REG_STDOUT_EXPECTED["${id}"]="${stdout_expected_file}"
  REG_INIT_PC["${id}"]="${init_pc}"
  REG_TIMER_TICK_DIV["${id}"]="${timer_tick_div}"
  REG_MEMORY_DELAY["${id}"]="${memory_delay}"
  REG_STOP_ON_TA0["${id}"]="${stop_on_ta0}"
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
aborted_on_build_fail=0

for id in "${SELECTED_IDS[@]}"; do
  rel="${REG_PATHS[${id}]}"
  tc="${REPO_ROOT}/${rel}"
  is_vprj=0
  if [[ "${rel}" == *.vprj ]]; then
    is_vprj=1
  fi
  name="$(echo "${id}" | tr '/' '_')"
  log="${REG_DIR}/${name}.log"

  cfg_num_cores="${REG_NUM_CORES[${id}]}"
  cfg_threads_per_core="${REG_THREADS_PER_CORE[${id}]}"
  cfg_active_threads="${REG_ACTIVE_THREADS[${id}]}"
  cfg_active_mask="${REG_ACTIVE_MASK[${id}]}"
  cfg_run_cycles="${REG_RUN_CYCLES[${id}]}"
  cfg_init_pc="${REG_INIT_PC[${id}]}"
  cfg_timer_tick_div="${REG_TIMER_TICK_DIV[${id}]}"
  cfg_memory_delay="${REG_MEMORY_DELAY[${id}]}"
  cfg_stop_on_ta0="${REG_STOP_ON_TA0[${id}]}"

  if [[ -z "${cfg_num_cores}" ]]; then
    cfg_num_cores="${AJIT_NUM_CORES:-4}"
  fi
  if [[ -z "${cfg_threads_per_core}" ]]; then
    cfg_threads_per_core="${AJIT_THREADS_PER_CORE:-2}"
  fi
  if [[ -z "${cfg_active_threads}" && "${is_vprj}" == "1" ]]; then
    # Validation .vprj tests expect single-thread architectural end-state unless
    # explicitly annotated otherwise in the registry.
    cfg_active_threads=1
  fi
  if [[ -z "${cfg_run_cycles}" ]]; then
    cfg_run_cycles="${RUN_CYCLES}"
  fi

  if ! [[ "${cfg_num_cores}" =~ ^[0-9]+$ ]]; then
    echo "FAIL ${id} (rc=16) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid num_cores='${cfg_num_cores}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if ! [[ "${cfg_threads_per_core}" =~ ^[0-9]+$ ]]; then
    echo "FAIL ${id} (rc=17) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid threads_per_core='${cfg_threads_per_core}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if ! [[ "${cfg_run_cycles}" =~ ^[0-9]+$ ]] || [[ "${cfg_run_cycles}" == "0" ]]; then
    echo "FAIL ${id} (rc=18) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid run_cycles='${cfg_run_cycles}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if [[ -n "${cfg_init_pc}" ]] && ! [[ "${cfg_init_pc}" =~ ^(0x[0-9a-fA-F]+|[0-9]+)$ ]]; then
    echo "FAIL ${id} (rc=19) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid init_pc='${cfg_init_pc}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if [[ -n "${cfg_timer_tick_div}" ]] && ! [[ "${cfg_timer_tick_div}" =~ ^[0-9]+$ ]] ; then
    echo "FAIL ${id} (rc=20) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid timer_tick_div='${cfg_timer_tick_div}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if [[ -n "${cfg_memory_delay}" ]] && ! [[ "${cfg_memory_delay}" =~ ^[0-9]+$ ]] ; then
    echo "FAIL ${id} (rc=21) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid memory_delay='${cfg_memory_delay}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi
  if [[ -z "${cfg_stop_on_ta0}" ]]; then
    cfg_stop_on_ta0="off"
  fi
  if [[ "${cfg_stop_on_ta0}" != "on" && "${cfg_stop_on_ta0}" != "off" ]]; then
    echo "FAIL ${id} (rc=22) log=${log}" | tee -a "${SUMMARY}"
    echo "ABORT: invalid stop_on_ta0='${cfg_stop_on_ta0}' for ${id}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    aborted_on_build_fail=1
    break
  fi

  build_key="${cfg_num_cores}:${cfg_threads_per_core}"
  if [[ "${build_key}" != "${last_build_key}" ]]; then
    cfg_skip_build=0
    last_build_key="${build_key}"
  else
    cfg_skip_build=1
  fi

  echo "===== ${id} -> ${rel} =====" | tee -a "${SUMMARY}"
  set +e
  (
    set -e
    echo "[case-id] ${id}"
    echo "[case-path] ${rel}"
    expect_name="${REG_EXPECTS[${id}]}"
    console_input_file=""
    console_expected_output=""
    console_actual_output="${REG_DIR}/${name}.console.out"
    console_input_paced=0
    rm -f "${console_actual_output}"
    if [[ -n "${REG_STDIN_FILES[${id}]}" ]]; then
      console_input_file="$(resolve_optional_registry_path "${REG_STDIN_FILES[${id}]}")"
    fi
    if [[ -n "${REG_STDOUT_EXPECTED[${id}]}" ]]; then
      console_expected_output="$(resolve_optional_registry_path "${REG_STDOUT_EXPECTED[${id}]}")"
    fi

    if [[ "${is_vprj}" == "1" ]]; then
      if [[ ! -f "${tc}" ]]; then
        echo "[error] vprj file missing: ${tc}"
        exit 12
      fi
      vprj_file="${tc}"
      vprj_dir="$(dirname "${vprj_file}")"
      vprj_base="$(basename "${vprj_file}" .vprj)"
      vprj_compiled_dir="${vprj_dir}/compiled/${vprj_base}"
      mkdir -p "${vprj_compiled_dir}"

      expect_file="${vprj_compiled_dir}/${vprj_base}.results"
      extract_vprj_results "${vprj_file}" "${expect_file}"
      if [[ ! -s "${expect_file}" ]]; then
        echo "[error] RESULTS section missing/empty in ${vprj_file}"
        exit 15
      fi
      if [[ -z "${console_input_file}" ]]; then
        console_input_file="$(resolve_console_sidecar "${vprj_dir}" "${vprj_base}" stdin || true)"
      fi
      if [[ -z "${console_expected_output}" ]]; then
        console_expected_output="$(resolve_console_sidecar "${vprj_dir}" "${vprj_base}" stdout_expected || true)"
      fi
      if [[ -n "${console_input_file}" && -n "${console_expected_output}" ]]; then
        if grep -qx '\?' "${console_expected_output}" 2>/dev/null; then
          console_input_paced=prompt
        else
          console_input_paced=1
        fi
      fi

      mapfile -t src_list < <(extract_vprj_sources "${vprj_file}")
      mapfile -t inc_list < <(parse_vprj_include_args "${vprj_file}" "${vprj_dir}")
      mapfile -t def_list < <(extract_vprj_defines "${vprj_file}")
      if [[ ${#src_list[@]} -eq 0 ]]; then
        echo "[error] SOURCES section missing/empty in ${vprj_file}"
        exit 14
      fi

      compile_cmd=(
        compileToSparc.ForValidation.py
        -E "${vprj_base}.elf"
        -V "${vprj_base}.vars"
        -H "${vprj_base}.hex"
        -M "${vprj_base}.mmap"
        -O "${vprj_base}.objdump"
        -L "${REPO_ROOT}/AjitPublicResources/tools/linker/validationLinkerScript.lnk"
      )
      for src in "${src_list[@]}"; do
        case "${src}" in
          *.c) compile_cmd+=(-c "${vprj_dir}/${src}") ;;
          *.s) compile_cmd+=(-s "${vprj_dir}/${src}") ;;
        esac
      done
      for inc in "${inc_list[@]}"; do
        kind="${inc%%$'\t'*}"
        path="${inc#*$'\t'}"
        case "${kind}" in
          C) compile_cmd+=(-C "${path}") ;;
          I) compile_cmd+=(-I "${path}") ;;
          *)
            echo "[error] unsupported include token in ${vprj_file}: ${inc}"
            exit 13
            ;;
        esac
      done
      for def in "${def_list[@]}"; do
        compile_cmd+=(-D "${def}")
      done
      echo "[step] compileToSparc.ForValidation.py (${vprj_base})"
      run_in_dir "${vprj_compiled_dir}" "${compile_cmd[@]}"

      mmap="${vprj_compiled_dir}/${vprj_base}.mmap"
      if [[ ! -f "${mmap}" ]]; then
        echo "[error] expected mmap not generated: ${mmap}"
        exit 11
      fi
      patch_validation_save_to_nop "${mmap}"

    else
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

      if [[ -z "${expect_name}" ]]; then
        expect_name="${EXPECT_NAME_DEFAULT}"
      fi
      expect_file="${tc}/${expect_name}"
      if [[ ! -f "${expect_file}" && -f "${tc}/cortos_build/${expect_name}" ]]; then
        expect_file="${tc}/cortos_build/${expect_name}"
        echo "[warn] missing ${tc}/${expect_name}; using ${expect_file}"
      fi
      if [[ ! -f "${expect_file}" && "${EXPECT_AUTO_PICK}" == "1" ]]; then
        mapfile -t alt_results < <(find "${tc}" -maxdepth 1 -type f -name "*.results" | sort)
        if [[ ${#alt_results[@]} -eq 1 ]]; then
          expect_file="${alt_results[0]}"
          echo "[warn] missing ${tc}/${expect_name}; auto-picked ${expect_file}"
        fi
      fi
      if [[ ! -f "${expect_file}" && "${EXPECT_AUTO_PICK}" == "1" ]]; then
        mapfile -t alt_results < <(find "${tc}/cortos_build" -maxdepth 1 -type f -name "*.results" 2>/dev/null | sort)
        if [[ ${#alt_results[@]} -eq 1 ]]; then
          expect_file="${alt_results[0]}"
          echo "[warn] missing ${tc}/${expect_name}; auto-picked ${expect_file} from cortos_build"
        fi
      fi
      tc_base="$(basename "${tc}")"
      if [[ -z "${console_input_file}" ]]; then
        console_input_file="$(resolve_console_sidecar "${tc}" "${tc_base}" stdin || true)"
      fi
      if [[ -z "${console_expected_output}" ]]; then
        console_expected_output="$(resolve_console_sidecar "${tc}" "${tc_base}" stdout_expected || true)"
      fi
      if [[ ! -f "${expect_file}" ]]; then
        if [[ -n "${console_expected_output}" ]]; then
          echo "[warn] expected results file missing: ${expect_file}; using console-output-only verification"
          expect_file=""
        else
          echo "[error] expected results file missing: ${expect_file}"
          exit 15
        fi
      fi

      if [[ -n "${console_input_file}" && -n "${console_expected_output}" ]]; then
        if grep -qx '\?' "${console_expected_output}" 2>/dev/null; then
          console_input_paced=prompt
        else
          console_input_paced=1
        fi
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
    fi

    echo "[cfg] AJIT_NUM_CORES=${cfg_num_cores} AJIT_THREADS_PER_CORE=${cfg_threads_per_core} AJIT_ACTIVE_THREADS=${cfg_active_threads:-auto} AJIT_ACTIVE_THREAD_MASK=${cfg_active_mask:-<none>}"
    echo "[cfg] RUN_CYCLES=${cfg_run_cycles}"
    if [[ -n "${cfg_init_pc}" ]]; then
      echo "[cfg] AJIT_INIT_PC=${cfg_init_pc}"
    fi
    if [[ -n "${cfg_timer_tick_div}" ]]; then
      echo "[cfg] AJIT_TIMER_TICK_DIV=${cfg_timer_tick_div}"
    fi
    if [[ -n "${cfg_memory_delay}" ]]; then
      echo "[cfg] AJIT_MEMORY_DELAY=${cfg_memory_delay}"
    fi
    echo "[cfg] AJIT_STOP_ON_TA0=${cfg_stop_on_ta0}"
    cfg_skip_check=0
    if [[ -z "${expect_file}" ]]; then
      cfg_skip_check=1
    fi
    echo "[cfg] AJIT_SKIP_BUILD=${cfg_skip_build} AJIT_SKIP_CHECK=${cfg_skip_check} (topology-key=${build_key})"

    if [[ "${RUN_LEGACY}" == "1" && -x "${tc}/run_cmodel.sh" ]]; then
      echo "[step] run_cmodel.sh"
      run_in_dir "${tc}" ./run_cmodel.sh
    fi

    if [[ "${is_vprj}" != "1" ]]; then
      mmap="${tc}/main.mmap.remapped"
      if [[ ! -f "${mmap}" ]]; then
        mmap="${tc}/main.mmap"
      fi
      if [[ ! -f "${mmap}" && -f "${tc}/cortos_build/main.mmap.remapped" ]]; then
        mmap="${tc}/cortos_build/main.mmap.remapped"
      fi
      if [[ ! -f "${mmap}" && -f "${tc}/cortos_build/main.mmap" ]]; then
        mmap="${tc}/cortos_build/main.mmap"
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
        mapfile -t alt_mmaps < <(find "${tc}/cortos_build" -maxdepth 1 -type f -name "*.mmap.remapped" 2>/dev/null | sort)
        if [[ ${#alt_mmaps[@]} -eq 1 ]]; then
          mmap="${alt_mmaps[0]}"
          echo "[warn] missing ${tc}/main.mmap.remapped; auto-picked ${mmap} from cortos_build"
        fi
      fi
      if [[ ! -f "${mmap}" ]]; then
        mapfile -t alt_mmaps < <(find "${tc}/cortos_build" -maxdepth 1 -type f -name "*.mmap" 2>/dev/null | sort)
        if [[ ${#alt_mmaps[@]} -eq 1 ]]; then
          mmap="${alt_mmaps[0]}"
          echo "[warn] missing ${tc}/main.mmap; auto-picked ${mmap} from cortos_build"
        fi
      fi
      if [[ ! -f "${mmap}" ]]; then
        echo "[error] no mmap found (expected main.mmap.remapped/main.mmap or single *.mmap[.remapped])"
        exit 11
      fi
    fi

    echo "[step] sitar flow"
    pushd "${SCRIPT_DIR}" > /dev/null
    AJIT_TEST_MEMMAP="${mmap}" \
    AJIT_EXPECT_RESULTS_FILE="${expect_file}" \
    AJIT_CONSOLE_INPUT_FILE="${console_input_file}" \
    AJIT_CONSOLE_OUTPUT_FILE="${console_actual_output}" \
    AJIT_CONSOLE_INPUT_PACED="${console_input_paced}" \
    AJIT_RDASR_USES_SITAR_TIME=1 \
    AJIT_NUM_CORES="${cfg_num_cores}" \
    AJIT_THREADS_PER_CORE="${cfg_threads_per_core}" \
    AJIT_INIT_PC="${cfg_init_pc}" \
    AJIT_TIMER_TICK_DIV="${cfg_timer_tick_div}" \
    AJIT_MEMORY_DELAY="${cfg_memory_delay}" \
    AJIT_STOP_ON_TA0="${cfg_stop_on_ta0}" \
    AJIT_ACTIVE_THREADS="${cfg_active_threads}" \
    AJIT_ACTIVE_THREAD_MASK="${cfg_active_mask}" \
    AJIT_SKIP_BUILD="${cfg_skip_build}" \
    AJIT_SKIP_CHECK="${cfg_skip_check}" \
    "${FLOW_SCRIPT}" "${cfg_run_cycles}"
    popd > /dev/null

    if [[ -n "${console_expected_output}" ]]; then
      compare_console_output "${console_expected_output}" "${console_actual_output}"
    fi

    if [[ "${RUN_CLEAN}" == "1" && -x "${tc}/clean.sh" ]]; then
      echo "[step] clean.sh"
      run_in_dir "${tc}" ./clean.sh
    fi
  ) > "${log}" 2>&1
  rc=$?
  set -e

  if [[ ${rc} -eq 0 ]]; then
    echo "PASS ${id}" | tee -a "${SUMMARY}"
    pass=$((pass + 1))
  else
    echo "FAIL ${id} (rc=${rc}) log=${log}" | tee -a "${SUMMARY}"
    fail=$((fail + 1))
    if ! grep -q "^\[flow 2/3\] run$" "${log}" 2>/dev/null; then
      echo "ABORT: build/compile/setup failure detected before runtime for ${id}; stopping regression." | tee -a "${SUMMARY}"
      aborted_on_build_fail=1
      break
    fi
  fi
done

echo | tee -a "${SUMMARY}"
echo "TOTAL: ${#SELECTED_IDS[@]} PASS: ${pass} FAIL: ${fail}" | tee -a "${SUMMARY}"
if [[ ${aborted_on_build_fail} -eq 1 ]]; then
  echo "ABORTED: yes (stopped on build/compile/setup failure)" | tee -a "${SUMMARY}"
else
  echo "ABORTED: no" | tee -a "${SUMMARY}"
fi
echo "Summary: ${SUMMARY}"

if [[ ${fail} -ne 0 ]]; then
  exit 1
fi
