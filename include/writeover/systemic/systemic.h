#pragma once
// WRITEOVER-07 Systemic Gameplay Foundation v1.2 kernel (final correction).
//
// This is the shared minimal systemic runtime contract. It is intentionally
// not a god object: it owns durable systemic state and typed transition
// helpers only. AI decisions, quest narrative logic, shop/hacking UI, player
// movement, and rendering stay outside this kernel.

#include "writeover/common/ids.h"
#include "writeover/common/result.h"
#include "writeover/common/serialize.h"
#include "writeover/common/types.h"
#include "writeover/common/world_event.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

// ---------------------------------------------------------------------------
// Enums
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
    Trespass = 28,
    IllegalWeapon = 29,
    Impersonation = 30,
    Report = 31,
    MedicalCall = 32,
    HelpCoverUp = 33,
    ItemTransfer = 34,
    ItemReturn = 35,
    RevokeCredential = 36,
    RestoreAuthorization = 37,
    QuestTransition = 38,
    KnowledgeTransfer = 39,
    TerminalAudit = 40,
    SearchPerformed = 41,
    SocialExchange = 42,
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

enum class BodyDragStatus : uint8_t {
    None = 0,
    Dragging = 1,
};

enum class BodyDiscoveryResponse : uint8_t {
    ReportSecurity = 0,
    CallMedical = 1,
    HelpCoverUp = 2,
    Ignore = 3,
    Investigate = 4,
    Flee = 5,
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

enum class CognitionTier : uint8_t {
    Full = 0,
    SemiHuman = 1,
};

enum class Role : uint8_t {
    Guard = 0,
    Cleaner = 1,
    Doctor = 2,
    Researcher = 3,
    Technician = 4,
    Administrator = 5,
    Executive = 6,
    Detained = 7,
    Civilian = 8,
    Other = 9,
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

enum class ItemLocationKind : uint8_t {
    Holder = 0,
    Ground = 1,
    Container = 2,
};

enum class ItemLifecycleAction : uint8_t {
    Transfer = 0,
    Loan = 1,
    AuthorizedTransfer = 2,
    Theft = 3,
    Return = 4,
    Drop = 5,
    PlaceInContainer = 6,
    RevokeCredential = 7,
    RestoreAuthorization = 8,
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

enum class QuestStatus : uint8_t {
    Offered = 0,
    Accepted = 1,
    Active = 2,
    Completed = 3,
    Failed = 4,
    Expired = 5,
    Abandoned = 6,
    Betrayed = 7,
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

enum class SearchTargetType : uint8_t {
    Person = 0,
    Body = 1,
    Desk = 2,
    Locker = 3,
    Bag = 4,
    Vehicle = 5,
    Cabinet = 6,
    Container = 7,
};

enum class SocialExchangeType : uint8_t {
    Give = 0,
    Request = 1,
    Trade = 2,
    Bribe = 3,
    AskFavor = 4,
};

enum class SocialExchangeOutcome : uint8_t {
    Accepted = 0,
    Rejected = 1,
    Countered = 2,
    AcceptedThenMayReport = 3,
};

enum class KnowledgeAssetType : uint8_t {
    Password = 0,
    Username = 1,
    Route = 2,
    ShiftSchedule = 3,
    CameraBlindSpot = 4,
    EmployeeIdentity = 5,
    SafeCombination = 6,
    AccessProcedure = 7,
    Secret = 8,
    Other = 9,
};

enum class TerminalAccessMethod : uint8_t {
    Credential = 0,
    Password = 1,
    UnlockedSession = 2,
    MaintenanceAccount = 3,
    NpcLogin = 4,
    PhysicalServicePort = 5,
};

enum class ObservationSourceType : uint8_t {
    Camera = 0,
    Microphone = 1,
    AccessReader = 2,
    Terminal = 3,
    Radio = 4,
    NpcReport = 5,
    TimelineAnomaly = 6,
    Other = 7,
};

// ---------------------------------------------------------------------------
// Records
// ---------------------------------------------------------------------------

struct ActorRecord {
    NpcId id;
    ResourceId data_key;
    Faction faction = Faction::GeneralStaff;
    CognitionTier cognition = CognitionTier::SemiHuman;
    Role role = Role::Other;
    StringId occupation;
    bool full_human_illusion = false;
    std::vector<EntityId> known_identities;
    std::vector<std::string> personality_tags;
};

struct ItemRecord {
    ItemId id;
    ItemType type = ItemType::Other;
    EntityId owner;
    EntityId issuer;
    EntityId legal_holder;
    EntityId current_holder;
    ItemLocationKind location = ItemLocationKind::Holder;
    Vec3 ground_position;
    RoomId room;
    ContainerId container;
    bool reported_stolen = false;
    bool revoked = false;
    uint8_t credential_level = 0;
    std::vector<std::string> provenance_tags;
};

struct BodyRecord {
    EntityId id;
    NpcId npc;
    BodyStatus status = BodyStatus::Alive;
    BodyDisposition disposition = BodyDisposition::Exposed;
    BodyDragStatus drag_status = BodyDragStatus::None;
    Vec3 position;
    RoomId room;
    ContainerId container;
    bool searched = false;
    bool has_weapon = false;
};

struct BodyDragRecord {
    EntityId actor;
    EntityId body;
    RoomId room;
    Vec3 position;
    uint64_t started_frame = 0;
    uint64_t last_update_frame = 0;
    bool sprint_forbidden = true;
    bool weapon_restricted = true;
    float movement_modifier = 0.5f;
    std::string noise_profile;
};

struct HideableContainer {
    ContainerId id;
    ContainerKind kind = ContainerKind::CleaningCart;
    Vec3 position;
    RoomId room;
    float capacity_volume = 1.0f;
    uint8_t concealment = 0;
    uint8_t accessibility = 0;
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
    float visibility = 1.0f;
    bool persists = true;
    uint64_t frame = 0;
    std::vector<NpcId> discovered_by;
};

struct MemoryRecord {
    MemoryId id;
    EntityId npc;
    MemoryKind kind = MemoryKind::Observation;
    EntityId subject;
    EntityId target;
    RoomId room;
    uint64_t frame = 0;
    float salience = 0.5f;
    float confidence = 0.5f;
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
    float debt = 0.0f;                  // a feels indebted to b
    float attachment = 0.0f;
    float ideological_alignment = 0.0f; // -1..1
};

struct PromiseRecord {
    PromiseId id;
    EntityId giver;
    EntityId receiver;
    std::string subject;
    uint64_t accepted_frame = 0;
    uint64_t deadline_frame = 0;
    uint64_t transition_frame = 0;
    std::string reason;
    PromiseStatus status = PromiseStatus::Offered;
    bool storylet_eligible = false;
};

struct QuestRecord {
    QuestId id;
    std::string title;
    std::string presentation_objective;
    QuestStatus status = QuestStatus::Offered;
    uint64_t transition_frame = 0;
    std::string reason;
};

struct KnowledgeAssetRecord {
    KnowledgeAssetId id;
    KnowledgeAssetType type = KnowledgeAssetType::Other;
    ResourceId source;
    float confidence = 0.5f;
    std::vector<EntityId> known_by;
};

struct TerminalRecord {
    TerminalId id;
    RoomId room;
    bool powered = true;
    uint8_t credential_requirement = 0;
    std::vector<std::string> access_scope;
};

struct TerminalSession {
    TerminalId terminal;
    EntityId user;
    TerminalAccessMethod method = TerminalAccessMethod::Credential;
    uint64_t started_frame = 0;
    bool active = false;
};

struct TerminalAuditLog {
    TerminalId terminal;
    EntityId user;
    TerminalAccessMethod method = TerminalAccessMethod::Credential;
    uint64_t frame = 0;
    std::string action;
    bool unauthorized = false;
};

struct ObservationSource {
    ObservationSourceId id;
    ObservationSourceType type = ObservationSourceType::Other;
    RoomId room;
    bool online = true;
    std::string network_segment;
    std::string provenance;
};

struct NarratorAuthorityState {
    uint8_t authority_stage = 0;
    float intervention_cost = 0.0f;
    uint64_t intervention_cooldown_frame = 0;
};

struct NarratorObservabilityState {
    std::vector<ObservationSource> sources;
    bool meta_observability = false;
};

struct AlertState {
    FacilityAlertLevel level = FacilityAlertLevel::Normal;
    uint64_t last_change_frame = 0;
    std::vector<RoomId> scope;
};

struct SearchAction {
    EntityId actor;
    EntityId target;
    SearchTargetType target_type = SearchTargetType::Container;
    bool consent = true;
    RoomId room;
    uint64_t frame = 0;
};

struct SearchOutcome {
    bool success = false;
    std::vector<ItemId> items_revealed;
    std::vector<ItemId> items_taken;
    std::vector<KnowledgeAssetId> knowledge_revealed;
    uint8_t noise = 0;
    uint8_t visibility = 0;
    LegalityClass legality = LegalityClass::Legal;
};

struct SocialExchangeRecord {
    SocialExchangeId id;
    SocialExchangeType type = SocialExchangeType::Give;
    EntityId actor;
    EntityId target;
    std::vector<ItemId> items;
    std::vector<KnowledgeAssetId> information;
    uint32_t cash = 0;
    std::vector<EntityId> witnesses;
    std::string risk_context;
    SocialExchangeOutcome outcome = SocialExchangeOutcome::Rejected;
    uint64_t frame = 0;
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

