#include "writeover/render/benchmark.h"
#include "writeover/render/frame_encoder.h"
#include "writeover/render/raycaster.h"
#include "writeover/systemic/systemic.h"
#include "writeover/world/grid.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

namespace writeover {

namespace {
// Synthetic stress grid: 64x64 arena with scattered solid pillars.
Grid MakeStressGrid() {
    Grid grid(64, 64);
    for (int32_t r = 0; r < 64; ++r) {
        for (int32_t c = 0; c < 64; ++c) {
            GridCell cell;
            if ((c % 7 == 0 && r % 5 == 0) || c == 63 || r == 63 || c == 0 || r == 0) {
                cell.flags = CellFlag_Solid;
            }
            grid.SetCell(c, r, cell);
        }
    }
    return grid;
}

// Representative gameplay-like 240x67 frame: ceiling run, floor run, wall
// spans, a HUD band and a little text — NOT per-cell rainbow noise. phase
// only nudges a small marker region (kept tiny for delta realism).
void MakeRepresentativeFrame(std::vector<CharCell>& frame, int w, int h,
                             uint8_t phase) {
    frame.resize(static_cast<size_t>(w) * h);
    const int horizon = h * 2 / 5;   // wall band from horizon to horizon+8
    const int floor_top = horizon + 9;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            CharCell& c = frame[static_cast<size_t>(y) * w + x];
            c.code_point = U' ';
            c.flags = 0;
            if (y < horizon) {
                // Ceiling: one dark blue-gray run.
                c.bg_r = 28; c.bg_g = 32; c.bg_b = 68;
                c.fg_r = 60; c.fg_g = 64; c.fg_b = 90;
            } else if (y < floor_top) {
                // Wall span: concrete/metal run with depth-ish variation.
                c.code_point = U'\u2588';
                c.fg_r = 120; c.fg_g = 116; c.fg_b = 108;
                c.bg_r = 120; c.bg_g = 116; c.bg_b = 108;
            } else {
                // Floor: one warm gray run.
                c.code_point = U'\u2591';
                c.fg_r = 88; c.fg_g = 76; c.fg_b = 48;
                c.bg_r = 64; c.bg_g = 54; c.bg_b = 34;
            }
        }
    }
    // HUD band (top two rows): bright text on dark.
    for (int x = 0; x < w; ++x) {
        CharCell& c = frame[static_cast<size_t>(x)];
        c.code_point = U' ';
        c.bg_r = 0; c.bg_g = 0; c.bg_b = 0;
        c.fg_r = 200; c.fg_g = 200; c.fg_b = 200;
    }
    const char* hud = "HP 100  AMMO 12/48  WRITEOVER-07";
    for (int i = 0; hud[i] != '\0'; ++i) {
        frame[static_cast<size_t>(2 + i)].code_point = static_cast<char32_t>(hud[i]);
    }
    // Small moving marker (a few cells) driven by phase.
    const int mx = 10 + static_cast<int>(phase) % (w - 20);
    const int my = floor_top + 2 + (static_cast<int>(phase) / 3) % (h - floor_top - 4);
    for (int dy = 0; dy < 2; ++dy) {
        CharCell& c = frame[static_cast<size_t>(my + dy) * w + mx];
        c.code_point = U'\u25CF';
        c.fg_r = 255; c.fg_g = 220; c.fg_b = 80;
        c.bg_r = 40; c.bg_g = 30; c.bg_b = 10;
    }
}

