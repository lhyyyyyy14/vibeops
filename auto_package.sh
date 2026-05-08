#!/bin/sh
set -eu

SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SELF_DIR"

APPS_DIR="${GBINTERNOVEL_APPS_DIR:-${APPS_DIR:-$SELF_DIR/dist/APPS}}"
LOG_DIR="$SELF_DIR/build/logs"
LOG_FILE="$LOG_DIR/auto_package_$(date +%Y%m%d_%H%M%S).log"
MAKE_CMD="${MAKE_CMD:-make}"

mkdir -p "$LOG_DIR"

echo "[gb_internovel] auto package"
echo "[gb_internovel] output: $APPS_DIR"
echo "[gb_internovel] log: $LOG_FILE"

{
  echo "===== $(date '+%F %T') ====="
  echo "[gb_internovel] APPS_DIR=$APPS_DIR"
  echo "[gb_internovel] MAKE_CMD=$MAKE_CMD"
  echo "[gb_internovel] make print-config"
  "$MAKE_CMD" print-config
  echo "[gb_internovel] make clean"
  "$MAKE_CMD" clean
  echo "[gb_internovel] make"
  "$MAKE_CMD"
  echo "[gb_internovel] package"
  APPS_DIR="$APPS_DIR" sh ./package_to_apps.sh
  echo "[gb_internovel] verify"
  test -x "$APPS_DIR/GBInternovel/gb_internovel_sdl"
  test -x "$APPS_DIR/GBInternovel.sh"
  test -f "$APPS_DIR/GBInternovel/native_config.ini"
  test -f "$APPS_DIR/GBInternovel/native_keymap.ini"
  echo "[gb_internovel] OK"
} >>"$LOG_FILE" 2>&1

echo "[gb_internovel] packaged:"
echo "  $APPS_DIR/GBInternovel.sh"
echo "  $APPS_DIR/GBInternovel/"
