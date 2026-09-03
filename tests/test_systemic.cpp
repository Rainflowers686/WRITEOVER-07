#include "tests/test_harness.h"

#include "writeover/core/save.h"
#include "writeover/systemic/systemic.h"

#include <cstdint>
#include <vector>

namespace writeover {

namespace {

const EntityId kPlayer = EntityId::New(1);
const EntityId kGuard = EntityId::New(10);
const EntityId kCleaner = EntityId::New(11);
const EntityId kPatrol = EntityId::New(12);

ActorRecord MakeGuard(NpcId id) {
    ActorRecord a;
    a.id = id;
    a.data_key = ResourceId::New(1000 + id.GetValue());
    a.faction = Faction::Security;
    a.cognition = CognitionTier::SemiHuman;
    a.role = Role::Guard;
    a.occupation = StringId::New(1);
    return a;
}

ItemRecord MakeBadge(ItemId id, EntityId owner) {
    ItemRecord item;
    item.id = id;
    item.type = ItemType::Badge;
    item.owner = owner;
    item.issuer = owner;
    item.legal_holder = owner;
    item.current_holder = owner;
    item.credential_level = 2;
    item.provenance_tags.push_back("security_badge");
    return item;
}

HideableContainer MakeCart(ContainerId id, float capacity) {
    HideableContainer c;
    c.id = id;
    c.kind = ContainerKind::CleaningCart;
    c.room = RoomId::New(1);
    c.position = Vec3{4.0f, 4.0f, 0.0f};
    c.capacity_volume = capacity;
    c.concealment = 95;
    c.accessibility = 80;
    c.routine_tags.push_back(RoutineTag::Cleaner);
    return c;
}

BodyRecord MakeUnconsciousBody(EntityId id, NpcId npc) {
    BodyRecord b;
    b.id = id;
    b.npc = npc;
    b.status = BodyStatus::Unconscious;
    b.disposition = BodyDisposition::Exposed;
    b.room = RoomId::New(1);
    b.position = Vec3{3.0f, 4.0f, 0.0f};
    return b;
}

bool SystemicBodyConcealmentChain() {
    SystemicWorld w;
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(10))));
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 0.6f)));
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(10))));
    ItemRecord badge = MakeBadge(ItemId::New(1), kGuard);
    badge.current_holder = kPlayer;
    WO_CHECK(w.AddItem(badge));

    // Search body: the player takes the badge.
    SearchAction search;
    search.actor = kPlayer;
    search.target = EntityId::New(20);
    search.target_type = SearchTargetType::Body;
    search.consent = false;
    search.room = RoomId::New(1);
    search.frame = 10;
    SearchOutcome outcome = w.PerformSearch(search);
    WO_CHECK(outcome.success);
    WO_CHECK(w.TransferItem(ItemId::New(1), kPlayer));

    // Drag lifecycle.
    WO_CHECK(w.BeginDrag(kPlayer, EntityId::New(20), 20));
    WO_CHECK(w.GetDrag(EntityId::New(20)) != nullptr);
    WO_CHECK(w.UpdateDrag(EntityId::New(20), Vec3{4.0f, 4.0f, 0.0f}, RoomId::New(1), 30));
    WO_CHECK(w.EndDrag(EntityId::New(20), 40));

    // Hide.
    WO_CHECK(w.HideBody(EntityId::New(20), ContainerId::New(1)));
    WO_CHECK(!w.CanDirectlyObserveBody(kPatrol, EntityId::New(20)));

    // Discovery is observation-only: no automatic alert.
    const FacilityAlertLevel before = w.AlertLevel();
    WO_CHECK(w.DiscoverBody(NpcId::New(11), ContainerId::New(1), 100));
    WO_CHECK(w.AlertLevel() == before);
    WO_CHECK(w.GetBody(EntityId::New(20)) != nullptr);
    WO_CHECK(w.EvidenceCount() == 1);
    WO_CHECK(w.MemoryCount() == 1);
    WO_CHECK(w.EventCount() > 0);

    // Separate response: report escalates only when explicitly applied.
    const EventId discovery_id = w.Events().back().id;
    WO_CHECK(w.ApplyDiscoveryResponse(kCleaner, discovery_id,
                                      BodyDiscoveryResponse::ReportSecurity, 110));
    WO_CHECK(static_cast<int>(w.AlertLevel()) >= static_cast<int>(FacilityAlertLevel::Suspicious));

    // Save/load preserves chain.
    const std::vector<uint8_t> bytes = w.Serialize();
    const auto res = SystemicWorld::Deserialize(bytes.data(), bytes.size());
    WO_CHECK(res.IsOk());
    if (!res.IsOk()) return false;
    SystemicWorld reloaded = res.Value();
    WO_CHECK(reloaded.GetBody(EntityId::New(20)) != nullptr);
    WO_CHECK(reloaded.EvidenceCount() == 1);
    WO_CHECK(reloaded.MemoryCount() == 1);
    const ItemRecord* rbadge = reloaded.GetItem(ItemId::New(1));
    WO_CHECK(rbadge != nullptr && rbadge->current_holder == kPlayer);
    return true;
}

