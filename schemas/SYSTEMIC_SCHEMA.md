# SYSTEMIC SCHEMA v1.2 — WRITEOVER-07

Human-readable authoring JSON schema v1.2.

## Enumerations

### CognitionTier
`Full`, `SemiHuman`

### Role
`Guard`, `Cleaner`, `Doctor`, `Researcher`, `Technician`, `Administrator`,
`Executive`, `Detained`, `Civilian`, `Other`

### Faction
`GeneralStaff`, `Security`, `Medical`, `Research`, `Maintenance`,
`Executive`, `Detained`, `Civilian`

### ItemType
`Badge`, `Weapon`, `Ammo`, `Cash`, `Medical`, `KeyItem`, `Tool`, `Battery`,
`Fuse`, `Decoy`, `FlashSmoke`, `PersonalItem`, `Credential`, `Other`

### ContainerKind
`IndustrialRefuseBin`, `CleaningCart`, `LaundryCart`, `LargeLocker`,
`VehicleTrunk`, `MaintenanceCompartment`, `RestroomStall`, `SealedRoom`

### RoutineTag
`Cleaner`, `Maintenance`, `Security`, `Owner`, `RoutineUser`

### PromiseStatus
`Offered`, `Accepted`, `Fulfilled`, `Broken`, `Expired`, `Cancelled`

### QuestStatus
`Offered`, `Accepted`, `Active`, `Completed`, `Failed`, `Expired`,
`Abandoned`, `Betrayed`

### AlertLevel
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
  "questSeeds": [],
  "knowledgeSeeds": [],
  "terminalSeeds": [],
  "observationSeeds": [],
  "alertDefaults": {},
  "narratorDefaults": {}
}
```

## Actor

```json
{
  "id": "guard_7",
  "faction": "Security",
  "cognition": "SemiHuman",
  "role": "Guard",
  "occupation": "Security_Guard",
  "fullHumanIllusion": false,
  "knownIdentities": ["officer_davis"],
  "personalityTags": ["professional", "loyal"]
}
```

Legacy `"class": "Guard"` migrates to `cognition=SemiHuman`, `role=Guard`.

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

## Promise

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

The Python validator enforces closed enums, required fields, ranges, finite
values, unique IDs, and cross-file references. It fails when no systemic seed
exists.