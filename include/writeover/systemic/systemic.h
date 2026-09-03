#pragma once
// WRITEOVER-07 Systemic Gameplay Foundation v1.2 kernel.
//
// This header is intentionally a small, dependency-light set of POD/value
// types plus one in-memory SystemicWorld kernel. It does not replace the
// existing Height-Span world, EventBus, MemoryStore, FactStore, or Save
// format; it is the cross-module contract used by M1/M3/M4/M5/M6 so that
// body concealment, evidence, social exchange, promises, alerts, and narrator
// observability share one state vocabulary.
//
// The kernel is content-agnostic: no full maps, no full NPC cast, no scripted
// dialogue, no shop UI, no hacking minigame. It is testable and serializable.

#include "writeover/common/ids.h"
#include "writeover/common/serialize.h"
#include "writeover/common/types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

// ---------------------------------------------------------------------------
// Shared systemic enums
// ---------------------------------------------------------------------------

enum class SystemicEventType : uint8_t {
    Generic = 0,
    BodyIncapacitated = 1,
    BodySearched = 2,
    BodyDragged = 3,
    BodyHidden = 4,
    BodyDiscovered = 5,
    Theft = 6,
    UnauthorizedAccess = 7,
    Hacking = 8,
    Bribery = 9,
    Vandalism = 10,
    Assault = 11,
    NonLethalTakedown = 12,
    Killing = 13,
    EvidenceTampering = 14,
    WitnessObservation = 15,
    MemoryFormed = 16,
    PromiseOffered = 17,
    PromiseAccepted = 18,
    PromiseFulfilled = 19,
    PromiseBroken = 20,
    AlertEscalated = 21,
    Communication = 22,
    InfrastructureChange = 23,
    Trade = 24,
    Give = 25,
    Request = 26,
    AskFavor = 27,
};

enum class LegalityClass : uint8_t {
    Legal = 0,
    MinorOffense = 1,
    Unauthorized = 2,
    Illegal = 3,
    JustifiedOrUnclear = 4,
};

enum class OutcomeType : uint8_t {
    NoOutcome = 0,
    Success = 1,
    Failure = 2,
    Partial = 3,
    Discovered = 4,
    Undetected = 5,
};

enum class BodyStatus : uint8_t {
    Alive = 0,
    Injured = 1,
    Unconscious = 2,
    Dead = 3,
};

enum class BodyDisposition : uint8_t {
    Exposed = 0,
    HiddenInContainer = 1,
};

enum class ContainerKind : uint8_t {
    IndustrialRefuseBin = 0,
    CleaningCart = 1,
    LaundryCart = 2,
    LargeLocker = 3,
    VehicleTrunk = 4,
    MaintenanceCompartment = 5,
    RestroomStall = 6,
    SealedRoom = 7,
};

enum class RoutineTag : uint8_t {
    Cleaner = 0,
    Maintenance = 1,
    Security = 2,
    Owner = 3,
    RoutineUser = 4,
};

enum class EvidenceType : uint8_t {
    VisibleBody = 0,
    UnconsciousBody = 1,
    Blood = 2,
    ShellCasing = 3,
    BrokenGlass = 4,
    BrokenLock = 5,
    CameraOutage = 6,
    TerminalLogin = 7,
    DoorAccessLog = 8,
    StolenCredential = 9,
    WeaponProvenance = 10,
    CctvRecord = 11,
};

enum class Faction : uint8_t {
    GeneralStaff = 0,
    Security = 1,
    Medical = 2,
    Research = 3,
    Maintenance = 4,
    Executive = 5,
    Detained = 6,
    Civilian = 7,
};

enum class ActorClass : uint8_t {
    Full = 0,
    SemiHuman = 1,
    Guard = 2,
};

enum class ItemType : uint8_t {
    Badge = 0,
    Weapon = 1,
    Ammo = 2,
    Cash = 3,
    Medical = 4,
    KeyItem = 5,
    Tool = 6,
    Battery = 7,
    Fuse = 8,
    Decoy = 9,
    FlashSmoke = 10,
    PersonalItem = 11,
    Credential = 12,
    Other = 13,
};

enum class MemoryKind : uint8_t {
    Observation = 0,
    EventRecall = 1,
    Promise = 2,
    Debt = 3,
    Fear = 4,
    IdentityMismatch = 5,
    BodyDiscovery = 6,
    Rumor = 7,
    Report = 8,
};

enum class KnowledgeSource : uint8_t {
    DirectWitness = 0,
    HeardSound = 1,
    Hearsay = 2,
    SystemFeed = 3,
    NarratorClaim = 4,
    EnvironmentalInference = 5,
    ResidualMemory = 6,
};

enum class PromiseStatus : uint8_t {
    Offered = 0,
    Accepted = 1,
    Fulfilled = 2,
    Broken = 3,
    Expired = 4,
    Cancelled = 5,
};

enum class FacilityAlertLevel : uint8_t {
    Normal = 0,
    Suspicious = 1,
    LocalAlert = 2,
    Search = 3,
    Lockdown = 4,
    Critical = 5,
};

