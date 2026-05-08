param(
  [string]$MsysRoot = "C:\msys64"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $projectRoot "dist\APPS\GBInternovel\gb_internovel_sdl.exe"

if (-not (Test-Path $exe)) {
  powershell -ExecutionPolicy Bypass -File (Join-Path $projectRoot "auto_package_windows.ps1") -MsysRoot $MsysRoot
}

if (-not (Test-Path $exe)) {
  throw "Preview executable not found: $exe"
}

$env:GBINTERNOVEL_WINDOWED = "1"
Start-Process -FilePath $exe -WorkingDirectory (Split-Path -Parent $exe)
