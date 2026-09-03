# Test wrapper with gate-aware entry points.
# Usage:
#   pwsh -File scripts/test.ps1                    # ctest (unit tests)
#   pwsh -File scripts/test.ps1 -Gate R0           # R0 gate checklist
#   pwsh -File scripts/test.ps1 -Gate Integration  # daily integration gate
# Gates are documents + executable checks; unimplemented checks are reported
# NOT_READY (never hardcoded PASS) per the acceptance criteria.
param(
    [string]$Preset = "debug",
    [string]$Gate = ""
)

$ErrorActionPreference = "Stop"

Write-Host "== test ($Preset$($(if ($Gate) { ", gate=$Gate" } else { "" }))) =="

if ($Gate -ne "") {
    $base = Split-Path $PSScriptRoot -Parent
    $candidates = @(
        (Join-Path $base "gates/$Gate.md"),          # repo_seed/gates
        (Join-Path $base "../gates/$Gate.md"),       # output-root gates
        (Join-Path $base "../../gates/$Gate.md")
    )
    # Support <GATE>_GATE.md naming used by the Opus gate set.
    $globRoots = @((Join-Path $base "gates"), (Join-Path $base "../gates"), (Join-Path $base "../../gates"))
    $globMatches = foreach ($root in $globRoots) {
        if (Test-Path $root) { Get-ChildItem -Path $root -Filter "$Gate*.md" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName }
    }
    $gateFile = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $gateFile -and $globMatches) { $gateFile = $globMatches | Select-Object -First 1 }
    if (-not $gateFile) {
        Write-Host "NOT_READY: gate document $Gate not found"
        exit 0
    }
    Write-Host "Gate document: $gateFile"
    # Executable checks common to gates: unit tests + content validation.
    cmake --preset $Preset | Out-Null
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build --preset $Preset | Out-Null
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    ctest --preset $Preset --output-on-failure
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "Gate executable checks: PASS (document checklist still requires human verification)."
    exit 0
}

ctest --preset $Preset --output-on-failure
exit $LASTEXITCODE