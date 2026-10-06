#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SITAR_DIR="${ROOT_DIR}/sitar"
RESULT_THREAD="${AJIT_RESULT_THREAD:-0}"
RUN_ERR_FILE="${AJIT_RESULTS_RUN_ERR:-${SITAR_DIR}/run.err}"
EXPECTED_RESULTS_FILE="${AJIT_EXPECT_RESULTS_FILE:-}"
ACTUAL_RESULTS_FILE="${AJIT_ACTUAL_RESULTS_FILE:-${SITAR_DIR}/main.results.sitar}"
CORE_ONLY_THREAD_DEFAULT="${AJIT_EXPECT_CORE_ONLY_THREAD:-0}"

normalize_hex() {
  local v="${1,,}"
  if [[ "${v}" =~ ^0x ]]; then
    v="${v:2}"
  elif [[ "${v}" =~ ^x[0-9a-f]+$ ]]; then
    v="${v:1}"
  fi
  while [[ "${v}" == 0* && "${v}" != "0" ]]; do
    v="${v#0}"
  done
  if [[ -z "${v}" ]]; then
    v="0"
  fi
  printf "0x%x" "$((16#${v}))"
}

hex8_no_prefix() {
  local v="${1,,}"
  v="${v#0x}"
  printf "%08x" "$((16#${v}))"
}

normalize_mask_hex() {
  local v="${1,,}"
  v="${v#0x}"
  while [[ "${v}" == 0* && "${v}" != "0" ]]; do
    v="${v#0}"
  done
  if [[ -z "${v}" ]]; then
    v="0"
  fi
  printf "0x%x" "$((16#${v} & 0xffffffff))"
}

if [[ ! -f "${RUN_ERR_FILE}" ]]; then
  echo "ERROR: ${RUN_ERR_FILE} not found. Run simulation first." >&2
  exit 1
fi

if [[ -z "${EXPECTED_RESULTS_FILE}" ]]; then
  echo "[results] no expected file specified (AJIT_EXPECT_RESULTS_FILE not set); creating actual results only"
fi

declare -A THREAD_CORE=()       # tid -> core-id
declare -A THREAD_GREG=()       # "tid:gN" -> hex
declare -A THREAD_ASR=()        # "tid:asrN" -> hex
declare -A THREAD_REG=()        # "tid:key" -> hex  (g/o/l/i/asr/f/psr/wim/tbr/y/pc/npc/fpsr/asi)
declare -A MEM_WORD=()          # "addr32hex" -> 0x...

while IFS= read -r line || [[ -n "${line}" ]]; do
  [[ "${line}" =~ BRIDGE-REGS-(TA0|END)[[:space:]]t([0-9]+)[[:space:]] ]] || continue
  tid="${BASH_REMATCH[2]}"
  if [[ "${line}" =~ [[:space:]]c([0-9]+)[[:space:]] ]]; then
    THREAD_CORE["${tid}"]="${BASH_REMATCH[1]}"
  fi
  for tok in ${line}; do
    if [[ "${tok}" == *=* ]]; then
      key="${tok%%=*}"
      raw_val="${tok#*=}"
      if [[ "${key}" =~ ^((g|o|l|i)[0-7]|asr[0-9]+|f([0-9]|[12][0-9]|3[01])|psr|wim|tbr|y|pc|npc|fpsr|asi)$ ]] \
         && [[ "${raw_val}" =~ ^0x[0-9a-fA-F]+$ ]]; then
        val="$(normalize_hex "${raw_val}")"
        THREAD_REG["${tid}:${key}"]="${val}"
        if [[ "${key}" =~ ^g[0-7]$ ]]; then
          THREAD_GREG["${tid}:${key}"]="${val}"
        elif [[ "${key}" =~ ^asr[0-9]+$ ]]; then
          THREAD_ASR["${tid}:${key}"]="${val}"
        fi
      fi
    fi
  done
done < "${RUN_ERR_FILE}"

while IFS= read -r line; do
  if [[ "${line}" =~ BRIDGE-MEM-(TA0|END)[[:space:]]+addr=(0x[0-9a-fA-F]+)[[:space:]]+data=(0x[0-9a-fA-F]+) ]]; then
    addr="$(hex8_no_prefix "${BASH_REMATCH[2]}")"
    data="$(normalize_hex "${BASH_REMATCH[3]}")"
    MEM_WORD["${addr}"]="${data}"
  fi
done < "${RUN_ERR_FILE}"

declare -A ACTUAL_GREGS=()
for reg in g0 g1 g2 g3 g4 g5 g6 g7; do
  v="${THREAD_GREG["${RESULT_THREAD}:${reg}"]:-}"
  if [[ -n "${v}" ]]; then
    ACTUAL_GREGS["${reg}"]="${v}"
  fi
