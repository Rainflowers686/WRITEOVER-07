# PVS-01 GOLD Real-Play Matrix

This file is the compact reproducible matrix for the production PVS route. It
records deterministic input replays as real application runs. A replay drives
the same `InputRuntime` and composition root used by the production executable;
it does not call private gameplay mutators or bypass SaveManager.

## Commands

Run from the repository root after a Release build:

```powershell
./out/build/release/Release/writeover_app.exe --data-dir data --width 120 --height 40 --frames 3500 --replay tools/replay/pvs01_normal.txt
./out/build/release/Release/writeover_app.exe --data-dir data --width 120 --height 40 --frames 3600 --replay tools/replay/pvs01_aggressive.txt
./out/build/release/Release/writeover_app.exe --data-dir data --width 120 --height 40 --frames 4500 --replay tools/replay/pvs01_stealth.txt
./out/build/release/Release/writeover_app.exe --data-dir data --width 120 --height 40 --frames 3500 --replay tools/replay/pvs01_systemic.txt
```

On POSIX, use the same arguments with
`./out/build/linux-release/writeover_app`.

## Matrix receipt

| Scenario | Route result | Runtime population | Autonomous proof | Systemic/save proof | Audio/presentation proof |
|---|---|---|---|---|---|
| Normal | B1 -> calibration -> service -> 1F | 5 total; 1 Full; 4 SemiHuman | 221 loops; discovery response 1 | non-empty journal/systemic/memory; Save and Load YES | Windows `winmm-procedural`; narrator typography active |
| Aggressive | same route | 5 total; 1 Full; 4 SemiHuman | 247 loops; discovery response 1 | non-empty journal/systemic/memory; Save and Load YES | gunfire/noise path; Windows procedural SFX |
| Stealth | same route with crouched approach | 5 total; 1 Full; 4 SemiHuman | 394 loops; discovery response 1 | body searched/dragged/hidden/discovered; Save and Load YES | reduced presentation path remains readable |
| Systemic | same route plus F1/F5/F9 | 5 total; 1 Full; 4 SemiHuman | 221 loops; discovery response 1 | Save and Load YES; systemic event/memory counts non-zero | full narrator typography receipt |

Every Windows Release run exited `0` and finished in `room_1f_security`.
The exact application summary contains `REPLAY_RESULT=PASS`, the route, NPC
counts, loop/response counts and Save/Load flags.

## Cross-platform save

1. Run the Windows Release save probe or the normal/systemic replay, which
   writes `saves/pvs_manual.wo07` through the real SaveManager.
2. Run:

   ```text
   ./out/build/linux-release/writeover_app --data-dir data --frames 40 --replay tools/replay/pvs01_cross_platform_load.txt
   ```

3. Require `LOAD_ATTEMPTED=YES` and `LOAD_OK=YES` in the Linux summary, with
   the same file left unchanged by the load-only replay.

The Windows-produced save loaded successfully in Linux Release. ARM64
execution and macOS physical execution remain external-host evidence, while
the format is fixed-width, bounded and platform-independent.

## Direct-control status

The native Windows input probe confirms the production keyboard/Raw Input
selection, focus handling and pointer/button seams. A complete direct human
route is not claimed from that probe. Deterministic replay is the reproducible
real-play evidence for this closure; a human should still perform the bounded
visual/game-feel playtest before broader content work.
