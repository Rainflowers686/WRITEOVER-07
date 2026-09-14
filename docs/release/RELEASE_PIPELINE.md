# Candidate release pipeline

A release is an immutable tag plus verified packages, not a change to a title.
The historical PVS and complete-campaign releases stay intact.

## One metadata authority

`PRODUCT_VERSION` supplies the version. Run:

```text
python tools/release/release_metadata.py --dry-run
```

This prints the tag, three platform archive names, title and versioned notes path.
It creates no tag and publishes nothing. A missing notes file or mismatched tag
fails closed. `--require-tag` also requires that the tag target equals checkout
HEAD. The workflow builds and validates that same checkout; validators are not
borrowed from a different revision of main.

## Before publication

1. Finish implementation and documentation on main. Preserve unrelated changes.
2. Build the current version and run [FAST_REQUIRED and EXTENDED](../engineering/TEST_STRATEGY.md).
3. Confirm exact-head CI, package manifests, clean-package smoke, negative probes
   and SHA-256. Record any genuinely human-only gaps.
4. Create a new `v<PRODUCT_VERSION>` tag pointing to the verified source commit.
   Do not move an existing tag.
5. Publish only validated assets as a new **Pre-release**, with both player guides,
   the versioned notes and SHA-256. Do not reuse an old binary under a new version.

## Optional three-platform workflow

`.github/workflows/release.yml` is manual-only. Choose an existing exact version
tag as `source_ref`. The default `publish=false` builds/tests/packages Windows x64,
Linux x64 and macOS arm64 and uploads workflow artifacts, without a Release.
Enable `publish` only for an authorized publication. The publish job waits for
all three clean-package checks, hashes precisely those three named archives,
requires both guides and notes, and refuses an existing release.

Only the publish job has repository write permission. A tag push itself does not
publish anything. The workflow does not establish the preceding five-platform CI
gate automatically; the release owner must verify that separately.

For a Windows-only authorized release, the same metadata, smoke and hash contract
applies. Do not claim Linux/macOS downloads exist unless those assets are actually
attached. Packaging scripts normally build and test; `SkipBuild` is only for an
already verified current binary, not permission to skip package validation.
Use a fresh distribution directory: the packaging tool rebuilds its own stage
and named archive there. Never aim it at historical evidence.

## Historical exception, preserved

On 2026-09-15 the user authorized publication of the unchanged previously tested
Windows ZIP without rerunning tests. Its tag is
`candidate-0.1.0-complete-campaign-20260915`, pointing to `d2654f8`.
That exception does not authorize skipping validation of later implementation.
The old workflow's PVS-specific archive/title/notes hardcoding is now removed.

Windows signing and macOS signing/notarization are not configured. Cross-link
success is not ARM hardware playtest evidence. Never disable OS protection as
a workaround for an unsigned candidate.
