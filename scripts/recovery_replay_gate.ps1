param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe",
    [string]$EvidenceDirectory = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exePath = (Resolve-Path (Join-Path $repoRoot $Executable)).Path
$dataRoot = (Resolve-Path (Join-Path $repoRoot "data")).Path
$replayRoot = (Resolve-Path (Join-Path $repoRoot "tools/replay")).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "writeover-recovery-replays-" + [Guid]::NewGuid().ToString("N"))

$cases = @(
    @{ Name = "recovery_b1_success"; File = "recovery_b1_success.txt"; Frames = 2000 },
    @{ Name = "recovery_b1_denied"; File = "recovery_b1_denied.txt"; Frames = 2000 },
    @{ Name = "recovery_b1_terminal_denied"; File = "recovery_b1_terminal_denied.txt"; Frames = 750 },
    @{ Name = "recovery_b1_badge_only"; File = "recovery_b1_badge_only.txt"; Frames = 800 },
    @{ Name = "recovery_b1_health_death"; File = "recovery_b1_health_death.txt"; Frames = 3300 },
    @{ Name = "alpha01_systemic_success"; File = "alpha01_systemic_success.txt"; Frames = 2200 },
    @{ Name = "alpha01_aggressive_success"; File = "alpha01_aggressive_success.txt"; Frames = 1100 },
    @{ Name = "alpha01_denied"; File = "alpha01_denied.txt"; Frames = 2200 },
    @{ Name = "alpha01_memory_consequence"; File = "alpha01_memory_consequence.txt"; Frames = 1900 },
    @{ Name = "chapter01_systemic"; File = "chapter01_systemic.txt"; Frames = 3600 },
    @{ Name = "chapter01_aggressive"; File = "chapter01_aggressive.txt"; Frames = 3500 },
    @{ Name = "chapter01_denied_or_blocked"; File = "chapter01_denied_or_blocked.txt"; Frames = 2400 },
    @{ Name = "chapter01_memory_consequence"; File = "chapter01_memory_consequence.txt"; Frames = 6000 },
    @{ Name = "chapter01_mid_save_load"; File = "chapter01_mid_save_load.txt"; Frames = 3600 },
    @{ Name = "chapter01_backtrack"; File = "chapter01_backtrack.txt"; Frames = 6000 },
    @{ Name = "chapter01_security_bypass"; File = "chapter01_security_bypass.txt"; Frames = 3500; Room = "" },
    @{ Name = "chapter01_no_save_death"; File = "chapter01_no_save_death.txt"; Frames = 3000; Room = "room_1f_security" },
    @{ Name = "chapter01_terminal_skip_denied"; File = "chapter01_terminal_skip_denied.txt"; Frames = 2000; Room = "" },
    @{ Name = "scenario_guard_other_room"; File = "scenario_guard_other_room.txt"; Frames = 120; Room = "room_service_medical" },
    @{ Name = "scenario_camera_offline"; File = "scenario_camera_offline.txt"; Frames = 1200; Room = "" },
    @{ Name = "normal_quit"; File = "normal_quit.txt"; Frames = 0; Room = "" }
)

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
try {
    foreach ($case in $cases) {
        $replayPath = Join-Path $replayRoot $case.File
        $userData = Join-Path $tempRoot $case.Name
        $logPath = Join-Path $tempRoot ($case.Name + ".log")
        New-Item -ItemType Directory -Path $userData -Force | Out-Null

        Write-Host ("== recovery replay: {0} ({1} frames) ==" -f $case.Name, $case.Frames)
        $arguments = @(
            "--replay", $replayPath,
            "--data-dir", $dataRoot,
            "--user-data-dir", $userData,
            "--frames", $case.Frames
        )
        if ($case.ContainsKey("Room") -and -not [string]::IsNullOrWhiteSpace($case.Room)) {
            $arguments += @("--room", $case.Room)
        }
        & $exePath @arguments *> $logPath
        $exitCode = $LASTEXITCODE
        $output = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
        $required = @(
            "REPLAY_PROCESS_EXIT_OK=YES",
            "REPLAY_INPUT_CONSUMED=YES",
            "REPLAY_EXPECTED_STATE_REACHED=YES",
            "REPLAY_RESULT=PASS"
        )
        $missing = @($required | Where-Object { $output -notmatch [regex]::Escape($_) })
        if ($exitCode -ne 0 -or $missing.Count -gt 0) {
            $missingText = if ($missing.Count -eq 0) { "none" } else { $missing -join ", " }
            throw ("{0} failed: exit={1}; missing={2}; log={3}" -f
                $case.Name, $exitCode, $missingText, $logPath)
        }
        Write-Host ("PASS {0}" -f $case.Name)
    }
    Write-Host "RECOVERY_REPLAY_GATE=PASS"
}
finally {
    if ($EvidenceDirectory) {
        Write-Host ("EVIDENCE_PRESERVED=" + $tempRoot)
    } elseif (Test-Path -LiteralPath $tempRoot) {
        $resolvedTemporary = (Resolve-Path -LiteralPath $tempRoot).Path
        $expectedParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        if ((Split-Path $resolvedTemporary -Parent) -ne $expectedParent -or
            (Split-Path $resolvedTemporary -Leaf) -notmatch '^writeover-(recovery-replays|chapter01-scenarios)-[a-f0-9]{32}$') {
            throw "Refusing cleanup outside the exact task-owned temporary directory."
        }
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
