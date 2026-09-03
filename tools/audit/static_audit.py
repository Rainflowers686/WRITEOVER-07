#!/usr/bin/env python3
"""WRITEOVER-07 static audit — regression gate for the Fable-hardened codebase.

Each check tests the FIXED invariant. Exit code 1 with COUNT>0 means a
hardening fix has regressed. This is a heuristic gate, not a proof: the
authoritative gates are the unit tests (ctest).
"""
from pathlib import Path
import re, sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
problems = []


def text(rel):
    p = root / rel
    if not p.exists():
        problems.append(f"MISSING {rel}")
        return ""
    return p.read_text(encoding="utf-8", errors="replace")


# G0: test oracle must fail fast. The WO_CHECK macro body must contain a
# `return false` (anywhere within the macro expansion, not only line 1).
h = text("tests/test_harness.h")
wo_block = h.split("#define WO_CHECK(expr)", 1)
if len(wo_block) < 2 or "return false" not in wo_block[1].split("#define WO_CHECK_EQ", 1)[0]:
    problems.append("TEST_ORACLE: WO_CHECK macro does not fail-fast (no return false)")

# G0 meta-test presence: the harness must prove the runner reports failures.
common = text("tests/test_common.cpp")
if "failfast_macro_proves_failure" not in common:
    problems.append("TEST_ORACLE: fail-fast meta-test missing")

# Input conflict policy: SetBinding must clear duplicate PhysicalKeys.
im = text("src/player/input_mapper.cpp")
if "void InputMapper::SetBinding" in im and "PhysicalKey::Unknown" not in im.split(
        "void InputMapper::SetBinding", 1)[1].split("PhysicalKey InputMapper::GetBinding", 1)[0]:
    problems.append("INPUT: SetBinding missing duplicate-key clearing (replace policy)")

# key-up path: win_input must process key-down AND key-up.
wi = text("src/platform/windows/win_input.cpp")
if "bKeyDown != FALSE" not in wi:
    problems.append("INPUT: key-up (released) path missing in keyboard backend")

# Engine loop must dispatch events.
eng = text("src/core/engine.cpp")
if "events->Dispatch" not in eng and "events.Dispatch" not in eng:
    problems.append("EVENT: Engine loop has no EventBus dispatch")

# Content compiler: --check must be implemented (byte-compare recompile).
cc = text("tools/contentc/contentc.py")
if 'parser.add_argument("--check"' in cc and "deterministic recompile matches" not in cc:
    problems.append("CONTENT: --check parsed but not implemented as byte-compare")

# RoomId must not be hardcoded to 1 for every room.
if 'struct.pack("<Q", 1)  # RoomId' in cc:
    problems.append("CONTENT: RoomId hardcoded to 1")

# Save: all 7 legal sections must parse.
save = text("src/core/save.cpp")
if "section_count >= static_cast<uint32_t>(SaveSectionId::Count)" in save:
    problems.append("SAVE: all-7-sections rejected")

# Terminal: full/delta/unchanged frame encoding must exist (HK-5 closure).
# Either an inline color-run path or the AnsiFrameEncoder satisfies this.
term = text("src/platform/windows/win_terminal.cpp")
enc = text("include/writeover/render/frame_encoder.h")
if "AnsiFrameEncoder" in term or "AnsiFrameEncoder" in enc:
    if "Encode" not in enc:
        problems.append("TERMINAL: frame encoder declared but Encode missing")
elif "AnsiTrueColorBackend" in term and "color-run compression" not in term:
    problems.append("TERMINAL: ANSI path missing color-run compression or frame encoder")

# Raycaster: floor-drop and ceiling-rise boundaries must be explicit.
ray = text("src/render/raycaster.cpp")
if "far_cell.floor_z < near_cell.floor_z" not in ray:
    problems.append("RAY: floor-drop boundary not explicitly handled")
if "SegFloorDrop" not in ray:
    problems.append("RAY: SegFloorDrop flag missing")

# Controller: GroundProbe must exist and guard against jump re-grounding.
ctrl = text("src/player/controller.cpp")
if "GroundProbe" not in ctrl:
    problems.append("CONTROLLER: no explicit GroundProbe")
if "!(ls.velocity.z > kEpsVelocity)" not in ctrl:
    problems.append("CONTROLLER: ground probe does not guard jump ascent")

# Controller: lean geometry clamp must reference kLeanOffset.
if "LeanClamp" not in ctrl:
    problems.append("CONTROLLER: no LeanClamp geometry helper")

# Storylet content/runtime split: fired state read must be guarded so pure
# content files (no runtime state) load cleanly.
story = text("src/narrative/storylet.cpp")
if "const uint32_t fired_count = d.ReadU32()" in story and "d.Remaining()" not in story:
    problems.append("STORYLET: fired-state read not guarded (content/runtime conflated)")

# Benchmark: field must be honestly named (worst_1pct, not p99).
bench_h = text("include/writeover/render/benchmark.h")
if "p99_ms" in bench_h:
    problems.append("BENCH: misleading p99_ms field name still present")
if "worst_1pct_avg_ms" not in bench_h:
    problems.append("BENCH: worst_1pct_avg_ms field missing")

print("STATIC_AUDIT")
for p in problems:
    print(" -", p)
print(f"COUNT={len(problems)}")
sys.exit(1 if problems else 0)
