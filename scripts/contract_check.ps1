# Contract hygiene entry: forbidden patterns + dependency boundaries + public
# header snapshot. One command for the Codex protocol block.
# Usage: pwsh -File scripts/contract_check.ps1

$ErrorActionPreference = "Stop"

Write-Host "== contract_check =="
& powershell -ExecutionPolicy Bypass -File tools/contract_check/check_forbidden.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& powershell -ExecutionPolicy Bypass -File tools/contract_check/check_deps.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& powershell -ExecutionPolicy Bypass -File tools/contract_check/check_public_headers.ps1
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "contract_check OK"