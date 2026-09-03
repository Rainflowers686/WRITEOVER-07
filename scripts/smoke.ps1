# Smoke: build + unit tests + content validation + app --smoke (boot 60 ticks,
# render via real terminal backend, atomic save, restore console, exit 0).
# Usage: pwsh -File scripts/smoke.ps1 [-Preset debug]
param(
    [string]$Preset = "debug"
)

$ErrorActionPreference = "Stop"

Write-Host "== smoke ($Preset) =="

cmake --preset $Preset | Out-Null
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build --preset $Preset | Out-Null
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

ctest --preset $Preset --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Validate compiled content (mapc on each .woc under data/rooms).
$rooms = Get-ChildItem -Path data/rooms -Filter *.woc -ErrorAction SilentlyContinue
if ($rooms) {
    foreach ($room in $rooms) {
        $mapc = "out/build/$Preset/Debug/mapc.exe"
        if (-not (Test-Path $mapc)) { $mapc = "out/build/$Preset/Release/mapc.exe" }
        if (Test-Path $mapc) {
            & $mapc $room.FullName
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
}

$exe = "out/build/$Preset/Debug/writeover_app.exe"
if (-not (Test-Path $exe)) { $exe = "out/build/$Preset/Release/writeover_app.exe" }
if (-not (Test-Path $exe)) {
    Write-Host "NOT_RUN: writeover_app.exe not built"
    exit 1
}
& $exe --smoke --data-dir data
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "smoke OK"