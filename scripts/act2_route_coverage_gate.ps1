param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe",
    [string]$EvidenceDirectory = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exePath = (Resolve-Path (Join-Path $repoRoot $Executable)).Path
$dataRoot = (Resolve-Path (Join-Path $repoRoot "data")).Path

$tempRun = $false
if ($EvidenceDirectory) {
    $candidateEvidence = [IO.Path]::GetFullPath((Join-Path $repoRoot $EvidenceDirectory))
    $allowedEvidence = [IO.Path]::GetFullPath((Join-Path $repoRoot "docs/production/evidence")) + [IO.Path]::DirectorySeparatorChar
    if (-not $candidateEvidence.StartsWith($allowedEvidence, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Evidence must remain below the canonical production evidence directory."
    }
    if (Test-Path -LiteralPath $candidateEvidence) {
        throw "Choose a new evidence directory; prior runs are protected."
    }
    $runRoot = $candidateEvidence
}
else {
    $runRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
        "writeover-act2-route-coverage-" + [Guid]::NewGuid().ToString("N"))
    $tempRun = $true
}

$cases = @(
    @{
        Name = "act2_power_route"
        File = "act2_power_route_from_concourse_probe.txt"
        Frames = 3900
        Required = @(
            "REPLAY_PROCESS_EXIT_OK=YES",
            "REPLAY_INPUT_CONSUMED=YES",
            "room_act2_service_concourse>room_act2_power_utility",
            "UTILITY / The building is running on borrowed current.",
            "TRANSIT: the maintenance bypass worked. It also made a noise.",
            "TRANSITION_DENIED=NO",
            "PLAYER_DIED=NO"
        )
    },
    @{
        Name = "act2_observation_route"
        File = "act2_observation_terminal_route_probe.txt"
        Frames = 4200
        Required = @(
            "REPLAY_PROCESS_EXIT_OK=YES",
            "REPLAY_INPUT_CONSUMED=YES",
            "room_act2_records_archive>room_act2_observation_gallery",
            "OBSERVATION: transit camera looped. The facility will remember the gap.",
            "TRANSITION_DENIED=NO",
            "PLAYER_DIED=NO"
        )
    },
    @{
        Name = "act2_transit_route"
        File = "act2_transit_route_probe.txt"
        Frames = 5050
        Required = @(
            "REPLAY_PROCESS_EXIT_OK=YES",
            "REPLAY_INPUT_CONSUMED=YES",
            "room_act2_service_concourse>room_act2_transit_control",
            "IMPACT. Health is now authoritative.",
            "Target down. Search the body before moving on.",
            "Act II-A checkpoint recorded. The facility has opened a deeper question.",
            "TRANSITION_DENIED=NO",
            "PLAYER_DIED=NO"
        )
    }
)

New-Item -ItemType Directory -Path $runRoot | Out-Null
$failures = [System.Collections.Generic.List[string]]::new()

try {
    foreach ($case in $cases) {
        $replayPath = (Resolve-Path (Join-Path $repoRoot ("tools/replay/" + $case.File))).Path
        $caseDir = Join-Path $runRoot $case.Name
        New-Item -ItemType Directory -Path $caseDir | Out-Null
        $userData = Join-Path $caseDir "user"
        $logPath = Join-Path $caseDir ($case.Name + ".log")
        New-Item -ItemType Directory -Path $userData | Out-Null

        $arguments = @(
            "--replay", $replayPath,
            "--data-dir", $dataRoot,
            "--user-data-dir", $userData,
            "--frames", $case.Frames
        )
        & $exePath @arguments *> $logPath
        $exitCode = $LASTEXITCODE
        $output = if (Test-Path -LiteralPath $logPath) {
            Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
        }
        else {
            ""
        }

        $missing = @($case.Required | Where-Object {
                [string]::IsNullOrEmpty($output) -or -not $output.Contains([string]$_)
            })
        if ($exitCode -ne 0 -or $missing.Count -gt 0) {
            $missingText = if ($missing.Count -eq 0) { "none" } else { $missing -join "; " }
            $failures.Add(("{0}: exit={1}; missing={2}; log={3}" -f
                    $case.Name, $exitCode, $missingText, $logPath))
            Write-Host ($case.Name.ToUpperInvariant() + "=FAIL")
        }
        else {
            Write-Host ($case.Name.ToUpperInvariant() + "=PASS")
        }
    }

    if ($failures.Count -gt 0) {
        throw ("Act II route coverage failed: " + ($failures -join " | "))
    }

    Write-Host "ACT2_ROUTE_COVERAGE_GATE=PASS"
    Write-Host ("ACT2_ROUTE_COVERAGE_EVIDENCE=" + $runRoot)
}
finally {
    if ($tempRun -and (Test-Path -LiteralPath $runRoot)) {
        $resolvedTemporary = (Resolve-Path -LiteralPath $runRoot).Path
        $expectedParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        if ((Split-Path $resolvedTemporary -Parent) -ne $expectedParent -or
            (Split-Path $resolvedTemporary -Leaf) -notmatch '^writeover-act2-route-coverage-[a-f0-9]{32}$') {
            throw "Refusing cleanup outside the exact task-owned temporary directory."
        }
        Remove-Item -LiteralPath $runRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
