#!/usr/bin/env python3
"""WRITEOVER-07 content compiler (authoring JSON -> compiled binary).

Runtime C++ reads ONLY the compiled binaries; JSON is never parsed at runtime
(closure of M-015). NPC profiles are emitted as `npcs/npcs.bin` and loaded by
the production adapter. The compiler is stdlib-only (json + struct) and
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
import math
import os
import struct
import sys
import tempfile
from pathlib import Path

WOC_MAGIC = 0x574F4331  # "WOC1"
WOC_VERSION = 1
NPC_MAGIC = 0x574E5043  # "WNPC"
NPC_VERSION = 1
SCENE_MAGIC = 0x57534331  # "WSC1"
SCENE_VERSION = 1

ERRORS = []

NPC_COGNITION = {"Full", "SemiHuman"}
NPC_FACTIONS = {"GeneralStaff", "Security", "Medical", "Research",
                 "Maintenance", "Executive", "Detained", "Civilian"}
NPC_ROLES = {"Guard", "Cleaner", "Doctor", "Researcher", "Technician",
             "Administrator", "Executive", "Detained", "Civilian", "Other"}
NPC_LEGACY_CLASSES = {"Full", "SemiHuman", "Guard"}
NPC_FACTION_IDS = {
    "GeneralStaff": 0, "Security": 1, "Medical": 2, "Research": 3,
    "Maintenance": 4, "Executive": 5, "Detained": 6, "Civilian": 7,
}
NPC_ROLE_IDS = {
    "Guard": 0, "Cleaner": 1, "Doctor": 2, "Researcher": 3,
    "Technician": 4, "Administrator": 5, "Executive": 6, "Detained": 7,
    "Civilian": 8, "Other": 9,
}
NPC_COGNITION_IDS = {"Full": 0, "SemiHuman": 1}

SCENE_KINDS = {"cart": 0, "camera": 1, "terminal": 2,
               "door_reader": 3, "door": 4, "crate": 5}
SCENE_VISUALS = {
    "security_guard": 0, "full_human": 1, "maintenance_worker": 2,
    "terminal": 3, "camera": 4, "crate": 5, "door": 6,
    "body_unconscious": 7, "body_dead": 8,
}


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
        if not isinstance(entries, list):
            fail(path.name, f"{domain} entries must be a list")
            continue
        for entry in sorted(entries, key=lambda k: k.get(id_key, "")
                             if isinstance(k, dict) else ""):
            if not isinstance(entry, dict):
                continue
            eid = entry.get(id_key, "")
            if eid and eid not in registry:
                registry[eid] = stable_id64(eid)
    return registry


def validate_npc_file(path: Path, seen_ids, room_ids=None):
    """Validate the runtime-facing NPC registry without compiling a second
    NPC binary. Identity is loaded from the systemic seed; this file owns the
    authored spawn/profile seam used by rooms and the PVS runtime adapter."""
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(path.name, f"invalid JSON: {exc}")
        return
    if data.get("schemaVersion") != 1:
        fail(path.name, "expected schemaVersion 1")
    entries = data.get("npcs", [])
    if not isinstance(entries, list):
        fail(path.name, "npcs must be a list")
        return
    for index, npc in enumerate(entries):
        item_path = f"{path.name}:npcs[{index}]"
        if not isinstance(npc, dict):
            fail(item_path, "entry must be an object")
            continue
        npc_id = npc.get("id")
        if not isinstance(npc_id, str) or not npc_id:
            fail(item_path, "id is required")
            continue
        if npc_id in seen_ids:
            fail(item_path, f"duplicate npc id {npc_id}")
        seen_ids.add(npc_id)
        legacy = npc.get("class")
        if legacy is not None and legacy not in NPC_LEGACY_CLASSES:
            fail(item_path, f"bad legacy class {legacy}")
        cognition = npc.get("cognition")
        if cognition not in NPC_COGNITION:
            fail(item_path, "cognition must be Full or SemiHuman")
        if npc.get("faction") not in NPC_FACTIONS:
            fail(item_path, "bad faction")
        if npc.get("role") not in NPC_ROLES:
            fail(item_path, "bad role")
        spawn = npc.get("spawn")
        if not isinstance(spawn, dict) or not isinstance(spawn.get("room"), str) or not spawn["room"]:
            fail(item_path, "spawn.room is required")
        else:
            if room_ids is not None and spawn["room"] not in room_ids:
                fail(item_path, f"unknown spawn.room '{spawn['room']}'")
            for field in ("x", "y", "yaw"):
                value = spawn.get(field, 0.0)
                if not isinstance(value, (int, float)) or not math.isfinite(float(value)):
                    fail(item_path, f"spawn.{field} must be finite")
        health = npc.get("health", 100)
        if not isinstance(health, int) or isinstance(health, bool) or not 1 <= health <= 1000:
            fail(item_path, "health must be an integer in 1..1000")
        if not isinstance(npc.get("isCritical", False), bool):
            fail(item_path, "isCritical must be bool")
        perception = npc.get("perception", {})
        if not isinstance(perception, dict):
            fail(item_path, "perception must be an object")
        else:
            ranges = (("sightRange", 0.0, 64.0),
                      ("sightFovRad", 0.0, 6.2832),
                      ("hearingRange", 0.0, 64.0))
            for field, lower, upper in ranges:
                value = perception.get(field, 0.0)
                if (not isinstance(value, (int, float)) or
                        not math.isfinite(float(value)) or
                        not lower <= float(value) <= upper):
                    fail(item_path, f"perception.{field} must be finite in {lower}..{upper}")


def validate_npc_seed_parity(data_dir: Path, npc_ids):
    """Keep the authored spawn/profile registry and systemic identity seed
    from silently describing different NPC populations."""
    seed_path = data_dir / "systemic" / "systemic_seed.json"
    if not seed_path.exists():
        return
    try:
        seed = json.loads(seed_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(seed_path.name, f"invalid JSON: {exc}")
        return
    actors = seed.get("actors", [])
    if not isinstance(actors, list):
        fail(seed_path.name, "actors must be a list")
        return
    actor_map = {actor.get("id"): actor for actor in actors
                 if isinstance(actor, dict) and isinstance(actor.get("id"), str)}
    for npc_id in sorted(npc_ids):
        if npc_id not in actor_map:
            fail("npcs/systemic parity", f"NPC '{npc_id}' has no systemic actor")
    for actor_id in sorted(actor_map):
        if actor_id not in npc_ids:
            fail("npcs/systemic parity", f"systemic actor '{actor_id}' has no NPC profile")


def compile_npc_profiles(npc_paths, out_dir: Path):
    """Compile validated NPC spawn/profile data for the production runtime.

    The runtime consumes this bounded binary, never the authoring JSON. The
    stable string IDs match the systemic actor seed and room file IDs.
    """
    entries = []
    for path in sorted(npc_paths):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        for npc in data.get("npcs", []):
            if isinstance(npc, dict) and isinstance(npc.get("id"), str):
                entries.append(npc)
    entries.sort(key=lambda npc: npc["id"])
    body = bytearray(struct.pack("<II", NPC_MAGIC, NPC_VERSION))
    body += struct.pack("<I", len(entries))
    for npc in entries:
        spawn = npc["spawn"]
        perception = npc.get("perception", {})
        body += struct.pack(
            "<QQQBBBffffHBfff",
            stable_id64(npc["id"]),
            stable_id64(npc["id"]),
            stable_id64(spawn["room"]),
            NPC_COGNITION_IDS[npc["cognition"]],
            NPC_FACTION_IDS[npc["faction"]],
            NPC_ROLE_IDS[npc["role"]],
            float(spawn.get("x", 0.0)),
            float(spawn.get("y", 0.0)),
            float(spawn.get("z", 0.0)),
            float(spawn.get("yaw", 0.0)),
            int(npc.get("health", 100)),
            1 if npc.get("isCritical", False) else 0,
            float(perception.get("sightRange", 0.0)),
            float(perception.get("sightFovRad", 0.0)),
            float(perception.get("hearingRange", 0.0)),
        )
    out = out_dir / "npcs" / "npcs.bin"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(body)
    print(f"npcs: {len(entries)} -> {out.name} ({len(body)} bytes)")


def _finite_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(float(value))


def load_text_resource_ids(data_dir: Path):
    """Load the bounded UTF-8 text table used by the recovery runtime.

    The runtime owns display lookup, while contentc owns the authoring-time
    cross-reference check. Keeping this as a plain table avoids introducing a
    second asset engine for the small recovery slice.
    """
    path = data_dir / "text" / "recovery_text.txt"
    if not path.exists():
        fail(path.name, "required recovery text resource is missing")
        return set()
    ids = set()
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        fail(path.name, f"cannot read text resource: {exc}")
        return set()
    if len(lines) > 512:
        fail(path.name, "text resource exceeds 512 lines")
    for index, line in enumerate(lines, 1):
        if not line or line.startswith("#"):
            continue
        if "\t" not in line:
            fail(f"{path.name}:{index}", "expected id<TAB>text")
            continue
        text_id, display = line.split("\t", 1)
        if not text_id or len(text_id) > 128 or not display or len(display) > 512:
            fail(f"{path.name}:{index}", "text id/display is outside bounds")
            continue
        if text_id in ids:
            fail(f"{path.name}:{index}", f"duplicate text id '{text_id}'")
        ids.add(text_id)
    return ids


def validate_storylet_text_refs(storylet_paths, text_ids):
    for path in storylet_paths:
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        for index, storylet in enumerate(data.get("storylets", [])):
            if not isinstance(storylet, dict):
                continue
            text_id = storylet.get("textId", "")
            if text_id and text_id not in text_ids:
                fail(f"{path.name}:storylets[{index}]",
                     f"missing text resource '{text_id}'")
            for action_index, action in enumerate(storylet.get("actions", [])):
                if not isinstance(action, dict) or action.get("type") not in ("narrator", "dialog"):
                    continue
                action_text_id = action.get("textId", "")
                if action_text_id not in text_ids:
                    fail(f"{path.name}:storylets[{index}].actions[{action_index}]",
                         f"missing text resource '{action_text_id}'")


def _scene_vec(entry, path, label):
    if not isinstance(entry, dict):
        fail(path, f"{label} must be an object")
        return (0.0, 0.0, 0.0)
    values = []
    for key in ("x", "y", "z"):
        value = entry.get(key, 0.0)
        if not _finite_number(value):
            fail(path, f"{label}.{key} must be finite")
        values.append(float(value) if _finite_number(value) else 0.0)
    return tuple(values)


def _scene_int(entry, key, path, default=0):
    value = entry.get(key, default)
    if not isinstance(value, int) or isinstance(value, bool) or value < 0:
        fail(path, f"{key} must be a non-negative integer")
        return default
    return value


def _load_room_specs(room_paths):
    """Load the small amount of room geometry needed for authoring checks.

    This is compile-time validation only.  The runtime still consumes the
    compiled room and scene binaries; the compiler uses the authored grid to
    reject placements that could make a link or patrol point impossible.
    """
    specs = {}
    for room_path in room_paths:
        try:
            data = json.loads(room_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        width = data.get("gridWidth")
        height = data.get("gridHeight")
        if (not isinstance(width, int) or isinstance(width, bool) or
                not isinstance(height, int) or isinstance(height, bool) or
                width <= 0 or height <= 0):
            continue
        cells = {}
        for cell in data.get("cells", []):
            if not isinstance(cell, dict):
                continue
            col = cell.get("col")
            row = cell.get("row")
            if (isinstance(col, int) and not isinstance(col, bool) and
                    isinstance(row, int) and not isinstance(row, bool) and
                    0 <= col < width and 0 <= row < height):
                # Keep the first definition for spatial diagnostics.  The
                # authoritative room compiler rejects duplicates below; this
                # prevents a malformed file from changing scene validation by
                # last-write-wins behaviour before that error is reported.
                cells.setdefault((col, row), cell)
        specs[room_path.stem] = {
            "width": width,
            "height": height,
            "cells": cells,
            "default_cell": data.get("defaultCell"),
        }
    return specs


def _scene_room_point_status(room_spec, point):
    if room_spec is None:
        return "unknown"
    x, y, _ = point
    if not (0.0 <= x < room_spec["width"] and
            0.0 <= y < room_spec["height"]):
        return "outside"
    cell = room_spec["cells"].get(
        (int(math.floor(x)), int(math.floor(y))),
        room_spec.get("default_cell") or {})
    flags = cell.get("flags", [])
    if isinstance(flags, list) and ("solid" in flags or "door" in flags):
        return "solid"
    return "walkable"


def _validate_scene_point(room_spec, point, path, label, require_walkable):
    status = _scene_room_point_status(room_spec, point)
    if status == "outside":
        fail(path, f"{label} is outside room bounds")
    elif status == "solid" and require_walkable:
        fail(path, f"{label} is inside a solid cell")


def validate_scene_file(path: Path, room_ids, npc_registry, room_specs=None):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(path.name, f"invalid JSON: {exc}")
        return None
    if data.get("schemaVersion") != 1:
        fail(path.name, "expected schemaVersion 1")
    entities = data.get("entities", [])
    transitions = data.get("transitions", [])
    routes = data.get("patrolRoutes", [])
    if not isinstance(entities, list) or len(entities) > 256:
        fail(path.name, "entities must be a list with at most 256 entries")
        entities = []
    if not isinstance(transitions, list) or len(transitions) > 128:
        fail(path.name, "transitions must be a list with at most 128 entries")
        transitions = []
    if not isinstance(routes, list) or len(routes) > 64:
        fail(path.name, "patrolRoutes must be a list with at most 64 entries")
        routes = []
    seen = set()
    entity_footprints = []
    for index, entity in enumerate(entities):
        p = f"{path.name}:entities[{index}]"
        if not isinstance(entity, dict):
            fail(p, "entry must be an object")
            continue
        eid = entity.get("id")
        if not isinstance(eid, str) or not eid:
            fail(p, "id is required")
        elif eid in seen:
            fail(p, f"duplicate id {eid}")
        else:
            seen.add(eid)
        room = entity.get("room")
        if room not in room_ids:
            fail(p, f"unknown room '{room}'")
        if entity.get("kind") not in SCENE_KINDS:
            fail(p, "kind is not a supported scene entity kind")
        if entity.get("visual") not in SCENE_VISUALS:
            fail(p, "visual is not a supported character visual")
        position = _scene_vec(entity.get("position"), p, "position")
        if room in room_ids and room_specs is not None:
            _validate_scene_point(room_specs.get(room), position, p,
                                  "position", False)
        for key in ("yaw", "radius", "height"):
            if not _finite_number(entity.get(key, 0.0)):
                fail(p, f"{key} must be finite")
        if float(entity.get("radius", 0.0)) <= 0.0 or float(entity.get("height", 0.0)) <= 0.0:
            fail(p, "radius and height must be positive")
        elif (room in room_ids and all(_finite_number(value) for value in position)):
            entity_footprints.append((room, eid or f"entity[{index}]",
                                      position[0], position[1],
                                      float(entity.get("radius", 0.0))))
        _scene_int(entity, "systemicId", p)
        _scene_int(entity, "linkId", p)
    transition_seen = set()
    for index, transition in enumerate(transitions):
        p = f"{path.name}:transitions[{index}]"
        if not isinstance(transition, dict):
            fail(p, "entry must be an object")
            continue
        tid = transition.get("id")
        if not isinstance(tid, str) or not tid:
            fail(p, "id is required")
        elif tid in transition_seen:
            fail(p, f"duplicate id {tid}")
        else:
            transition_seen.add(tid)
        source = transition.get("sourceRoom")
        dest = transition.get("destinationRoom")
        if source not in room_ids: fail(p, f"unknown sourceRoom '{source}'")
        if dest not in room_ids: fail(p, f"unknown destinationRoom '{dest}'")
        bounds = transition.get("bounds")
        if not isinstance(bounds, dict):
            fail(p, "bounds is required")
            bounds = {}
        for key in ("minX", "maxX", "minY", "maxY"):
            if not _finite_number(bounds.get(key)):
                fail(p, f"bounds.{key} must be finite")
        if (_finite_number(bounds.get("minX")) and _finite_number(bounds.get("maxX")) and
                float(bounds["minX"]) > float(bounds["maxX"])):
            fail(p, "bounds minX must not exceed maxX")
        if (_finite_number(bounds.get("minY")) and _finite_number(bounds.get("maxY")) and
                float(bounds["minY"]) > float(bounds["maxY"])):
            fail(p, "bounds minY must not exceed maxY")
        spawn = _scene_vec(transition.get("destinationSpawn"), p,
                           "destinationSpawn")
        if source in room_ids and room_specs is not None:
            source_spec = room_specs.get(source)
            if source_spec is not None and all(
                    _finite_number(bounds.get(key)) for key in
                    ("minX", "maxX", "minY", "maxY")):
                if (float(bounds["maxX"]) < 0.0 or
                        float(bounds["minX"]) > source_spec["width"] or
                        float(bounds["maxY"]) < 0.0 or
                        float(bounds["minY"]) > source_spec["height"]):
                    fail(p, "bounds do not intersect source room")
        if dest in room_ids and room_specs is not None:
            _validate_scene_point(room_specs.get(dest), spawn, p,
                                  "destinationSpawn", True)
        if not _finite_number(transition.get("destinationYaw", 0.0)):
            fail(p, "destinationYaw must be finite")
    route_seen = set()
    for index, route in enumerate(routes):
        p = f"{path.name}:patrolRoutes[{index}]"
        if not isinstance(route, dict):
            fail(p, "entry must be an object")
            continue
        npc = route.get("npcRef")
        if npc not in npc_registry: fail(p, f"unknown npcRef '{npc}'")
        if npc in route_seen: fail(p, f"duplicate npcRef '{npc}'")
        route_seen.add(npc)
        if route.get("room") not in room_ids: fail(p, "unknown room")
        points = route.get("points")
        if not isinstance(points, list) or not 1 <= len(points) <= 32:
            fail(p, "points must contain 1..32 entries")
        else:
            for point_index, point in enumerate(points):
                point_path = f"{p}:points[{point_index}]"
                point_value = _scene_vec(point, point_path, "point")
                if route.get("room") in room_ids and room_specs is not None:
                    _validate_scene_point(room_specs.get(route["room"]),
                                          point_value, point_path, "point", True)
                for entity_room, entity_id, entity_x, entity_y, entity_radius in entity_footprints:
                    if route.get("room") != entity_room:
                        continue
                    dx = point_value[0] - entity_x
                    dy = point_value[1] - entity_y
                    if dx * dx + dy * dy <= entity_radius * entity_radius:
                        fail(point_path,
                             f"patrol point overlaps placed entity '{entity_id}' footprint")
                        break
    return data


def compile_scene(json_path: Path, out_dir: Path, room_ids, npc_registry,
                  room_specs=None):
    data = validate_scene_file(json_path, room_ids, npc_registry, room_specs)
    if data is None:
        return
    entities = sorted(data.get("entities", []), key=lambda item: item.get("id", ""))
    transitions = sorted(data.get("transitions", []), key=lambda item: item.get("id", ""))
    routes = sorted(data.get("patrolRoutes", []), key=lambda item: item.get("npcRef", ""))
    body = bytearray(struct.pack("<II", SCENE_MAGIC, SCENE_VERSION))
    body += struct.pack("<I", len(entities))
    for entity in entities:
        position = entity.get("position", {})
        body += struct.pack("<QBB", stable_id64(entity["id"]),
                            SCENE_KINDS[entity["kind"]],
                            SCENE_VISUALS[entity["visual"]])
        body += utf8(entity["id"])
        body += utf8(entity["room"])
        body += struct.pack("<QQ", _scene_int(entity, "systemicId", json_path.name),
                            _scene_int(entity, "linkId", json_path.name))
        body += struct.pack("<ffffff", float(position.get("x", 0.0)),
                            float(position.get("y", 0.0)), float(position.get("z", 0.0)),
                            float(entity.get("yaw", 0.0)), float(entity.get("radius", 1.0)),
                            float(entity.get("height", 1.0)))
    body += struct.pack("<I", len(transitions))
    for transition in transitions:
        bounds = transition["bounds"]
        spawn = transition["destinationSpawn"]
        body += struct.pack("<Q", stable_id64(transition["id"]))
        body += utf8(transition["id"])
        body += utf8(transition["sourceRoom"])
        body += struct.pack("<ffff", float(bounds["minX"]), float(bounds["maxX"]),
                            float(bounds["minY"]), float(bounds["maxY"]))
        body += utf8(transition["destinationRoom"])
        body += struct.pack("<ffff", float(spawn.get("x", 0.0)), float(spawn.get("y", 0.0)),
                            float(spawn.get("z", 0.0)), float(transition.get("destinationYaw", 0.0)))
        body += utf8(transition.get("unavailableMessage", "Route unavailable."))
    body += struct.pack("<I", len(routes))
    for route in routes:
        body += struct.pack("<Q", stable_id64(route["npcRef"]))
        body += utf8(route["room"])
        points = route["points"]
        body += struct.pack("<I", len(points))
        for point in points:
            body += struct.pack("<fff", float(point.get("x", 0.0)),
                                float(point.get("y", 0.0)), float(point.get("z", 0.0)))
    out = out_dir / "scenes" / (json_path.stem + ".bin")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(body)
    print(f"scene: {json_path.name} -> {out.name} ({len(body)} bytes)")


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
    w_raw = data.get("gridWidth", 0)
    h_raw = data.get("gridHeight", 0)
    if (not isinstance(w_raw, int) or isinstance(w_raw, bool) or
            not isinstance(h_raw, int) or isinstance(h_raw, bool)):
        fail(json_path.name, "gridWidth/gridHeight must be integers")
        return
    w = w_raw
    h = h_raw
    if w <= 0 or h <= 0 or w > 64 or h > 64:
        fail(json_path.name, "gridWidth/gridHeight must be 1..64")
        return

    materials = ["wall", "metal", "glass", "dirt", "concrete",
                 "wood", "grate", "hazard"]
    flags_map = {"solid": 1, "door": 2, "breakable": 4, "special": 8}

    def validate_cell(cell, path, coordinates):
        if not isinstance(cell, dict):
            fail(path, "cell must be an object")
            return False
        if coordinates:
            col = cell.get("col")
            row = cell.get("row")
            if (not isinstance(col, int) or isinstance(col, bool) or
                    not isinstance(row, int) or isinstance(row, bool) or
                    not 0 <= col < w or not 0 <= row < h):
                fail(path, "col/row must be integers inside room bounds")
                return False
        floor = cell.get("floor", 0.0)
        ceiling = cell.get("ceiling", 4.0)
        if not _finite_number(floor) or not _finite_number(ceiling):
            fail(path, "floor and ceiling must be finite")
        elif float(ceiling) <= float(floor):
            fail(path, "ceiling must be greater than floor")
        material = cell.get("material", "wall")
        if material not in materials:
            fail(path, f"unknown material '{material}'")
        light = cell.get("light", 255)
        if (not isinstance(light, int) or isinstance(light, bool) or
                not 0 <= light <= 255):
            fail(path, "light must be an integer in 0..255")
        flags = cell.get("flags", [])
        if not isinstance(flags, list) or any(flag not in flags_map for flag in flags):
            fail(path, "flags must be a list of known values")
        return True

    cells = data.get("cells", [])
    if not isinstance(cells, list):
        fail(json_path.name, "cells must be a list")
        cells = []
    cell_map = {}
    for index, c in enumerate(cells):
        cell_path = f"{json_path.name}:cells[{index}]"
        if not validate_cell(c, cell_path, True):
            continue
        key = (c["col"], c["row"])
        if key in cell_map:
            fail(cell_path, f"duplicate cell definition ({key[0]},{key[1]})")
            continue
        cell_map[key] = c

    default_cell = data.get("defaultCell")
    if len(cell_map) < w * h and default_cell is None:
        fail(json_path.name,
             "sparse room requires explicit defaultCell policy")
    elif default_cell is not None:
        validate_cell(default_cell, f"{json_path.name}:defaultCell", False)

    spawn = data.get("spawnPoint", {"x": 1.5, "y": 1.5, "z": 0.0, "yaw": 0.0})
    if not isinstance(spawn, dict):
        fail(json_path.name, "spawnPoint must be an object")
        spawn = {"x": 0.0, "y": 0.0, "z": 0.0, "yaw": 0.0}
    for field in ("x", "y", "z", "yaw"):
        if not _finite_number(spawn.get(field, 0.0)):
            fail(json_path.name, f"spawnPoint.{field} must be finite")

    # Do not emit a partial artifact for a malformed room.  The top-level
    # compiler stages every artifact, so a failed compile cannot replace a
    # previously valid runtime binary.
    if any(error.startswith(f"{json_path.name}:") or error == json_path.name
           for error in ERRORS):
        return

    body = bytearray()
    # Stable RoomId from FNV-1a64 over the canonical room id string.
    # Deterministic independent of file order (Issue D.2 closure).
    room_str_id = json_path.stem
    room_id = stable_id64(room_str_id)
    body += struct.pack("<Q", room_id)
    body += utf8(name)
    body += struct.pack("<ii", w, h)
    body += struct.pack("<ffff", float(spawn["x"]), float(spawn["y"]),
                        float(spawn["z"]), float(spawn.get("yaw", 0.0)))
    body += struct.pack("<I", w * h)
    for row in range(h):
        for col in range(w):
            cell = cell_map.get((col, row), default_cell or {})
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
        with tempfile.TemporaryDirectory() as tmp:
            tmp_dir = Path(tmp)
            try:
                _compile_all(data_dir, tmp_dir)
            except Exception as exc:
                fail("compiler", f"malformed content: {exc}")
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

    # Compile into a sibling staging tree first.  Authoring errors therefore
    # cannot leave a half-updated set of binaries or stale files from a prior
    # successful compile.  Each validated artifact is then installed with an
    # atomic same-volume replace.
    try:
        with tempfile.TemporaryDirectory(prefix=".contentc-stage-",
                                          dir=str(out_dir.parent)) as tmp:
            staged = Path(tmp)
            _compile_all(data_dir, staged)
            if ERRORS:
                for err in ERRORS:
                    print(f"CONTENT ERROR: {err}", file=sys.stderr)
                sys.exit(1)
            for produced in sorted(staged.rglob("*")):
                if not produced.is_file():
                    continue
                destination = out_dir / produced.relative_to(staged)
                destination.parent.mkdir(parents=True, exist_ok=True)
                os.replace(str(produced), str(destination))
    except Exception as exc:
        fail("compiler", f"malformed content or atomic install failure: {exc}")
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
    scene_files = sorted(data_dir.glob("scenes/*.json"))
    room_ids = {path.stem for path in room_files}
    room_specs = _load_room_specs(room_files)
    text_ids = load_text_resource_ids(data_dir)
    validate_storylet_text_refs(storylet_files, text_ids)

    npc_ids = set()
    for path in npc_files:
        validate_npc_file(path, npc_ids, room_ids)
    validate_npc_seed_parity(data_dir, npc_ids)

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
    for path in scene_files:
        compile_scene(path, out_dir, room_ids, npc_registry, room_specs)
    if not ERRORS:
        compile_npc_profiles(npc_files, out_dir)


if __name__ == "__main__":
    main()
