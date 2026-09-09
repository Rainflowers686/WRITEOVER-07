#!/usr/bin/env python3
"""Content compiler validation tests (content.*).

Runs against tools/contentc/contentc.py directly (imported, not subprocess).
Exit 0 = all pass, exit 1 = failure. Added for the final Foundation closure
(content reference validation / stable ID collision / light=0 preservation).
"""
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import contentc as cc

FAILURES = []


def check(name, ok):
    print(f"[{'PASS' if ok else 'FAIL'}] content.{name}")
    if not ok:
        FAILURES.append(name)


def compile_to(tmp, room_json, facts_json=None, storylets_json=None,
               npcs_json=None, scene_json=None, inject_default=True):
    """Writes authoring JSON into tmp and runs _compile_all; returns False if
    any compile error was recorded."""
    rooms = tmp / "rooms"
    rooms.mkdir(parents=True, exist_ok=True)
    facts = tmp / "facts"
    facts.mkdir(parents=True, exist_ok=True)
    storylets = tmp / "storylets"
    storylets.mkdir(parents=True, exist_ok=True)
    npcs = tmp / "npcs"
    npcs.mkdir(parents=True, exist_ok=True)
    scenes = tmp / "scenes"
    scenes.mkdir(parents=True, exist_ok=True)
    text = tmp / "text"
    text.mkdir(parents=True, exist_ok=True)
    # _compile_all validates the same required production text table used by
    # the runtime.  Keep the fixture self-contained instead of accidentally
    # testing every content case against a missing shared resource.
    (text / "recovery_text.txt").write_text(
        "t1\tFixture storylet text.\n"
        "n1\tFixture narrator text.\n",
        encoding="utf-8",
    )
    # Small fixtures historically omit the explicit sparse-grid policy.  Keep
    # those legacy fixtures focused on their individual assertion, while a
    # dedicated regression below verifies that production authoring without a
    # defaultCell is rejected.
    room_for_test = dict(room_json)
    if inject_default:
        room_for_test.setdefault(
            "defaultCell",
            {"floor": 0.0, "ceiling": 4.0, "material": "wall",
             "light": 255, "flags": []})
    (rooms / "r1.json").write_text(json.dumps(room_for_test), encoding="utf-8")
    if facts_json is not None:
        (facts / "f1.json").write_text(json.dumps(facts_json), encoding="utf-8")
    if storylets_json is not None:
        (storylets / "s1.json").write_text(json.dumps(storylets_json), encoding="utf-8")
    if npcs_json is not None:
        (npcs / "n1.json").write_text(json.dumps(npcs_json), encoding="utf-8")
    if scene_json is not None:
        (scenes / "scene.json").write_text(json.dumps(scene_json), encoding="utf-8")
    cc.ERRORS = []
    cc._compile_all(tmp, tmp)
    return len(cc.ERRORS) == 0, list(cc.ERRORS)


