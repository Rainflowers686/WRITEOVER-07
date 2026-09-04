# Compatibility entry point for the formal Windows player package.
# Usage: pwsh -File scripts/package.ps1 [-SkipBuild]
param(
    [switch]$SkipBuild,
    [string]$DistRoot = "dist"
)

$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "package_windows.ps1") -SkipBuild:$SkipBuild -DistRoot $DistRoot
exit $LASTEXITCODE
