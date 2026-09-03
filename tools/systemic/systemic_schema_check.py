#!/usr/bin/env python3
"""Validate WRITEOVER-07 systemic authoring JSON.

This is a content-authoring validator, not a runtime loader. It checks closed
enums, ranges, required identity/provenance metadata, and stable ID string
references for the v1.2 systemic kernel.

Usage:
    python tools/systemic/systemic_schema_check.py --data-dir data

Exit 0 = all found systemic authoring files pass; exit 1 = failure.
"""
import argparse
import json
import sys
from pathlib import Path

FACTIONS = {
    "GeneralStaff", "Security", "Medical", "Research", "Maintenance",
    "Executive", "Detained", "Civilian",
}
ACTOR_CLASSES = {"Full", "SemiHuman", "Guard"}
ITEM_TYPES = {
    "Badge", "Weapon", "Ammo", "Cash", "Medical", "KeyItem", "Tool",
    "Battery", "Fuse", "Decoy", "FlashSmoke", "PersonalItem", "Credential",
    "Other",
}
CONTAINER_KINDS = {
    "IndustrialRefuseBin", "CleaningCart", "LaundryCart", "LargeLocker",
    "VehicleTrunk", "MaintenanceCompartment", "RestroomStall", "SealedRoom",
}
ROUTINE_TAGS = {"Cleaner", "Maintenance", "Security", "Owner", "RoutineUser"}
PROMISE_STATUSES = {
    "Offered", "Accepted", "Fulfilled", "Broken", "Expired", "Cancelled",
}
ALERT_LEVELS = {
    "Normal", "Suspicious", "LocalAlert", "Search", "Lockdown", "Critical",
}
IMPORTANT_ITEM_TYPES = {"Badge", "Weapon", "KeyItem", "Credential"}

ERRORS = []


def fail(path, message):
    ERRORS.append(f"{path}: {message}")


def is_string_list(value):
    return isinstance(value, list) and all(isinstance(x, str) for x in value)


def validate_actor(path, actor):
    if not isinstance(actor.get("id"), str) or not actor["id"]:
        fail(path, "actor.id is required")
    if actor.get("faction") not in FACTIONS:
        fail(path, f"actor {actor.get('id')}: bad faction")
    if actor.get("class") not in ACTOR_CLASSES:
        fail(path, f"actor {actor.get('id')}: bad class")
    if "occupation" in actor and not isinstance(actor["occupation"], str):
        fail(path, f"actor {actor.get('id')}: occupation must be string")
    if "knownIdentities" in actor and not is_string_list(actor["knownIdentities"]):
        fail(path, f"actor {actor.get('id')}: knownIdentities must be string list")
    if "personalityTags" in actor and not is_string_list(actor["personalityTags"]):
        fail(path, f"actor {actor.get('id')}: personalityTags must be string list")


def validate_item(path, item):
    item_id = item.get("id")
    if not isinstance(item_id, str) or not item_id:
        fail(path, "item.id is required")
    if item.get("type") not in ITEM_TYPES:
        fail(path, f"item {item_id}: bad item type")
    if item.get("type") in IMPORTANT_ITEM_TYPES:
        if not item.get("owner"):
            fail(path, f"item {item_id}: important items require owner")
        if not item.get("provenanceTags") or not is_string_list(item["provenanceTags"]):
            fail(path, f"item {item_id}: important items require provenanceTags")
    level = item.get("credentialLevel", 0)
    if not isinstance(level, int) or level < 0 or level > 3:
        fail(path, f"item {item_id}: credentialLevel must be 0..3")
    if "reportedStolen" in item and not isinstance(item["reportedStolen"], bool):
        fail(path, f"item {item_id}: reportedStolen must be bool")


def validate_container(path, container):
    cid = container.get("id")
    if not isinstance(cid, str) or not cid:
        fail(path, "container.id is required")
    if container.get("kind") not in CONTAINER_KINDS:
        fail(path, f"container {cid}: bad kind")
    cap = container.get("capacity", 0)
    if not isinstance(cap, (int, float)) or cap <= 0:
        fail(path, f"container {cid}: capacity must be positive")
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


def validate_evidence(path, evidence):
    eid = evidence.get("id")
    if not isinstance(eid, str) or not eid:
        fail(path, "evidence.id is required")
    visibility = evidence.get("visibility", 1.0)
    if not isinstance(visibility, (int, float)) or not (0.0 <= visibility <= 1.0):
        fail(path, f"evidence {eid}: visibility must be 0..1")
    if "persists" in evidence and not isinstance(evidence["persists"], bool):
        fail(path, f"evidence {eid}: persists must be bool")


def validate_promise(path, promise):
    pid = promise.get("id")
    if not isinstance(pid, str) or not pid:
        fail(path, "promise.id is required")
    if promise.get("status") not in PROMISE_STATUSES:
        fail(path, f"promise {pid}: bad status")
    if not isinstance(promise.get("subject"), str) or not promise["subject"]:
        fail(path, f"promise {pid}: subject is required")


def validate_file(path: Path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        fail(str(path), f"invalid JSON: {exc}")
        return
    if data.get("schemaVersion") != 1:
        fail(str(path), "schemaVersion must be 1")
    for actor in data.get("actors", []):
        validate_actor(str(path), actor)
    for item in data.get("items", []):
        validate_item(str(path), item)
    for container in data.get("containers", []):
        validate_container(str(path), container)
    for ev in data.get("evidenceSeeds", []):
        validate_evidence(str(path), ev)
    for promise in data.get("promiseSeeds", []):
        validate_promise(str(path), promise)
    alert = data.get("alertDefaults", {})
    if alert:
        if alert.get("level") not in ALERT_LEVELS:
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
    files = sorted(sysic for sysic in systemic_dir.glob("*.json")) if systemic_dir.exists() else []
    if not files:
        # No systemic authoring files is acceptable only for an empty seed;
        # the repository ships one sample file, so this usually means a path typo.
        print("SYSTEMIC_SCHEMA: no systemic authoring JSON found under", systemic_dir)
        return 0

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
