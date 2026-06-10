#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CYCLES="${1:-8000}"
MIN_HITS="${AJIT_OP3_MIN_HITS:-1}"
STRICT_MISSING_OP3="${AJIT_OP3_STRICT_MISSING:-0}"
PROBE_PROFILE="${AJIT_TEST_PROFILE:-mem_bw_acq}"
PROBE_MEMMAP_OVERRIDE="${AJIT_TEST_MEMMAP:-}"
OP3_ONLY_RAW="${AJIT_OP3_ONLY:-}"
REPO_ROOT="$(cd "${ROOT_DIR}/../../../.." && pwd)"

cd "${ROOT_DIR}"
LOG_DIR="${ROOT_DIR}/sitar/logs"
mkdir -p "${LOG_DIR}"
STAMP="$(date +%Y%m%d_%H%M%S)"
RESULT_LOG="${LOG_DIR}/mem_op3_probe_${STAMP}.log"

TARGET_OP3=(
  "00 ld/lda-family"
  "01 ldub-family"
  "03 lddf/ldd-family"
  "04 st/sta-family"
  "05 stb-family"
  "07 std-family"
  "09 ldsb-family"
  "11 lduba-family"
  "15 stba-family"
  "23 ldx-family"
  "24 stx-family"
  "0d ldstub-family"
  "1d ldstuba-family"
  "0f swap-family"
  "1f swapa-family"
  "2f casa-family"
  "3f casxa-family"
  "17 stda-family"
  "30 ldc-family"
)

find_pc_for_op3() {
  local mmap_file="$1"
  local op3_hex="$2"
  python3 - "$mmap_file" "$op3_hex" <<'PY'
import re, sys
mmap, target_hex = sys.argv[1], sys.argv[2]
target = int(target_hex, 16)
hex_re = re.compile(r'^(?:0x)?[0-9a-fA-F]+$')
mem = {}

def first_op3_addr_from_bytes(mem_dict, want):
    if not mem_dict:
        return None
    addrs = sorted(mem_dict.keys())
    amin = addrs[0]
    amax = addrs[-1]
    # Build 32-bit instruction as big-endian bytes, which matches existing mmap usage.
    for a in range(amin & ~0x3, amax + 1, 4):
        b0 = mem_dict.get(a, None)
        b1 = mem_dict.get(a + 1, None)
        b2 = mem_dict.get(a + 2, None)
        b3 = mem_dict.get(a + 3, None)
        if None in (b0, b1, b2, b3):
            continue
        inst = ((b0 & 0xFF) << 24) | ((b1 & 0xFF) << 16) | ((b2 & 0xFF) << 8) | (b3 & 0xFF)
        op = (inst >> 30) & 0x3
        op3 = (inst >> 19) & 0x3F
        if op == 3 and op3 == want:
            return a
    return None

try:
    with open(mmap, "r", encoding="utf-8", errors="ignore") as f:
        # Accept both legacy word-per-line and byte-per-line memmaps.
        for line in f:
            s = line.strip()
            if not s or s.startswith(("#", "//")):
                continue
            parts = s.split()
            if len(parts) < 2:
                continue
            a = parts[0].rstrip(":,;")
            d = parts[1].rstrip(":,;")
            if not (hex_re.match(a) and hex_re.match(d)):
                continue
            addr = int(a, 16)
            data = int(d, 16)
            if data <= 0xFF:
                mem[addr] = data
            else:
                inst = data & 0xFFFFFFFF
                op = (inst >> 30) & 0x3
                op3 = (inst >> 19) & 0x3F
                if op == 3 and op3 == target:
                    print(f"0x{addr:08x}")
                    sys.exit(0)
except FileNotFoundError:
    pass
addr = first_op3_addr_from_bytes(mem, target)
if addr is not None:
    print(f"0x{addr:08x}")
    sys.exit(0)
sys.exit(1)
PY
}

echo "OP3 probe: cycles=${CYCLES} min_hits=${MIN_HITS}" | tee -a "${RESULT_LOG}"
echo "Using profile=${PROBE_PROFILE}" | tee -a "${RESULT_LOG}"
if [[ -n "${PROBE_MEMMAP_OVERRIDE}" ]]; then
  echo "Using mmap override=${PROBE_MEMMAP_OVERRIDE}" | tee -a "${RESULT_LOG}"
fi
if [[ -n "${OP3_ONLY_RAW}" ]]; then
  echo "Filtering op3 list to: ${OP3_ONLY_RAW}" | tee -a "${RESULT_LOG}"
fi

fail_count=0
skip_count=0

MMAP_FILE="${ROOT_DIR}/test_memmap_input_0.txt"
if [[ -n "${PROBE_MEMMAP_OVERRIDE}" ]]; then
  override_path="${PROBE_MEMMAP_OVERRIDE}"
  if [[ ! -f "${override_path}" ]]; then
    if [[ -f "${ROOT_DIR}/${PROBE_MEMMAP_OVERRIDE}" ]]; then
      override_path="${ROOT_DIR}/${PROBE_MEMMAP_OVERRIDE}"
    elif [[ -f "${REPO_ROOT}/${PROBE_MEMMAP_OVERRIDE}" ]]; then
      override_path="${REPO_ROOT}/${PROBE_MEMMAP_OVERRIDE}"
    fi
  fi
  if [[ ! -f "${override_path}" ]]; then
    echo "missing override mmap: ${PROBE_MEMMAP_OVERRIDE}" | tee -a "${RESULT_LOG}"
    exit 2
  fi
  cp "${override_path}" "${MMAP_FILE}"