bool SystemicStolenIdentityChain() {
    SystemicWorld w;
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(1))));
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(2))));
    ActorRecord knowing = MakeGuard(NpcId::New(3));
    knowing.known_identities.push_back(kGuard);
    WO_CHECK(w.AddActor(knowing));

    ItemRecord badge = MakeBadge(ItemId::New(1), kGuard);
    badge.current_holder = kPlayer;
    badge.reported_stolen = true;
    WO_CHECK(w.AddItem(badge));

    WO_CHECK(w.ReaderAcceptsItem(ItemId::New(1), 2));
    WO_CHECK(w.NpcAcceptsPresentedIdentity(NpcId::New(2), kGuard, kPlayer));
    WO_CHECK(!w.NpcAcceptsPresentedIdentity(NpcId::New(3), kGuard, kPlayer));

    RelationshipRecord helpful;
    helpful.a = EntityId::New(3);
    helpful.b = kPlayer;
    helpful.trust = 0.9f;
    helpful.fear = 0.1f;
    helpful.debt = 0.7f;
    WO_CHECK(w.SetRelationship(helpful));
    WO_CHECK(w.ReactionToIdentityMismatch(NpcId::New(3), kGuard, kPlayer) ==
             IdentityReaction::HelpCoverUp);

    // Revoked credential no longer opens readers.
    WO_CHECK(w.RevokeCredential(ItemId::New(1), 1));
    WO_CHECK(!w.ReaderAcceptsItem(ItemId::New(1), 2));
    return true;
}

bool SystemicPromiseChain() {
    SystemicWorld w;
    RelationshipRecord rel;
    rel.a = EntityId::New(10);
    rel.b = kPlayer;
    rel.trust = 0.5f;
    rel.debt = 0.2f;
    WO_CHECK(w.SetRelationship(rel));

    PromiseRecord p;
    p.id = PromiseId::New(1);
    p.giver = EntityId::New(10);
    p.receiver = kPlayer;
    p.subject = "find_intern";
    p.status = PromiseStatus::Accepted;
    WO_CHECK(w.AddPromise(p));

    const auto bytes = w.Serialize();
    const auto res = SystemicWorld::Deserialize(bytes.data(), bytes.size());
    WO_CHECK(res.IsOk());
    if (!res.IsOk()) return false;
    SystemicWorld reloaded = res.Value();
    WO_CHECK(reloaded.GetPromise(PromiseId::New(1)) != nullptr);

    const float rel_before = reloaded.PlayerState().reliability;
    WO_CHECK(reloaded.TransitionPromise(PromiseId::New(1), PromiseStatus::Broken, 500, "failed"));
    WO_CHECK(reloaded.GetPromise(PromiseId::New(1))->status == PromiseStatus::Broken);
    WO_CHECK(reloaded.PlayerState().reliability < rel_before);
    WO_CHECK(reloaded.PlayerState().promises_broken == 1);

    PromiseRecord ok;
    ok.id = PromiseId::New(2);
    ok.giver = EntityId::New(11);
    ok.receiver = kPlayer;
    ok.subject = "return_badge";
    ok.status = PromiseStatus::Accepted;
    WO_CHECK(reloaded.AddPromise(ok));
    const float before2 = reloaded.PlayerState().reliability;
    WO_CHECK(reloaded.TransitionPromise(PromiseId::New(2), PromiseStatus::Fulfilled, 600, "done"));
    WO_CHECK(reloaded.PlayerState().reliability > before2);
    return true;
}

