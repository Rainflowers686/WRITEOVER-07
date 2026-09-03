#!/usr/bin/env python3
"""Validate WRITEOVER-07 systemic authoring JSON.

Exit 0 = all systemic authoring files are valid; exit 1 = failure.
If no systemic seed exists, the validator fails.
"""
import argparse
import json
import math
import sys
from pathlib import Path

FACTIONS = {"GeneralStaff", "Security", "Medical", "Research", "Maintenance", "Executive", "Detained", "Civilian"}
COGNITION = {"Full", "SemiHuman"}
ROLES = {"Guard", "Cleaner", "Doctor", "Researcher", "Technician", "Administrator", "Executive", "Detained", "Civilian", "Other"}
LEGACY_CLASSES = {"Full", "SemiHuman", "Guard"}
ITEM_TYPES = {"Badge", "Weapon", "Ammo", "Cash", "Medical", "KeyItem", "Tool", "Battery", "Fuse", "Decoy", "FlashSmoke", "PersonalItem", "Credential", "Other"}
CONTAINER_KINDS = {"IndustrialRefuseBin", "CleaningCart", "LaundryCart", "LargeLocker", "VehicleTrunk", "MaintenanceCompartment", "RestroomStall", "SealedRoom"}
ROUTINE_TAGS = {"Cleaner", "Maintenance", "Security", "Owner", "RoutineUser"}
PROMISE_STATUSES = {"Offered", "Accepted", "Fulfilled", "Broken", "Expired", "Cancelled"}
QUEST_STATUSES = {"Offered", "Accepted", "Active", "Completed", "Failed", "Expired", "Abandoned", "Betrayed"}
ALERT_LEVELS = {"Normal", "Suspicious", "LocalAlert", "Search", "Lockdown", "Critical"}
IMPORTANT_ITEM_TYPES = {"Badge", "Weapon", "KeyItem", "Credential"}
SPECIAL_ENTITIES = {"player"}
ERRORS = []


def fail(path, message):
    ERRORS.append(f"{path}: {message}")


def is_string_list(value):
    return isinstance(value, list) and all(isinstance(x, str) for x in value)


def finite(value):
    if isinstance(value, bool):
        return True
    if isinstance(value, (int, float)):
        return math.isfinite(float(value))
    return True


def validate_actor(path, actor, actor_ids):
    if not isinstance(actor.get("id"), str) or not actor["id"]:
        fail(path, "actor.id is required")
    if actor.get("id") in actor_ids:
        fail(path, f"duplicate actor id {actor['id']}")
    actor_ids.add(actor["id"])
    if actor.get("faction") not in FACTIONS:
        fail(path, f"actor {actor.get('id')}: bad faction")
    cls = actor.get("class")
    if cls is not None and cls not in LEGACY_CLASSES:
        fail(path, f"actor {actor.get('id')}: bad legacy class")
    if actor.get("cognition") is not None and actor["cognition"] not in COGNITION:
        fail(path, f"actor {actor.get('id')}: bad cognition")
    if actor.get("role") is not None and actor["role"] not in ROLES:
        fail(path, f"actor {actor.get('id')}: bad role")
    if not is_string_list(actor.get("knownIdentities", [])):
        fail(path, f"actor {actor.get('id')}: knownIdentities must be string list")
    if not is_string_list(actor.get("personalityTags", [])):
        fail(path, f"actor {actor.get('id')}: personalityTags must be string list")


def validate_item(path, item, actor_ids, item_ids):
    item_id = item.get("id")
    if not isinstance(item_id, str) or not item_id:
        fail(path, "item.id is required")
    if item_id in item_ids:
        fail(path, f"duplicate item id {item_id}")
    item_ids.add(item_id)
    if item.get("type") not in ITEM_TYPES:
        fail(path, f"item {item_id}: bad item type")
    if item.get("type") in IMPORTANT_ITEM_TYPES:
        if not item.get("owner"):
            fail(path, f"item {item_id}: important items require owner")
        if not item.get("provenanceTags") or not is_string_list(item["provenanceTags"]):
            fail(path, f"item {item_id}: important items require provenanceTags")
        for ref in ("owner", "issuer", "legalHolder", "currentHolder"):
            if ref in item and item[ref] and item[ref] not in actor_ids and item[ref] not in SPECIAL_ENTITIES:
                fail(path, f"item {item_id}: unknown {ref} '{item[ref]}'")
    level = item.get("credentialLevel", 0)
    if not isinstance(level, int) or level < 0 or level > 3:
        fail(path, f"item {item_id}: credentialLevel must be 0..3")
    if not finite(item.get("value", 0)):
        fail(path, f"item {item_id}: non-finite value")