enum class IdentityReaction : uint8_t {
    Accept = 0,
    Report = 1,
    Confront = 2,
    HelpCoverUp = 3,
};

// ---------------------------------------------------------------------------
// Core systemic records
// ---------------------------------------------------------------------------

struct ActorRecord {
    NpcId id;
    ResourceId data_key;
    Faction faction = Faction::GeneralStaff;
    ActorClass actor_class = ActorClass::SemiHuman;
    StringId occupation;
    bool full_human_illusion = false;
    std::vector<EntityId> known_identities;       // NPCs this actor personally knows
    std::vector<std::string> personality_tags;
};

struct ItemRecord {
    ItemId id;
    ItemType type = ItemType::Other;
    EntityId owner;                                // original owner / provenance root
    EntityId issuer;                               // issuing faction or person
    EntityId legal_holder;
    EntityId current_holder;
    bool reported_stolen = false;
    uint8_t credential_level = 0;                  // 0 = none, 1 = employee, 2 = security, 3 = executive
    std::vector<std::string> provenance_tags;
};

struct BodyRecord {
    EntityId id;
    NpcId npc;
    BodyStatus status = BodyStatus::Alive;
    BodyDisposition disposition = BodyDisposition::Exposed;
    Vec3 position;
    RoomId room;
    ContainerId container;
    bool searched = false;
    bool has_weapon = false;                       // content/weapon inventory detail
};

struct HideableContainer {
    ContainerId id;
    ContainerKind kind = ContainerKind::CleaningCart;
    Vec3 position;
    RoomId room;
    float capacity_volume = 1.0f;                  // m3; enough for a body only for large kinds
    uint8_t concealment = 0;                       // 0..100
    uint8_t accessibility = 0;                     // 0..100
    std::vector<RoutineTag> routine_tags;
    std::vector<EntityId> current_occupants;
};

struct EvidenceRecord {
    EvidenceId id;
    EvidenceType type = EvidenceType::VisibleBody;
    EventId source_event;
    EntityId subject;
    EntityId owner;
    RoomId room;
    Vec3 position;
    float visibility = 1.0f;                       // 0..1 (not forensic accuracy)
    bool persists = true;
    uint64_t frame = 0;
    std::vector<NpcId> discovered_by;
};

struct MemoryRecord {
    MemoryId id;
    EntityId npc;                              // the actor whose memory this is
    MemoryKind kind = MemoryKind::Observation;
    EntityId subject;
    EntityId target;
    RoomId room;
    uint64_t frame = 0;
    float salience = 0.5f;                         // 0..1
    float confidence = 0.5f;                       // 0..1
    KnowledgeSource source = KnowledgeSource::DirectWitness;
    ResourceId text_key;
    std::vector<std::string> tags;
};

struct RelationshipRecord {
    EntityId a;
    EntityId b;
    float trust = 0.0f;
    float fear = 0.0f;
    float respect = 0.0f;
    float suspicion = 0.0f;
    float debt = 0.0f;
    float attachment = 0.0f;
    float ideological_alignment = 0.0f;            // -1..1
};

struct PromiseRecord {
    PromiseId id;
    EntityId giver;
    EntityId receiver;
    std::string subject;
    uint64_t accepted_frame = 0;
    uint64_t deadline_frame = 0;                   // 0 = no deadline
    PromiseStatus status = PromiseStatus::Offered;
    bool storylet_eligible = false;
};

struct GlobalPlayerState {
    float humanity = 0.5f;
    float violence = 0.0f;
    float reliability = 0.5f;
    float coercion = 0.0f;

    float public_trust = 0.5f;
    float security_standing = 0.5f;
    float medical_research_standing = 0.5f;
    float maintenance_standing = 0.5f;

    float narrator_alignment = 0.0f;
    float narrator_dominance = 0.0f;
    float autonomy = 0.5f;

    float truth_exposure = 0.0f;
    float self_knowledge = 0.0f;

    float facility_alert = 0.0f;
    float infrastructure_integrity = 1.0f;

    float timeline_instability = 0.0f;
    float residual_memory_pressure = 0.0f;

    uint32_t civilian_casualties = 0;
    uint32_t security_casualties = 0;
    uint32_t promises_broken = 0;
    uint32_t evidence_count = 0;
};

struct NarratorObservabilityState {
    uint8_t authority_stage = 0;                   // 0..3
    float intervention_cost = 0.0f;
    uint64_t intervention_cooldown_frame = 0;

    bool observes_cameras = false;
    bool observes_microphones = false;
    bool observes_access_readers = false;
    bool observes_terminals = false;
    bool observes_radio = false;
    bool observes_npc_reports = false;
    bool observes_timeline_anomalies = false;
    bool meta_observability = false;               // late-game only
};

struct AlertState {
    FacilityAlertLevel level = FacilityAlertLevel::Normal;
    uint64_t last_change_frame = 0;
    std::vector<RoomId> scope;                     // empty = global facility
};

