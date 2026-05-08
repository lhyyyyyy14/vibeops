#!/bin/sh
set -eu

SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SELF_DIR"

LOG_DIR="$SELF_DIR/build/logs"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/build_run_$(date +%Y%m%d_%H%M%S).log"
MAKE_CMD="${MAKE_CMD:-make}"

echo "[gb_internovel] log: $LOG_FILE"

{
  echo "===== $(date '+%F %T') ====="
  echo "[gb_internovel] make print-config"
  "$MAKE_CMD" print-config
  echo "[gb_internovel] make clean"
  "$MAKE_CMD" clean
  echo "[gb_internovel] make"
  "$MAKE_CMD"
  echo "[gb_internovel] run"
  GBINTERNOVEL_WINDOWED="${GBINTERNOVEL_WINDOWED:-1}" ./build/gb_internovel_sdl
} >>"$LOG_FILE" 2>&1
