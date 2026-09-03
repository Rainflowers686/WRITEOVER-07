# SYSTEMIC SCHEMA v1.2 — WRITEOVER-07

Human-readable authoring JSON schema for systemic seed/content.

## Enumerations

Faction:
`GeneralStaff`, `Security`, `Medical`, `Research`, `Maintenance`,
`Executive`, `Detained`, `Civilian`

ActorClass:
`Full`, `SemiHuman`, `Guard`

ItemType:
`Badge`, `Weapon`, `Ammo`, `Cash`, `Medical`, `KeyItem`, `Tool`, `Battery`,
`Fuse`, `Decoy`, `FlashSmoke`, `PersonalItem`, `Credential`, `Other`

ContainerKind:
`IndustrialRefuseBin`, `CleaningCart`, `LaundryCart`, `LargeLocker`,
`VehicleTrunk`, `MaintenanceCompartment`, `RestroomStall`, `SealedRoom`

RoutineTag:
`Cleaner`, `Maintenance`, `Security`, `Owner`, `RoutineUser`

PromiseStatus:
`Offered`, `Accepted`, `Fulfilled`, `Broken`, `Expired`, `Cancelled`

AlertLevel:
`Normal`, `Suspicious`, `LocalAlert`, `Search`, `Lockdown`, `Critical`

## Root Object

```json
{
  "schemaVersion": 1,
  "actors": [],
  "items": [],
  "containers": [],
  "evidenceSeeds": [],
  "promiseSeeds": [],
  "alertDefaults": {},
  "narratorDefaults": {}
}
```

## Actor

```json
{
  "id": "guard_7",
  "faction": "Security",
  "class": "Guard",
  "occupation": "Security_Guard",
  "fullHumanIllusion": false,
  "knownIdentities": ["officer_davis"],
  "personalityTags": ["professional", "loyal"]
}
```

## Item

```json
{
  "id": "badge_guard_7",
  "type": "Badge",
  "owner": "officer_davis",
  "issuer": "facility_security",
  "legalHolder": "officer_davis",
  "reportedStolen": false,
  "credentialLevel": 2,
  "provenanceTags": ["security", "badge"]
}
```

## Container

```json
{
  "id": "cart_b1_clean",
  "kind": "CleaningCart",
  "room": "B1_Maintenance",
  "capacity": 0.6,
  "concealment": 95,
  "accessibility": 80,
  "routineTags": ["Cleaner"]
}
```

## Evidence Seed

```json
{
  "id": "ev_broken_glass_b1",
  "type": "BrokenGlass",
  "subject": "window_b1_04",
  "room": "B1_Corridor",
  "visibility": 0.8,
  "persists": true
}
```

## Promise Seed

```json
{
  "id": "promise_find_intern",
  "giver": "npc_doctor",
  "receiver": "player",
  "subject": "find_intern",
  "status": "Offered"
}
```

## Validation

The Python validator enforces closed enum values, numeric ranges, required
fields, and stable string ID references.
