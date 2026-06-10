#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FP_FILE="${ROOT_DIR}/AjitPublicResources/processor/64bit/C_multi_core_multi_thread/cpu/src/FpExceptions.c"

usage() {
  cat <<'USAGE'
Usage: tests/verification/fp_qnan32_patch.sh <status|apply|revert>

status: print current state (patched/original/unknown)
apply:  change isQNaN32() ranges to qNaN (0x7fc00000..0x7fffffff, 0xffc00000..0xffffffff)
revert: restore original ranges (0x7f800001..0x7fbfffff, 0xff800001..0xffbfffff)
USAGE
}

if [[ $# -ne 1 ]]; then
  usage
  exit 2
fi

cmd="$1"

python3 - "$cmd" "$FP_FILE" <<'PY'
import re
import sys
from pathlib import Path

cmd = sys.argv[1]
path = Path(sys.argv[2])

text = path.read_text()

m = re.search(r'uint8_t\s+isQNaN32\s*\(float\s+op\s*\)\s*\{(?P<body>.*?)\n\}', text, re.S)
if not m:
    print("ERROR: isQNaN32() not found", file=sys.stderr)
    sys.exit(1)

body = m.group("body")

orig_consts = ("0x7f800001", "0x7fbfffff", "0xff800001", "0xffbfffff")
patched_consts = ("0x7fc00000", "0x7fffffff", "0xffc00000", "0xffffffff")

def has_all(body_text, consts):
    return all(c.lower() in body_text.lower() for c in consts)

is_orig = has_all(body, orig_consts)
is_patched = has_all(body, patched_consts)

if cmd == "status":
    if is_patched and not is_orig:
        print("patched")
        sys.exit(0)
    if is_orig and not is_patched:
        print("original")
        sys.exit(0)
    if is_patched and is_orig:
        print("mixed")
        sys.exit(0)
    print("unknown")
    sys.exit(0)

def replace_consts(body_text, src_consts, dst_consts):
    out = body_text
    for src, dst in zip(src_consts, dst_consts):
        out = re.sub(r'\b' + re.escape(src) + r'\b', dst, out, flags=re.I)
    return out

if cmd == "apply":
    if is_patched and not is_orig:
        print("already patched")
        sys.exit(0)
    new_body = replace_consts(body, orig_consts, patched_consts)
elif cmd == "revert":
    if is_orig and not is_patched:
        print("already original")
        sys.exit(0)
    new_body = replace_consts(body, patched_consts, orig_consts)
else:
    print("ERROR: unknown command", file=sys.stderr)
    sys.exit(2)

if new_body == body:
    print("ERROR: no changes made (unexpected format?)", file=sys.stderr)
    sys.exit(1)

new_text = text[:m.start("body")] + new_body + text[m.end("body"):]
path.write_text(new_text)
print("updated")
PY