// Unified event record for systemic meaningful actions. This is intentionally
// a POD/value type and not an inheritance tree.
struct SystemicEvent {
    EventId id;
    SystemicEventType type = SystemicEventType::Generic;
    EntityId actor;
    EntityId target;
    RoomId location;
    uint64_t frame = 0;
    std::vector<NpcId> witnesses;
    uint8_t severity = 0;                          // 0..100
    LegalityClass legality = LegalityClass::Legal;
    EntityId owner;
    std::string method;
    OutcomeType outcome = OutcomeType::NoOutcome;
    std::vector<EvidenceId> evidence;
    std::vector<std::string> tags;
};

// ---------------------------------------------------------------------------
// SystemicWorld kernel
// ---------------------------------------------------------------------------

class SystemicWorld {
public:
    // Actor / identity
    void AddActor(const ActorRecord& actor);
    const ActorRecord* GetActor(NpcId id) const;
    size_t ActorCount() const { return actors_.size(); }

    // Items / ownership / provenance
    ItemId AddItem(const ItemRecord& item);
    const ItemRecord* GetItem(ItemId id) const;
    void SetItemHolder(ItemId id, EntityId holder);
    void MarkReportedStolen(ItemId id);
    size_t ItemCount() const { return items_.size(); }

    // Body / hideable containers
    ContainerId AddContainer(const HideableContainer& container);
    const HideableContainer* GetContainer(ContainerId id) const;
    size_t ContainerCount() const { return containers_.size(); }

    EntityId AddBody(const BodyRecord& body);
    const BodyRecord* GetBody(EntityId id) const;
    size_t BodyCount() const { return bodies_.size(); }

    bool HideBody(EntityId body_id, ContainerId container_id);
    bool CanDirectlyObserveBody(EntityId observer_id, EntityId body_id) const;
    bool DiscoverBody(NpcId discoverer, ContainerId container_id, uint64_t frame);

    // Evidence
    EvidenceId AddEvidence(const EvidenceRecord& evidence);
    const EvidenceRecord* GetEvidence(EvidenceId id) const;
    size_t EvidenceCount() const { return evidence_.size(); }

    // Memory / social
    MemoryId AddMemory(const MemoryRecord& memory);
    const MemoryRecord* GetMemory(MemoryId id) const;
    std::vector<MemoryRecord> MemoriesOf(EntityId actor) const;
    size_t MemoryCount() const { return memories_.size(); }

    // Relationships
    void SetRelationship(const RelationshipRecord& rel);
    const RelationshipRecord* GetRelationship(EntityId a, EntityId b) const;
    size_t RelationshipCount() const { return relationships_.size(); }

    // Promises
    PromiseId AddPromise(const PromiseRecord& promise);
    const PromiseRecord* GetPromise(PromiseId id) const;
    bool SettlePromise(PromiseId id, PromiseStatus status, uint64_t frame);
    size_t PromiseCount() const { return promises_.size(); }

    // Alert
    void SetAlert(FacilityAlertLevel level, const std::vector<RoomId>& scope,
                  uint64_t frame);
    FacilityAlertLevel AlertLevel() const { return alert_.level; }
    const AlertState& Alert() const { return alert_; }

    // Player / global social state
    GlobalPlayerState& PlayerState() { return player_state_; }
    const GlobalPlayerState& PlayerState() const { return player_state_; }

    // Narrator authority vs observability
    NarratorObservabilityState& Narrator() { return narrator_; }
    const NarratorObservabilityState& Narrator() const { return narrator_; }

    // Systemic event log
    EventId AddSystemicEvent(const SystemicEvent& event);
    size_t EventCount() const { return events_.size(); }
    const std::vector<SystemicEvent>& Events() const { return events_; }

    // Identity / social helpers for the stolen-identity kernel
    bool ReaderAcceptsItem(ItemId id, uint8_t required_clearance) const;
    bool NpcAcceptsPresentedIdentity(NpcId observer, EntityId expected_owner,
                                     EntityId presenter) const;
    IdentityReaction ReactionToIdentityMismatch(NpcId observer,
                                                EntityId original_owner,
                                                EntityId presenter) const;

    // Persistence: explicit binary serialization (same field-by-field policy
    // as the core save format; can be embedded as SaveSectionId::Systemic).
    void Save(Serializer& s) const;
    void Load(Deserializer& d);
    std::vector<uint8_t> Serialize() const;
    static SystemicWorld Deserialize(const uint8_t* data, size_t size);

private:
    std::vector<ActorRecord> actors_;
    std::vector<ItemRecord> items_;
    std::vector<BodyRecord> bodies_;
    std::vector<HideableContainer> containers_;
    std::vector<EvidenceRecord> evidence_;
    std::vector<MemoryRecord> memories_;
    std::vector<RelationshipRecord> relationships_;
    std::vector<PromiseRecord> promises_;
    std::vector<SystemicEvent> events_;
    GlobalPlayerState player_state_;
    NarratorObservabilityState narrator_;
    AlertState alert_;
};

} // namespace writeover
