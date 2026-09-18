# Version and provenance

## Current source and public package

`PRODUCT_VERSION` is `0.2.0-candidate.1`. The current public download is the
GitHub Latest Release **v0.2.0-course** (2026-09-18), whose tag resolves to
commit `d5017c172dcd97aa36d27e7cf75282ed925611ab`. The Windows archive was
rebuilt from that commit and keeps the existing package label
`WRITEOVER-07-v0.2.0-candidate.1-win-x64.zip`; its `version.json` records the
same commit and CI run `35290223738`. This is a bilingual course baseline, not
Product Gold, visual acceptance or a guaranteed course grade.

The earlier `v0.2.0-candidate.1` Pre-release remains published for provenance
and no longer represents the current source. Versioned notes for that package
are in [RELEASE_NOTES_v0.2.0-candidate.1.md](RELEASE_NOTES_v0.2.0-candidate.1.md).
Source version alone does **not** prove that a binary was rebuilt, CI passed or
a release was published. Final verified receipts, rather than this descriptive
page, must establish the exact commit, CI run, assets and hashes.

Every new package includes `version.json`, a file manifest and checksums. The
recorded Git commit must identify its actual source. A subsequent documentation
commit does not silently redefine an older binary's provenance. Use
[the release pipeline](RELEASE_PIPELINE.md) and inspect the published release's
actual assets before claiming a platform download.

## Historical Windows candidate (unchanged)

| Item | Recorded value |
|---|---|
| Version | 0.1.0-complete-campaign-candidate |
| Tag | candidate-0.1.0-complete-campaign-20260915 |
| Package implementation / tag | d2654f8974922a1e8db9d23e1ef99819cb5f5524 |
| Tested delivery commit | d5ef249adf54431e605603c3436000cc39421c36 |
| CI run | [34872278294](https://github.com/Rainflowers686/WRITEOVER-07/actions/runs/34872278294) |
| Archive | WRITEOVER-07-audit-candidate.zip |
| Bytes | 609629 |

Its checksum is preserved in
[SHA256SUMS_complete-campaign-candidate.txt](SHA256SUMS_complete-campaign-candidate.txt).
The earlier documentation-only publication did not rebuild that archive.
The older `v0.1.0-pvs01-gold` tag is also preserved; “Gold” in its historical
name does not mean the current full campaign has passed human acceptance.

## Compatibility and license

Back up saves before changing versions. Current readers retain support for the
previous complete Player payload and reject ambiguous truncated payloads; this
is not a promise to accept every historical or edited file. Settings and saves
remain separate from installed game data.

No open-source license has been granted in this repository. Public visibility
does not itself authorize unrestricted redistribution or commercial use.
See [third-party notices](THIRD_PARTY_NOTICES.txt).
