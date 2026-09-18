# Version and provenance

## Current source and public package

`PRODUCT_VERSION` is `0.2.0-candidate.1`. This is a bilingual post-audit candidate,
not Product Gold, visual acceptance or a guaranteed course grade. The public
Windows Pre-release keeps this version label. The current course source may be a
later commit on `main` containing compatibility and packaging fixes; its exact
provenance is the commit recorded in the final delivery receipt, not the older
public package name.

The intended tag is `v0.2.0-candidate.1`; the metadata resolver requires an exact
match. Versioned notes are in
[RELEASE_NOTES_v0.2.0-candidate.1.md](RELEASE_NOTES_v0.2.0-candidate.1.md).
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
