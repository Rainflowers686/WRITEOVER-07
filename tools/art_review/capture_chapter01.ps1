param(
    [Parameter(Mandatory=$true)][string]$Batch,
    [string]$Executable = "out/build/release/Release/writeover_app.exe"
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
if ($Batch -notmatch '^[a-zA-Z0-9_-]+$') { throw "Batch must be a simple new directory name." }
$output = Join-Path $repo "docs/production/evidence/chapter01_creative_polish/$Batch"
if (Test-Path -LiteralPath $output) { throw "Preserve existing evidence: choose a new batch name." }
$exe = (Resolve-Path (Join-Path $repo $Executable)).Path
$data = Join-Path $repo "data"
$cases = @(
    @{Name="b1"; Room="room_b1_revival"},
    @{Name="security"; Room="room_1f_security"},
    @{Name="elevator"; Room="room_elevator_lobby"},
    @{Name="medical"; Room="room_service_medical"},
    @{Name="staff"; Room="room_restroom_staff"},
    @{Name="calibration"; Room="room_01_calibration"},
    @{Name="human"; Room="room_b1_revival"; Camera=@(5.6,12.15,-1.07,0)},
    @{Name="guard"; Room="room_b1_revival"; Camera=@(17.94,6.42,-0.53,0)},
    @{Name="cleaner"; Room="room_b1_revival"; Camera=@(13.4,5.6,-0.65,0)},
    @{Name="technician"; Room="room_b1_revival"; Camera=@(4.3,6.5,-1.47,0)},
    @{Name="security_desk"; Room="room_1f_security"; Camera=@(5.5,12.5,-0.6,0)},
    @{Name="lift_front"; Room="room_elevator_lobby"; Camera=@(8.0,5.5,0,0)},
    @{Name="lift_left"; Room="room_elevator_lobby"; Camera=@(8.0,4.5,0.25,0)},
    @{Name="lift_right"; Room="room_elevator_lobby"; Camera=@(8.0,6.5,-0.25,0)},
    @{Name="gate_front"; Room="room_1f_security"; Camera=@(20.0,8.5,0,0)},
    @{Name="gate_left"; Room="room_1f_security"; Camera=@(20.0,7.3,0.3,0)},
    @{Name="gate_right"; Room="room_1f_security"; Camera=@(20.0,9.7,-0.3,0)}
)
New-Item -ItemType Directory -Path $output | Out-Null
foreach ($case in $cases) {
    $arguments = @("--smoke","--frames","8","--width","240","--height","67",
        "--data-dir",$data,"--room",$case.Room,
        "--user-data-dir",(Join-Path $output ("scratch_" + $case.Name)),
        "--dump-frame",(Join-Path $output ($case.Name + ".svg")))
    if ($case.ContainsKey("Camera")) {
        $arguments += "--camera"
        $arguments += @($case.Camera | ForEach-Object {
            ([double]$_).ToString([System.Globalization.CultureInfo]::InvariantCulture)
        })
    }
    & $exe @arguments *> (Join-Path $output ($case.Name + ".log"))
    $code = $LASTEXITCODE
    if ($code -ne 0) { throw ("Capture failed: " + $case.Name + " exit=" + $code) }
    Write-Output ("PRODUCTION_CAPTURE=" + $case.Name + " EXIT=0")
}
Write-Output "PRODUCTION_CAPTURE_COUNT=17 FOREGROUND_ACCEPTANCE=NOT_CLAIMED"
