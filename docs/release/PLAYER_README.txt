WRITEOVER-07
============
Version: @VERSION@
Platform: @PLATFORM@

How to start
------------
@START@
The player entry is @ENTRY@. Game data is bundled with the entry and is
located relative to it; do not move the data directory by itself.

Controls (release defaults)
---------------------------
WASD              Move
Shift             Sprint
Space             Jump
Ctrl              Crouch
Z                 Prone
Q / E             Lean left / right
Mouse             Look
Left mouse        Fire
Right mouse       Aim down sights
F                  Interact
R                  Reload
V                  Melee
1 / 2 / 3          Select pistol / SMG / stunner
Esc               Pause
F1                Help / narrator information
F5                Save
F9                Load
F3                Development diagnostics (not included in this release)

Save and settings
-----------------
Save files: @SAVE_DIR@
Settings file (optional): @SETTINGS_DIR@
The package itself is read-only content; saves and settings are kept in the
platform user-data directory.

Known platform limitations
---------------------------
Windows uses the native raw-mouse and WinMM procedural audio paths when the
device is available. Linux and macOS use terminal keyboard input and retain
all gameplay text through subtitles; this PVS package does not bundle a
third-party audio middleware runtime. macOS requires an arm64 system for this
package.

Bug reports
-----------
Please report the platform, version, exact player entry, and the terminal
error text (if any) at:
https://github.com/Rainflowers686/WRITEOVER-07/issues

Credits and distribution notice
--------------------------------
See THIRD_PARTY_NOTICES.txt in this package. This is the WRITEOVER-07 PVS-01
Gold player distribution, not a developer/source distribution.