bool SystemicBodyDragLifecycle() {
    SystemicWorld w;
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(1))));
    WO_CHECK(w.BeginDrag(kPlayer, EntityId::New(20), 10));
    const BodyDragRecord* d = w.GetDrag(EntityId::New(20));
    WO_CHECK(d != nullptr);
    if (!d) return false;
    WO_CHECK(d->sprint_forbidden);
    WO_CHECK(d->weapon_restricted);
    WO_CHECK(w.UpdateDrag(EntityId::New(20), Vec3{5,5,0}, RoomId::New(2), 20));
    const BodyRecord* b = w.GetBody(EntityId::New(20));
    WO_CHECK(b && b->position.x == 5.0f && b->room == RoomId::New(2));
    WO_CHECK(w.EndDrag(EntityId::New(20), 30));
    WO_CHECK(w.GetDrag(EntityId::New(20)) == nullptr);
    return true;
}

bool SystemicBodyHideRejectsAlive() {
    SystemicWorld w;
    BodyRecord b;
    b.id = EntityId::New(20);
    b.npc = NpcId::New(1);
    b.status = BodyStatus::Alive;
    b.room = RoomId::New(1);
    WO_CHECK(w.AddBody(b));
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 1.0f)));
    WO_CHECK(!w.HideBody(EntityId::New(20), ContainerId::New(1)));
    return true;
}

bool SystemicBodyContainerCapacity() {
    SystemicWorld w;
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 0.1f)));
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(1))));
    WO_CHECK(!w.HideBody(EntityId::New(20), ContainerId::New(1)));
    return true;
}

bool SystemicDiscoveryObservationOnly() {
    SystemicWorld w;
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 0.6f)));
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(1))));
    WO_CHECK(w.HideBody(EntityId::New(20), ContainerId::New(1)));
    const FacilityAlertLevel before = w.AlertLevel();
    WO_CHECK(w.DiscoverBody(NpcId::New(2), ContainerId::New(1), 50));
    WO_CHECK(w.AlertLevel() == before);
    WO_CHECK(w.EventCount() == 1);
    return true;
}

bool SystemicDiscoveryMedicalDoesNotEscalateSecurity() {
    SystemicWorld w;
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 0.6f)));
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(1))));
    WO_CHECK(w.HideBody(EntityId::New(20), ContainerId::New(1)));
    WO_CHECK(w.DiscoverBody(NpcId::New(2), ContainerId::New(1), 60));
    const EventId eid = w.Events().back().id;
    WO_CHECK(w.ApplyDiscoveryResponse(kCleaner, eid, BodyDiscoveryResponse::CallMedical, 70));
    WO_CHECK(w.AlertLevel() == FacilityAlertLevel::Normal);
    const auto& ev = w.Events().back();
    WO_CHECK(ev.type == SystemicEventType::MedicalCall);
    return true;
}

