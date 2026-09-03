# Dependency boundary scanner (05_MODULE_DEPENDENCY_RULES.md contract).
# Scans #include <writeover/...> in src/ and maps them against the frozen DAG.
# Usage: powershell -ExecutionPolicy Bypass -File tools/contract_check/check_deps.ps1

$ErrorActionPreference = "Stop"

$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent

# Frozen dependency table (module -> allowed writeover/ top-level dirs).
$allowed = @{
    "common"     = @()
    "core"       = @("common")
    "world"      = @("common")
    "player"     = @("common", "world")
    "ai"         = @("common", "world", "player")
    "narrative"  = @("common", "world", "player", "ai")
    "render"     = @("common", "world", "player")
    "platform"   = @("common", "core", "world", "player", "ai", "narrative", "render")
    "app"        = @("common", "core", "world", "player", "ai", "narrative", "render")
}

$violations = @()
$srcRoot = Join-Path $root "src"
$files = Get-ChildItem -Path $srcRoot -Recurse -File -Include *.cpp,*.h

foreach ($file in $files) {
    $rel = $file.FullName.Substring($root.Length + 1)
    $relNormalized = $rel -replace "\\", "/"
    # Determine owning module: first path segment after src/
    $segments = $relNormalized.Split("/")
    if ($segments.Count -lt 2) { continue }
    $owner = $segments[1]
    if (-not $allowed.ContainsKey($owner)) { continue }
    $allowList = $allowed[$owner]

    foreach ($line in Get-Content -Path $file.FullName -ErrorAction SilentlyContinue) {
        if ($line -match "#include\s*<writeover/([^/]+)/") {
            $dep = $Matches[1]
            if ($dep -ne $owner -and $allowList -notcontains $dep) {
                $violations += "$rel : includes <writeover/$dep/...> (not allowed for $owner)"
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host "DEPENDENCY VIOLATIONS:" -ForegroundColor Red
    $violations | ForEach-Object { Write-Host "  $_" }
    exit 1
}
Write-Host "check_deps: OK"