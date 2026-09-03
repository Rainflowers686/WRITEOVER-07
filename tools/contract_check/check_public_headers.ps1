# Public header drift checker: snapshots SHA256 of include/writeover headers.
# - With no baseline: writes .contract_baseline.json and passes (first run).
# - With baseline: fails when a public header changed without updating it.
# Changes require ADR + owner approval (27_ADR_POLICY.md).
# Usage: powershell -ExecutionPolicy Bypass -File tools/contract_check/check_public_headers.ps1
#
# Normalized SHA256: strips CR (0x0D) bytes before hashing so the hash is
# independent of git autocrlf LF<->CRLF conversion. This ensures the same
# header content produces the same hash on Windows CI and local dev machines.

$ErrorActionPreference = "Stop"

# Normalized hash: reads raw bytes, strips CR, hashes the normalized content.
function Get-NormalizedHash($path) {
    $bytes = [System.IO.File]::ReadAllBytes($path)
    $normalized = $bytes | Where-Object { $_ -ne 0x0D }
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $hash = $sha.ComputeHash($normalized)
    $hashString = -join ($hash | ForEach-Object { $_.ToString("X2") })
    return $hashString
}

$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$includeRoot = Join-Path $root "include/writeover"
$baselinePath = Join-Path $root "tools/contract_check/.contract_baseline.json"

$snapshot = @{}
Get-ChildItem -Path $includeRoot -Recurse -File -Filter *.h | Sort-Object FullName | ForEach-Object {
    $rel = $_.FullName.Substring($includeRoot.Length + 1) -replace "\\", "/"
    $snapshot[$rel] = Get-NormalizedHash $_.FullName
}

if (-not (Test-Path $baselinePath)) {
    $snapshot | ConvertTo-Json | Set-Content -Path $baselinePath
    Write-Host "check_public_headers: baseline recorded (first run)"
    exit 0
}

$baseline = Get-Content -Path $baselinePath | ConvertFrom-Json
$drift = @()
foreach ($key in $snapshot.Keys) {
    $baselineHash = $baseline.$key
    if ($null -eq $baselineHash -or $baselineHash -ne $snapshot[$key]) {
        $drift += $key
    }
}
foreach ($key in $baseline.PSObject.Properties.Name) {
    if (-not $snapshot.ContainsKey($key)) {
        $drift += "$key (removed)"
    }
}

if ($drift.Count -gt 0) {
    Write-Host "PUBLIC HEADER DRIFT (needs ADR + owner approval):" -ForegroundColor Red
    $drift | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "check_public_headers: OK"