bool SystemicDirectedRelationship() {
    SystemicWorld w;
    RelationshipRecord ab;
    ab.a = EntityId::New(1);
    ab.b = EntityId::New(2);
    ab.trust = 0.8f;
    WO_CHECK(w.SetRelationship(ab));
    WO_CHECK(w.GetRelationship(EntityId::New(1), EntityId::New(2)) != nullptr);
    WO_CHECK(w.GetRelationship(EntityId::New(2), EntityId::New(1)) == nullptr);
    RelationshipRecord ba;
    ba.a = EntityId::New(2);
    ba.b = EntityId::New(1);
    ba.trust = 0.2f;
    WO_CHECK(w.SetRelationship(ba));
    WO_CHECK(w.GetRelationship(EntityId::New(2), EntityId::New(1))->trust == 0.2f);
    WO_CHECK(w.GetRelationship(EntityId::New(1), EntityId::New(2))->trust == 0.8f);
    return true;
}

bool SystemicPromiseTransitionMatrix() {
    SystemicWorld w;
    PromiseRecord p;
    p.id = PromiseId::New(1);
    p.status = PromiseStatus::Offered;
    WO_CHECK(w.AddPromise(p));
    // Offered -> Accepted legal.
    WO_CHECK(w.TransitionPromise(PromiseId::New(1), PromiseStatus::Accepted, 1, ""));
    // Accepted -> Fulfilled legal.
    WO_CHECK(w.TransitionPromise(PromiseId::New(1), PromiseStatus::Fulfilled, 2, ""));
    // Terminal states reject.
    WO_CHECK(!w.TransitionPromise(PromiseId::New(1), PromiseStatus::Broken, 3, ""));

    PromiseRecord p2;
    p2.id = PromiseId::New(2);
    p2.status = PromiseStatus::Offered;
    WO_CHECK(w.AddPromise(p2));
    WO_CHECK(w.TransitionPromise(PromiseId::New(2), PromiseStatus::Cancelled, 1, ""));
    WO_CHECK(!w.TransitionPromise(PromiseId::New(2), PromiseStatus::Accepted, 2, ""));
    return true;
}

bool SystemicQuestTransitionMatrix() {
    SystemicWorld w;
    QuestRecord q;
    q.id = QuestId::New(1);
    q.title = "task";
    q.presentation_objective = "presentation";
    q.status = QuestStatus::Offered;
    WO_CHECK(w.AddQuest(q));
    WO_CHECK(w.TransitionQuest(QuestId::New(1), QuestStatus::Accepted, 1, ""));
    WO_CHECK(w.TransitionQuest(QuestId::New(1), QuestStatus::Active, 2, ""));
    WO_CHECK(w.TransitionQuest(QuestId::New(1), QuestStatus::Completed, 3, ""));
    WO_CHECK(!w.TransitionQuest(QuestId::New(1), QuestStatus::Failed, 4, ""));
    return true;
}

bool SystemicEventBridgeExactlyOnce() {
    SystemicWorld w;
    WorldEvent we;
    we.id = EventId::New(77);
    we.sim_frame = 5;
    we.payload = EventNpcSpeak{NpcId::New(1), StringId::New(1)};
    WO_CHECK(w.BridgeWorldEventOnce(we));
    const size_t count = w.EventCount();
    WO_CHECK(!w.BridgeWorldEventOnce(we));
    WO_CHECK(w.EventCount() == count);
    if (count > 0) {
        WO_CHECK(w.Events().back().source_world_event_id == we.id);
    }
    return true;
}

bool SystemicItemTheftReturnAuthorization() {
    SystemicWorld w;
    ItemRecord item = MakeBadge(ItemId::New(1), kGuard);
    item.current_holder = kGuard;
    WO_CHECK(w.AddItem(item));
    WO_CHECK(w.TheftItem(ItemId::New(1), kPlayer, 10));
    WO_CHECK(w.GetItem(ItemId::New(1))->current_holder == kPlayer);
    WO_CHECK(w.GetItem(ItemId::New(1))->reported_stolen);
    WO_CHECK(w.RevokeCredential(ItemId::New(1), 20));
    WO_CHECK(!w.ReaderAcceptsItem(ItemId::New(1), 2));
    WO_CHECK(w.RestoreAuthorization(ItemId::New(1), 30));
    WO_CHECK(w.ReaderAcceptsItem(ItemId::New(1), 2));
    WO_CHECK(w.ReturnItem(ItemId::New(1), 40));
    WO_CHECK(w.GetItem(ItemId::New(1))->current_holder == kGuard);
    return true;
}