done

if [[ -z "${ACTUAL_GREGS[g0]:-}" ]]; then
  echo "[results] could not find BRIDGE-REGS dump for thread ${RESULT_THREAD}; skipped verdict"
  exit 1
fi

resolve_tid_for_core_thread() {
  local core="$1"
  local ltid="$2"
  local tid

  # Prefer explicit local-thread id derived from asr29.
  for tid in "${!THREAD_CORE[@]}"; do
    [[ "${THREAD_CORE[${tid}]}" == "${core}" ]] || continue
    local asr29="${THREAD_REG["${tid}:asr29"]:-}"
    if [[ -n "${asr29}" ]]; then
      local asr29_u32=$((16#${asr29#0x}))
      local tid_from_asr=$((asr29_u32 & 0xff))
      if [[ "${tid_from_asr}" -eq "${ltid}" ]]; then
        echo "${tid}"
        return 0
      fi
    fi
  done

  # Legacy fallback (2 threads/core layout).
  echo $((core * 2 + ltid))
}

: > "${ACTUAL_RESULTS_FILE}"
for reg in g0 g1 g2 g3 g4 g5 g6 g7; do
  if [[ -n "${ACTUAL_GREGS[${reg}]:-}" ]]; then
    echo "${reg}=${ACTUAL_GREGS[${reg}]}" >> "${ACTUAL_RESULTS_FILE}"
  fi
done
for tid in "${!THREAD_CORE[@]}"; do
  for asr in asr15 asr16 asr17 asr18 asr19 asr29; do
    v="${THREAD_ASR["${tid}:${asr}"]:-}"
    if [[ -n "${v}" ]]; then
      core="${THREAD_CORE[${tid}]}"
      ltid=$((tid % 2))
      echo "${asr}=${v} core ${core} thread ${ltid}" >> "${ACTUAL_RESULTS_FILE}"
    fi
  done
done
echo "[results] wrote actual results to ${ACTUAL_RESULTS_FILE}"

if [[ -z "${EXPECTED_RESULTS_FILE}" || ! -f "${EXPECTED_RESULTS_FILE}" ]]; then
  echo "[results] expected file missing; verdict skipped"
  exit 0
fi

fail=0
checked=0
mem_count=0
declare -a MEM_ADDRS=()
declare -a MEM_EXPS=()
declare -a MEM_MASKS=()
while IFS= read -r line || [[ -n "${line}" ]]; do
  [[ -z "${line}" ]] && continue
  [[ "${line}" =~ ^[[:space:]]*# ]] && continue
  # Format A/B/C (general register/state):
  #   key=0x...
  #   key=0x... core C
  #   key=0x... core C thread T
  # where key in {g/o/l/i}[0-7], asrNN, psr, wim, tbr, y, pc, npc.
  if [[ "${line}" =~ ^[[:space:]]*([^[:space:]=]+)[[:space:]]*=[[:space:]]*([^[:space:]]+)(.*)$ ]]; then
    key="${BASH_REMATCH[1]}"
    exp_raw="${BASH_REMATCH[2]}"
    rest="${BASH_REMATCH[3]}"
    # Legacy doval semantics: "asi=..." in RESULTS is memory-access context
    # metadata (used with memory checks), not a register/state compare.
    if [[ "${key}" == "asi" ]]; then
      continue
    fi
    if ! [[ "${key}" =~ ^((g|o|l|i)[0-7]|asr[0-9]+|f([0-9]|[12][0-9]|3[01])|psr|wim|tbr|y|pc|npc|fpsr|asi)$ ]]; then
      continue
    fi
    if ! [[ "${exp_raw}" =~ ^([0-9]+|0x[0-9a-fA-F]+)$ ]]; then
      continue
    fi
    exp="$(normalize_hex "${exp_raw}")"
    mask_raw="0xffffffff"
    for tok in ${rest}; do
      if [[ "${tok}" =~ ^mask=(0x[0-9a-fA-F]+)$ ]]; then
        mask_raw="${BASH_REMATCH[1]}"
        break
      fi
      if [[ "${tok}" =~ ^0x[0-9a-fA-F]+$ ]]; then
        mask_raw="${tok}"
        break
      fi
    done
    mask="$(normalize_mask_hex "${mask_raw}")"

    core=""
    ltid=""
    if [[ "${rest}" =~ core[[:space:]]+([0-9]+) ]]; then
      core="${BASH_REMATCH[1]}"
      if [[ "${rest}" =~ thread[[:space:]]+([0-9]+) ]]; then
        ltid="${BASH_REMATCH[1]}"
      else
        ltid="${CORE_ONLY_THREAD_DEFAULT}"
      fi
      gid="$(resolve_tid_for_core_thread "${core}" "${ltid}")"
      act="${THREAD_REG["${gid}:${key}"]:-}"
      checked=$((checked + 1))
      if [[ -z "${act}" ]]; then
        echo "[results] missing ${key} for core ${core} thread ${ltid} (gid=${gid})"
        fail=1
        continue
      fi
      exp_u32=$((16#${exp#0x}))
      act_u32=$((16#${act#0x}))
      mask_u32=$((16#${mask#0x}))
      if (( (act_u32 & mask_u32) != (exp_u32 & mask_u32) )); then
        echo "[results] mismatch ${key} core ${core} thread ${ltid} mask=${mask}: expected=${exp} actual=${act}"
        fail=1
      fi
      continue
    fi

    act="${THREAD_REG["${RESULT_THREAD}:${key}"]:-}"
    checked=$((checked + 1))
    if [[ -z "${act}" ]]; then
      echo "[results] missing ${key} for thread ${RESULT_THREAD}"
      fail=1
      continue
    fi
    exp_u32=$((16#${exp#0x}))
    act_u32=$((16#${act#0x}))
    mask_u32=$((16#${mask#0x}))
    if (( (act_u32 & mask_u32) != (exp_u32 & mask_u32) )); then
      echo "[results] mismatch ${key} mask=${mask}: expected=${exp} actual=${act}"
      fail=1
    fi
    continue
  fi

  # Format D: m[0xADDR]=0xVALUE [optional_mask] [optional metadata]
  if [[ "${line}" =~ ^[[:space:]]*m\[([[:space:]]*0x[0-9a-fA-F]+[[:space:]]*)\][[:space:]]*=[[:space:]]*(0x[0-9a-fA-F]+)(.*)$ ]]; then
    addr_raw="${BASH_REMATCH[1]}"
    exp_raw="${BASH_REMATCH[2]}"
    rest="${BASH_REMATCH[3]}"
    mask_raw="0xffffffff"
    for tok in ${rest}; do
      if [[ "${tok}" =~ ^mask=(0x[0-9a-fA-F]+)$ ]]; then
        mask_raw="${BASH_REMATCH[1]}"
        break
      fi
      if [[ "${tok}" =~ ^0x[0-9a-fA-F]+$ ]]; then
        mask_raw="${tok}"
        break
      fi
    done

    MEM_ADDRS+=("$(hex8_no_prefix "${addr_raw}")")
    MEM_EXPS+=("$(normalize_hex "${exp_raw}")")
    MEM_MASKS+=("$(normalize_mask_hex "${mask_raw}")")
    mem_count=$((mem_count + 1))
    continue
  fi
done < "${EXPECTED_RESULTS_FILE}"

if [[ ${mem_count} -gt 0 ]]; then
  for idx in "${!MEM_ADDRS[@]}"; do
    addr_hex="${MEM_ADDRS[${idx}]}"
    exp_hex="${MEM_EXPS[${idx}]}"
    mask_hex="${MEM_MASKS[${idx}]}"
    act_hex="${MEM_WORD[${addr_hex}]:-}"

    checked=$((checked + 1))
    if [[ -z "${act_hex}" ]]; then
      echo "[results] missing m[0x${addr_hex}] from BRIDGE-MEM-TA0 dump in run.err"
      fail=1
      continue
    fi

    exp_u32=$((16#${exp_hex#0x}))
    act_u32=$((16#${act_hex#0x}))
    mask_u32=$((16#${mask_hex#0x}))

    echo "m[0x${addr_hex}]=${act_hex} mask=${mask_hex}" >> "${ACTUAL_RESULTS_FILE}"

    if (( (act_u32 & mask_u32) != (exp_u32 & mask_u32) )); then
      echo "[results] mismatch m[0x${addr_hex}] mask=${mask_hex}: expected=$(printf "0x%x" "${exp_u32}") actual=${act_hex}"
      fail=1
    fi
  done
fi

if [[ ${checked} -eq 0 ]]; then
  echo "[results] no comparable keys parsed from expected file: ${EXPECTED_RESULTS_FILE}"
  echo "Test Failed"
  verdict_rc=1
elif [[ ${fail} -eq 0 ]]; then
  echo "Test Successful"
  verdict_rc=0
else
  echo "Test Failed"
  verdict_rc=1
fi
echo "[results] expected=${EXPECTED_RESULTS_FILE} actual=${ACTUAL_RESULTS_FILE}"
exit "${verdict_rc}"
