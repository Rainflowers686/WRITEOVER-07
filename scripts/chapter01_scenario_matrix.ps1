param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exePath = (Resolve-Path (Join-Path $repoRoot $Executable)).Path
$dataRoot = (Resolve-Path (Join-Path $repoRoot "data")).Path
$replayRoot = (Resolve-Path (Join-Path $repoRoot "tools/replay")).Path
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "writeover-chapter01-scenarios-" + [Guid]::NewGuid().ToString("N"))

$genericReceipt = @(
    "REPLAY_PROCESS_EXIT_OK=YES",
    "REPLAY_INPUT_CONSUMED=YES",
    "REPLAY_EXPECTED_STATE_REACHED=YES",
    "REPLAY_RESULT=PASS"
)

# These rows deliberately cover different outcome classes.  Every row is
# classified as a player-facing outcome.  The Disposition column separately
# records execution, game-rule invalidity, or a valid state without a fixture.
$scenarios = @(
    [pscustomobject]@{ Name="quiet_b1_success"; Classification="CHAPTER_COMPLETABLE"; Fixture="recovery_b1_success.txt"; Frames=2000; Room=""; Expected=@() },
    [pscustomobject]@{ Name="b1_reader_without_badge"; Classification="EXPECTED_DENIAL"; Fixture="recovery_b1_denied.txt"; Frames=2000; Room=""; Expected=@("ACCESS_DENIED=YES") },
    [pscustomobject]@{ Name="b1_terminal_without_badge"; Classification="EXPECTED_DENIAL"; Fixture="recovery_b1_terminal_denied.txt"; Frames=750; Room=""; Expected=@("TERMINAL_DENIED=YES") },
    [pscustomobject]@{ Name="badge_obtained_before_reader"; Classification="CHAPTER_COMPLETABLE"; Fixture="recovery_b1_badge_only.txt"; Frames=800; Room=""; Expected=@("BADGE_HELD_BY_PLAYER=YES") },
    [pscustomobject]@{ Name="death_with_checkpoint"; Classification="EXPECTED_DEATH_RESTART"; Disposition="EXECUTED"; Fixture="recovery_b1_health_death.txt"; Frames=3300; Room=""; Expected=@("PLAYER_RECOVERED=YES") },
    [pscustomobject]@{ Name="alpha_systemic_body_route"; Classification="CHAPTER_COMPLETABLE"; Fixture="alpha01_systemic_success.txt"; Frames=2200; Room=""; Expected=@() },
    [pscustomobject]@{ Name="alpha_aggressive_route"; Classification="CHAPTER_COMPLETABLE"; Fixture="alpha01_aggressive_success.txt"; Frames=1100; Room=""; Expected=@("CAMERA_SURVEILLANCE_RESPONSE=SECURITY_ALERT") },
    [pscustomobject]@{ Name="alpha_reader_denial"; Classification="EXPECTED_DENIAL"; Fixture="alpha01_denied.txt"; Frames=2200; Room=""; Expected=@("ACCESS_DENIED=YES") },
    [pscustomobject]@{ Name="alpha_cleaner_history"; Classification="CHAPTER_COMPLETABLE"; Fixture="alpha01_memory_consequence.txt"; Frames=1900; Room=""; Expected=@("CLEANER_HELP_COVERUP=YES") },
    [pscustomobject]@{ Name="chapter_quiet_route"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_systemic.txt"; Frames=3600; Room=""; Expected=@() },
    [pscustomobject]@{ Name="chapter_aggressive_route"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_aggressive.txt"; Frames=3500; Room=""; Expected=@("CAMERA_SURVEILLANCE_RESPONSE=SECURITY_ALERT") },
    [pscustomobject]@{ Name="chapter_denied_route"; Classification="EXPECTED_DENIAL"; Fixture="chapter01_denied_or_blocked.txt"; Frames=2400; Room=""; Expected=@("ACCESS_DENIED=YES") },
    [pscustomobject]@{ Name="chapter_memory_route"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_memory_consequence.txt"; Frames=6000; Room=""; Expected=@("CLEANER_HELP_COVERUP=YES") },
    [pscustomobject]@{ Name="chapter_mid_save_load"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_mid_save_load.txt"; Frames=3600; Room=""; Expected=@("LOAD_OK=YES") },
    [pscustomobject]@{ Name="chapter_backtrack"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_backtrack.txt"; Frames=6000; Room=""; Expected=@() },
    [pscustomobject]@{ Name="security_disabled_guard_bypass"; Classification="CHAPTER_COMPLETABLE"; Fixture="chapter01_security_bypass.txt"; Frames=3500; Room=""; Expected=@("FACT_CHAPTER_SECURITY_CHECKPOINT=YES") },
    [pscustomobject]@{ Name="death_before_first_save"; Classification="EXPECTED_DEATH_RESTART"; Disposition="EXECUTED"; Fixture="chapter01_no_save_death.txt"; Frames=3000; Room="room_1f_security"; Expected=@("PLAYER_RESTARTED=YES") },
    [pscustomobject]@{ Name="b1_terminal_skip_denied"; Classification="EXPECTED_DENIAL"; Fixture="chapter01_terminal_skip_denied.txt"; Frames=2000; Room=""; Expected=@("TRANSITION_DENIED=YES") },
    [pscustomobject]@{ Name="guard_alive_body_hidden"; Classification="INVALID_BY_GAME_RULES"; Disposition="GAME_RULE_INVALID"; Fixture=""; Frames=0; Room=""; Expected=@() },
    [pscustomobject]@{ Name="guard_stunned_body_exposed"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_security_bypass.txt"; Frames=3500; Room=""; Expected=@("NONLETHAL_HIT=YES", "BODY_CREATED=YES", "FACT_CHAPTER_SECURITY_CHECKPOINT=YES") },
    [pscustomobject]@{ Name="guard_dead_body_hidden"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="scenario_dead_body_hidden.txt"; Frames=1400; Room=""; Expected=@("BODY_STATUS=DEAD", "BODY_HIDDEN=YES", "NONLETHAL_HIT=NO") },
    [pscustomobject]@{ Name="camera_offline_cleaner_neutral"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="scenario_camera_offline.txt"; Frames=1200; Room=""; Expected=@("FACT_B1_CAMERA_DISABLED=YES", "CAMERA_SURVEILLANCE_RESPONSE=BLIND_SPOT") },
    [pscustomobject]@{ Name="camera_online_cleaner_history_positive"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="alpha01_memory_consequence.txt"; Frames=1900; Room=""; Expected=@("CLEANER_HELP_COVERUP=YES", "FACT_B1_CAMERA_DISABLED=NO") },
    [pscustomobject]@{ Name="quiet_route_pre_elevator_save"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_mid_save_load.txt"; Frames=3600; Room=""; Expected=@("LOAD_OK=YES", "FACT_CHAPTER_QUIET_ROUTE=YES") },
    [pscustomobject]@{ Name="aggressive_route_pre_elevator_save"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_aggressive.txt"; Frames=3500; Room=""; Expected=@("LOAD_OK=YES", "FACT_CHAPTER_AGGRESSIVE_ROUTE=YES", "FACT_CHAPTER_SECURITY_CHECKPOINT=YES", "CAMERA_SURVEILLANCE_RESPONSE=SECURITY_ALERT") },
    [pscustomobject]@{ Name="body_exposed_cleaner_blocked"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="chapter01_aggressive.txt"; Frames=3500; Room=""; Expected=@("BODY_HIDDEN=NO", "CLEANER_RESPONSE=NO") },
    [pscustomobject]@{ Name="body_hidden_cleaner_arrives"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_systemic.txt"; Frames=3600; Room=""; Expected=@("BODY_HIDDEN=YES", "BODY_DISCOVERED=YES", "CLEANER_RESPONSE=YES") },
    [pscustomobject]@{ Name="terminal_active_badge_revoked"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="scenario_terminal_active_badge_revoked.txt"; Frames=1900; Room=""; Expected=@("TERMINAL_SESSION=YES", "BADGE_REVOKED=YES", "TERMINAL_DENIED=YES") },
    [pscustomobject]@{ Name="door_open_after_failed_reader"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="chapter01_terminal_skip_denied.txt"; Frames=2000; Room=""; Expected=@("TRANSITION_DENIED=YES", "GATE_OPEN=YES") },
    [pscustomobject]@{ Name="player_dead_without_checkpoint"; Classification="EXPECTED_DEATH_RESTART"; Disposition="EXECUTED"; Fixture="chapter01_no_save_death.txt"; Frames=3000; Room="room_1f_security"; Expected=@("PLAYER_RESTARTED=YES") },
    [pscustomobject]@{ Name="player_restarted_with_durable_history"; Classification="EXPECTED_RECOVERABLE_FAILURE"; Disposition="EXECUTED"; Fixture="scenario_player_restarted_with_durable_history.txt"; Frames=2700; Room=""; Expected=@("PLAYER_RESTARTED=YES", "DURABLE_HISTORY_PRESERVED=YES") },
    [pscustomobject]@{ Name="security_guard_in_other_room"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="scenario_guard_other_room.txt"; Frames=120; Room="room_service_medical"; Expected=@("GUARD_ATTACKS=0", "PLAYER_HEALTH=100") },
    [pscustomobject]@{ Name="wall_blocked_guard_line_of_sight"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="scenario_wall_blocked_guard_los.txt"; Frames=450; Room="room_1f_security"; Expected=@("GUARD_LINE_OF_SIGHT_BLOCKED=YES", "GUARD_ATTACKS=0", "PLAYER_HEALTH=100") },
    [pscustomobject]@{ Name="staff_route_after_backtrack"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_backtrack.txt"; Frames=6000; Room=""; Expected=@("FACT_CHAPTER_STAFF_ROUTE=YES", "BACKTRACK_MEDICAL=YES") },
    [pscustomobject]@{ Name="elevator_entry_without_route_fact"; Classification="EXPECTED_DENIAL"; Disposition="EXECUTED"; Fixture="scenario_elevator_without_route.txt"; Frames=700; Room="room_elevator_lobby"; Expected=@("ELEVATOR_ENTRY_ATTEMPTED=YES", "ELEVATOR_ENTRY_DENIED=YES", "CHAPTER_CHECKPOINT_REACHED=NO") },
    [pscustomobject]@{ Name="objective_presented_before_completion"; Classification="CHAPTER_COMPLETABLE"; Disposition="EXECUTED"; Fixture="chapter01_systemic.txt"; Frames=3600; Room=""; Expected=@("QUEST_PRESENTED_DURING_ROUTE=YES", "QUEST_PRESENTATION_VISIBLE=YES") }
)

New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
$results = [System.Collections.Generic.List[object]]::new()
try {
    foreach ($scenario in $scenarios) {
        if ($scenario.Disposition -eq "GAME_RULE_INVALID") {
            $results.Add([pscustomobject]@{
                Name = $scenario.Name
                Classification = $scenario.Classification
                Result = "INVALID_BY_GAME_RULES"
                Detail = "The requested state contradicts the current game rules."
            })
            continue
        }

        if ($scenario.Disposition -eq "VALID_PLAYER_STATE_NOT_YET_COVERED") {
            $results.Add([pscustomobject]@{
                Name = $scenario.Name
                Classification = $scenario.Classification
                Result = "VALID_STATE_NOT_COVERED"
                Detail = "Valid state identified, but no non-contrived authored fixture is present."
            })
            continue
        }

        $replayPath = Join-Path $replayRoot $scenario.Fixture
        $userData = Join-Path $tempRoot $scenario.Name
        $logPath = Join-Path $tempRoot ($scenario.Name + ".log")
        New-Item -ItemType Directory -Path $userData -Force | Out-Null
        $arguments = @(
            "--replay", $replayPath,
            "--data-dir", $dataRoot,
            "--user-data-dir", $userData,
            "--frames", $scenario.Frames
        )
        if (-not [string]::IsNullOrWhiteSpace($scenario.Room)) {
            $arguments += @("--room", $scenario.Room)
        }
        & $exePath @arguments *> $logPath
        $exitCode = $LASTEXITCODE
        $output = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
        $missing = @($genericReceipt + $scenario.Expected | Where-Object {
            $output -notmatch [regex]::Escape($_)
        })
        if ($exitCode -ne 0 -or $missing.Count -gt 0) {
            throw ("{0} failed: exit={1}; missing={2}; log={3}" -f
                $scenario.Name, $exitCode, ($missing -join ", "), $logPath)
        }
        $results.Add([pscustomobject]@{
            Name = $scenario.Name
            Classification = $scenario.Classification
            Result = "PASS"
            Detail = ($scenario.Fixture + " @ " + $scenario.Frames + " frames")
        })
        Write-Host ("PASS {0}" -f $scenario.Name)
    }

    $runCount = @($results | Where-Object { $_.Result -eq "PASS" }).Count
    $invalidCount = @($results | Where-Object { $_.Result -eq "INVALID_BY_GAME_RULES" }).Count
    $validGapCount = @($results | Where-Object { $_.Result -eq "VALID_STATE_NOT_COVERED" }).Count
    $successCount = @($results | Where-Object { $_.Classification -eq "CHAPTER_COMPLETABLE" -and $_.Result -eq "PASS" }).Count
    $denialCount = @($results | Where-Object { $_.Classification -eq "EXPECTED_DENIAL" -and $_.Result -eq "PASS" }).Count
    $failureCount = @($results | Where-Object { $_.Classification -eq "EXPECTED_RECOVERABLE_FAILURE" -and $_.Result -eq "PASS" }).Count
    $deathRestartCount = @($results | Where-Object { $_.Classification -eq "EXPECTED_DEATH_RESTART" -and $_.Result -eq "PASS" }).Count
    Write-Host ("SCENARIO_MATRIX_TOTAL={0}" -f $scenarios.Count)
    Write-Host ("SCENARIOS_EXECUTED={0}" -f $runCount)
    Write-Host ("SCENARIOS_EXPECTED_SUCCESS={0}" -f $successCount)
    Write-Host ("SCENARIOS_EXPECTED_DENIAL={0}" -f $denialCount)
    Write-Host ("SCENARIOS_EXPECTED_RECOVERABLE_FAILURE={0}" -f $failureCount)
    Write-Host ("SCENARIOS_EXPECTED_DEATH_RESTART={0}" -f $deathRestartCount)
    Write-Host ("SCENARIOS_INVALID_BY_GAME_RULES={0}" -f $invalidCount)
    Write-Host ("SCENARIOS_VALID_STATE_NOT_COVERED={0}" -f $validGapCount)
    Write-Host "SCENARIO_MATRIX=PASS"
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
