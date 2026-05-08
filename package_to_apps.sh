#!/bin/sh
set -eu

SELF_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SELF_DIR"

APPS_DIR="${GBINTERNOVEL_APPS_DIR:-${APPS_DIR:-/Roms/APPS}}"
OUT_DIR="$APPS_DIR/GBInternovel"
LAUNCHER="$APPS_DIR/GBInternovel.sh"
BIN_NAME="gb_internovel_sdl"
BIN_PATH="$SELF_DIR/build/$BIN_NAME"
MAKE_CMD="${MAKE_CMD:-make}"

if [ ! -x "$BIN_PATH" ] && [ -x "$BIN_PATH.exe" ]; then
  BIN_PATH="$BIN_PATH.exe"
fi

if [ ! -x "$BIN_PATH" ]; then
  "$MAKE_CMD"
fi

if [ ! -x "$BIN_PATH" ] && [ -x "$BIN_PATH.exe" ]; then
  BIN_PATH="$BIN_PATH.exe"
fi

if [ ! -x "$BIN_PATH" ]; then
  echo "[gb_internovel] missing binary: $SELF_DIR/build/$BIN_NAME" >&2
  exit 1
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"
cp "$BIN_PATH" "$OUT_DIR/$BIN_NAME"

case "$(uname -s 2>/dev/null || echo unknown)" in
MINGW*|MSYS*|CYGWIN*)
  if command -v ldd >/dev/null 2>&1; then
    ldd "$BIN_PATH" | awk '/=> \/(mingw64|ucrt64|clang64)\// {print $3}' | while read -r dll_path; do
      [ -f "$dll_path" ] && cp "$dll_path" "$OUT_DIR/"
    done
  fi
  ;;
esac

if [ -d "$SELF_DIR/assets" ]; then
  cp -a "$SELF_DIR/assets" "$OUT_DIR/"
fi
cp "$SELF_DIR/native_config.ini" "$OUT_DIR/"
cp "$SELF_DIR/native_keymap.ini" "$OUT_DIR/"
if [ -f "$SELF_DIR/experiment_config.json" ]; then
  cp "$SELF_DIR/experiment_config.json" "$OUT_DIR/"
fi
for curl_path in /mingw64/bin/curl.exe /ucrt64/bin/curl.exe /usr/bin/curl.exe /c/Windows/System32/curl.exe; do
  if [ -f "$curl_path" ]; then
    cp "$curl_path" "$OUT_DIR/curl.exe"
    if command -v ldd >/dev/null 2>&1; then
      ldd "$curl_path" | awk '/=> \// {print $3}' | while read -r dll_path; do
        case "$dll_path" in
          /mingw64/*|/ucrt64/*|/clang64/*|/usr/bin/*)
            [ -f "$dll_path" ] && cp "$dll_path" "$OUT_DIR/"
            ;;
        esac
      done
    fi
    for ca_path in /etc/ssl/certs/ca-bundle.crt /usr/ssl/certs/ca-bundle.crt /mingw64/ssl/certs/ca-bundle.crt /ucrt64/ssl/certs/ca-bundle.crt; do
      if [ -f "$ca_path" ]; then
        cp "$ca_path" "$OUT_DIR/ca-bundle.crt"
        break
      fi
    done
    break
  fi
done

cat >"$LAUNCHER" <<'EOF'
#!/bin/sh
APP_DIR="$(cd "$(dirname "$0")/GBInternovel" && pwd)"
cd "$APP_DIR"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export SDL_NOMOUSE="${SDL_NOMOUSE:-1}"
exec "$APP_DIR/gb_internovel_sdl" "$@"
EOF

chmod +x "$OUT_DIR/$BIN_NAME"
chmod +x "$LAUNCHER"

echo "[gb_internovel] packaged:"
echo "  $LAUNCHER"
echo "  $OUT_DIR/"
