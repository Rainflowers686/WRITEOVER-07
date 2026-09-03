# Benchmark wrapper. Prints CSV + BUDGET verdict from the real bench binary.
# Usage: pwsh -File scripts/bench.ps1 [-Preset release]
param(
    [string]$Preset = "release"
)

$ErrorActionPreference = "Stop"

Write-Host "== bench ($Preset) =="

$exe = "out/build/$Preset/Release/writeover_bench.exe"
if (-not (Test-Path $exe)) {
    Write-Host "NOT_RUN: release bench not built (run: scripts/build.ps1 -Preset release)"
    exit 0
}
& $exe
exit $LASTEXITCODE