    float infrastructure_integrity = 1.0f;

    float timeline_instability = 0.0f;
    float residual_memory_pressure = 0.0f;

    uint32_t civilian_casualties = 0;
    uint32_t security_casualties = 0;
    uint32_t promises_broken = 0;
    uint32_t previous_cycle_evidence_count = 0;

    bool operator_room_found = false;
};

struct SystemicEvent {
    EventId id;
    EventId source_world_event_id;      // 0 = not linked to a WorldEvent
    SystemicEventType type = SystemicEventType::Generic;
    EntityId actor;
    EntityId target;
    RoomId location;
    uint64_t frame = 0;
    std::vector<NpcId> witnesses;
    uint8_t severity = 0;
    LegalityClass legality = LegalityClass::Legal;
    EntityId owner;
    std::string method;
    OutcomeType outcome = OutcomeType::NoOutcome;
    std::vector<EvidenceId> evidence;
    std::vector<std::string> tags;
};

class SystemicWorld;

// Canonical runtime adapter: EventBus WorldEvents -> SystemicEvent ledger.
class SystemicEventBridge {
public:
    explicit SystemicEventBridge(SystemicWorld* world);

    void Register(EventBus& bus);
    void OnWorldEvent(const WorldEvent& event);
    size_t BridgedCount() const { return bridged_count_; }
    bool IsRegistered() const { return consumer_id_ != 0; }

private:
    SystemicWorld* world_ = nullptr;
    EventBus::ConsumerId consumer_id_ = 0;
    size_t bridged_count_ = 0;
};
// ---------------------------------------------------------------------------
// SystemicWorld
// ---------------------------------------------------------------------------

class SystemicWorld {
public:
    // Actor / identity
    bool AddActor(const ActorRecord& actor);
    const ActorRecord* GetActor(NpcId id) const;
    size_t ActorCount() const { return actors_.size(); }