def test_light_zero_preserved():
    # A room with an explicit light=0 cell must compile to a .woc whose cell
    # light byte is 0 (not coerced back to 255). The cell uses non-zero flags
    # (solid) so a wrong byte offset cannot pass the check.
    room = {
        "schemaVersion": 1, "gridWidth": 2, "gridHeight": 2,
        "cells": [{"col": 0, "row": 0, "light": 0, "flags": ["solid"]}],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, _ = compile_to(Path(tmp), room)
        if not ok:
            check("light_zero_preserved", False)
            return
        woc = Path(tmp) / "rooms" / "r1.woc"
        data = woc.read_bytes()
        # header 8 bytes; room payload: RoomId 8 + name-len4+name + w/h 8 + spawn 16
        # then cell count 4, then cells (ffBBB = 4+4+1+1+1 = 11 bytes each).
        off = 8
        off += 8  # RoomId
        name_len = struct.unpack_from("<I", data, off)[0]
        off += 4 + name_len
        off += 8  # w,h
        off += 16  # spawn
        count = struct.unpack_from("<I", data, off)[0]
        off += 4
        if count != 4:
            check("light_zero_preserved", False)
            return
        cell0 = data[off:off + 11]
        light = cell0[9]  # f(4)+f(4)+mat(1)+light(1)+flags(1) -> light at offset 9
        flags = cell0[10]
        check("light_zero_preserved", light == 0 and flags != 0)


def _first_fact_ref(storylets_bin: bytes):
    """Returns the numeric fact reference of the first fact condition in the
    first storylet of storylets.bin (header 8 bytes + storylet payload)."""
    off = 8  # magic + version header
    count = struct.unpack_from("<I", storylets_bin, off)[0]
    off += 4
    if count == 0:
        return None
    off += 8  # StoryletId
    text_len = struct.unpack_from("<I", storylets_bin, off)[0]
    off += 4 + text_len
    off += 3  # priority H(2) + once B(1)
    n_conds = struct.unpack_from("<I", storylets_bin, off)[0]
    off += 4
    for _ in range(n_conds):
        ctype = storylets_bin[off]
        off += 1
        if ctype == 0:  # fact condition: Q fact + B equals
            fact_ref = struct.unpack_from("<Q", storylets_bin, off)[0]
            off += 8 + 1
            return fact_ref
        elif ctype == 1:  # room: Q
            off += 8
        elif ctype == 2:  # npcstate: Q + B
            off += 9
        elif ctype == 3:  # frame: Q + Q
            off += 16
        elif ctype == 4:  # difficulty: B
            off += 1
        elif ctype == 5:  # flag: len4 + utf8
            flag_len = struct.unpack_from("<I", storylets_bin, off)[0]
            off += 4 + flag_len
    return None


def test_stable_ids_survive_insertion():
    # Real A/B compile: a storylet references fact "zeta". Compile with only
    # "zeta", record its numeric fact reference; then add alphabetically
    # earlier fact "alpha" and recompile. The "zeta" reference must be
    # identical (FNV-1a64 stable; the old sorted 1..N model would shift it).
    base_room = {"schemaVersion": 1, "gridWidth": 1, "gridHeight": 1}
    base_storylets = {
        "schemaVersion": 1,
        "storylets": [{
            "id": "s1", "textId": "t1", "priority": 1, "once": True,
            "conditions": [{"type": "fact", "fact": "zeta", "equals": True}],
            "actions": [{"type": "narrator", "textId": "n1", "persona": 0}],
        }],
    }

    def compile_ref(facts_json):
        with tempfile.TemporaryDirectory() as tmp:
            ok, _ = compile_to(Path(tmp), base_room, facts_json=facts_json,
                               storylets_json=base_storylets)
            if not ok:
                return None
            bin_path = Path(tmp) / "storylets" / "storylets.bin"
            if not bin_path.exists():
                return None
            return _first_fact_ref(bin_path.read_bytes())

    ref_a = compile_ref({"schemaVersion": 1, "facts": [{"id": "zeta"}]})
    ref_b = compile_ref({"schemaVersion": 1,
                         "facts": [{"id": "alpha"}, {"id": "zeta"}]})
    check("stable_ids_survive_insertion",
          ref_a is not None and ref_a == ref_b)


def test_unknown_npc_ref_rejected():
    room = {
        "schemaVersion": 1, "gridWidth": 1, "gridHeight": 1,
        "npcRefs": ["engineeer_01"],  # typo: not in the registry
    }
    npcs = {"schemaVersion": 1, "npcs": [{"id": "engineer_01"}]}
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, npcs_json=npcs)
        check("unknown_npc_ref_rejected", not ok and any("unknown npc ref" in e for e in errs))


def test_unknown_storylet_ref_rejected():
    room = {
        "schemaVersion": 1, "gridWidth": 1, "gridHeight": 1,
        "storyletRefs": ["storylet_missing"],
    }
    storylets = {"schemaVersion": 1, "storylets": [{"id": "storylet_real"}]}
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, storylets_json=storylets)
        check("unknown_storylet_ref_rejected", not ok and any("unknown storylet ref" in e for e in errs))


def test_hash_collision_detected():
    # Inject two different canonical strings mapping to the same numeric id
    # into the collision handler: must fail.
    cc.ERRORS = []
    cc.check_id_collisions("test", {"a": 42, "b": 42})
    check("hash_collision_detected", any("ID collision" in e for e in cc.ERRORS))
    cc.ERRORS = []


def test_invalid_npc_profile_rejected():
    room = {"schemaVersion": 1, "gridWidth": 1, "gridHeight": 1}
    npcs = {
        "schemaVersion": 1,
        "npcs": [{
            "id": "guard_bad", "cognition": "Light", "faction": "Security",
            "role": "Guard", "spawn": {"room": "r1", "x": 0.0, "y": 0.0, "yaw": 0.0},
            "health": 50, "isCritical": False,
            "perception": {"sightRange": 12.0, "sightFovRad": 2.09, "hearingRange": 8.0},
        }],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, npcs_json=npcs)
        check("invalid_npc_profile_rejected",
              not ok and any("cognition must be Full or SemiHuman" in e for e in errs))


