param(
    [ValidateSet("FAST_REQUIRED", "EXTENDED")][string]$Tier = "FAST_REQUIRED",
    [string]$BuildDirectory = "out/build/release",
    [string]$Configuration = "Release",
    [string]$EvidenceDirectory = ""
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
if (-not $EvidenceDirectory) {
    $EvidenceDirectory = "docs/production/evidence/qa-" + $Tier.ToLowerInvariant() + "-" + [Guid]::NewGuid().ToString("N")
}
$evidence = [IO.Path]::GetFullPath((Join-Path $repo $EvidenceDirectory))
$allowed = [IO.Path]::GetFullPath((Join-Path $repo "docs/production/evidence")) + [IO.Path]::DirectorySeparatorChar
if (-not $evidence.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) { throw "Evidence outside canonical scope" }
if (Test-Path -LiteralPath $evidence) { throw "Existing QA evidence is protected; choose a fresh directory" }
New-Item -ItemType Directory -Path $evidence | Out-Null
$build = (Resolve-Path (Join-Path $repo $BuildDirectory)).Path
$exe = (Resolve-Path (Join-Path $build "$Configuration/writeover_app.exe")).Path
$gateExe = [IO.Path]::GetRelativePath($repo, $exe)
$bench = (Resolve-Path (Join-Path $build "$Configuration/writeover_bench.exe")).Path
$results = [Collections.Generic.List[object]]::new()
$status = "FAIL"

function Invoke-Step([string]$Name, [string]$Program, [string[]]$Arguments, [string[]]$Required = @()) {
    $started = [DateTimeOffset]::UtcNow
    $log = Join-Path $evidence "$Name.log"
    & $Program @Arguments *> $log
    $code = $LASTEXITCODE
    $text = Get-Content -LiteralPath $log -Raw
    $missing = @($Required | Where-Object { -not $text.Contains($_) })
    $passed = $code -eq 0 -and $missing.Count -eq 0
    $results.Add([pscustomobject]@{
        name = $Name; exitCode = $code; passed = $passed; missing = $missing
        seconds = [Math]::Round(([DateTimeOffset]::UtcNow - $started).TotalSeconds, 3)
        log = "$Name.log"
    })
    if (-not $passed) { throw "$Name failed: exit=$code missing=$($missing -join ','); $log" }
    Write-Output "QA_STEP=$Name PASS"
}

Push-Location -LiteralPath $repo
try {
    if ($Tier -eq "FAST_REQUIRED") {
        # CTest runs unit once plus the standalone public-header executable.
        Invoke-Step "unit-and-headers" "ctest" @("--test-dir", $build, "-C", $Configuration, "-V", "--output-on-failure")
        Invoke-Step "content" "python" @("tools/contentc/contentc.py", "--data-dir", "data", "--out-dir", "data", "--check")
        Invoke-Step "content-negative" "python" @("tools/contentc/test_contentc.py")
        Invoke-Step "schema" "python" @("tools/systemic/systemic_schema_check.py", "--data-dir", "data")
        Invoke-Step "schema-negative" "python" @("tools/systemic/test_systemic_schema.py")
        Invoke-Step "localization" "python" @("scripts/check_localization.py")
        Invoke-Step "release-metadata-tests" "python" @("tools/release/test_release_metadata.py")
        Invoke-Step "release-dry-run" "python" @("tools/release/release_metadata.py", "--dry-run")
        Invoke-Step "invalid-seed-startup" "python" @("tools/systemic/test_runtime_invalid_seed.py", "--executable", $exe)
        foreach ($language in @("en", "zh-CN")) {
            $userData = Join-Path $evidence "smoke-$language"
            New-Item -ItemType Directory -Path $userData | Out-Null
            [IO.File]::WriteAllText((Join-Path $userData "settings.cfg"), "language=$language")
            Invoke-Step "smoke-$language" $exe @("--smoke", "--frames", "61", "--data-dir", "data",
                "--user-data-dir", $userData, "--width", "80", "--height", "30") @("exit=0")
        }
        Invoke-Step "campaign-fast" "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
            "scripts/complete_game_campaign_gate.ps1", "-Executable", $gateExe,
            "-EvidenceDirectory", "$EvidenceDirectory/campaign", "-Fast") @("COMPLETE_GAME_CAMPAIGN_GATE=PASS")
        Invoke-Step "product-save" "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
            "scripts/player_product_gate.ps1", "-Executable", $gateExe,
            "-EvidenceDirectory", "$EvidenceDirectory/product") @("PLAYER_PRODUCT_GATE=PASS")
        Invoke-Step "rebind" $exe @("--replay", "tools/replay/product_rebind.txt", "--frames", "65",
            "--data-dir", "data", "--user-data-dir", (Join-Path $evidence "rebind-user")) @("PRODUCT_REBIND_PROBE=PASS", "REPLAY_RESULT=PASS")
        $art = Join-Path $evidence "art"
        New-Item -ItemType Directory -Path $art | Out-Null
        Invoke-Step "facing-and-held-poses" (Join-Path $build "$Configuration/writeover_art_review.exe") @(
            "data/characters/b1_character_art.txt", $art) @("FACING_MAPPING_COUNT=36 EXACT_ASSET_COUNT=36")
        $package = Join-Path $evidence "package"
        $archive = Join-Path $package "WRITEOVER-07-player.zip"
        Invoke-Step "package" "python" @("tools/release/package_release.py", "--platform", "windows-x64",
            "--binary", $exe, "--source-root", $repo, "--dist-root", $package, "--archive", $archive)
        Invoke-Step "package-smoke" "python" @("tools/release/package_smoke.py", "--platform", "windows-x64", "--archive", $archive)
        Invoke-Step "package-negative" "python" @("tools/release/package_negative_probe.py", "--platform", "windows-x64", "--archive", $archive)
        Invoke-Step "benchmark" $bench @()
        foreach ($check in @("check_forbidden", "check_deps", "check_public_headers")) {
            Invoke-Step $check "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/contract_check/$check.ps1")
        }
        Invoke-Step "static-audit" "python" @("tools/audit/static_audit.py", ".")
    } else {
        # No duplicate unit/content/package checks here. Retain all existing
        # route assertions, fault paths and completed-save reloads.
        foreach ($gate in @("recovery_replay_gate", "chapter01_scenario_matrix", "act2_route_coverage_gate", "complete_game_campaign_gate")) {
            Invoke-Step $gate "pwsh" @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "scripts/$gate.ps1",
                "-Executable", $gateExe, "-EvidenceDirectory", "$EvidenceDirectory/$gate")
        }
    }
    $status = "PASS"
} finally {
    $receipt = [ordered]@{
        tier = $Tier; status = $status; head = (git rev-parse HEAD).Trim()
        executable = $exe; configuration = $Configuration; steps = $results.ToArray()
        humanAcceptance = "NOT_ESTABLISHED_BY_AUTOMATION"
    }
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $evidence "qa-result.json") -Encoding utf8
    Pop-Location
    Write-Output "QA_TIER=$Tier STATUS=$status EVIDENCE=$evidence"
}
