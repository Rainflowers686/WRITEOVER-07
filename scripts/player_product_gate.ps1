param(
    [string]$Executable = "out/build/release/Release/writeover_app.exe",
    [string]$EvidenceDirectory = ""
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exe = (Resolve-Path (Join-Path $repo $Executable)).Path
$root = Join-Path ([IO.Path]::GetTempPath()) ("writeover-product-" + [Guid]::NewGuid().ToString("N"))
if ($EvidenceDirectory) {
    $root = [IO.Path]::GetFullPath((Join-Path $repo $EvidenceDirectory))
    $allowed = [IO.Path]::GetFullPath((Join-Path $repo "docs/production/evidence")) + [IO.Path]::DirectorySeparatorChar
    if (-not $root.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw "Evidence outside canonical scope" }
}
if (Test-Path -LiteralPath $root) { throw "Existing evidence is protected; choose a fresh directory" }
New-Item -ItemType Directory -Path $root | Out-Null
$userData = Join-Path $root "user"
function Invoke-ProductProbe([string]$fixture, [string]$label, [int]$frames, [string[]]$required) {
    $log = Join-Path $root ($label + ".log")
    & $exe --replay (Join-Path $repo "tools/replay/$fixture") --data-dir (Join-Path $repo "data") --user-data-dir $userData --frames $frames *> $log
    $code = $LASTEXITCODE
    $text = Get-Content -LiteralPath $log -Raw
    if ($code -ne 0) { throw "$label process failed: $code; $log" }
    foreach ($receipt in $required) { if (-not $text.Contains($receipt)) { throw "$label missing $receipt; $log" } }
    Write-Output "PRODUCT_PROBE=$label PASS"
}
try {
    Invoke-ProductProbe "campaign_probe_amend_product.txt" "integrated" 7200 @("PRODUCT_INTEGRATED_PROBE=PASS", "REPLAY_RESULT=PASS", "CAMPAIGN_COMPLETION_REACHED=YES")
    $saves = Join-Path $userData "saves"
    foreach ($slot in @("pvs_manual", "pvs_checkpoint", "pvs_pre_final", "pvs_completion", "pvs_resume")) {
        if (-not (Test-Path -LiteralPath (Join-Path $saves "$slot.wo07"))) { throw "Missing save role $slot" }
    }
    $before = @(Get-ChildItem -LiteralPath $saves -File | Get-FileHash | Sort-Object Path | ForEach-Object { $_.Hash }) -join ":"
    Invoke-ProductProbe "product_new_game.txt" "new_game" 80 @("previous_completed=YES", "PRODUCT_NEW_GAME_INITIAL_STATE=PASS", "ROOM=room_b1_revival HEALTH=100 EVIDENCE=0")
    $after = @(Get-ChildItem -LiteralPath $saves -File | Get-FileHash | Sort-Object Path | ForEach-Object { $_.Hash }) -join ":"
    if ($before -ne $after) { throw "New Game modified existing saves" }
    Write-Output "PRODUCT_NEW_GAME_SAVE_PRESERVATION=PASS"
    $priorFault = [Environment]::GetEnvironmentVariable("WRITEOVER_RECOVERY_FAIL_FINAL_COMMIT_STAGE", "Process")
    try {
        foreach ($stage in @("after_room", "after_world", "after_systemic", "after_events", "after_rng", "after_narrative", "after_ai", "after_player")) {
            [Environment]::SetEnvironmentVariable("WRITEOVER_RECOVERY_FAIL_FINAL_COMMIT_STAGE", $stage, "Process")
            Invoke-ProductProbe "campaign_completed_reload.txt" $stage 10 @("SAVE_FINAL_COMMIT_FAILURE_STAGE=$stage", "SAVE_FINAL_COMMIT_ROLLBACK=PASS", "LOAD_OK=NO")
        }
    } finally { [Environment]::SetEnvironmentVariable("WRITEOVER_RECOVERY_FAIL_FINAL_COMMIT_STAGE", $priorFault, "Process") }
    Invoke-ProductProbe "campaign_completed_reload.txt" "completed_reload" 10 @("REPLAY_RESULT=PASS", "LOAD_OK=YES")
    Write-Output "PLAYER_PRODUCT_GATE=PASS"
} finally {
    if ($EvidenceDirectory) { Write-Output "PRODUCT_EVIDENCE=$root" }
    elseif (Test-Path -LiteralPath $root) {
        $resolved = (Resolve-Path -LiteralPath $root).Path
        $parent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/')
        if ((Split-Path $resolved -Parent) -ne $parent -or (Split-Path $resolved -Leaf) -notmatch '^writeover-product-[a-f0-9]{32}$') { throw "Refusing unsafe temporary cleanup" }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
