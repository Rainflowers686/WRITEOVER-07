# Recovery Implementation-01 — Real Windows Terminal Evidence

This record separates evidence captured from the running Release executable from
the CharCell/SVG reconstruction images committed by Character Reboot-01.

## Runtime launch

- executable: `out/build/release/writeover_app.exe`
- build configuration: Windows Release
- entry mode: default player launch, no replay file
- data root: repository `data/`
- capture surface: maximized Windows Terminal desktop
- capture image: `real_windows_terminal_c01.png`
- capture video: `real_windows_terminal_c01.mp4`
- visual acceptance: pending Rain review

Captured startup diagnostics from the executable:

```text
WRITEOVER-07 v0.1.0-pvs01-gold
[INFO][audio] winmm-procedural
TERMINAL_BACKEND=ansi-truecolor TERMINAL_DIMENSIONS=240x67 TERMINAL_QUALITY_PRESET=ULTRA120 TERMINAL_COLOR_CAPABILITY=TRUECOLOR TERMINAL_PROBE=windows-terminal
writeover_game exit=0
```

The first capture before the Windows console initialization correction exposed
raw CSI sequences and garbled UTF-8.  The current capture was taken after the
minimal VT-processing and UTF-8 code-page initialization in
`src/platform/windows/win_terminal.cpp`; the current image has no raw CSI
control text or UTF-8 mojibake.

## Video probe

The committed runtime capture was checked with `ffprobe`:

```text
codec_name=h264
width=2560
height=1440
avg_frame_rate=10/1
nb_frames=80
duration=8.000000
size=330827
```

This is real continuous executable output, but it is not a manual 60–90 second
playthrough.  Manual play duration and Rain's visual acceptance remain
`NOT_VERIFIED` / `PENDING` respectively.

## Evidence boundary

- `real_windows_terminal_c01.png`: real Windows Terminal desktop capture.
- `real_windows_terminal_c01.mp4`: real continuous desktop capture of the
  executable.
- `replay_b1_*.txt`: deterministic replay output with explicit gameplay
  assertions; these are not a substitute for manual play.
- `character_reboot/*.png`: CharCell/SVG-to-PNG reconstruction evidence from
  the earlier visual checkpoint, not terminal screenshots.

No Gold, product-release, or public-release claim is made by this evidence.
