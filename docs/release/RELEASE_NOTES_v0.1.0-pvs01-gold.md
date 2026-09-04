# WRITEOVER-07 — PVS-01 Gold

`0.1.0-pvs01-gold` is a bounded player preview for human playtesting.

## What this is

This release contains the playable B1 Wake → Calibration / Logistics →
Service / Medical → 1F Security route. It demonstrates the production
terminal renderer, pistol/game-feel path, systemic NPC reactions, narrator
presentation, and Save/Load for the PVS slice. It is not the complete
41-floor game or a 1.0 release.

## Platforms

- Windows x64: `WRITEOVER-07.exe` in the Windows ZIP.
- Linux x64: `WRITEOVER-07` in the Linux tar.gz.
- macOS arm64: `WRITEOVER-07.app` in the macOS ZIP.

The ARM64/Kunpeng result is a cross-compile/link verification platform, not a
normal player download in this release.

## Controls

The complete default control sheet is included in each package's `README.txt`.
The main actions are WASD movement, mouse look/fire/aim, Shift sprint, F
interact, R reload, Esc pause, F5 save, and F9 load.

## Known limitations

- Linux and macOS use terminal keyboard input and a subtitles-only audio
  fallback in this repository. Windows uses procedural WinMM cues when an
  audio device is available.
- macOS requires arm64. The package is not signed/notarized with a Developer
  ID certificate in this classroom release, so Gatekeeper may show a warning.
- Windows is not code-signed; SmartScreen may show a warning for a downloaded
  portable ZIP.
- The full building, future M5/M6 content, and a large hand-authored art/audio
  pack are outside this PVS-01 Gold scope.

## Bug reporting

Include the platform, `version.json`, player entry, and exact terminal error
text when reporting a problem:
https://github.com/Rainflowers686/WRITEOVER-07/issues
