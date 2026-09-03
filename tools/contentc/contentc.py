#!/usr/bin/env python3
"""WRITEOVER-07 content compiler (authoring JSON -> compiled binary).

Runtime C++ reads ONLY the compiled binaries; JSON is never parsed at runtime
(closure of M-015). The compiler is stdlib-only (json + struct) and
deterministic: stable numeric IDs use FNV-1a64 over the canonical string id,
so identical inputs always produce identical binaries independent of insertion
order. Collision-checked per run.

Binary formats mirror writeover C++ loaders exactly:
  rooms/<room>.woc   -> RoomCodec (writeover/world/room.h)
  storylets/storylets.bin -> StoryletEngine::Load (narrative/storylet.h)

Facts compile to facts/facts.bin as a validation artifact; the runtime
FactStore is seeded by world systems (smoke path verifiable via mapc).

Usage:
  python contentc.py --data-dir data [--out-dir data] [--check]
"""
import argparse
import json
import struct
import sys
from pathlib import Path

WOC_MAGIC = 0x574F4331  # "WOC1"
WOC_VERSION = 1

ERRORS = []


def fail(path, message):
    ERRORS.append(f"{path}: {message}")


def utf8(text: str) -> bytes:
    encoded = text.encode("utf-8")
    return struct.pack("<I", len(encoded)) + encoded


# FNV-1a 64-bit: stable deterministic ID derived from the canonical string id.
# Independent of insertion order / sorted position, so adding earlier content
# never shifts existing ids (Issue D.2 closure). Collision-checked per run.
def stable_id64(name: str) -> int:
    h = 0xCBF29CE484222325
    for b in name.encode("utf-8"):
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


def stable_id(name: str, registry):
    """Maps a string id to its stable deterministic numeric id."""
    if name not in registry:
        fail("id-registry", f"unknown reference '{name}' (registered: {sorted(registry)[:8]}...)")
        return 0
    return registry[name]


def check_id_collisions(domain: str, registry):
    """Fails on FNV-1a64 collision within one id domain (Issue E.2 closure):
    two distinct canonical strings mapping to the same numeric id are a hard
    error. Deterministic; never silently resolves."""
    numeric_to_string = {}
    for name in sorted(registry):
        numeric_id = registry[name]
        if numeric_id in numeric_to_string:
            other = numeric_to_string[numeric_id]
            if other != name:
                fail(domain,
                     f"ID collision: '{other}' and '{name}' both map to "
                     f"{numeric_id}")
        else:
            numeric_to_string[numeric_id] = name


def load_id_registry(domain: str, paths, id_key="id"):
    """Loads a deterministic {string_id: stable_id64} registry from JSON files.
    Returns {} on malformed input (errors are collected globally)."""
    registry = {}
    for path in sorted(paths):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as exc:
            fail(path.name, f"invalid JSON: {exc}")
            continue
        entries = data.get("npcs", data.get("storylets", data.get("facts", [])))
        for entry in sorted(entries, key=lambda k: k.get(id_key, "")):
            eid = entry.get(id_key, "")
            if eid and eid not in registry:
                registry[eid] = stable_id64(eid)
    return registry


def compile_room(json_path: Path, out_dir: Path, npc_registry, storylet_registry):
    try:
        data = json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(json_path.name, f"invalid JSON: {exc}")
        return

    schema = data.get("schemaVersion", 0)
    if schema != 1:
        fail(json_path.name, f"expected schemaVersion 1, got {schema}")
        return

    name = data.get("displayName", data.get("id", "room"))
    w = int(data.get("gridWidth", 0))
    h = int(data.get("gridHeight", 0))
    if w <= 0 or h <= 0 or w > 64 or h > 64:
        fail(json_path.name, "gridWidth/gridHeight must be 1..64")
        return

    cell_map = {}
    for c in data.get("cells", []):
        cell_map[(int(c["col"]), int(c["row"]))] = c

    materials = ["wall", "metal", "glass", "dirt", "concrete",
                 "wood", "grate", "hazard"]
    flags_map = {"solid": 1, "door": 2, "breakable": 4, "special": 8}

    body = bytearray()
    # Stable RoomId from FNV-1a64 over the canonical room id string.
    # Deterministic independent of file order (Issue D.2 closure).
    room_str_id = json_path.stem
    room_id = stable_id64(room_str_id)
    body += struct.pack("<Q", room_id)
    body += utf8(name)
    body += struct.pack("<ii", w, h)
    spawn = data.get("spawnPoint", {"x": 1.5, "y": 1.5, "z": 0.0, "yaw": 0.0})
    body += struct.pack("<ffff", float(spawn["x"]), float(spawn["y"]),
                        float(spawn["z"]), float(spawn.get("yaw", 0.0)))
    body += struct.pack("<I", w * h)
    for row in range(h):
        for col in range(w):
            cell = cell_map.get((col, row), {})
            floor = float(cell.get("floor", 0.0))
            ceiling = float(cell.get("ceiling", 4.0))
            material = materials.index(cell.get("material", "wall")) \
                if cell.get("material") in materials else 0
            # Issue D.3: light=0 must be preserved (all-black / unpowered),
            # so we must NOT apply `or 255` to an explicit 0.
            light_raw = cell.get("light", 255)
            light = 255 if light_raw is None else int(light_raw)
            flags = 0
            for f in cell.get("flags", []):
                flags |= flags_map.get(f, 0)
            body += struct.pack("<ffBBB", floor, ceiling, material, light, flags)
    # Issue E.1: NPC / storylet refs must resolve to real registry entries.
    # A typo in a stable string id must be a compile error, not a silent hash.
    npc_refs = []
    for ref in data.get("npcRefs", []):
        if ref not in npc_registry:
            fail(json_path.name, f"unknown npc ref '{ref}'")
            continue
        npc_refs.append(stable_id64(ref))
    storylet_refs = []
    for ref in data.get("storyletRefs", []):
        if ref not in storylet_registry:
            fail(json_path.name, f"unknown storylet ref '{ref}'")
            continue
        storylet_refs.append(stable_id64(ref))
    body += struct.pack("<I", len(npc_refs))
    for ref in npc_refs:
        body += struct.pack("<Q", ref)
    body += struct.pack("<I", len(storylet_refs))
    for ref in storylet_refs:
        body += struct.pack("<Q", ref)

    out = out_dir / "rooms" / (json_path.stem + ".woc")
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "wb") as fh:
        fh.write(struct.pack("<II", WOC_MAGIC, WOC_VERSION))
        fh.write(bytes(body))
    print(f"room: {json_path.name} -> {out.name} ({len(body)} bytes)")


