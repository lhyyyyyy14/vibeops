param(
  [string]$AppsDir = "",
  [string]$PreviewDir = "",
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

if (-not $PreviewDir) {
  $PreviewDir = Join-Path $projectRoot "preview\GBInternovel-dev"
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
$buildDir = Join-Path $projectRoot "build"
$msysHomeWin = Join-Path $buildDir "msys-home"
$tmpWin = Join-Path $buildDir "tmp"
New-Item -ItemType Directory -Force -Path $msysHomeWin, $tmpWin | Out-Null
$msysHomeUnix = Convert-ToMsysPath $msysHomeWin
$tmpUnix = Convert-ToMsysPath $tmpWin
$env:HOME = $msysHomeWin
$env:TMPDIR = $tmpWin
$env:TMP = $tmpWin
$env:TEMP = $tmpWin

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
export HOME='$msysHomeUnix'
export TMPDIR='$tmpUnix'
export TMP='$tmpWin'
export TEMP='$tmpWin'
mkdir -p "`$HOME" "`$TMPDIR"
cd '$projectUnix'
APPS_DIR='$appsUnix' MAKE_CMD='$makeCmd' sh ./auto_package.sh
"@

& $bash -lc $command
if ($LASTEXITCODE -ne 0) {
  throw "auto package failed with exit code $LASTEXITCODE"
}

$runtimeDir = Join-Path $AppsDir "GBInternovel"
$runtimeExe = Join-Path $runtimeDir "gb_internovel_sdl.exe"
if (-not (Test-Path $runtimeExe)) {
  throw "Packaged Windows executable not found: $runtimeExe"
}

if (Test-Path $PreviewDir) {
  Remove-Item -LiteralPath $PreviewDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $PreviewDir | Out-Null

# The preview folder is the safe-to-share Windows build. It excludes local
# history/log files and replaces experiment_config.json with the empty-key
# example config below.
$excludeNames = @("data", "gb_internovel_runtime.log", "gb_internovel_*.json")
Get-ChildItem -LiteralPath $runtimeDir -Force | ForEach-Object {
  $item = $_
  $skip = $false
  foreach ($pattern in $excludeNames) {
    if ($item.Name -like $pattern) {
      $skip = $true
      break
    }
  }
  if (-not $skip) {
    Copy-Item -LiteralPath $item.FullName -Destination $PreviewDir -Recurse -Force
  }
}

$previewConfig = Join-Path $PreviewDir "experiment_config.json"
if (Test-Path (Join-Path $projectRoot "experiment_config.example.json")) {
  Copy-Item -LiteralPath (Join-Path $projectRoot "experiment_config.example.json") -Destination $previewConfig -Force
}

$readme = @"
GB Internovel - Windows development preview

Run:
  gb_internovel_sdl.exe

Controls:
  Arrow keys: move
  Enter: confirm / A
  Esc: cancel / B

API config:
  Edit experiment_config.json and set api_key, or set DEEPSEEK_API_KEY in the environment.
  Do not share a package containing a private API key.

Runtime data:
  Local history is created under data\gb_internovel.db after launch.
"@
Set-Content -LiteralPath (Join-Path $PreviewDir "README.txt") -Value $readme -Encoding UTF8

Write-Host "[gb_internovel] packaged successfully"
Write-Host "[gb_internovel] shareable preview:"
Write-Host "  $PreviewDir"
