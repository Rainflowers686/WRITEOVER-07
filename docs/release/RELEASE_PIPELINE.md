# WRITEOVER-07 Release Pipeline

This document covers the small portable player packages for
`0.1.0-pvs01-gold`. It does not change the normal Gold CI workflow.

## Source of truth

`VERSION` is the single release version source. CMake reads its numeric prefix
for the project version, and the package tools use the complete value for
`version.json`, archive names, README text, and the release tag.

Runtime content is copied only from checked-in compiled `.woc` and `.bin`
files. Authoring JSON, source, tests, build trees, and user data do not enter
a player package.

## Local packaging

From the repository root after a clean `main` checkout:

```powershell
pwsh -File scripts/package_windows.ps1
python tools/release/package_smoke.py --platform windows-x64 --archive dist/WRITEOVER-07-v0.1.0-pvs01-gold-win-x64.zip
```

On Linux:

```bash
bash scripts/package_linux.sh
python3 tools/release/package_smoke.py --platform linux-x64 --archive dist/WRITEOVER-07-v0.1.0-pvs01-gold-linux-x64.tar.gz
```

On an arm64 macOS runner:

```bash
bash scripts/package_macos.sh
python3 tools/release/package_smoke.py --platform macos-arm64 --archive dist/WRITEOVER-07-v0.1.0-pvs01-gold-macos-arm64.zip
```

Each script performs the Release build/tests/bench, creates the same
`dist/stage/<platform>/` payload used by future Steam depots, writes a
per-package manifest and `SHA256SUMS.txt`, and creates one archive. The smoke
tool extracts the archive into a temporary directory, runs the player with a
different working directory, verifies executable-relative data lookup, and
checks that the smoke save is written to a separate user-data directory.

`--skip-build` is available on the platform scripts when a caller has already
run the exact Release gates. It does not weaken the clean-package smoke.

## Tagged release

The release workflow is `.github/workflows/release.yml`. It accepts a `v*` tag
but verifies that the tag is exactly `v$(Get-Content VERSION)` before doing
any release work. Each platform job checks content, builds Release, runs
tests/bench, packages, and clean-package smokes. The aggregate job computes
the archive checksums and creates a GitHub pre-release with the three archives
and `SHA256SUMS.txt`.

The safe publication order is:

1. Keep `main` clean and wait for the normal Gold CI to pass.
2. Run all locally available package and clean-package gates.
3. Confirm the tag does not already exist.
4. Create `v0.1.0-pvs01-gold` on the already verified `main` commit and push
   that tag. Do not force-move a tag.
5. Wait for the tagged release workflow and verify the pre-release assets and
   checksums.

The workflow requires only the GitHub Actions `contents: write` permission;
it does not read Apple or Windows signing secrets. Signing and notarization
remain explicit `NOT_CONFIGURED` states until real credentials are supplied.
