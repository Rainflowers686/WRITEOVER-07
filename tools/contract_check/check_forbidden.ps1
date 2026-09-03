# Forbidden-pattern grep scan (26_FORBIDDEN_PATTERNS.md contract).
# Exits 1 on any hit. Scans src/ and include/.
# Usage: powershell -ExecutionPolicy Bypass -File tools/contract_check/check_forbidden.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$code = Get-ChildItem -Path (Join-Path $root "src"), (Join-Path $root "include") -Recurse -File -Include *.cpp,*.h,*.hpp
$violations = @()

foreach ($file in $code) {
    $rel = $file.FullName.Substring($root.Length + 1)
    $relNormalized = $rel -replace "\\", "/"
    if ($relNormalized -like "src/platform/windows/*") {
        continue  # platform edge may use Win32 API
    }
    foreach ($line in Get-Content -Path $file.FullName -ErrorAction SilentlyContinue) {
        # Windows/socket/GPU headers outside platform edge.
        if ($line -match '#include\s*[<"](windows\.h|winsock2\.h|d3d11\.h|SDL[^>"]*)[>"]') {
            $violations += "$rel : $line"
        }
        if ($line -match '\bstd::rand\s*\(') {
            $violations += "$rel : $line"
        }
        if ($line -match '\bsystem\s*\(\s*["'']') {
            # shell-out like system("cls") — quoted string literal required
            $violations += "$rel : $line"
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host "FORBIDDEN PATTERNS FOUND:" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "check_forbidden: OK"