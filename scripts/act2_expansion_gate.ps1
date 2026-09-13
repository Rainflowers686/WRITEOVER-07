param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe",
    [string]$EvidenceDirectory = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exePath = (Resolve-Path (Join-Path $repoRoot $Executable)).Path
$dataRoot = (Resolve-Path (Join-Path $repoRoot "data")).Path
$replayPath = (Resolve-Path (Join-Path $repoRoot "tools/replay/act2_records_route_probe.txt")).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "writeover-act2-expansion-" + [Guid]::NewGuid().ToString("N"))

if ($EvidenceDirectory) {
    $candidateEvidence = [IO.Path]::GetFullPath((Join-Path $repoRoot $EvidenceDirectory))
    $allowedEvidence = [IO.Path]::GetFullPath((Join-Path $repoRoot "docs/production/evidence")) + [IO.Path]::DirectorySeparatorChar
    if (-not $candidateEvidence.StartsWith($allowedEvidence, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Evidence must remain below the canonical production evidence directory."
    }
    if (Test-Path -LiteralPath $candidateEvidence) { throw "Choose a new evidence directory; prior runs are protected." }
    $tempRoot = $candidateEvidence
}

New-Item -ItemType Directory -Path $tempRoot | Out-Null
$userData = Join-Path $tempRoot "user"
$logPath = Join-Path $tempRoot "act2_records_route.log"
New-Item -ItemType Directory -Path $userData | Out-Null

try {
    $arguments = @(
        "--replay", $replayPath,
        "--data-dir", $dataRoot,
        "--user-data-dir", $userData,
        "--frames", 4100
    )
    & $exePath @arguments *> $logPath
    $exitCode = $LASTEXITCODE
    $output = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue

    # This fixture deliberately continues after the Chapter One checkpoint.
    # The generic replay receipt therefore does not claim Chapter One closure;
    # this gate validates the separate Act II route contract instead.
    $required = @(
        "REPLAY_PROCESS_EXIT_OK=YES",
        "REPLAY_INPUT_CONSUMED=YES",
        "REPLAY_ROUTE=room_b1_revival>room_01_calibration>room_service_medical>room_1f_security>room_elevator_lobby>room_act2_service_concourse>room_act2_records_archive>room_act2_service_concourse",
        "SUBTITLE_TRACE frame=.*room_act2_service_concourse.*Dispatch has three ways",
        "SUBTITLE_TRACE frame=.*room_act2_records_archive.*ARCHIVE: manifest indexed",
        "SUBTITLE_TRACE frame=.*room_act2_records_archive.*Operator: Your file arrived",
        "SAVE_OK=YES",
        "LOAD_OK=YES",
        "PLAYER_DIED=NO",
        "TRANSITION_DENIED=NO",
        "PLAYER_STATE=room_room_act2_service_concourse pos_12.000_2.500_0.000"
    )
    $missing = @($required | Where-Object {
        $output -notmatch $_
    })
    if ($exitCode -ne 0 -or $missing.Count -gt 0) {
        $missingText = if ($missing.Count -eq 0) { "none" } else { $missing -join "; " }
        throw ("Act II expansion route failed: exit={0}; missing={1}; log={2}" -f
            $exitCode, $missingText, $logPath)
    }
    Write-Host "ACT2_EXPANSION_ROUTE=PASS"
    Write-Host ("ACT2_EVIDENCE=" + $tempRoot)
}
finally {
    if (-not $EvidenceDirectory -and (Test-Path -LiteralPath $tempRoot)) {
        $resolvedTemporary = (Resolve-Path -LiteralPath $tempRoot).Path
        $expectedParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        if ((Split-Path $resolvedTemporary -Parent) -ne $expectedParent -or
            (Split-Path $resolvedTemporary -Leaf) -notmatch '^writeover-act2-expansion-[a-f0-9]{32}$') {
            throw "Refusing cleanup outside the exact task-owned temporary directory."
        }
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
