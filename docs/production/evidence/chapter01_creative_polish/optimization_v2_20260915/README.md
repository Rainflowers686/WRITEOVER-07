# Production character-frame review — optimization continuation

Source: d2654f8974922a1e8db9d23e1ef99819cb5f5524.
The 17 named scene SVG/log pairs use the production app at 240x67 and the existing
capture_chapter01.ps1 cameras. PNGs are readable previews of those real CharCell
exports, not screenshots, generated game art or evidence of a native foreground
Windows Terminal session.

The final change after the 17 scene captures suppresses HUD on modal pages only;
boot_final and controls_final capture it. The earlier boot/controls files retain
the preceding modal iteration. ANSI output correction affects terminal escape
sequences, not these SVG cells, and is validated separately by protocol tests.

Review: B1, human, guard, Security and six door angles were checked. Door leaves
now have steel underpaint; the authored far/mid/near surround is a joined header
and jamb instead of stacked rounded lines. Projection remains a real world plane.
The gate cameras look at the Medical-return doorway in Security, not the lift.
First iteration and the original director captures remain available for comparison.

boot_final: stable option order and selected band; quiet frame and first-use hints;
gameplay HUD no longer competes with the menu. controls_final: 48x18 minimum,
wrapped bindings, scroll range and footer remain visible.
settings_final: real menu input cycles the frame limit to 120 and persists it in
task-owned settings.cfg; selected row and Back remain reachable at 48x18.
ending_final: earned completion loaded through F9 and opened through Pause; the
summary retains the actual known evidence and recorded consequences.

Boundary: distant human/prop art is still stylized; no Rain visual acceptance,
human audio listening or first-time classmate playtest is claimed. Raw SVGs and
scratch save/config directories are preserved locally without being added to Git.