bool SystemicTerminalSessionAudit() {
    SystemicWorld w;
    TerminalRecord t;
    t.id = TerminalId::New(1);
    t.room = RoomId::New(1);
    t.credential_requirement = 2;
    WO_CHECK(w.AddTerminal(t));
    TerminalSession s;
    s.terminal = TerminalId::New(1);
    s.user = kPlayer;
    s.method = TerminalAccessMethod::Credential;
    s.active = true;
    WO_CHECK(w.AddTerminalSession(s));
    TerminalAuditLog a;
    a.terminal = TerminalId::New(1);
    a.user = kPlayer;
    a.method = TerminalAccessMethod::Credential;
    a.action = "door_open";
    a.unauthorized = false;
    WO_CHECK(w.AddTerminalAudit(a));
    WO_CHECK(w.TerminalCount() == 1);
    WO_CHECK(w.TerminalSessionCount() == 1);
    WO_CHECK(w.TerminalAuditCount() == 1);
    return true;
}

bool SystemicNarratorObservabilityPerSource() {
    SystemicWorld w;
    ObservationSource cam;
    cam.id = ObservationSourceId::New(1);
    cam.type = ObservationSourceType::Camera;
    cam.room = RoomId::New(1);
    cam.online = true;
    WO_CHECK(w.AddObservationSource(cam));
    ObservationSource mic;
    mic.id = ObservationSourceId::New(2);
    mic.type = ObservationSourceType::Microphone;
    mic.room = RoomId::New(2);
    mic.online = true;
    WO_CHECK(w.AddObservationSource(mic));
    WO_CHECK(w.NarratorCanObserveRoom(RoomId::New(1), ObservationSourceType::Camera));
    WO_CHECK(w.NarratorCanObserveRoom(RoomId::New(2), ObservationSourceType::Microphone));
    WO_CHECK(!w.NarratorCanObserveRoom(RoomId::New(2), ObservationSourceType::Camera));

    ObservationSource offline;
    offline.id = ObservationSourceId::New(3);
    offline.type = ObservationSourceType::Camera;
    offline.room = RoomId::New(1);
    offline.online = false;
    WO_CHECK(w.AddObservationSource(offline));
    // The remaining online camera source is still enough; a single source
    // being offline does not disable all sensors.
    WO_CHECK(w.NarratorCanObserveRoom(RoomId::New(1), ObservationSourceType::Camera));
    WO_CHECK(!w.NarratorObserves(ObservationSourceId::New(3)));
    return true;
}

bool SystemicSaveCorruptionRejected() {
    SystemicWorld w;
    // Build a valid world then corrupt the wire in several ways.
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(1))));
    const auto bytes = w.Serialize();
    {
        // Truncated buffer must fail.
        const auto res = SystemicWorld::Deserialize(bytes.data(), bytes.size() - 1);
        WO_CHECK(res.IsError());
    }
    {
        // Huge count simulated by memory not directly possible without custom
        // bytes; at least require the deserializer to reject empty data.
        const auto res = SystemicWorld::Deserialize(nullptr, 0);
        WO_CHECK(res.IsError());
    }
    {
        // Invalid enum: make a tiny actor buffer with bad faction byte.
        std::vector<uint8_t> bad = bytes;
        // First actor first enum byte after id(8)+data_key(8) + faction at 16.
        if (bad.size() > 16) {
            bad[20] = 0xFF;
            const auto res = SystemicWorld::Deserialize(bad.data(), bad.size());
            WO_CHECK(res.IsError());
        }
    }
    return true;
}