    // Items / lifecycle
    bool AddItem(const ItemRecord& item);
    const ItemRecord* GetItem(ItemId id) const;
    size_t ItemCount() const { return items_.size(); }

    bool TransferItem(ItemId id, EntityId to);
    bool LoanItem(ItemId id, EntityId to);
    bool AuthorizedTransferItem(ItemId id, EntityId to);
    bool TheftItem(ItemId id, EntityId to, uint64_t frame);
    bool ReportItemStolen(ItemId id, uint64_t frame);
    bool ReturnItem(ItemId id, uint64_t frame);
    bool DropItem(ItemId id, const Vec3& pos, RoomId room, uint64_t frame);
    bool PlaceItemInContainer(ItemId id, ContainerId container, uint64_t frame);
    bool RevokeCredential(ItemId id, uint64_t frame);
    bool RestoreAuthorization(ItemId id, uint64_t frame);

    // Bodies / containers / drag
    bool AddBody(const BodyRecord& body);
    const BodyRecord* GetBody(EntityId id) const;
    size_t BodyCount() const { return bodies_.size(); }

    bool AddContainer(const HideableContainer& container);
    const HideableContainer* GetContainer(ContainerId id) const;
    size_t ContainerCount() const { return containers_.size(); }

    bool BeginDrag(EntityId actor, EntityId body_id, uint64_t frame);
    bool UpdateDrag(EntityId body_id, const Vec3& pos, RoomId room, uint64_t frame);
    bool EndDrag(EntityId body_id, uint64_t frame);
    const BodyDragRecord* GetDrag(EntityId body_id) const;
    size_t DragCount() const { return drags_.size(); }

