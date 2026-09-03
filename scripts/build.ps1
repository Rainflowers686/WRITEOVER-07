# Build wrapper.
# Usage:
#   pwsh -File scripts/build.ps1              # debug
#   pwsh -File scripts/build.ps1 -Preset release
#   pwsh -File scripts/build.ps1 -Config Release
param(
    [string]$Preset = "debug",
    [string]$Config = ""   # Debug/Release; defaults per preset
)

$ErrorActionPreference = "Stop"

Write-Host "== build ($Preset) =="
cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if ($Config -ne "") {
    cmake --build --preset $Preset --config $Config
} else {
    cmake --build --preset $Preset
}
exit $LASTEXITCODE