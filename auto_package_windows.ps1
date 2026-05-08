param(
  [string]$AppsDir = "",
  [string]$MsysRoot = "C:\msys64"
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$bashCandidates = @(
  (Join-Path $MsysRoot "usr\bin\bash.exe"),
  "C:\msys64\usr\bin\bash.exe",
  "D:\msys64\usr\bin\bash.exe"
) | Select-Object -Unique

$bash = $null
foreach ($candidate in $bashCandidates) {
  if (Test-Path $candidate) {
    $bash = $candidate
    break
  }
}

if (-not $bash) {
  throw "MSYS2 bash.exe not found. Pass -MsysRoot, for example: .\auto_package_windows.ps1 -MsysRoot C:\msys64"
}

if (-not $AppsDir) {
  $AppsDir = Join-Path $projectRoot "dist\APPS"
}

function Convert-ToMsysPath([string]$Path) {
  $lines = & $bash -lc "cygpath -u '$Path'"
  $pathLine = $lines | Where-Object { $_ -match '^/[A-Za-z]/' -or $_ -match '^/[^ ]+$' } | Select-Object -Last 1
  if (-not $pathLine) {
    throw "Could not convert path for MSYS2: $Path"
  }
  return $pathLine.Trim()
}

$projectUnix = Convert-ToMsysPath $projectRoot
$appsUnix = Convert-ToMsysPath $AppsDir

Write-Host "[gb_internovel] MSYS2 bash: $bash"
Write-Host "[gb_internovel] project: $projectRoot"
Write-Host "[gb_internovel] output: $AppsDir"

$profilePath = "/ucrt64/bin"
$makeCmd = "make"
$probe = @"
if [ -x /ucrt64/bin/g++ ] && { command -v make >/dev/null 2>&1 || command -v mingw32-make >/dev/null 2>&1; }; then
  echo ucrt64
elif [ -x /mingw64/bin/g++ ]; then
  echo mingw64
else
  echo missing
fi
"@
$profile = (& $bash -lc $probe).Trim()
if ($profile -eq "mingw64") {
  $profilePath = "/mingw64/bin"
  $makeCmd = "mingw32-make"
} elseif ($profile -eq "ucrt64") {
  $makeCheck = (& $bash -lc "export PATH=/ucrt64/bin:/usr/bin:`$PATH; if command -v make >/dev/null 2>&1; then echo make; elif command -v mingw32-make >/dev/null 2>&1; then echo mingw32-make; fi").Trim()
  if ($makeCheck) { $makeCmd = $makeCheck }
} else {
  throw "No MSYS2 MinGW toolchain found. Install mingw-w64-ucrt-x86_64-toolchain or mingw-w64-x86_64-toolchain."
}

Write-Host "[gb_internovel] MSYS2 profile path: $profilePath"
Write-Host "[gb_internovel] make command: $makeCmd"

$command = @"
set -eu
export PATH=${profilePath}:/usr/bin:`$PATH
export HOME='$projectUnix/build/msys-home'
export TMPDIR='$projectUnix/build/tmp'
mkdir -p "`$HOME" "`$TMPDIR"
cd '$projectUnix'
APPS_DIR='$appsUnix' MAKE_CMD='$makeCmd' sh ./auto_package.sh
"@

& $bash -lc $command
if ($LASTEXITCODE -ne 0) {
  throw "auto package failed with exit code $LASTEXITCODE"
}

Write-Host "[gb_internovel] packaged successfully"