    bool HideBody(EntityId body_id, ContainerId container_id, uint64_t frame = 0);
    bool CanDirectlyObserveBody(EntityId observer_id, EntityId body_id) const;

    bool DiscoverBody(NpcId discoverer, ContainerId container_id, uint64_t frame);
    bool ApplyDiscoveryResponse(EntityId actor, EventId discovery_event_id,
                                BodyDiscoveryResponse response, uint64_t frame);

    // Evidence / memory
    bool AddEvidence(const EvidenceRecord& evidence);
    const EvidenceRecord* GetEvidence(EvidenceId id) const;
    size_t GetEvidenceCount() const { return evidence_.size(); }
    size_t EvidenceCount() const { return evidence_.size(); }

    bool AddMemory(const MemoryRecord& memory);
    const MemoryRecord* GetMemory(MemoryId id) const;
    std::vector<MemoryRecord> MemoriesOf(EntityId actor) const;
    size_t MemoryCount() const { return memories_.size(); }

    // Relationships (directed)
    bool SetRelationship(const RelationshipRecord& rel);
    const RelationshipRecord* GetRelationship(EntityId a, EntityId b) const;
    size_t RelationshipCount() const { return relationships_.size(); }

    // Promises
    bool AddPromise(const PromiseRecord& promise);
    const PromiseRecord* GetPromise(PromiseId id) const;
    bool TransitionPromise(PromiseId id, PromiseStatus next, uint64_t frame,
                           const std::string& reason);
    size_t PromiseCount() const { return promises_.size(); }

    // Quests
    bool AddQuest(const QuestRecord& quest);
    const QuestRecord* GetQuest(QuestId id) const;
    bool TransitionQuest(QuestId id, QuestStatus next, uint64_t frame,
                         const std::string& reason);
    size_t QuestCount() const { return quests_.size(); }

    // Knowledge assets
    bool AddKnowledgeAsset(const KnowledgeAssetRecord& asset);
    const KnowledgeAssetRecord* GetKnowledgeAsset(KnowledgeAssetId id) const;
    size_t KnowledgeCount() const { return knowledge_.size(); }

    // Search
    SearchOutcome PerformSearch(const SearchAction& action);
    size_t SearchCount() const { return searches_.size(); }

    // Social exchange
    bool AddSocialExchange(const SocialExchangeRecord& exchange);
    const SocialExchangeRecord* GetSocialExchange(SocialExchangeId id) const;
    size_t SocialExchangeCount() const { return exchanges_.size(); }

    // Terminals
    bool AddTerminal(const TerminalRecord& terminal);
    const TerminalRecord* GetTerminal(TerminalId id) const;
    bool AddTerminalSession(const TerminalSession& session);
    bool AddTerminalAudit(const TerminalAuditLog& audit);
    size_t TerminalCount() const { return terminals_.size(); }
    size_t TerminalSessionCount() const { return sessions_.size(); }
    size_t TerminalAuditCount() const { return audits_.size(); }

    // Narrator observability
    bool AddObservationSource(const ObservationSource& source);
    const ObservationSource* GetObservationSource(ObservationSourceId id) const;
    size_t ObservationSourceCount() const { return sources_.size(); }
    bool SetObservationSourceOnline(ObservationSourceId id, bool online);
    bool NarratorObserves(ObservationSourceId id) const;
    bool NarratorCanObserveRoom(RoomId room, ObservationSourceType type) const;

    // Alert
    void SetAlert(FacilityAlertLevel level, const std::vector<RoomId>& scope,
                  uint64_t frame);
    FacilityAlertLevel AlertLevel() const { return alert_.level; }
    const AlertState& Alert() const { return alert_; }
    float DerivedAlertScalar() const;
    size_t EvidenceCountDerived() const { return evidence_.size(); }

