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

bool SystemicBodyConcealmentChain() {
    SystemicWorld w;

    // Identity-bearing guard.
    ActorRecord guard;
    guard.id = NpcId::New(10);
    guard.data_key = ResourceId::New(1001);
    guard.faction = Faction::Security;
    guard.actor_class = ActorClass::Guard;
    w.AddActor(guard);

    // Cleaning cart is a large hideable container, not a tiny trash bin.
    HideableContainer cart;
    cart.id = ContainerId::New(1);
    cart.kind = ContainerKind::CleaningCart;
    cart.room = RoomId::New(1);
    cart.position = Vec3{4.0f, 4.0f, 0.0f};
    cart.capacity_volume = 0.6f;
    cart.concealment = 95;
    cart.accessibility = 80;
    cart.routine_tags.push_back(RoutineTag::Cleaner);
    w.AddContainer(cart);

    // Guard is incapacitated; player searches and takes the badge first.
    BodyRecord body;
    body.id = EntityId::New(20);
    body.npc = NpcId::New(10);
    body.status = BodyStatus::Unconscious;
    body.disposition = BodyDisposition::Exposed;
    body.room = RoomId::New(1);
    body.position = Vec3{3.0f, 4.0f, 0.0f};
    w.AddBody(body);

    ItemRecord badge;
    badge.id = ItemId::New(1);
    badge.type = ItemType::Badge;
    badge.owner = kGuard;
    badge.issuer = kGuard;
    badge.legal_holder = kGuard;
    badge.current_holder = kPlayer;          // stolen and now in player inventory
    badge.credential_level = 2;
    badge.provenance_tags.push_back("security_badge");
    badge.provenance_tags.push_back("owner_guard_10");
    w.AddItem(badge);

    // The body is visible before being hidden.
    WO_CHECK(w.CanDirectlyObserveBody(kPatrol, EntityId::New(20)));

    // Player drags and hides the guard in the cleaning cart.
    WO_CHECK(w.HideBody(EntityId::New(20), ContainerId::New(1)));

    // A patrol passes but does NOT see the hidden body.
    WO_CHECK(!w.CanDirectlyObserveBody(kPatrol, EntityId::New(20)));

    // Routine advances; cleaner opens the cleaning cart and discovers the body.
    WO_CHECK(w.DiscoverBody(NpcId::New(11), ContainerId::New(1), 1000));

    // The body is not deleted; it remains in WorldState, now exposed.
    const BodyRecord* found_body = w.GetBody(EntityId::New(20));
    WO_CHECK(found_body != nullptr);
    if (!found_body) return false;
    WO_CHECK(found_body->disposition == BodyDisposition::Exposed);
    WO_CHECK(!found_body->container.IsValid());

    // Cleaner has a persistent memory and a report-equivalent event.
    const auto cleaner_memories = w.MemoriesOf(kCleaner);
    WO_CHECK(!cleaner_memories.empty());
    if (!cleaner_memories.empty()) {
        WO_CHECK(cleaner_memories[0].kind == MemoryKind::BodyDiscovery);
    }
    WO_CHECK(w.EvidenceCount() == 1);
    WO_CHECK(w.EventCount() > 0);
    if (w.EventCount() > 0) {
        WO_CHECK(w.Events().back().type == SystemicEventType::BodyDiscovered);
        WO_CHECK(w.Events().back().outcome == OutcomeType::Discovered);
    }

    // Local alert has escalated.
    WO_CHECK(static_cast<int>(w.AlertLevel()) >=
             static_cast<int>(FacilityAlertLevel::LocalAlert));

    // Save / load: all state remains consistent including stolen badge.
    const std::vector<uint8_t> bytes = w.Serialize();
    SystemicWorld reloaded = SystemicWorld::Deserialize(bytes.data(), bytes.size());

    const BodyRecord* r_body = reloaded.GetBody(EntityId::New(20));
    WO_CHECK(r_body != nullptr);
    if (!r_body) return false;
    WO_CHECK(r_body->disposition == BodyDisposition::Exposed);
    WO_CHECK(reloaded.EvidenceCount() == 1);
    WO_CHECK(reloaded.MemoryCount() == 1);
    WO_CHECK(reloaded.AlertLevel() == FacilityAlertLevel::LocalAlert);

    const ItemRecord* r_badge = reloaded.GetItem(ItemId::New(1));
    WO_CHECK(r_badge != nullptr);
    if (!r_badge) return false;
    WO_CHECK(r_badge->current_holder == kPlayer);
    WO_CHECK(r_badge->owner == kGuard);
    WO_CHECK(r_badge->credential_level == 2);
    WO_CHECK(r_badge->provenance_tags.size() == 2);

    // The badge itself is still a functional reader credential.
    WO_CHECK(reloaded.ReaderAcceptsItem(ItemId::New(1), 2));

    return true;
}

