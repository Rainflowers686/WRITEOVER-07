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


if __name__ == "__main__":
    test_positive()
    test_duplicate_actor()
    test_invalid_enum()
    test_bad_cross_ref()
    test_bad_range()
    print(f"{5 - len(FAILURES)}/5 systemic schema tests passed")
    sys.exit(1 if FAILURES else 0)
