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
ITEM_LOCATIONS = {"Holder", "Ground", "Container"}
EVIDENCE_TYPES = {
    "VisibleBody", "UnconsciousBody", "Blood", "ShellCasing", "BrokenGlass",
    "BrokenLock", "CameraOutage", "TerminalLogin", "DoorAccessLog",
    "StolenCredential", "WeaponProvenance", "CctvRecord",
}
SPECIAL_ENTITIES = {"player"}
REQUIRED_ARRAY_FIELDS = ("actors", "items", "containers", "evidenceSeeds", "promiseSeeds")
RESERVED_UNCOMPILED_FIELDS = ("questSeeds", "knowledgeSeeds", "terminalSeeds", "observationSeeds")
ERRORS = []


def fail(path, message):
    ERRORS.append(f"{path}: {message}")


def is_string_list(value):
    return isinstance(value, list) and all(isinstance(x, str) for x in value)


def finite(value):
    return (isinstance(value, (int, float)) and not isinstance(value, bool)
            and math.isfinite(float(value)))


def validate_actor(path, actor, actor_ids):
    if not isinstance(actor, dict):
        fail(path, "actor entry must be an object")
        return
    actor_id = actor.get("id")
    if not isinstance(actor_id, str) or not actor_id:
        fail(path, "actor.id is required")
        actor_id = None
    elif actor_id in actor_ids:
        fail(path, f"duplicate actor id {actor_id}")
    else:
        actor_ids.add(actor_id)
    if actor.get("faction") not in FACTIONS:
        fail(path, f"actor {actor_id}: bad faction")
    cls = actor.get("class")
    if cls is not None and cls not in LEGACY_CLASSES:
        fail(path, f"actor {actor_id}: bad legacy class")
    cognition = actor.get("cognition")
    if cognition is None:
        if cls is None:
            fail(path, f"actor {actor_id}: cognition or legacy class is required")
    elif cognition not in COGNITION:
        fail(path, f"actor {actor_id}: bad cognition")
    role = actor.get("role")
    if role is None and cls != "Guard":
        fail(path, f"actor {actor_id}: role is required")
    elif role is not None and role not in ROLES:
        fail(path, f"actor {actor_id}: bad role")
    if not is_string_list(actor.get("knownIdentities", [])):
        fail(path, f"actor {actor_id}: knownIdentities must be string list")
    if not is_string_list(actor.get("personalityTags", [])):
        fail(path, f"actor {actor_id}: personalityTags must be string list")
    if "fullHumanIllusion" in actor and not isinstance(actor["fullHumanIllusion"], bool):
        fail(path, f"actor {actor_id}: fullHumanIllusion must be bool")
    if "occupation" in actor and (
            not isinstance(actor["occupation"], str) or not actor["occupation"]):
        fail(path, f"actor {actor_id}: occupation must be a non-empty string")


def validate_item(path, item, actor_ids, item_ids):
    if not isinstance(item, dict):
        fail(path, "item entry must be an object")
        return
    item_id = item.get("id")
    if not isinstance(item_id, str) or not item_id:
        fail(path, "item.id is required")
        item_id = None
    elif item_id in item_ids:
        fail(path, f"duplicate item id {item_id}")
    else:
        item_ids.add(item_id)
    if item.get("type") not in ITEM_TYPES:
        fail(path, f"item {item_id}: bad item type")
    location = item.get("location", "Holder")
    if location not in ITEM_LOCATIONS:
        fail(path, f"item {item_id}: bad location")
    if item.get("type") in IMPORTANT_ITEM_TYPES:
        if not item.get("owner"):
            fail(path, f"item {item_id}: important items require owner")
        if not item.get("provenanceTags") or not is_string_list(item["provenanceTags"]):
            fail(path, f"item {item_id}: important items require provenanceTags")
    elif "provenanceTags" in item and not is_string_list(item["provenanceTags"]):
        fail(path, f"item {item_id}: provenanceTags must be string list")
    for ref in ("owner", "issuer", "legalHolder", "currentHolder"):
        if ref in item and item[ref] and (
                not isinstance(item[ref], str) or
                (item[ref] not in actor_ids and item[ref] not in SPECIAL_ENTITIES)):
            fail(path, f"item {item_id}: unknown {ref} '{item[ref]}'")
    if location == "Ground" and (
            not isinstance(item.get("room"), str) or not item["room"]):
        fail(path, f"item {item_id}: Ground items require room")
    if location == "Container" and (
            not isinstance(item.get("container"), str) or not item["container"]):
        fail(path, f"item {item_id}: Container items require container")
    for field in ("reportedStolen", "revoked"):
        if field in item and not isinstance(item[field], bool):
            fail(path, f"item {item_id}: {field} must be bool")
    level = item.get("credentialLevel", 0)
    if not isinstance(level, int) or isinstance(level, bool) or level < 0 or level > 3:
        fail(path, f"item {item_id}: credentialLevel must be 0..3")
    if not finite(item.get("value", 0)):
        fail(path, f"item {item_id}: non-finite value")