def compile_facts(json_path: Path, out_dir: Path):
    try:
        data = json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(json_path.name, f"invalid JSON: {exc}")
        return
    facts = []
    for fact in data.get("facts", []):
        fid = fact.get("id", "")
        if not fid:
            fail(json_path.name, "fact without id")
            continue
        facts.append(fid)
    if len(set(facts)) != len(facts):
        fail(json_path.name, "duplicate fact ids")
        return
    body = bytearray()
    body += struct.pack("<I", len(facts))
    for fid in sorted(facts):
        body += utf8(fid)
        body += struct.pack("<B", 0)  # predicate State
    out = out_dir / "facts" / "facts.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "wb") as fh:
        fh.write(struct.pack("<II", WOC_MAGIC, WOC_VERSION))
        fh.write(bytes(body))
    print(f"facts: {len(facts)} -> {out.name}")


CONDITION_TYPES = {
    "fact": 0, "room": 1, "npcstate": 2, "frame": 3, "difficulty": 4, "flag": 5,
}
ACTION_TYPES = {
    "narrator": 0, "dialog": 1, "worldcommand": 2, "endgame": 3,
}


def compile_storylets(json_path: Path, out_dir: Path, fact_registry):
    try:
        data = json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(json_path.name, f"invalid JSON: {exc}")
        return
    storylets = data.get("storylets", [])
    body = bytearray()
    body += struct.pack("<I", len(storylets))
    for s in sorted(storylets, key=lambda k: k.get("id", "")):
        sid = s.get("id", "")
        body += struct.pack("<Q", stable_id64(sid))            # StoryletId (stable FNV-1a64)
        body += utf8(s.get("textId", s.get("id", "")))      # text id
        body += struct.pack("<HB", int(s.get("priority", 0)),
                            1 if s.get("once", True) else 0)
        conds = s.get("conditions", [])
        body += struct.pack("<I", len(conds))
        for c in conds:
            ctype = c.get("type", "frame")
            if ctype not in CONDITION_TYPES:
                fail(json_path.name, f"unknown condition type '{ctype}'")
                continue
            body += struct.pack("<B", CONDITION_TYPES[ctype])
            if ctype == "fact":
                fact = c.get("fact", "")
                body += struct.pack("<QB", stable_id(fact, fact_registry),
                                    1 if c.get("equals", True) else 0)
            elif ctype == "room":
                body += struct.pack("<Q", int(c.get("roomId", 0)))
            elif ctype == "npcstate":
                body += struct.pack("<QB", int(c.get("npcId", 0)),
                                    int(c.get("state", 0)))
            elif ctype == "frame":
                body += struct.pack("<QQ", int(c.get("minFrame", 0)),
                                    int(c.get("maxFrame", 0)))
            elif ctype == "difficulty":
                body += struct.pack("<B", int(c.get("minLevel", 0)))
            elif ctype == "flag":
                body += utf8(str(c.get("flag", "")))
        acts = s.get("actions", [])
        body += struct.pack("<I", len(acts))
        for a in acts:
            atype = a.get("type", "narrator")
            if atype not in ACTION_TYPES:
                fail(json_path.name, f"unknown action type '{atype}'")
                continue
            body += struct.pack("<B", ACTION_TYPES[atype])
            if atype == "narrator":
                body += utf8(a.get("textId", ""))
                body += struct.pack("<B", int(a.get("persona", 0)))
            elif atype == "dialog":
                body += utf8(a.get("textId", ""))
            elif atype == "worldcommand":
                cmd = a.get("command", a.get("cmd", "interact"))
                if cmd == "interact":
                    body += struct.pack("<B", 7)  # CommandInteract
                elif cmd == "usecheckpoint":
                    body += struct.pack("<B", 10)  # CommandUseCheckpoint
                elif cmd == "setdoor":
                    body += struct.pack("<B", 9)  # CommandSetDoor
                    body += struct.pack("<Q", int(a.get("doorId", 0)))
                    body += struct.pack("<B", 1 if a.get("open", True) else 0)
                elif cmd == "setpower":
                    body += struct.pack("<B", 8)  # CommandSetPower
                    body += struct.pack("<Q", int(a.get("systemId", 0)))
                    body += struct.pack("<B", 1 if a.get("powered", True) else 0)
                else:
                    fail(json_path.name, f"unknown world command '{cmd}'")
            elif atype == "endgame":
                body += struct.pack("<B", int(a.get("ending", 0)))
    out = out_dir / "storylets" / "storylets.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    with open(out, "wb") as fh:
        fh.write(struct.pack("<II", WOC_MAGIC, WOC_VERSION))
        fh.write(bytes(body))
    print(f"storylets: {len(storylets)} -> {out.name} ({len(body)} bytes)")


