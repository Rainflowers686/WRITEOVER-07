#!/usr/bin/env python3
from pathlib import Path
import re, sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else "CURRENT_REPAIRED_FOUNDATION/repo_seed")
problems = []

def text(rel):
    p = root / rel
    if not p.exists():
        problems.append(f"MISSING {rel}")
        return ""
    return p.read_text(encoding="utf-8", errors="replace")

h = text("tests/test_harness.h")
if "#define WO_CHECK(expr)" in h and "return false" not in h.split("#define WO_CHECK(expr)",1)[1].splitlines()[0]:
    problems.append("TEST_ORACLE: WO_CHECK does not fail-fast")

im = text("src/player/input_mapper.cpp")
if "for (size_t i = 0; i < kGameActionCount" in im and "return static_cast<GameAction>(i)" in im:
    problems.append("INPUT: first-match global mapping still present")

wi = text("src/platform/windows/win_input.cpp")
if "FlushConsoleInputBuffer" in wi:
    problems.append("INPUT: FlushConsoleInputBuffer still used")
if "bKeyDown)" in wi and "KEY_EVENT" in wi:
    problems.append("INPUT: inspect key-up path; foundation code previously ignored releases")

eng = text("src/core/engine.cpp")
if "events->Dispatch" not in eng and "events.Dispatch" not in eng:
    problems.append("EVENT: Engine loop has no EventBus dispatch")

cc = text("tools/contentc/contentc.py")
if 'parser.add_argument("--check"' in cc and "args.check" not in cc:
    problems.append("CONTENT: --check parsed but unused")
if 'struct.pack("<Q", 1)' in cc:
    problems.append("CONTENT: RoomId hardcoded to 1")
if 'struct.pack("<B", 0)  # marker' in cc:
    problems.append("CONTENT: worldcommand payload marker-only")

save = text("src/core/save.cpp")
if "section_count >= static_cast<uint32_t>(SaveSectionId::Count)" in save:
    problems.append("SAVE: all-7-sections rejected")

term = text("src/platform/windows/win_terminal.cpp")
if 'out.append("\\x1b[38;2;")' in term and 'for (int x = 0; x < width; ++x)' in term:
    problems.append("TERMINAL: per-cell truecolor SGR path still present")

ray = text("src/render/raycaster.cpp")
if "far_cell.floor_z > near_cell.floor_z" in ray and "far_cell.floor_z < near_cell.floor_z" not in ray:
    problems.append("RAY: floor-drop boundary not explicitly handled")
if "t_max_x <= t_max_y" in ray:
    problems.append("RAY: exact corner tie policy still one-axis")

ctrl = text("src/player/controller.cpp")
if "world.FloorHeightAt" in ctrl and "GroundProbe" not in ctrl:
    problems.append("CONTROLLER: no explicit GroundProbe")
if "SetLean" in ctrl and "kLeanOffset" not in ctrl:
    problems.append("CONTROLLER: lean state has no geometry clamp")

story = text("src/narrative/storylet.cpp")
if "const uint32_t fired_count = d.ReadU32()" in story:
    problems.append("STORYLET: Load still expects fired state after definitions")

print("STATIC_AUDIT")
for p in problems:
    print(" -", p)
print(f"COUNT={len(problems)}")
sys.exit(1 if problems else 0)
