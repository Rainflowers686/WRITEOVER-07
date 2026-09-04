# WRITEOVER-07 RELEASE-01 — Player Distribution & Packaging Closure

This receipt records the PVS-01 Gold player-distribution closure. It covers
release engineering only; it does not change gameplay, content, NPC behavior,
or the systemic architecture.

## Release identity

```text
START_HEAD = 1e7f138a5228e79f4e821cd74ca18cfd38184200
FINAL_MAIN_HEAD = 269510b7184ebc2f5149f13191f608c09e97f4fa
TAG = v0.1.0-pvs01-gold
TAG_COMMIT = 174d2ceaa0605889002d9526ecb6bd0781375d80
PACKAGE_SOURCE_COMMIT = 174d2ceaa0605889002d9526ecb6bd0781375d80
VERSION = 0.1.0-pvs01-gold
REPOSITORY_VISIBILITY = PUBLIC (pre-existing; unchanged)
```

`FINAL_MAIN_HEAD` is the main commit containing the release-pipeline
validation used for the immutable-tag recovery. The final receipt commit is a
docs-only commit after that validation head and is recorded by Git history.

## Player entries and archives

```text
WINDOWS_PLAYER_ENTRY = WRITEOVER-07.exe
LINUX_PLAYER_ENTRY = WRITEOVER-07
MACOS_PLAYER_ENTRY = WRITEOVER-07.app

WINDOWS_PACKAGE = WRITEOVER-07-v0.1.0-pvs01-gold-win-x64.zip
LINUX_PACKAGE = WRITEOVER-07-v0.1.0-pvs01-gold-linux-x64.tar.gz
MACOS_PACKAGE = WRITEOVER-07-v0.1.0-pvs01-gold-macos-arm64.zip

WINDOWS_PACKAGE_SIZE = 382658 bytes
LINUX_PACKAGE_SIZE = 316054 bytes
MACOS_PACKAGE_SIZE = 236797 bytes
```

The GitHub assets contain one versioned top-level directory. Each package has
compiled runtime data only, `version.json`, `manifest.json`,
`SHA256SUMS.txt`, player documentation, and third-party notices. The package
inspection found ten compiled `.bin`/`.woc` files per platform and no source
tree, authoring JSON, build tree, CMake cache, or development executable.

```text
WINDOWS_PACKAGE_SMOKE = PASS (release run 33850387882; downloaded asset recheck)
LINUX_PACKAGE_SMOKE = PASS (release run 33850387882)
MACOS_PACKAGE_SMOKE = PASS (release run 33850387882 on macos-14 arm64)

WINDOWS_SAVE_LOAD = PASS (clean-package smoke external user-data save/load path)
LINUX_SAVE_LOAD = PASS (clean-package smoke external user-data save/load path)
MACOS_SAVE_LOAD = PASS (clean-package smoke external user-data save/load path)
```

The package smoke launches from an unrelated working directory. It validates
the package's executable-relative resource lookup and writes smoke save data
outside the package. The Linux and macOS smoke validator restores the POSIX
execute bit after Python ZIP extraction so the test exercises the extracted
player rather than an extractor mode-preservation quirk.

## Runtime path policy

```text
RESOURCE_ROOT_POLICY =
  explicit --data-dir first;
  otherwise executable-directory/data;
  otherwise macOS bundle Contents/Resources/data;
  development-only cwd/data fallback;
  packaged players therefore do not require the source tree.

USER_DATA_POLICY =
  Windows: %LOCALAPPDATA%/WRITEOVER-07;
  Linux: $XDG_DATA_HOME/WRITEOVER-07, fallback ~/.local/share/WRITEOVER-07;
  macOS: ~/Library/Application Support/WRITEOVER-07;
  --user-data-dir is an explicit test/operator override.

VERSION_SOURCE_OF_TRUTH = PRODUCT_VERSION
```

## Package integrity and provenance

