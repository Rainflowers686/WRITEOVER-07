# Steam Distribution Staging — WRITEOVER-07

Steam is not integrated or uploaded in RELEASE-01. The same staged content
used to produce the GitHub Release archives is kept ready for a future
SteamPipe depot layout.

## Depot sources

| Future depot | Source staging directory | Player launch entry |
|---|---|---|
| Windows x64 | `dist/stage/windows-x64/` | `WRITEOVER-07.exe` |
| Linux x64 | `dist/stage/linux-x64/` | `WRITEOVER-07` |
| macOS arm64 | `dist/stage/macos-arm64/` | `WRITEOVER-07.app` |

The staging directories contain the package payload without a versioned
archive wrapper. The GitHub ZIP/tar.gz archives are created directly from
those directories with a versioned top-level folder.

## Future Steam design

- Base App: one future Steam application owned by the project.
- Depots: Windows x64, Linux x64, and macOS arm64.
- Future launch options: point at the platform-specific player entry above.
- Future common redistributables: none are currently required by the
  `/MT` Windows package or the POSIX system-library builds; re-check when the
  toolchain changes.
- Steam Cloud preparation: sync the platform user-data save files under the
  `WRITEOVER-07/saves/` directory, not the package's `data/` directory.
- Demo split: create a separate Steam AppID/depot only when a demo product is
  actually authorized; no AppID is invented here.
- Signing/notarization: attach real Windows and Apple credentials in a future
  protected release job; this PVS release is unsigned/ad-hoc-ready only.

`STEAM_READY_STAGING = YES`
`STEAM_UPLOADED = NO`
