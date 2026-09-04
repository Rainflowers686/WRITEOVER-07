#!/usr/bin/env python3
"""Negative and positive tests for the systemic schema validator."""
import json
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import systemic_schema_check as v

FAILURES = []


def check(name, ok):
    print(f"[{'PASS' if ok else 'FAIL'}] systemic_schema.{name}")
    if not ok:
        FAILURES.append(name)


def validate_obj(data):
    with tempfile.TemporaryDirectory() as tmp:
        p = Path(tmp) / "seed.json"
        p.write_text(json.dumps(data), encoding="utf-8")
        v.ERRORS = []
        v.validate_file(p)
        return len(v.ERRORS) == 0, list(v.ERRORS)


def base():
    return {
        "schemaVersion": 1,
        "actors": [{"id": "a1", "faction": "Security", "class": "SemiHuman", "role": "Guard"}],
        "items": [],
        "containers": [],
        "evidenceSeeds": [],
        "promiseSeeds": [],
    }


def test_positive():
    ok, _ = validate_obj(base())
    check("positive", ok)


def test_duplicate_actor():
    data = base()
    data["actors"].append({"id": "a1", "faction": "Security", "class": "SemiHuman", "role": "Guard"})
    ok, _ = validate_obj(data)
    check("duplicate_actor_rejected", not ok)


def test_invalid_enum():
    data = base()
    data["actors"][0]["faction"] = "Nope"
    ok, _ = validate_obj(data)
    check("invalid_enum_rejected", not ok)


def test_bad_cross_ref():
    data = base()
    data["items"] = [{
        "id": "i1", "type": "Badge", "owner": "missing_actor",
        "issuer": "a1", "legalHolder": "a1", "credentialLevel": 2,
        "provenanceTags": ["x"],
    }]
    ok, _ = validate_obj(data)
    check("bad_cross_ref_rejected", not ok)


def test_bad_range():
    data = base()
    data["containers"] = [{"id": "c1", "kind": "CleaningCart", "capacity": 0.1,
                           "concealment": 120, "accessibility": 50, "routineTags": ["Cleaner"]}]
    ok, _ = validate_obj(data)
    check("bad_range_rejected", not ok)


def test_required_array():
    data = base()
    del data["evidenceSeeds"]
    ok, _ = validate_obj(data)
    check("required_array_rejected", not ok)


def test_unknown_known_identity():
    data = base()
    data["actors"][0]["knownIdentities"] = ["missing_actor"]
    ok, _ = validate_obj(data)
    check("unknown_known_identity_rejected", not ok)


def test_bad_item_location():
    data = base()
    data["items"] = [{"id": "i1", "type": "Tool", "location": "Container",
                      "provenanceTags": ["tool"], "container": "missing_container"}]
    ok, _ = validate_obj(data)
    check("bad_item_location_rejected", not ok)


def test_bad_evidence_type():
    data = base()
    data["evidenceSeeds"] = [{"id": "e1", "type": "NotEvidence",
                               "subject": "thing", "room": "room"}]
    ok, _ = validate_obj(data)
    check("bad_evidence_type_rejected", not ok)


def test_reserved_uncompiled_section():
    data = base()
    data["questSeeds"] = [{"id": "q1"}]
    ok, _ = validate_obj(data)
    check("reserved_uncompiled_section_rejected", not ok)


if __name__ == "__main__":
    test_positive()
    test_duplicate_actor()
    test_invalid_enum()
    test_bad_cross_ref()
    test_bad_range()
    test_required_array()
    test_unknown_known_identity()
    test_bad_item_location()
    test_bad_evidence_type()
    test_reserved_uncompiled_section()
    print(f"{10 - len(FAILURES)}/10 systemic schema tests passed")
    sys.exit(1 if FAILURES else 0)