def test_npc_profile_binary_emitted():
    room = {"schemaVersion": 1, "gridWidth": 1, "gridHeight": 1}
    npcs = {
        "schemaVersion": 1,
        "npcs": [{
            "id": "guard_01", "cognition": "SemiHuman", "faction": "Security",
            "role": "Guard",
            "spawn": {"room": "r1", "x": 1.0, "y": 2.0, "yaw": 0.5},
            "health": 100, "isCritical": False,
            "perception": {"sightRange": 12.0, "sightFovRad": 2.09,
                            "hearingRange": 8.0},
        }],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, _ = compile_to(Path(tmp), room, npcs_json=npcs)
        output = Path(tmp) / "npcs" / "npcs.bin"
        header = output.read_bytes() if output.exists() else b""
        parsed = struct.unpack_from("<III", header, 0) if len(header) >= 12 else ()
        check("npc_profile_binary_emitted",
              ok and parsed == (cc.NPC_MAGIC, cc.NPC_VERSION, 1))


def test_scene_spatial_validation():
    scene = {
        "schemaVersion": 1,
        "entities": [{
            "id": "terminal", "kind": "terminal", "room": "r1",
            "position": {"x": 0.5, "y": 0.5, "z": 0.0},
            "radius": 1.0, "height": 1.0, "visual": "terminal",
        }],
        "transitions": [{
            "id": "exit", "sourceRoom": "r1",
            "bounds": {"minX": 0.0, "maxX": 1.0, "minY": 0.0, "maxY": 1.0},
            "destinationRoom": "r1",
            "destinationSpawn": {"x": 1.5, "y": 1.5, "z": 0.0},
        }],
        "patrolRoutes": [],
    }
    room = {
        "schemaVersion": 1, "gridWidth": 2, "gridHeight": 2,
        "cells": [{"col": 1, "row": 1, "flags": ["solid"]}],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, scene_json=scene)
        check("scene_spatial_validation",
              not ok and any("destinationSpawn is inside a solid cell" in e
                             for e in errs))


def test_sparse_room_requires_default_policy():
    room = {"schemaVersion": 1, "gridWidth": 2, "gridHeight": 2,
            "cells": [{"col": 0, "row": 0, "flags": []}]}
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, inject_default=False)
        check("sparse_room_requires_default_policy",
              not ok and any("requires explicit defaultCell policy" in e
                             for e in errs))


def test_duplicate_room_cell_rejected():
    room = {
        "schemaVersion": 1, "gridWidth": 1, "gridHeight": 1,
        "cells": [
            {"col": 0, "row": 0, "flags": ["solid"]},
            {"col": 0, "row": 0, "flags": []},
        ],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room)
        check("duplicate_room_cell_rejected",
              not ok and any("duplicate cell definition (0,0)" in e
                             for e in errs))


def test_nonfinite_room_geometry_rejected():
    room = {
        "schemaVersion": 1, "gridWidth": 1, "gridHeight": 1,
        "cells": [{"col": 0, "row": 0, "floor": float("nan"),
                   "flags": []}],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room)
        check("nonfinite_room_geometry_rejected",
              not ok and any("floor and ceiling must be finite" in e
                             for e in errs))


def test_malformed_room_values_report_content_error():
    room = {
        "schemaVersion": 1, "gridWidth": 1, "gridHeight": 1,
        "cells": [{"col": 0, "row": 0, "light": "bright", "flags": []}],
    }
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room)
        check("malformed_room_values_report_content_error",
              not ok and any("light must be an integer" in e for e in errs))


def test_patrol_point_entity_overlap_rejected():
    scene = {
        "schemaVersion": 1,
        "entities": [{
            "id": "cart", "kind": "cart", "room": "r1",
            "position": {"x": 1.0, "y": 1.0, "z": 0.0},
            "radius": 1.0, "height": 1.0, "visual": "crate",
        }],
        "transitions": [],
        "patrolRoutes": [{
            "npcRef": "npc", "room": "r1",
            "points": [{"x": 1.0, "y": 1.0, "z": 0.0}],
        }],
    }
    room = {
        "schemaVersion": 1, "gridWidth": 3, "gridHeight": 3,
        "cells": [],
        "defaultCell": {"floor": 0.0, "ceiling": 4.0,
                         "material": "concrete", "light": 100, "flags": []},
    }
    npcs = {"schemaVersion": 1, "npcs": [{"id": "npc"}]}
    with tempfile.TemporaryDirectory() as tmp:
        ok, errs = compile_to(Path(tmp), room, npcs_json=npcs,
                              scene_json=scene)
        check("patrol_point_entity_overlap_rejected",
              not ok and any("overlaps placed entity 'cart' footprint" in e
                             for e in errs))


if __name__ == "__main__":
    test_light_zero_preserved()
    test_stable_ids_survive_insertion()
    test_unknown_npc_ref_rejected()
    test_unknown_storylet_ref_rejected()
    test_hash_collision_detected()
    test_invalid_npc_profile_rejected()
    test_npc_profile_binary_emitted()
    test_scene_spatial_validation()
    test_sparse_room_requires_default_policy()
    test_duplicate_room_cell_rejected()
    test_nonfinite_room_geometry_rejected()
    test_malformed_room_values_report_content_error()
    test_patrol_point_entity_overlap_rejected()
    print(f"{13 - len(FAILURES)}/13 content tests passed")
    sys.exit(1 if FAILURES else 0)