// Full-screen adversarial color churn (worst case): every cell alternates
// between two very different colors across frames.
void MakeWorstcaseFrame(std::vector<CharCell>& frame, int w, int h, bool on) {
    frame.resize(static_cast<size_t>(w) * h);
    for (size_t i = 0; i < frame.size(); ++i) {
        CharCell& c = frame[i];
        c.code_point = U'\u2588';
        if (on) {
            c.fg_r = 255; c.fg_g = 0; c.fg_b = 0;
            c.bg_r = 0;   c.bg_g = 0;   c.bg_b = 64;
        } else {
            c.fg_r = 0;   c.fg_g = 128; c.fg_b = 255;
            c.bg_r = 64;  c.bg_g = 0;   c.bg_b = 0;
        }
    }
}
// Synthetic systemic simulation benchmark. It uses the real SystemicWorld
// kernel, not a stub: 25 identity-bearing NPCs, 500 evidence records, 500
// memories, 100 world events, 30 hideable containers, 100 items, run at
// 120Hz simulation cadence.
double SystemicKernelLookupBenchmark() {
    using Clock = std::chrono::steady_clock;
    SystemicWorld w;

    // 25 identity NPCs; 5 Full-style high-frequency, 20 semi/offscreen.
    for (int i = 0; i < 25; ++i) {
        ActorRecord a;
        a.id = NpcId::New(i + 1);
        a.data_key = ResourceId::New(1000 + i);
        a.faction = Faction::GeneralStaff;
        a.cognition = i < 5 ? CognitionTier::Full : CognitionTier::SemiHuman; a.role = Role::Guard;
        if (i < 5) {
            a.personality_tags.push_back("full");
        }
        w.AddActor(a);
    }
    for (int i = 0; i < 5; ++i) {
        RelationshipRecord rel;
        rel.a = EntityId::New(i + 1);
        rel.b = EntityId::New(999);
        rel.trust = 0.5f;
        rel.fear = 0.2f;
        rel.suspicion = 0.1f;
        w.SetRelationship(rel);
    }

    // 100 items with ownership/provenance metadata.
    for (int i = 0; i < 100; ++i) {
        ItemRecord item;
        item.id = ItemId::New(i + 1);
        item.type = ItemType::Badge;
        item.owner = EntityId::New((i % 25) + 1);
        item.current_holder = EntityId::New((i % 25) + 1);
        item.credential_level = static_cast<uint8_t>((i % 3) + 1);
        item.provenance_tags.push_back("synthetic");
        w.AddItem(item);
    }

    // 30 hideable containers.
    for (int i = 0; i < 30; ++i) {
        HideableContainer c;
        c.id = ContainerId::New(i + 1);
        c.kind = ContainerKind::CleaningCart;
        c.room = RoomId::New(1);
        c.capacity_volume = 0.6f;
        c.concealment = 80;
        c.routine_tags.push_back(RoutineTag::Cleaner);
        w.AddContainer(c);
    }

    // 500 evidence records.
    for (int i = 0; i < 500; ++i) {
        EvidenceRecord e;
        e.id = EvidenceId::New(i + 1);
        e.type = EvidenceType::VisibleBody;
        e.subject = EntityId::New((i % 25) + 1);
        e.room = RoomId::New(1);
        e.visibility = 0.5f;
        e.persists = true;
        e.frame = static_cast<uint64_t>(i);
        w.AddEvidence(e);
    }

    // 500 structured memories.
    for (int i = 0; i < 500; ++i) {
        MemoryRecord m;
        m.id = MemoryId::New(i + 1);
        m.npc = EntityId::New((i % 25) + 1);
        m.kind = MemoryKind::Observation;
        m.subject = EntityId::New(1);
        m.frame = static_cast<uint64_t>(i);
        m.salience = 0.5f;
        m.confidence = 0.7f;
        m.source = KnowledgeSource::DirectWitness;
        w.AddMemory(m);
    }

    // 100 system events.
    for (int i = 0; i < 100; ++i) {
        SystemicEvent ev;
        ev.id = EventId::New(i + 1);
        ev.type = SystemicEventType::BodyDiscovered;
        ev.actor = EntityId::New(1);
        ev.target = EntityId::New(2);
        ev.location = RoomId::New(1);
        ev.frame = static_cast<uint64_t>(i);
        ev.severity = 50;
        ev.tags.push_back("synthetic");
        w.AddSystemicEvent(ev);
    }

    FrameTimeSampler sampler;
    constexpr int kTicks = 1200;  // 10s at 120Hz
    for (int tick = 0; tick < kTicks; ++tick) {
        const auto t0 = Clock::now();

        // Full-style NPC decision/memory proximity.
        for (int i = 0; i < 5; ++i) {
            (void)w.GetRelationship(EntityId::New(i + 1), EntityId::New(999));
            (void)w.MemoriesOf(EntityId::New(i + 1)).size();
        }
        // Semi/offscreen identity touches.
        for (int i = 5; i < 25; ++i) {
            (void)w.GetActor(NpcId::New(i + 1));
        }
        // Container routine access checks.
        for (int i = 0; i < 30; ++i) {
            const HideableContainer* c = w.GetContainer(ContainerId::New(i + 1));
            if (c) {
                (void)c->current_occupants.size();
            }
        }
        // Item/inventory/credential checks.
        for (int i = 0; i < 100; ++i) {
            if (((tick + i) % 7) == 0) {
                (void)w.ReaderAcceptsItem(ItemId::New(i + 1), 2);
            } else {
                (void)w.GetItem(ItemId::New(i + 1));
            }
        }
        // Evidence and memory lookups.
        for (int i = 0; i < 20; ++i) {
            (void)w.GetEvidence(EvidenceId::New(((tick + i) % 500) + 1));
            (void)w.GetMemory(MemoryId::New(((tick + i) % 500) + 1));
        }
        const auto& events = w.Events();
        if (!events.empty()) {
            (void)events.back().severity;
        }

        const auto t1 = Clock::now();
        sampler.AddSample(
            std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    PrintCsv("systemic_kernel_lookup", sampler.Compute());
    return sampler.Compute().worst_1pct_avg_ms;
}
double SystemicUpdateBenchmark() {
    using Clock = std::chrono::steady_clock;
    SystemicWorld w;

    for (int i = 0; i < 25; ++i) {
        ActorRecord a;
        a.id = NpcId::New(i + 1);
        a.data_key = ResourceId::New(2000 + i);
        a.faction = Faction::GeneralStaff;
        a.cognition = i < 5 ? CognitionTier::Full : CognitionTier::SemiHuman;
        a.role = Role::Guard;
        w.AddActor(a);
    }
    for (int i = 0; i < 5; ++i) {
        RelationshipRecord rel;
        rel.a = EntityId::New(i + 1);
        rel.b = EntityId::New(999);
        rel.trust = 0.5f;
        w.SetRelationship(rel);
    }
    for (int i = 0; i < 100; ++i) {
        ItemRecord item;
        item.id = ItemId::New(i + 1);
        item.type = ItemType::Badge;
        item.owner = EntityId::New((i % 25) + 1);
        item.current_holder = EntityId::New(1);
        item.credential_level = 2;
        w.AddItem(item);
    }
    for (int i = 0; i < 30; ++i) {
        HideableContainer c;
        c.id = ContainerId::New(i + 1);
        c.kind = ContainerKind::CleaningCart;
        c.room = RoomId::New(1);
        c.capacity_volume = 0.6f;
        c.concealment = 80;
        w.AddContainer(c);
    }
    for (int i = 0; i < 500; ++i) {
        EvidenceRecord e;
        e.id = EvidenceId::New(i + 1);
        e.type = EvidenceType::VisibleBody;
        e.room = RoomId::New(1);
        e.visibility = 0.5f;
        w.AddEvidence(e);
    }
    for (int i = 0; i < 500; ++i) {
        MemoryRecord m;
        m.id = MemoryId::New(i + 1);
        m.npc = EntityId::New((i % 25) + 1);
        m.kind = MemoryKind::Observation;
        m.salience = 0.5f;
        m.confidence = 0.7f;
        m.source = KnowledgeSource::DirectWitness;
        w.AddMemory(m);
    }
    for (int i = 0; i < 100; ++i) {
        SystemicEvent ev;
        ev.id = EventId::New(i + 1);
        ev.type = SystemicEventType::Generic;
        ev.frame = static_cast<uint64_t>(i);
        w.AddSystemicEvent(ev);
    }
    BodyRecord body;
    body.id = EntityId::New(20);
    body.npc = NpcId::New(1);
    body.status = BodyStatus::Unconscious;
    body.room = RoomId::New(1);
    w.AddBody(body);
    w.BeginDrag(EntityId::New(1), EntityId::New(20), 0);

    FrameTimeSampler sampler;
    constexpr int kTicks = 1200;
    for (int tick = 0; tick < kTicks; ++tick) {
        const auto t0 = Clock::now();

        for (int i = 0; i < 5; ++i) {
            (void)w.GetRelationship(EntityId::New(i + 1), EntityId::New(999));
            RelationshipRecord r;
            r.a = EntityId::New(i + 1);
            r.b = EntityId::New(999);
            r.trust = 0.4f + 0.001f * static_cast<float>(i + tick);
            w.SetRelationship(r);
        }

        MemoryRecord m;
        m.id = MemoryId::New(1000 + tick);
        m.npc = EntityId::New(1);
        m.kind = MemoryKind::Observation;
        m.salience = 0.5f;
        m.confidence = 0.7f;
        w.AddMemory(m);
        (void)w.MemoriesOf(EntityId::New(1)).size();

        EvidenceRecord e;
        e.id = EvidenceId::New(2000 + tick);
        e.type = EvidenceType::Blood;
        e.room = RoomId::New(1);
        e.visibility = 0.5f;
        w.AddEvidence(e);
        (void)w.GetEvidence(EvidenceId::New((tick % 500) + 1));

        (void)w.UpdateDrag(EntityId::New(20), Vec3{1.0f + tick * 0.001f, 2.0f, 0.0f},
                           RoomId::New(1), static_cast<uint64_t>(tick));

        if ((tick % 100) == 0) {
            ItemRecord item;
            item.id = ItemId::New(200 + tick);
            item.type = ItemType::Badge;
            item.owner = EntityId::New(1);
            item.credential_level = 2;
            w.AddItem(item);
            w.TheftItem(item.id, EntityId::New(2), static_cast<uint64_t>(tick));
        } else {
            (void)w.GetItem(ItemId::New((tick % 100) + 1));
        }

        PromiseRecord p;
        p.id = PromiseId::New(5000 + tick);
        p.status = PromiseStatus::Offered;
        p.giver = EntityId::New(1);
        p.receiver = EntityId::New(2);
        p.subject = "bench";
        w.AddPromise(p);
        w.TransitionPromise(p.id, PromiseStatus::Accepted, static_cast<uint64_t>(tick), "");

        if ((tick % 10) == 0) {
            QuestRecord q;
            q.id = QuestId::New(500 + tick);
            q.title = "bench";
            q.status = QuestStatus::Offered;
            w.AddQuest(q);
            w.TransitionQuest(q.id, QuestStatus::Accepted, static_cast<uint64_t>(tick), "");
            w.TransitionQuest(q.id, QuestStatus::Active, static_cast<uint64_t>(tick), "");
        }

        SearchAction search;
        search.actor = EntityId::New(1);
        search.target = EntityId::New(20);
        search.target_type = SearchTargetType::Body;
        search.room = RoomId::New(1);
        search.frame = static_cast<uint64_t>(tick);
        (void)w.PerformSearch(search);

        SocialExchangeRecord ex;
        ex.id = SocialExchangeId::New(7000 + tick);
        ex.type = SocialExchangeType::Give;
        ex.actor = EntityId::New(1);
        ex.target = EntityId::New(2);
        ex.outcome = SocialExchangeOutcome::Accepted;
        ex.frame = static_cast<uint64_t>(tick);
        w.AddSocialExchange(ex);

        TerminalAuditLog audit;
        audit.terminal = TerminalId::New(1);
        audit.user = EntityId::New(1);
        audit.action = "read";
        audit.frame = static_cast<uint64_t>(tick);
        w.AddTerminalAudit(audit);

        WorldEvent we;
        we.id = EventId::New(9000 + tick);
        we.sim_frame = static_cast<uint64_t>(tick);
        we.payload = EventNpcSpeak{NpcId::New(1), StringId::New(1)};
        w.BridgeWorldEventOnce(we);

        const auto t1 = Clock::now();
        sampler.AddSample(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }

    PrintCsv("systemic_update_workload", sampler.Compute());
    return sampler.Compute().worst_1pct_avg_ms;
}
} // namespace

} // namespace writeover

int main() {
    using Clock = std::chrono::steady_clock;

    writeover::Grid grid = writeover::MakeStressGrid();
    const int grid_w = grid.Width();
    const int grid_h = grid.Height();
    const writeover::GridCell* cells = grid.Data().data();

    // ---- A. raycast column sweep (240 columns) ----
    constexpr int kColumns = 240;
    constexpr int kFrames = 240;  // ~2s of render at 120Hz
    writeover::FrameTimeSampler ray_sampler;

    for (int frame = 0; frame < kFrames; ++frame) {
        const auto t0 = Clock::now();
        for (int col = 0; col < kColumns; ++col) {
            writeover::RayConfig cfg;
            cfg.origin_xy = writeover::Vec2{
                2.5f + static_cast<float>(frame % 3),
                2.5f + static_cast<float>(frame % 5)};
            cfg.yaw = 0.02f * static_cast<float>(col) +
                      0.001f * static_cast<float>(frame);
            (void)writeover::CastColumnRay(cfg, cells, grid_w, grid_h);
        }
        const auto t1 = Clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        ray_sampler.AddSample(ms);
    }

    writeover::PrintCsv("raycast_column_sweep", ray_sampler.Compute());
    const writeover::FrameStats ray_stats = ray_sampler.Compute();
    const bool ray_pass = ray_stats.worst_1pct_avg_ms < 4.0;

    // ---- Terminal encoder benchmarks (240x67 = 16080 cells) ----
    constexpr int kTw = 240;
    constexpr int kTh = 67;
    constexpr int kEncFrames = 120;
    std::vector<writeover::CharCell> base_frame;
    std::vector<writeover::CharCell> work_frame;
    std::string scratch;

    // ---- B. FULL representative ----
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        for (int frame = 0; frame < kEncFrames; ++frame) {
            writeover::MakeRepresentativeFrame(base_frame, kTw, kTh,
                                               static_cast<uint8_t>(frame));
            writeover::AnsiFrameEncoder enc;  // fresh -> FULL
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(base_frame.data(), kTw, kTh, scratch,
                                        writeover::EncodeMode::ForceFull);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_full_representative", enc_sampler.Compute());
        std::printf("TERMINAL_FULL_MAX_BYTES=%zu\n", max_bytes);
    }

    // ---- C. DELTA typical: one base frame, then small local changes ----
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        writeover::MakeRepresentativeFrame(base_frame, kTw, kTh, 0);
        writeover::AnsiFrameEncoder enc;
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);  // prime previous
        for (int frame = 1; frame < kEncFrames; ++frame) {
            // Copy the base and change only a small region (16-64 cells):
            // HUD ammo digit, subtitle row, and a small target marker.
            work_frame = base_frame;
            const size_t total = work_frame.size();
            const size_t ammo_idx = static_cast<size_t>(6);
            work_frame[ammo_idx].code_point = U'0' + static_cast<char32_t>(frame % 10);
            const int sub_y = kTh - 2;
            for (int x = 4; x < 36; ++x) {
                work_frame[static_cast<size_t>(sub_y) * kTw + x].code_point = U'.';
            }
            for (int i = 0; i < 24; ++i) {
                const size_t idx = static_cast<size_t>(
                    (frame * 37 + i * 101) % (total - kTw * 2)) + kTw * 2;
                work_frame[idx].code_point = U'\u2591';
            }
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(work_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_delta_typical", enc_sampler.Compute());
        std::printf("TERMINAL_DELTA_TYPICAL_MAX_BYTES=%zu\n", max_bytes);
    }

    // ---- D. UNCHANGED ----
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        writeover::MakeRepresentativeFrame(base_frame, kTw, kTh, 7);
        writeover::AnsiFrameEncoder enc;
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);  // prime previous
        for (int frame = 0; frame < kEncFrames; ++frame) {
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(base_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_unchanged", enc_sampler.Compute());
        std::printf("TERMINAL_UNCHANGED_MAX_BYTES=%zu\n", max_bytes);
    }

    // ---- E. WORSTCASE: full-screen adversarial color churn ----
    {
        writeover::FrameTimeSampler enc_sampler;
        size_t max_bytes = 0;
        writeover::AnsiFrameEncoder enc;
        writeover::MakeWorstcaseFrame(base_frame, kTw, kTh, true);
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);  // prime previous
        for (int frame = 0; frame < kEncFrames; ++frame) {
            writeover::MakeWorstcaseFrame(work_frame, kTw, kTh,
                                          (frame % 2) == 0);
            scratch.clear();
            const auto t0 = Clock::now();
            const auto res = enc.Encode(work_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            enc_sampler.AddSample(
                std::chrono::duration<double, std::milli>(t1 - t0).count());
            max_bytes = std::max(max_bytes, scratch.size());
            (void)res;
        }
        writeover::PrintCsv("terminal_encode_worstcase", enc_sampler.Compute());
        std::printf("TERMINAL_WORSTCASE_MAX_BYTES=%zu\n", max_bytes);
    }

    // ---- Budgets (real gates; each contributes to the exit code) ----
    // Representative full: <= 150 KB/frame, worst-1% encode <= 2.0 ms.
    constexpr size_t kFullBytesBudget = 150 * 1024;
    constexpr double kFullTimeBudget = 2.0;
    // Typical delta: <= 60 KB/frame, worst-1% encode <= 1.0 ms.
    constexpr size_t kDeltaBytesBudget = 60 * 1024;
    constexpr double kDeltaTimeBudget = 1.0;
    // Unchanged: bytes == 0, worst-1% encode <= 0.25 ms.
    constexpr double kUnchangedTimeBudget = 0.25;
    // Worstcase safety: <= 1 MB/frame.
    constexpr size_t kWorstcaseBytesLimit = 1024 * 1024;

    // Rerun the encoders to capture budgeted stats in one pass.
    // (Each scenario above measured bytes; recompute time+bytes per gate.)
    bool full_pass = false, delta_pass = false, unchanged_pass = false,
         worstcase_pass = false;
    {
        writeover::FrameTimeSampler s;
        size_t bytes = 0;
        for (int f = 0; f < kEncFrames; ++f) {
            writeover::MakeRepresentativeFrame(base_frame, kTw, kTh,
                                               static_cast<uint8_t>(f));
            writeover::AnsiFrameEncoder enc;
            scratch.clear();
            const auto t0 = Clock::now();
            enc.Encode(base_frame.data(), kTw, kTh, scratch,
                       writeover::EncodeMode::ForceFull);
            const auto t1 = Clock::now();
            s.AddSample(std::chrono::duration<double, std::milli>(t1 - t0).count());
            bytes = std::max(bytes, scratch.size());
        }
        const auto st = s.Compute();
        full_pass = bytes <= kFullBytesBudget &&
                    st.worst_1pct_avg_ms <= kFullTimeBudget;
        std::printf("TERMINAL_FULL_BUDGET=%s\n",
                    full_pass ? "PASS" : "FAIL");
        std::printf("TERMINAL_FULL_TIME_MS=%.3f TERMINAL_FULL_BYTES=%zu\n",
                    st.worst_1pct_avg_ms, bytes);
    }
    {
        writeover::FrameTimeSampler s;
        size_t bytes = 0;
        writeover::MakeRepresentativeFrame(base_frame, kTw, kTh, 0);
        writeover::AnsiFrameEncoder enc;
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);
        for (int f = 1; f < kEncFrames; ++f) {
            work_frame = base_frame;
            for (int i = 0; i < 24; ++i) {
                work_frame[static_cast<size_t>((f * 37 + i * 101) % work_frame.size() +
                                               kTw * 2) % work_frame.size()].code_point = U'\u2591';
            }
            scratch.clear();
            const auto t0 = Clock::now();
            enc.Encode(work_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            s.AddSample(std::chrono::duration<double, std::milli>(t1 - t0).count());
            bytes = std::max(bytes, scratch.size());
        }
        const auto st = s.Compute();
        delta_pass = bytes <= kDeltaBytesBudget &&
                     st.worst_1pct_avg_ms <= kDeltaTimeBudget;
        std::printf("TERMINAL_DELTA_BUDGET=%s\n",
                    delta_pass ? "PASS" : "FAIL");
        std::printf("TERMINAL_DELTA_TIME_MS=%.3f TERMINAL_DELTA_BYTES=%zu\n",
                    st.worst_1pct_avg_ms, bytes);
    }
    {
        writeover::FrameTimeSampler s;
        size_t bytes = 0;
        writeover::MakeRepresentativeFrame(base_frame, kTw, kTh, 7);
        writeover::AnsiFrameEncoder enc;
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);
        for (int f = 0; f < kEncFrames; ++f) {
            scratch.clear();
            const auto t0 = Clock::now();
            enc.Encode(base_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            s.AddSample(std::chrono::duration<double, std::milli>(t1 - t0).count());
            bytes = std::max(bytes, scratch.size());
        }
        const auto st = s.Compute();
        unchanged_pass = bytes == 0 &&
                         st.worst_1pct_avg_ms <= kUnchangedTimeBudget;
        std::printf("TERMINAL_UNCHANGED_BUDGET=%s\n",
                    unchanged_pass ? "PASS" : "FAIL");
        std::printf("TERMINAL_UNCHANGED_TIME_MS=%.3f TERMINAL_UNCHANGED_BYTES=%zu\n",
                    st.worst_1pct_avg_ms, bytes);
    }
    {
        writeover::FrameTimeSampler s;
        size_t bytes = 0;
        writeover::AnsiFrameEncoder enc;
        writeover::MakeWorstcaseFrame(base_frame, kTw, kTh, true);
        scratch.clear();
        enc.Encode(base_frame.data(), kTw, kTh, scratch);
        for (int f = 0; f < kEncFrames; ++f) {
            writeover::MakeWorstcaseFrame(work_frame, kTw, kTh,
                                          (f % 2) == 0);
            scratch.clear();
            const auto t0 = Clock::now();
            enc.Encode(work_frame.data(), kTw, kTh, scratch);
            const auto t1 = Clock::now();
            s.AddSample(std::chrono::duration<double, std::milli>(t1 - t0).count());
            bytes = std::max(bytes, scratch.size());
        }
        const auto st = s.Compute();
        worstcase_pass = bytes <= kWorstcaseBytesLimit;
        std::printf("TERMINAL_WORSTCASE_SAFETY=%s\n",
                    worstcase_pass ? "PASS" : "FAIL");
        std::printf("TERMINAL_WORSTCASE_TIME_MS=%.3f TERMINAL_WORSTCASE_BYTES=%zu\n",
                    st.worst_1pct_avg_ms, bytes);
    }

    // ---- F. Systemic kernel lookup and current-foundation update workloads ----
    const double lookup_ms = writeover::SystemicKernelLookupBenchmark();
    const bool lookup_pass = lookup_ms <= 2.0;
    std::printf("SYSTEMIC_LOOKUP_BUDGET=%s\n", lookup_pass ? "PASS" : "FAIL");
    std::printf("SYSTEMIC_LOOKUP_TIME_MS=%.3f (worst1_avg)\n", lookup_ms);

    const double update_ms = writeover::SystemicUpdateBenchmark();
    const bool update_pass = update_ms <= 2.0;
    std::printf("SYSTEMIC_UPDATE_BUDGET=%s\n", update_pass ? "PASS" : "FAIL");
    std::printf("SYSTEMIC_UPDATE_TIME_MS=%.3f (worst1_avg)\n", update_ms);

    const bool overall_pass = ray_pass && full_pass && delta_pass &&
                              unchanged_pass && worstcase_pass && lookup_pass && update_pass;
    std::printf("RAYCAST_BUDGET=%s\n", ray_pass ? "PASS" : "FAIL");
    std::printf("OVERALL_BUDGET=%s\n", overall_pass ? "PASS" : "FAIL");
    return overall_pass ? 0 : 1;
}