bool SystemicStolenIdentityChain() {
    SystemicWorld w;

    ActorRecord owner_guard;
    owner_guard.id = NpcId::New(1);
    owner_guard.data_key = ResourceId::New(2001);
    owner_guard.faction = Faction::Security;
    owner_guard.actor_class = ActorClass::Guard;
    w.AddActor(owner_guard);

    ActorRecord unknown_guard;
    unknown_guard.id = NpcId::New(2);
    unknown_guard.data_key = ResourceId::New(2002);
    unknown_guard.faction = Faction::Security;
    unknown_guard.actor_class = ActorClass::Guard;
    w.AddActor(unknown_guard);

    ActorRecord knowing_guard;
    knowing_guard.id = NpcId::New(3);
    knowing_guard.data_key = ResourceId::New(2003);
    knowing_guard.faction = Faction::Security;
    knowing_guard.actor_class = ActorClass::Guard;
    knowing_guard.known_identities.push_back(kGuard);   // personally knows owner A
    w.AddActor(knowing_guard);

    ItemRecord badge;
    badge.id = ItemId::New(1);
    badge.type = ItemType::Badge;
    badge.owner = kGuard;
    badge.issuer = kGuard;
    badge.legal_holder = kGuard;
    badge.current_holder = kPlayer;
    badge.reported_stolen = true;                       // reported, but reader still accepts
    badge.credential_level = 2;
    w.AddItem(badge);

    // Reader accepts stolen badge (physical credential, not social knowledge).
    WO_CHECK(w.ReaderAcceptsItem(ItemId::New(1), 2));

    // Unknown guard accepts the presented identity.
    WO_CHECK(w.NpcAcceptsPresentedIdentity(NpcId::New(2), kGuard, kPlayer));

    // Guard who personally knows owner A recognizes the mismatch.
    WO_CHECK(!w.NpcAcceptsPresentedIdentity(NpcId::New(3), kGuard, kPlayer));

    // Reaction is relationship-dependent, not a fixed "evil points" result.
    RelationshipRecord helpful;
    helpful.a = EntityId::New(3);   // knowing guard
    helpful.b = kPlayer;
    helpful.trust = 0.9f;
    helpful.fear = 0.1f;
    helpful.debt = 0.7f;
    w.SetRelationship(helpful);
    WO_CHECK(w.ReactionToIdentityMismatch(NpcId::New(3), kGuard, kPlayer) ==
             IdentityReaction::HelpCoverUp);

    RelationshipRecord hostile;
    hostile.a = EntityId::New(4);   // a different knowing observer
    hostile.b = kPlayer;
    hostile.trust = 0.1f;
    hostile.fear = 0.8f;
    hostile.debt = 0.0f;
    // Register a second knowing observer.
    ActorRecord second_knowing;
    second_knowing.id = NpcId::New(4);
    second_knowing.faction = Faction::Security;
    second_knowing.known_identities.push_back(kGuard);
    w.AddActor(second_knowing);
    w.SetRelationship(hostile);
    WO_CHECK(w.ReactionToIdentityMismatch(NpcId::New(4), kGuard, kPlayer) ==
             IdentityReaction::Report);

    // Save/load preserves identity knowledge and reactions.
    const auto bytes = w.Serialize();
    SystemicWorld reloaded = SystemicWorld::Deserialize(bytes.data(), bytes.size());
    WO_CHECK(reloaded.ReaderAcceptsItem(ItemId::New(1), 2));
    WO_CHECK(reloaded.NpcAcceptsPresentedIdentity(NpcId::New(2), kGuard, kPlayer));
    WO_CHECK(!reloaded.NpcAcceptsPresentedIdentity(NpcId::New(3), kGuard, kPlayer));
    WO_CHECK(reloaded.ReactionToIdentityMismatch(NpcId::New(3), kGuard, kPlayer) ==
             IdentityReaction::HelpCoverUp);

    return true;
}

