param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe"
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
    @{ Name = "alpha01_memory_consequence"; File = "alpha01_memory_consequence.txt"; Frames = 1900 }
)

New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
try {
    foreach ($case in $cases) {
        $replayPath = Join-Path $replayRoot $case.File
        $userData = Join-Path $tempRoot $case.Name
        $logPath = Join-Path $tempRoot ($case.Name + ".log")
        New-Item -ItemType Directory -Path $userData -Force | Out-Null

        Write-Host ("== recovery replay: {0} ({1} frames) ==" -f $case.Name, $case.Frames)
        & $exePath --replay $replayPath --data-dir $dataRoot `
            --user-data-dir $userData --frames $case.Frames *> $logPath
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
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