def main():
    parser = argparse.ArgumentParser(description="WRITEOVER-07 content compiler")
    parser.add_argument("--data-dir", default="data", help="authoring JSON root")
    parser.add_argument("--out-dir", default="data", help="compiled output root")
    parser.add_argument("--check", action="store_true",
                        help="verify deterministic recompile (CI use)")
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    out_dir = Path(args.out_dir)

    # --check: compile to a fresh temp dir and byte-compare against the
    # existing compiled outputs (F-10 closure: detects schema drift and
    # non-deterministic compilation).
    check_errors = 0
    if args.check:
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            tmp_dir = Path(tmp)
            _compile_all(data_dir, tmp_dir)
            if ERRORS:
                for err in ERRORS:
                    print(f"CONTENT ERROR: {err}", file=sys.stderr)
                sys.exit(1)
            for produced in sorted(tmp_dir.rglob("*")):
                if not produced.is_file():
                    continue
                existing = out_dir / produced.relative_to(tmp_dir)
                if not existing.exists():
                    print(f"CHECK ERROR: {existing} missing (schema drift?)",
                          file=sys.stderr)
                    check_errors += 1
                    continue
                if produced.read_bytes() != existing.read_bytes():
                    print(f"CHECK ERROR: {existing} differs from recompile "
                          f"(non-deterministic or schema drift)",
                          file=sys.stderr)
                    check_errors += 1
        if check_errors:
            sys.exit(1)
        print(f"contentc --check: OK (deterministic recompile matches)")
        sys.exit(0)

    _compile_all(data_dir, out_dir)
    if ERRORS:
        for err in ERRORS:
            print(f"CONTENT ERROR: {err}", file=sys.stderr)
        sys.exit(1)
    room_files = sorted(data_dir.glob("rooms/*.json"))
    fact_files = sorted(data_dir.glob("facts/*.json"))
    storylet_files = sorted(data_dir.glob("storylets/*.json"))
    print(f"contentc: OK ({len(room_files)} rooms, {len(fact_files)} fact files, "
          f"{len(storylet_files)} storylet files)")
    sys.exit(0)


def _compile_all(data_dir: Path, out_dir: Path):
    """Compiles every authoring JSON under data_dir into out_dir."""
    global ERRORS
    ERRORS = []
    room_files = sorted(data_dir.glob("rooms/*.json"))
    fact_files = sorted(data_dir.glob("facts/*.json"))
    storylet_files = sorted(data_dir.glob("storylets/*.json"))
    npc_files = sorted(data_dir.glob("npcs/*.json"))

    # Deterministic stable id registries per domain (FNV-1a64 over string id),
    # then collision-checked per domain (Issue E.2).
    fact_registry = load_id_registry("facts", fact_files)
    storylet_registry = load_id_registry("storylets", storylet_files)
    npc_registry = load_id_registry("npcs", npc_files)
    check_id_collisions("fact", fact_registry)
    check_id_collisions("storylet", storylet_registry)
    check_id_collisions("npc", npc_registry)
    # Room ids derive from the file stem; collision-check across rooms too.
    room_registry = {p.stem: stable_id64(p.stem) for p in room_files}
    check_id_collisions("room", room_registry)

    for path in room_files:
        compile_room(path, out_dir, npc_registry, storylet_registry)
    for path in fact_files:
        compile_facts(path, out_dir)
    for path in storylet_files:
        compile_storylets(path, out_dir, fact_registry)


if __name__ == "__main__":
    main()