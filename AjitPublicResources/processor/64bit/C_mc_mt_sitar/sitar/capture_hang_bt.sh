#!/usr/bin/env bash
set -u

CYCLES="${1:-2000}"
OUT_PREFIX="${2:-hang_capture}"
WAIT_SECS="${3:-5}"
KILL_AFTER_CAPTURE="${KILL_AFTER_CAPTURE:-0}"

RUN_OUT="${OUT_PREFIX}.run.out"
RUN_ERR="${OUT_PREFIX}.run.err"
GDB_BT="${OUT_PREFIX}.gdb_bt.txt"
PID_FILE="${OUT_PREFIX}.pid"
EXIT_FILE="${OUT_PREFIX}.exit.txt"

./sitar_sim "$CYCLES" >"$RUN_OUT" 2>"$RUN_ERR" &
SIMPID=$!
echo "$SIMPID" > "$PID_FILE"

echo "Started sitar_sim pid=$SIMPID cycles=$CYCLES"
echo "Waiting ${WAIT_SECS}s before attach..."
sleep "$WAIT_SECS"

if kill -0 "$SIMPID" 2>/dev/null; then
  gdb -q -batch ./sitar_sim \
    -ex "set pagination off" \
    -ex "attach $SIMPID" \
    -ex "thread apply all bt" \
    -ex "detach" \
    -ex "quit" \
    > "$GDB_BT" 2>&1 || true
  echo "Captured backtrace in: $GDB_BT"
else
  wait "$SIMPID" 2>/dev/null
  EXIT_CODE=$?
  {
    echo "Process pid=$SIMPID exited before gdb attach."
    echo "Exit code: $EXIT_CODE"
    echo "No backtrace captured."
  } > "$GDB_BT"
  echo "$EXIT_CODE" > "$EXIT_FILE"
fi

if kill -0 "$SIMPID" 2>/dev/null; then
  if [[ "$KILL_AFTER_CAPTURE" == "1" ]]; then
    kill "$SIMPID" 2>/dev/null || true
    echo "Stopped pid=$SIMPID after capture."
  else
    echo "Process still running with pid=$SIMPID (left running)."
  fi
else
  echo "Process pid=$SIMPID is no longer running."
fi

echo "Run stderr in: $RUN_ERR"
echo "Run stdout in: $RUN_OUT"
if [[ -f "$EXIT_FILE" ]]; then
  echo "Exit code file: $EXIT_FILE"
fi