bool SystemicSaveSectionRoundTrip() {
    SystemicWorld w;
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(5))));
    ItemRecord item = MakeBadge(ItemId::New(1), EntityId::New(5));
    item.current_holder = EntityId::New(1);
    WO_CHECK(w.AddItem(item));

    const std::vector<uint8_t> systemic_bytes = w.Serialize();
    std::vector<SaveSection> sections;
    sections.push_back({SaveSectionId::Systemic, systemic_bytes});
    const auto wire = ComposeSaveBuffer(sections);
    const auto parsed = ParseSaveBuffer(wire.data(), wire.size());
    WO_CHECK(parsed.IsOk());
    if (!parsed.IsOk()) return false;
    const auto& loaded = parsed.Value()[0];
    const auto res = SystemicWorld::Deserialize(loaded.data.data(), loaded.data.size());
    WO_CHECK(res.IsOk());
    if (!res.IsOk()) return false;
    SystemicWorld reloaded = res.Value();
    WO_CHECK(reloaded.ActorCount() == 1);
    WO_CHECK(reloaded.ItemCount() == 1);
    return true;
}

bool SystemicRuntimeSeedLoadsInvalidRejected() {
    // Valid seed bytes: header + empty sections.
    std::vector<uint8_t> valid;
    {
        Serializer s(valid);
        s.WriteU32(0x574F5344u);
        s.WriteU32(1);
        s.WriteU32(0); // actors
        s.WriteU32(0); // items
        s.WriteU32(0); // containers
        s.WriteU32(0); // evidence
        s.WriteU32(0); // promises
    }
    SystemicWorld w;
    const auto ok = w.LoadSeedBytes(valid.data(), valid.size());
    WO_CHECK(ok.IsOk());

    // Invalid magic/version must fail closed.
    std::vector<uint8_t> bad = valid;
    bad[0] = 0;
    SystemicWorld w2;
    const auto err = w2.LoadSeedBytes(bad.data(), bad.size());
    WO_CHECK(err.IsError());
    return true;
}

bool SystemicDiscoveryReportEscalatesAlert() {
    SystemicWorld w;
    WO_CHECK(w.AddContainer(MakeCart(ContainerId::New(1), 0.6f)));
    WO_CHECK(w.AddBody(MakeUnconsciousBody(EntityId::New(20), NpcId::New(1))));
    WO_CHECK(w.HideBody(EntityId::New(20), ContainerId::New(1)));
    WO_CHECK(w.DiscoverBody(NpcId::New(2), ContainerId::New(1), 10));
    const EventId eid = w.Events().back().id;
    WO_CHECK(w.ApplyDiscoveryResponse(kCleaner, eid, BodyDiscoveryResponse::ReportSecurity, 20));
    WO_CHECK(static_cast<int>(w.AlertLevel()) >= static_cast<int>(FacilityAlertLevel::Suspicious));
    return true;
}

bool SystemicNarratorAuthorityVsObservability() {
    SystemicWorld w;
    w.NarratorAuthority().authority_stage = 3;
    w.NarratorAuthority().intervention_cost = 2.0f;
    WO_CHECK(!w.NarratorCanObserveRoom(RoomId::New(1), ObservationSourceType::Camera));
    WO_CHECK(w.NarratorAuthority().authority_stage == 3);
    ObservationSource cam;
    cam.id = ObservationSourceId::New(1);
    cam.type = ObservationSourceType::Camera;
    cam.room = RoomId::New(1);
    cam.online = false;
    WO_CHECK(w.AddObservationSource(cam));
    WO_CHECK(!w.NarratorObserves(ObservationSourceId::New(1)));
    cam.online = true;
    cam.id = ObservationSourceId::New(2);
    cam.room = RoomId::New(2);
    WO_CHECK(w.AddObservationSource(cam));
    // Disabling one source does not disable another.
    WO_CHECK(w.NarratorCanObserveRoom(RoomId::New(2), ObservationSourceType::Camera));
    return true;
}

