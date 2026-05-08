# GB Internovel

GB Internovel is a C++17 + SDL2 handheld-style interactive fiction demo. It keeps the Python prototype in `main.py`, while the native shell under `src/` provides a 720x480 SDL app with Chinese UI, scene navigation, and a DeepSeek-backed story session.

## Build

```sh
make
```

On Windows with MSYS2 installed:

```powershell
powershell -ExecutionPolicy Bypass -File .\auto_package_windows.ps1
powershell -ExecutionPolicy Bypass -File .\run_windows_preview.ps1
```

## Config

Copy `experiment_config.example.json` to `experiment_config.json`, then set `api_key` locally or provide `DEEPSEEK_API_KEY` in your environment. The real config file is intentionally ignored by git.

## Packaging

```sh
APPS_DIR=./dist/APPS ./package_to_apps.sh
```

The package is written to `dist/APPS/GBInternovel/`.
