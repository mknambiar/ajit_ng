#!/usr/bin/env bash
set -euo pipefail

CYCLES="${1:-2000}"
OUT_PREFIX="${2:-crash_capture}"

GDB_BT="${OUT_PREFIX}.gdb_bt.txt"

# Direct gdb run avoids attach race for fast SIGSEGV.
gdb -q -batch ./sitar_sim \
  -ex "set pagination off" \
  -ex "run $CYCLES" \
  -ex "bt" \
  -ex "frame 0" \
  -ex "info locals" \
  -ex "quit" \
  > "$GDB_BT" 2>&1 || true

echo "Captured direct crash backtrace in: $GDB_BT"
