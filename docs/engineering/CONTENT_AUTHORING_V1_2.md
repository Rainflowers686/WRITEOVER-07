# CONTENT AUTHORING v1.2 — WRITEOVER-07

## 1. Principle

Content authors write JSON. Runtime reads compiled binaries or seed data; it
never parses authoring JSON directly.

The systemic kernel is content-agnostic but requires a schema for future
authoring:
- actors / NPC identities
- hideable containers
- item ownership/provenance
- evidence seeds
- promises / storylet eligibility
- alert scopes / narrator observability presets

## 2. Files

- `schemas/SYSTEMIC_SCHEMA.md` — human-readable schema
- `tools/systemic/systemic_schema_check.py` — Python validator for systemic
  authoring JSON
- Future content compiler may emit systemic seed records, but v1.2 does not
  require a new compiled binary format for gameplay runtime.

## 3. Validation Rules

- IDs are stable canonical strings
- Factions and classes must be from the closed enum lists
- Hideable containers must have capacity > 0 and a concealment value in 0..100
- Item credential_level must be 0..3
- Provenance tags must be present for badges, weapons, key items
- Evidence references must point to known source events or be empty
- Relationship values are floats in [-1, 1] for alignment / [0, 1] for trust,
  fear, suspicion, debt, respect, attachment
- Promise status is one of the six closed values
- Alert levels are 0..5
- Narrator observability is boolean capability set; AuthorityStage is 0..3

## 4. Example

```json
{
  "schemaVersion": 1,
  "actors": [
    {
      "id": "guard_7",
      "faction": "Security",
      "class": "Guard",
      "occupation": "Security_Guard",
      "fullHumanIllusion": false,
      "knownIdentities": ["officer_davis"]
    }
  ],
  "containers": [
    {
      "id": "cart_b1_clean",
      "kind": "CleaningCart",
      "capacity": 0.6,
      "concealment": 95,
      "accessibility": 80,
      "routineTags": ["Cleaner"]
    }
  ],
  "items": [
    {
      "id": "badge_guard_7",
      "type": "Badge",
      "owner": "officer_davis",
      "credentialLevel": 2,
      "provenanceTags": ["security", "badge"]
    }
  ]
}
```

## 5. Python Content Tests

Run:

```powershell
python tools/systemic/systemic_schema_check.py --data-dir data
```

This validator must PASS before content check is considered green. It does not
generate runtime content yet; it catches malformed authoring early.