    // Global / narrator state
    GlobalPlayerState& PlayerState() { return player_state_; }
    const GlobalPlayerState& PlayerState() const { return player_state_; }
    NarratorAuthorityState& NarratorAuthority() { return authority_; }
    const NarratorAuthorityState& NarratorAuthority() const { return authority_; }
    NarratorObservabilityState& NarratorObservability() { return observability_; }
    const NarratorObservabilityState& NarratorObservability() const { return observability_; }

    // Systemic event ledger + WorldEvent bridge
    bool AddSystemicEvent(const SystemicEvent& event);
    bool BridgeWorldEventOnce(const WorldEvent& world_event);
    size_t EventCount() const { return events_.size(); }
    const std::vector<SystemicEvent>& Events() const { return events_; }

    // Identity helper
    bool ReaderAcceptsItem(ItemId id, uint8_t required_clearance) const;
    bool NpcAcceptsPresentedIdentity(NpcId observer, EntityId expected_owner,
                                     EntityId presenter) const;
    IdentityReaction ReactionToIdentityMismatch(NpcId observer,
                                                EntityId original_owner,
                                                EntityId presenter) const;

    // Persistence. Deserialize is fail-closed: on any invalid count, enum,
    // NaN, range, duplicate id, or bad reference it returns an error.
    void Save(Serializer& s) const;
    std::vector<uint8_t> Serialize() const;
    static Result<SystemicWorld> Deserialize(const uint8_t* data, size_t size);

    // Seed binary loading (compiled from data/systemic JSON by the Python
    // seed compiler). Runtime never parses authoring JSON.
    Result<void> LoadSeedBinary(const std::string& path);
    Result<void> LoadSeedBytes(const uint8_t* data, size_t size);

    const std::vector<ActorRecord>& Actors() const { return actors_; }
    const std::vector<ItemRecord>& Items() const { return items_; }
    const std::vector<BodyRecord>& Bodies() const { return bodies_; }
    const std::vector<HideableContainer>& Containers() const { return containers_; }
    const std::vector<EvidenceRecord>& Evidence() const { return evidence_; }
    const std::vector<MemoryRecord>& Memories() const { return memories_; }
    const std::vector<RelationshipRecord>& Relationships() const { return relationships_; }
    const std::vector<PromiseRecord>& Promises() const { return promises_; }
    const std::vector<QuestRecord>& Quests() const { return quests_; }
    const std::vector<KnowledgeAssetRecord>& Knowledge() const { return knowledge_; }
    const std::vector<BodyDragRecord>& Drags() const { return drags_; }
    const std::vector<SearchAction>& Searches() const { return searches_; }
    const std::vector<SocialExchangeRecord>& SocialExchanges() const { return exchanges_; }
    const std::vector<TerminalRecord>& Terminals() const { return terminals_; }
    const std::vector<TerminalSession>& TerminalSessions() const { return sessions_; }
    const std::vector<TerminalAuditLog>& TerminalAudits() const { return audits_; }
    const std::vector<ObservationSource>& ObservationSources() const { return sources_; }
    const std::vector<SystemicEvent>& SystemEvents() const { return events_; }

private:
    std::vector<ActorRecord> actors_;
    std::vector<ItemRecord> items_;
    std::vector<BodyRecord> bodies_;
    std::vector<HideableContainer> containers_;
    std::vector<BodyDragRecord> drags_;
    std::vector<EvidenceRecord> evidence_;
    std::vector<MemoryRecord> memories_;
    std::vector<RelationshipRecord> relationships_;
    std::vector<PromiseRecord> promises_;
    std::vector<QuestRecord> quests_;
    std::vector<KnowledgeAssetRecord> knowledge_;
    std::vector<SearchAction> searches_;
    std::vector<SocialExchangeRecord> exchanges_;
    std::vector<TerminalRecord> terminals_;
    std::vector<TerminalSession> sessions_;
    std::vector<TerminalAuditLog> audits_;
    std::vector<ObservationSource> sources_;
    std::vector<SystemicEvent> events_;
    GlobalPlayerState player_state_;
    NarratorAuthorityState authority_;
    NarratorObservabilityState observability_;
    AlertState alert_;
};

} // namespace writeover
