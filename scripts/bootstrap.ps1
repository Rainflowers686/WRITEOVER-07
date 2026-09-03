# Bootstrap: verify toolchain and compile authored content.
# Usage: pwsh -File scripts/bootstrap.ps1

$ErrorActionPreference = "Stop"

Write-Host "== WRITEOVER-07 bootstrap =="

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Error "cmake not found on PATH. Install VS2022 Build Tools (Desktop C++)."
    exit 1
}
Write-Host "cmake: $($cmake.Source)"

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    Write-Error "python not found on PATH (needed for contentc.py)."
    exit 1
}
Write-Host "python: $($python.Source)"

Write-Host "Compiling authored content (JSON -> compiled binaries)..."
python tools/contentc/contentc.py --data-dir data --out-dir data
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Configuring debug preset..."
cmake --preset debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "bootstrap OK"