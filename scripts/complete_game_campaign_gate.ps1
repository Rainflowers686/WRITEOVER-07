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
    "writeover-complete-game-campaign-" + [Guid]::NewGuid().ToString("N"))

$cases = @(
    @{ Name = "campaign_probe_amend"; File = "campaign_probe_amend.txt"; Frames = 6800; Ending = "amend" },
    @{ Name = "campaign_probe_disclose"; File = "campaign_probe_disclose.txt"; Frames = 7000; Ending = "disclose" },
    @{ Name = "campaign_probe_breach"; File = "campaign_probe_breach.txt"; Frames = 6800; Ending = "breach" },
    @{ Name = "campaign_probe_discovery_poor"; File = "campaign_probe_discovery_poor.txt"; Frames = 6800; Ending = "breach" }
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

        Write-Host ("== campaign replay: {0} ({1} frames) ==" -f $case.Name, $case.Frames)
        $arguments = @(
            "--replay", $replayPath,
            "--data-dir", $dataRoot,
            "--user-data-dir", $userData,
            "--frames", $case.Frames
        )
        & $exePath @arguments *> $logPath
        $exitCode = $LASTEXITCODE
        $output = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
        $required = @(
            "REPLAY_PROCESS_EXIT_OK=YES",
            "REPLAY_INPUT_CONSUMED=YES",
            "REPLAY_EXPECTED_STATE_REACHED=YES",
            "REPLAY_RESULT=PASS",
            "CAMPAIGN_COMPLETION_REACHED=YES",
            "CAMPAIGN_END_SCREEN_READY=YES",
            "SAVE_OK=YES",
            "LOAD_OK=YES",
            ("CAMPAIGN_FACTS .* {0}=YES" -f $case.Ending)
        )
        $missing = @($required | Where-Object { $output -notmatch $_ })
        if ($exitCode -ne 0 -or $missing.Count -gt 0) {
            $missingText = if ($missing.Count -eq 0) { "none" } else { $missing -join "; " }
            throw ("{0} failed: exit={1}; missing={2}; log={3}" -f
                $case.Name, $exitCode, $missingText, $logPath)
        }
        Write-Host ("PASS {0}" -f $case.Name)
    }
    Write-Host "COMPLETE_GAME_CAMPAIGN_GATE=PASS"
    Write-Host ("CAMPAIGN_EVIDENCE=" + $tempRoot)
}
finally {
    if ($EvidenceDirectory) {
        Write-Host ("EVIDENCE_PRESERVED=" + $tempRoot)
    } elseif (Test-Path -LiteralPath $tempRoot) {
        $resolvedTemporary = (Resolve-Path -LiteralPath $tempRoot).Path
        $expectedParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        if ((Split-Path $resolvedTemporary -Parent) -ne $expectedParent -or
            (Split-Path $resolvedTemporary -Leaf) -notmatch '^writeover-complete-game-campaign-[a-f0-9]{32}$') {
            throw "Refusing cleanup outside the exact task-owned temporary directory."
        }
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