def validate_container(path, container, container_ids):
    cid = container.get("id")
    if not isinstance(cid, str) or not cid:
        fail(path, "container.id is required")
    if cid in container_ids:
        fail(path, f"duplicate container id {cid}")
    container_ids.add(cid)
    if container.get("kind") not in CONTAINER_KINDS:
        fail(path, f"container {cid}: bad kind")
    cap = container.get("capacity", 0)
    if not isinstance(cap, (int, float)) or cap <= 0 or not finite(cap):
        fail(path, f"container {cid}: capacity must be positive finite")
    for field in ("concealment", "accessibility"):
        val = container.get(field, 0)
        if not isinstance(val, (int, float)) or not (0 <= val <= 100):
            fail(path, f"container {cid}: {field} must be 0..100")
    tags = container.get("routineTags", [])
    if not is_string_list(tags):
        fail(path, f"container {cid}: routineTags must be string list")
    for tag in tags:
        if tag not in ROUTINE_TAGS:
            fail(path, f"container {cid}: unknown routine tag {tag}")


def validate_evidence(path, evidence, evidence_ids):
    eid = evidence.get("id")
    if not isinstance(eid, str) or not eid:
        fail(path, "evidence.id is required")
    if eid in evidence_ids:
        fail(path, f"duplicate evidence id {eid}")
    evidence_ids.add(eid)
    visibility = evidence.get("visibility", 1.0)
    if not isinstance(visibility, (int, float)) or not (0.0 <= visibility <= 1.0) or not finite(visibility):
        fail(path, f"evidence {eid}: visibility must be 0..1 finite")
    if "persists" in evidence and not isinstance(evidence["persists"], bool):
        fail(path, f"evidence {eid}: persists must be bool")


def validate_promise(path, promise, actor_ids, promise_ids):
    pid = promise.get("id")
    if not isinstance(pid, str) or not pid:
        fail(path, "promise.id is required")
    if pid in promise_ids:
        fail(path, f"duplicate promise id {pid}")
    promise_ids.add(pid)
    if promise.get("status") not in PROMISE_STATUSES:
        fail(path, f"promise {pid}: bad status")
    if not isinstance(promise.get("subject"), str) or not promise["subject"]:
        fail(path, f"promise {pid}: subject is required")
    for ref in ("giver", "receiver"):
        if promise.get(ref) not in actor_ids and promise.get(ref) not in SPECIAL_ENTITIES:
            fail(path, f"promise {pid}: unknown {ref} '{promise.get(ref)}'")


def validate_file(path: Path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(str(path), f"invalid JSON: {exc}")
        return
    if data.get("schemaVersion") != 1:
        fail(str(path), "schemaVersion must be 1")
    actor_ids = set()
    item_ids = set()
    container_ids = set()
    evidence_ids = set()
    promise_ids = set()
    for actor in data.get("actors", []):
        validate_actor(str(path), actor, actor_ids)
    for item in data.get("items", []):
        validate_item(str(path), item, actor_ids, item_ids)
    for container in data.get("containers", []):
        validate_container(str(path), container, container_ids)
    for ev in data.get("evidenceSeeds", []):
        validate_evidence(str(path), ev, evidence_ids)
    for promise in data.get("promiseSeeds", []):
        validate_promise(str(path), promise, actor_ids, promise_ids)
    alert = data.get("alertDefaults", {})
    if alert and alert.get("level") not in ALERT_LEVELS:
        fail(str(path), f"alertDefaults.level invalid: {alert.get('level')}")
    narrator = data.get("narratorDefaults", {})
    if narrator:
        stage = narrator.get("authorityStage", 0)
        if not isinstance(stage, int) or not (0 <= stage <= 3):
            fail(str(path), "narratorDefaults.authorityStage must be 0..3")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="data")
    args = parser.parse_args()
    root = Path(args.data_dir)
    systemic_dir = root / "systemic"
    files = sorted(path for path in systemic_dir.glob("*.json")) if systemic_dir.exists() else []
    if not files:
        print("SYSTEMIC_SCHEMA_CHECK=FAIL: no systemic authoring JSON found", file=sys.stderr)
        return 1
    for path in files:
        print(f"Validating {path}")
        validate_file(path)
    if ERRORS:
        for err in ERRORS:
            print(f"ERROR: {err}", file=sys.stderr)
        print(f"SYSTEMIC_SCHEMA_CHECK=FAIL ({len(ERRORS)} errors)")
        return 1
    print(f"SYSTEMIC_SCHEMA_CHECK=PASS ({len(files)} file(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())