def validate_container(path, container, container_ids):
    if not isinstance(container, dict):
        fail(path, "container entry must be an object")
        return
    cid = container.get("id")
    if not isinstance(cid, str) or not cid:
        fail(path, "container.id is required")
        cid = None
    elif cid in container_ids:
        fail(path, f"duplicate container id {cid}")
    else:
        container_ids.add(cid)
    if container.get("kind") not in CONTAINER_KINDS:
        fail(path, f"container {cid}: bad kind")
    if not isinstance(container.get("room"), str) or not container["room"]:
        fail(path, f"container {cid}: room is required")
    cap = container.get("capacity", 0)
    if not finite(cap) or cap <= 0:
        fail(path, f"container {cid}: capacity must be positive finite")
    for field in ("concealment", "accessibility"):
        val = container.get(field, 0)
        if not finite(val) or not (0 <= val <= 100):
            fail(path, f"container {cid}: {field} must be 0..100")
    tags = container.get("routineTags", [])
    if not is_string_list(tags):
        fail(path, f"container {cid}: routineTags must be string list")
    for tag in tags:
        if tag not in ROUTINE_TAGS:
            fail(path, f"container {cid}: unknown routine tag {tag}")


def validate_evidence(path, evidence, evidence_ids):
    if not isinstance(evidence, dict):
        fail(path, "evidence entry must be an object")
        return
    eid = evidence.get("id")
    if not isinstance(eid, str) or not eid:
        fail(path, "evidence.id is required")
        eid = None
    elif eid in evidence_ids:
        fail(path, f"duplicate evidence id {eid}")
    else:
        evidence_ids.add(eid)
    if evidence.get("type") not in EVIDENCE_TYPES:
        fail(path, f"evidence {eid}: bad type")
    for field in ("subject", "room"):
        if not isinstance(evidence.get(field), str) or not evidence[field]:
            fail(path, f"evidence {eid}: {field} is required")
    visibility = evidence.get("visibility", 1.0)
    if not finite(visibility) or not (0.0 <= visibility <= 1.0):
        fail(path, f"evidence {eid}: visibility must be 0..1 finite")
    if "persists" in evidence and not isinstance(evidence["persists"], bool):
        fail(path, f"evidence {eid}: persists must be bool")


def validate_promise(path, promise, actor_ids, promise_ids):
    if not isinstance(promise, dict):
        fail(path, "promise entry must be an object")
        return
    pid = promise.get("id")
    if not isinstance(pid, str) or not pid:
        fail(path, "promise.id is required")
        pid = None
    elif pid in promise_ids:
        fail(path, f"duplicate promise id {pid}")
    else:
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
    if not isinstance(data, dict):
        fail(str(path), "root must be an object")
        return
    if data.get("schemaVersion") != 1:
        fail(str(path), "schemaVersion must be 1")
    for field in REQUIRED_ARRAY_FIELDS:
        if field not in data:
            fail(str(path), f"{field} is required")
        elif not isinstance(data[field], list):
            fail(str(path), f"{field} must be a list")
    for field in RESERVED_UNCOMPILED_FIELDS:
        if field in data:
            if not isinstance(data[field], list):
                fail(str(path), f"{field} must be a list")
            elif data[field]:
                fail(str(path), f"{field} is reserved until its compiler is implemented")
    actor_ids = set()
    item_ids = set()
    container_ids = set()
    evidence_ids = set()
    promise_ids = set()
    actors = data.get("actors", [])
    items = data.get("items", [])
    containers = data.get("containers", [])
    evidence_seeds = data.get("evidenceSeeds", [])
    promise_seeds = data.get("promiseSeeds", [])
    for actor in actors if isinstance(actors, list) else []:
        validate_actor(str(path), actor, actor_ids)
    for actor in actors if isinstance(actors, list) else []:
        if isinstance(actor, dict) and is_string_list(actor.get("knownIdentities", [])):
            for identity in actor.get("knownIdentities", []):
                if identity not in actor_ids and identity not in SPECIAL_ENTITIES:
                    fail(str(path), f"actor {actor.get('id')}: unknown knownIdentity '{identity}'")
    for item in items if isinstance(items, list) else []:
        validate_item(str(path), item, actor_ids, item_ids)
    for container in containers if isinstance(containers, list) else []:
        validate_container(str(path), container, container_ids)
    for item in items if isinstance(items, list) else []:
        if isinstance(item, dict) and item.get("location", "Holder") == "Container":
            container = item.get("container")
            if isinstance(container, str) and container not in container_ids:
                fail(str(path), f"item {item.get('id')}: unknown container '{container}'")
    for ev in evidence_seeds if isinstance(evidence_seeds, list) else []:
        validate_evidence(str(path), ev, evidence_ids)
    for promise in promise_seeds if isinstance(promise_seeds, list) else []:
        validate_promise(str(path), promise, actor_ids, promise_ids)
    alert = data.get("alertDefaults", {})
    if not isinstance(alert, dict):
        fail(str(path), "alertDefaults must be an object")
    elif alert and alert.get("level") not in ALERT_LEVELS:
        fail(str(path), f"alertDefaults.level invalid: {alert.get('level')}")
    narrator = data.get("narratorDefaults", {})
    if not isinstance(narrator, dict):
        fail(str(path), "narratorDefaults must be an object")
    elif narrator:
        stage = narrator.get("authorityStage", 0)
        if not isinstance(stage, int) or isinstance(stage, bool) or not (0 <= stage <= 3):
            fail(str(path), "narratorDefaults.authorityStage must be 0..3")
        cost = narrator.get("interventionCost", 0.0)
        if not finite(cost) or cost < 0:
            fail(str(path), "narratorDefaults.interventionCost must be finite and non-negative")


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