bool SystemicRuntimeSaveLoadRoundtrip() {
    SystemicWorld w;
    WO_CHECK(w.AddActor(MakeGuard(NpcId::New(1))));
    WO_CHECK(w.AddItem(MakeBadge(ItemId::New(1), kGuard)));
    const auto bytes = w.Serialize();
    const auto res = SystemicWorld::Deserialize(bytes.data(), bytes.size());
    WO_CHECK(res.IsOk());
    if (!res.IsOk()) return false;
    SystemicWorld loaded = res.Value();
    WO_CHECK(loaded.ActorCount() == 1);
    WO_CHECK(loaded.ItemCount() == 1);
    return true;
}
} // namespace

bool SystemicRuntimeSeedLoadsFile();
void RegisterSystemicTests(TestHarness& test) {
    test.Add("systemic.body_concealment_chain", &SystemicBodyConcealmentChain);
    test.Add("systemic.stolen_identity_chain", &SystemicStolenIdentityChain);
    test.Add("systemic.promise_chain", &SystemicPromiseChain);
    test.Add("systemic.body_drag_lifecycle", &SystemicBodyDragLifecycle);
    test.Add("systemic.body_hide_rejects_alive", &SystemicBodyHideRejectsAlive);
    test.Add("systemic.body_container_capacity", &SystemicBodyContainerCapacity);
    test.Add("systemic.discovery_observation_only", &SystemicDiscoveryObservationOnly);
    test.Add("systemic.discovery_medical_does_not_escalate_security", &SystemicDiscoveryMedicalDoesNotEscalateSecurity);
    test.Add("systemic.directed_relationship", &SystemicDirectedRelationship);
    test.Add("systemic.promise_transition_matrix", &SystemicPromiseTransitionMatrix);
    test.Add("systemic.event_bridge_exactly_once", &SystemicEventBridgeExactlyOnce);
    test.Add("systemic.quest_transition_matrix", &SystemicQuestTransitionMatrix);
    test.Add("systemic.item_theft_return_authorization", &SystemicItemTheftReturnAuthorization);
    test.Add("systemic.terminal_session_audit", &SystemicTerminalSessionAudit);
    test.Add("systemic.narrator_observability_per_source", &SystemicNarratorObservabilityPerSource);
    test.Add("systemic.narrator_authority_vs_observability", &SystemicNarratorAuthorityVsObservability);
    test.Add("systemic.save_corruption_rejected", &SystemicSaveCorruptionRejected);
    test.Add("systemic.save_section_round_trip", &SystemicSaveSectionRoundTrip);
    test.Add("systemic.runtime_save_load_roundtrip", &SystemicRuntimeSaveLoadRoundtrip);
    test.Add("systemic.discovery_report_escalates_alert", &SystemicDiscoveryReportEscalatesAlert);
    test.Add("systemic.runtime_seed_loads_invalid_rejected", &SystemicRuntimeSeedLoadsInvalidRejected);
    test.Add("systemic.runtime_seed_loads_file", &SystemicRuntimeSeedLoadsFile);
}


bool SystemicRuntimeSeedLoadsFile() {
    std::string derived = __FILE__;
    const size_t slash = derived.find_last_of("\\/");
    if (slash != std::string::npos) {
        derived = derived.substr(0, slash) + "/../data/systemic/systemic_seed.bin";
    }
    const std::vector<std::string> candidates = {
        "data/systemic/systemic_seed.bin",
        derived,
        "../data/systemic/systemic_seed.bin",
        "../../data/systemic/systemic_seed.bin",
        "../../../data/systemic/systemic_seed.bin",
    };
    SystemicWorld w;
    bool loaded = false;
    for (const auto& path : candidates) {
        if (path.empty()) continue;
        const auto res = w.LoadSeedBinary(path);
        if (res.IsOk()) { loaded = true; break; }
    }
    WO_CHECK(loaded);
    if (!loaded) return false;
    WO_CHECK(w.ActorCount() >= 2);
    WO_CHECK(w.ItemCount() >= 1);
    WO_CHECK(w.ContainerCount() >= 1);
    return true;
}
} // namespace writeover
