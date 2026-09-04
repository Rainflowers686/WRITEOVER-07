# Build and package the Windows x64 player distribution.
param(
    [switch]$SkipBuild,
    [string]$DistRoot = "dist"
)

$ErrorActionPreference = "Stop"
$repo = Split-Path $PSScriptRoot -Parent
Push-Location $repo
try {
    if (-not $SkipBuild) {
        python tools/contentc/contentc.py --data-dir data --out-dir data --check
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        python tools/contentc/test_contentc.py
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        python tools/systemic/systemic_schema_check.py --data-dir data
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        python tools/systemic/test_systemic_schema.py
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        python tools/systemic/compile_systemic_seed.py --src data/systemic/systemic_seed.json --out data/systemic/systemic_seed.bin
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        cmake --preset release
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        cmake --build --preset release --config Release
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & "out\build\release\Release\writeover_tests.exe"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        ctest --test-dir out/build/release -C Release --output-on-failure
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & "out\build\release\Release\writeover_bench.exe"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    $commit = (git rev-parse HEAD).Trim()
    $version = (Get-Content -LiteralPath VERSION -Raw).Trim()
    python tools/release/package_release.py --platform windows-x64 --binary "out\build\release\Release\writeover_app.exe" --source-root $repo --dist-root $DistRoot --version $version --commit $commit
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