```text
PACKAGE_MANIFEST = PASS (manifest.json in every package)
SHA256SUMS = PASS (per-package and GitHub aggregate; downloaded assets rechecked)
LICENSE_SCAN = PASS (THIRD_PARTY_NOTICES.txt present in every package)
SECRET_SCAN = PASS
PATH_SCAN = PASS (no source/workspace/build/private test paths)
```

GitHub Release asset checksums, independently recomputed after download:

```text
1549988dba8e75af169b210ae1e516d1bab03ec2b258bdc9a8372d3577026ad6  WRITEOVER-07-v0.1.0-pvs01-gold-win-x64.zip
36b0b659f8b00fb29916ebbab996b8dc8260c26d3930da41ea66be8a8dde5012  WRITEOVER-07-v0.1.0-pvs01-gold-linux-x64.tar.gz
0bde80d72d501383243b87793730595c2e269064d7e635a645f47954ec27f342  WRITEOVER-07-v0.1.0-pvs01-gold-macos-arm64.zip
```

The downloaded `version.json` in all three archives reports product
`WRITEOVER-07`, version `0.1.0-pvs01-gold`, Release build, and package source
commit `174d2ceaa0605889002d9526ecb6bd0781375d80`.

## Signing and Steam staging

```text
WINDOWS_CODE_SIGNING = NOT_CONFIGURED (unsigned package; no certificate supplied)
MACOS_SIGNING = NOT_CONFIGURED (no Developer ID supplied)
MACOS_NOTARIZATION = NOT_CONFIGURED (no Apple credentials supplied)

STEAM_READY_STAGING = YES
STEAM_WINDOWS_ROOT = dist/stage/windows-x64/
STEAM_LINUX_ROOT = dist/stage/linux-x64/
STEAM_MACOS_ROOT = dist/stage/macos-arm64/
STEAM_UPLOADED = NO
STEAM_APP_ID = NOT_CONFIGURED
STEAM_DEPOT_IDS = NOT_CONFIGURED
```

The staging roots are the unwrapped package payloads and are documented for a
future SteamPipe upload. No Steam AppID, SDK, upload, or release-visibility
change was invented or performed.

## GitHub publication and CI

```text
GITHUB_RELEASE = CREATED
GITHUB_RELEASE_TYPE = PRE-RELEASE
GITHUB_RELEASE_URL = https://github.com/Rainflowers686/WRITEOVER-07/releases/tag/v0.1.0-pvs01-gold
GITHUB_RELEASE_ASSETS =
  WRITEOVER-07-v0.1.0-pvs01-gold-win-x64.zip (382658 bytes)
  WRITEOVER-07-v0.1.0-pvs01-gold-linux-x64.tar.gz (316054 bytes)
  WRITEOVER-07-v0.1.0-pvs01-gold-macos-arm64.zip (236797 bytes)
  SHA256SUMS.txt (336 bytes)

MAIN_CI = SUCCESS (run 33849914907, head 269510b)
RELEASE_CI = SUCCESS (run 33850387882, workflow_dispatch source_ref=v0.1.0-pvs01-gold)
```

The first tag-triggered release run (`33849321864`) stopped only at macOS
clean-package launch because Python's ZIP extractor did not restore the
archive's POSIX executable mode. A recovery validator was committed to
`main`, the normal `main` CI passed, and the immutable tag was then rerun
without moving or deleting it. One intermediate recovery attempt
(`33850137843`) hit a transient macOS runner benchmark overrun before package
smoke; the subsequent unchanged-source run `33850387882` passed all platform
jobs and published the release. These failed runs remain visible as historical
CI evidence and are not represented as successes.

## Closure status

```text
OPEN_FATAL = 0
OPEN_MAJOR = 0
OPEN_P0 = 0
OPEN_P1 = 0

MOST_LIKELY_DISTRIBUTION_COMPLAINT =
  unsigned Windows SmartScreen or macOS Gatekeeper warning. The package
  includes a clear player entry, compiled data, version metadata, notices,
  and external save paths; it does not depend on the source tree or a
  development launcher. Signing/notarization are intentionally deferred
  until real platform credentials are supplied.

RELEASE01_VERDICT = READY_FOR_PLAYER_DISTRIBUTION
```

