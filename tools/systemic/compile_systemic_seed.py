#!/usr/bin/env python3
"""Compile data/systemic/systemic_seed.json -> systemic_seed.bin.

The runtime C++ SystemicWorld reads this binary; it never parses authoring
JSON. This compiler is deterministic and stdlib-only.
"""
import argparse
import json
import struct
from pathlib import Path

MAGIC = 0x574F5344  # "WOSD"
VERSION = 1


def stable_id64(name: str) -> int:
    h = 0xCBF29CE484222325
    for b in name.encode("utf-8"):
        h ^= b
        h = (h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    return h


FACTION = {
    "GeneralStaff": 0, "Security": 1, "Medical": 2, "Research": 3,
    "Maintenance": 4, "Executive": 5, "Detained": 6, "Civilian": 7,
}
COGNITION = {"Full": 0, "SemiHuman": 1}
ROLE = {
    "Guard": 0, "Cleaner": 1, "Doctor": 2, "Researcher": 3, "Technician": 4,
    "Administrator": 5, "Executive": 6, "Detained": 7, "Civilian": 8, "Other": 9,
}
ITEM_TYPE = {
    "Badge": 0, "Weapon": 1, "Ammo": 2, "Cash": 3, "Medical": 4,
    "KeyItem": 5, "Tool": 6, "Battery": 7, "Fuse": 8, "Decoy": 9,
    "FlashSmoke": 10, "PersonalItem": 11, "Credential": 12, "Other": 13,
}
ITEM_LOCATION = {"Holder": 0, "Ground": 1, "Container": 2}
CONTAINER_KIND = {
    "IndustrialRefuseBin": 0, "CleaningCart": 1, "LaundryCart": 2,
    "LargeLocker": 3, "VehicleTrunk": 4, "MaintenanceCompartment": 5,
    "RestroomStall": 6, "SealedRoom": 7,
}
ROUTINE_TAG = {
    "Cleaner": 0, "Maintenance": 1, "Security": 2, "Owner": 3, "RoutineUser": 4,
}
EVIDENCE_TYPE = {
    "VisibleBody": 0, "UnconsciousBody": 1, "Blood": 2, "ShellCasing": 3,
    "BrokenGlass": 4, "BrokenLock": 5, "CameraOutage": 6, "TerminalLogin": 7,
    "DoorAccessLog": 8, "StolenCredential": 9, "WeaponProvenance": 10,
    "CctvRecord": 11,
}
PROMISE_STATUS = {
    "Offered": 0, "Accepted": 1, "Fulfilled": 2, "Broken": 3,
    "Expired": 4, "Cancelled": 5,
}


def s(text: str) -> bytes:
    b = text.encode("utf-8")
    return struct.pack("<I", len(b)) + b


def write_string_list(out: bytearray, values):
    out.extend(struct.pack("<I", len(values)))
    for v in values:
        out.extend(s(v))


def write_id_list(out: bytearray, values):
    out.extend(struct.pack("<I", len(values)))
    for v in values:
        out.extend(struct.pack("<Q", stable_id64(v)))


def write_actor(out: bytearray, a: dict):
    out.extend(struct.pack("<Q", stable_id64(a["id"])))
    out.extend(struct.pack("<Q", stable_id64(a["id"])))  # data_key same stable id
    out.extend(struct.pack("<B", FACTION[a["faction"]]))
    cls = a.get("class", "SemiHuman")
    cognition = COGNITION.get(cls, COGNITION["SemiHuman"]) if cls in ("Full", "SemiHuman") else COGNITION["SemiHuman"]
    role = ROLE.get(a.get("role", "Other"), ROLE["Other"])
    if cls == "Guard": role = ROLE["Guard"]
    out.extend(struct.pack("<B", cognition))
    out.extend(struct.pack("<B", role))
    out.extend(struct.pack("<Q", stable_id64(a.get("occupation", "Other"))))
    out.extend(struct.pack("<B", 1 if a.get("fullHumanIllusion", False) else 0))
    write_id_list(out, a.get("knownIdentities", []))
    write_string_list(out, a.get("personalityTags", []))


def write_item(out: bytearray, it: dict):
    out.extend(struct.pack("<Q", stable_id64(it["id"])))
    out.extend(struct.pack("<B", ITEM_TYPE[it["type"]]))
    out.extend(struct.pack("<Q", stable_id64(it.get("owner", ""))))
    out.extend(struct.pack("<Q", stable_id64(it.get("issuer", ""))))
    out.extend(struct.pack("<Q", stable_id64(it.get("legalHolder", it.get("owner", "")))))
    out.extend(struct.pack("<Q", stable_id64(it.get("currentHolder", it.get("owner", "")))))
    out.extend(struct.pack("<B", ITEM_LOCATION.get(it.get("location", "Holder"), 0)))
    out.extend(struct.pack("<fff", 0.0, 0.0, 0.0))
    out.extend(struct.pack("<Q", stable_id64(it.get("room", ""))))
    out.extend(struct.pack("<Q", 0))  # container
    out.extend(struct.pack("<B", 1 if it.get("reportedStolen", False) else 0))
    out.extend(struct.pack("<B", 1 if it.get("revoked", False) else 0))
    out.extend(struct.pack("<B", int(it.get("credentialLevel", 0))))
    write_string_list(out, it.get("provenanceTags", []))


def write_container(out: bytearray, c: dict):
    out.extend(struct.pack("<Q", stable_id64(c["id"])))
    out.extend(struct.pack("<B", CONTAINER_KIND[c["kind"]]))
    out.extend(struct.pack("<fff", 0.0, 0.0, 0.0))
    out.extend(struct.pack("<Q", stable_id64(c.get("room", ""))))
    out.extend(struct.pack("<f", float(c["capacity"])))
    out.extend(struct.pack("<B", int(c.get("concealment", 0))))
    out.extend(struct.pack("<B", int(c.get("accessibility", 0))))
    out.extend(struct.pack("<I", len(c.get("routineTags", []))))
    for tag in c.get("routineTags", []):
        out.extend(struct.pack("<B", ROUTINE_TAG[tag]))
    out.extend(struct.pack("<I", 0))  # current_occupants


def write_evidence(out: bytearray, e: dict):
    out.extend(struct.pack("<Q", stable_id64(e["id"])))
    out.extend(struct.pack("<B", EVIDENCE_TYPE[e["type"]]))
    out.extend(struct.pack("<Q", 0))  # source_event
    out.extend(struct.pack("<Q", stable_id64(e.get("subject", ""))))
    out.extend(struct.pack("<Q", 0))  # owner
    out.extend(struct.pack("<Q", stable_id64(e.get("room", ""))))
    out.extend(struct.pack("<fff", 0.0, 0.0, 0.0))
    out.extend(struct.pack("<f", float(e.get("visibility", 1.0))))
    out.extend(struct.pack("<B", 1 if e.get("persists", True) else 0))
    out.extend(struct.pack("<Q", 0))  # frame
    out.extend(struct.pack("<I", 0))  # discovered_by


def write_promise(out: bytearray, p: dict):
    out.extend(struct.pack("<Q", stable_id64(p["id"])))
    out.extend(struct.pack("<Q", stable_id64(p.get("giver", ""))))
    out.extend(struct.pack("<Q", stable_id64(p.get("receiver", ""))))
    out.extend(s(p.get("subject", "")))
    out.extend(struct.pack("<Q", 0))  # accepted_frame
    out.extend(struct.pack("<Q", 0))  # deadline_frame
    out.extend(struct.pack("<Q", 0))  # transition_frame
    out.extend(s(""))
    out.extend(struct.pack("<B", PROMISE_STATUS[p["status"]]))
    out.extend(struct.pack("<B", 0))  # storylet_eligible


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", default="data/systemic/systemic_seed.json")
    parser.add_argument("--out", default="data/systemic/systemic_seed.bin")
    args = parser.parse_args()
    data = json.loads(Path(args.src).read_text(encoding="utf-8"))
    out = bytearray()
    out.extend(struct.pack("<II", MAGIC, VERSION))
    actors = data.get("actors", [])
    out.extend(struct.pack("<I", len(actors)))
    for a in actors:
        write_actor(out, a)
    items = data.get("items", [])
    out.extend(struct.pack("<I", len(items)))
    for it in items:
        write_item(out, it)
    containers = data.get("containers", [])
    out.extend(struct.pack("<I", len(containers)))
    for c in containers:
        write_container(out, c)
    evidence = data.get("evidenceSeeds", [])
    out.extend(struct.pack("<I", len(evidence)))
    for e in evidence:
        write_evidence(out, e)
    promises = data.get("promiseSeeds", [])
    out.extend(struct.pack("<I", len(promises)))
    for p in promises:
        write_promise(out, p)
    Path(args.out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.out).write_bytes(out)
    print(f"systemic seed compiled: {args.out} ({len(out)} bytes)")


if __name__ == "__main__":
    main()