else
  # Prime selected profile so test_memmap_input_0.txt matches this probe.
  AJIT_TEST_PROFILE="${PROBE_PROFILE}" ./run_sitar_flow_real_thread.sh 1 >/tmp/run_sitar_mem_op3_probe_prep.out 2>/tmp/run_sitar_mem_op3_probe_prep.err || true
fi

if [[ ! -f "${MMAP_FILE}" ]]; then
  echo "missing ${MMAP_FILE} after prep" | tee -a "${RESULT_LOG}"
  exit 2
fi

for entry in "${TARGET_OP3[@]}"; do
  op3_hex="$(echo "${entry}" | awk '{print $1}')"
  label="$(echo "${entry}" | awk '{print $2}')"

  if [[ -n "${OP3_ONLY_RAW}" ]]; then
    if ! python3 - "${op3_hex}" "${OP3_ONLY_RAW}" <<'PY'
import sys
op3 = int(sys.argv[1], 16)
raw = sys.argv[2]
vals = set()
for tok in raw.replace(',', ' ').split():
    t = tok.strip().lower()
    if t.startswith('0x'):
        t = t[2:]
    if t:
        vals.add(int(t, 16))
sys.exit(0 if op3 in vals else 1)
PY
    then
      continue
    fi
  fi

  init_pc="$(find_pc_for_op3 "${MMAP_FILE}" "${op3_hex}" || true)"

  if [[ -z "${init_pc}" ]]; then
    echo "---- probe op3=0x${op3_hex} (${label}) : SKIP (not found in mmap)" | tee -a "${RESULT_LOG}"
    skip_count=$((skip_count + 1))
    if [[ "${STRICT_MISSING_OP3}" -eq 1 ]]; then
      fail_count=$((fail_count + 1))
    fi
    continue
  fi

  echo "---- probe op3=0x${op3_hex} init_pc=${init_pc} (${label})" | tee -a "${RESULT_LOG}"
  AJIT_TEST_PROFILE="${PROBE_PROFILE}" AJIT_TEST_MEMMAP="${PROBE_MEMMAP_OVERRIDE}" AJIT_INIT_PC="${init_pc}" AJIT_STEP_PC_TRACE_N=0 \
    ./run_sitar_flow_real_thread.sh "${CYCLES}" >/tmp/run_sitar_mem_op3_probe.out 2>/tmp/run_sitar_mem_op3_probe.err

  run_err="${ROOT_DIR}/sitar/run.err"
  total="$(sed -n 's/.*mem-class-total=\([0-9][0-9]*\).*/\1/p' "${run_err}" | tail -n 1)"
  hit="$(sed -n "s/.*mem-op3\\[0x${op3_hex}\\]=\\([0-9][0-9]*\\).*/\\1/p" "${run_err}" | tail -n 1)"
  fetch_trap="$(sed -n 's/.*fetch-trap=\([0-9][0-9]*\).*/\1/p' "${run_err}" | tail -n 1)"
  exec_fail="$(sed -n 's/.*exec-fail=\([0-9][0-9]*\).*/\1/p' "${run_err}" | tail -n 1)"
  unhandled="$( (rg -n 'STEP-TASK-MEM-UNHANDLED' "${run_err}" || true) | wc -l | tr -d ' ' )"
  rdfail="$( (rg -n 'STEP-TASK-MEM-RD-FAIL' "${run_err}" || true) | wc -l | tr -d ' ' )"
  wrfail="$( (rg -n 'STEP-TASK-MEM-WR-FAIL' "${run_err}" || true) | wc -l | tr -d ' ' )"

  echo "result: mem-total=${total:-NA} op3-hit=${hit:-0} fetch-trap=${fetch_trap:-NA} exec-fail=${exec_fail:-NA} unhandled=${unhandled} rd-fail=${rdfail} wr-fail=${wrfail}" | tee -a "${RESULT_LOG}"
  contention_lines="$(rg -n 'BRIDGE-CONTENTION|BRIDGE-MMU-SOURCE' "${run_err}" || true)"
  if [[ -n "${contention_lines}" ]]; then
    echo "contention:" | tee -a "${RESULT_LOG}"
    echo "${contention_lines}" | tee -a "${RESULT_LOG}"
  else
    echo "contention: NA" | tee -a "${RESULT_LOG}"
  fi
  if [[ -z "${total}" || -z "${hit}" || -z "${fetch_trap}" || -z "${exec_fail}" || "${hit}" -lt "${MIN_HITS}" || "${fetch_trap}" -ne 0 || "${exec_fail}" -ne 0 || "${unhandled}" -ne 0 || "${rdfail}" -ne 0 || "${wrfail}" -ne 0 ]]; then
    echo "status: FAIL" | tee -a "${RESULT_LOG}"
    fail_count=$((fail_count + 1))
  else
    echo "status: PASS" | tee -a "${RESULT_LOG}"
  fi
done

echo "==== probe summary: fail_count=${fail_count} skip_count=${skip_count} strict_missing=${STRICT_MISSING_OP3}" | tee -a "${RESULT_LOG}"
echo "log: ${RESULT_LOG}" | tee -a "${RESULT_LOG}"
if [[ "${fail_count}" -ne 0 ]]; then
  exit 2
fi