bool SystemicPromiseChain() {
    SystemicWorld w;

    RelationshipRecord rel;
    rel.a = EntityId::New(10);       // NPC giver
    rel.b = kPlayer;
    rel.trust = 0.5f;
    rel.debt = 0.2f;
    w.SetRelationship(rel);

    PromiseRecord promise;
    promise.id = PromiseId::New(1);
    promise.giver = EntityId::New(10);
    promise.receiver = kPlayer;
    promise.subject = "find_intern";
    promise.accepted_frame = 100;
    promise.status = PromiseStatus::Accepted;
    w.AddPromise(promise);

    const float reliability_before = w.PlayerState().reliability;
    const float trust_before = w.GetRelationship(EntityId::New(10), kPlayer)->trust;
    const float debt_before = w.GetRelationship(EntityId::New(10), kPlayer)->debt;

    // Accept -> save/load -> later failure must generate memory callback state.
    const auto bytes = w.Serialize();
    SystemicWorld reloaded = SystemicWorld::Deserialize(bytes.data(), bytes.size());
    WO_CHECK(reloaded.GetPromise(PromiseId::New(1)) != nullptr);
    if (!reloaded.GetPromise(PromiseId::New(1))) return false;

    WO_CHECK(reloaded.SettlePromise(PromiseId::New(1), PromiseStatus::Broken, 500));

    const PromiseRecord* settled = reloaded.GetPromise(PromiseId::New(1));
    WO_CHECK(settled != nullptr);
    if (!settled) return false;
    WO_CHECK(settled->status == PromiseStatus::Broken);
    WO_CHECK(settled->storylet_eligible);

    WO_CHECK(reloaded.PlayerState().reliability < reliability_before);
    WO_CHECK(reloaded.PlayerState().promises_broken == 1);
    WO_CHECK(reloaded.GetRelationship(EntityId::New(10), kPlayer)->trust < trust_before);
    WO_CHECK(reloaded.GetRelationship(EntityId::New(10), kPlayer)->debt > debt_before);
    WO_CHECK(!reloaded.MemoriesOf(EntityId::New(10)).empty());
    WO_CHECK(reloaded.EventCount() > 0);

    // Also verify fulfilment path in a separate promise.
    PromiseRecord fulfilled;
    fulfilled.id = PromiseId::New(2);
    fulfilled.giver = EntityId::New(11);
    fulfilled.receiver = kPlayer;
    fulfilled.subject = "report_schedule";
    fulfilled.accepted_frame = 200;
    fulfilled.status = PromiseStatus::Accepted;
    reloaded.AddPromise(fulfilled);
    const float rel_before = reloaded.PlayerState().reliability;
    WO_CHECK(reloaded.SettlePromise(PromiseId::New(2), PromiseStatus::Fulfilled, 600));
    WO_CHECK(reloaded.PlayerState().reliability > rel_before);
    WO_CHECK(reloaded.Events().back().type == SystemicEventType::PromiseFulfilled);

    return true;
}

bool SystemicSaveSectionRoundTrip() {
    SystemicWorld w;
    ActorRecord actor;
    actor.id = NpcId::New(5);
    actor.data_key = ResourceId::New(3001);
    actor.faction = Faction::Medical;
    actor.actor_class = ActorClass::SemiHuman;
    w.AddActor(actor);

    ItemRecord item;
    item.id = ItemId::New(1);
    item.type = ItemType::Badge;
    item.owner = EntityId::New(5);
    item.current_holder = EntityId::New(1);
    item.credential_level = 1;
    w.AddItem(item);

    const std::vector<uint8_t> systemic_bytes = w.Serialize();
    std::vector<SaveSection> sections;
    sections.push_back({SaveSectionId::Systemic, systemic_bytes});
    const auto wire = ComposeSaveBuffer(sections);
    const auto parsed = ParseSaveBuffer(wire.data(), wire.size());
    WO_CHECK(parsed.IsOk());
    if (!parsed.IsOk()) return false;

    WO_CHECK_EQ(static_cast<int64_t>(parsed.Value().size()), 1);
    const auto& loaded_section = parsed.Value()[0];
    SystemicWorld reloaded =
        SystemicWorld::Deserialize(loaded_section.data.data(),
                                   loaded_section.data.size());
    WO_CHECK(reloaded.ActorCount() == 1);
    WO_CHECK(reloaded.ItemCount() == 1);
    const ItemRecord* r = reloaded.GetItem(ItemId::New(1));
    WO_CHECK(r != nullptr);
    if (!r) return false;
    WO_CHECK(r->current_holder == EntityId::New(1));
    return true;
}

} // namespace

void RegisterSystemicTests(TestHarness& test) {
    test.Add("systemic.body_concealment_chain",
             &SystemicBodyConcealmentChain);
    test.Add("systemic.stolen_identity_chain",
             &SystemicStolenIdentityChain);
    test.Add("systemic.promise_chain",
             &SystemicPromiseChain);
    test.Add("systemic.save_section_round_trip",
             &SystemicSaveSectionRoundTrip);
}

} // namespace writeover
