# Package a distributable release folder + zip.
# Usage: pwsh -File scripts/package.ps1
param(
    [string]$Preset = "release"
)

$ErrorActionPreference = "Stop"

Write-Host "== package ($Preset) =="

cmake --preset $Preset | Out-Null
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build --preset $Preset --config Release | Out-Null
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$dist = "dist/WRITEOVER-07"
New-Item -ItemType Directory -Force -Path $dist | Out-Null

$exe = "out/build/$Preset/Release/writeover_app.exe"
if (-not (Test-Path $exe)) {
    Write-Host "NOT_RUN: release binary missing ($exe)"
    exit 0
}
Copy-Item $exe "$dist/WRITEOVER-07.exe"
Copy-Item "data" "$dist/data" -Recurse -Force
Copy-Item "README.txt" "$dist/README.txt" -ErrorAction SilentlyContinue

$sha = (Get-FileHash "$dist/WRITEOVER-07.exe" -Algorithm SHA256).Hash
Write-Host "SHA256: $sha"
Set-Content -Path "$dist/SHA256SUMS.txt" -Value "WRITEOVER-07.exe  $sha"

$zip = "dist/WRITEOVER-07-v0.1.0-build$(Get-Date -Format yyyyMMdd).zip"
Compress-Archive -Path "$dist/*" -DestinationPath $zip -Force
Write-Host "package OK -> $zip"