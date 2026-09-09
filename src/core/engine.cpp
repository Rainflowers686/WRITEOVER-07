#include "writeover/core/engine.h"

#include <algorithm>
#include <chrono>
#include <thread>

namespace writeover {

void Engine::RegisterModule(IEngineModule* module) { modules_.push_back(module); }
void Engine::SetRenderModule(IRenderModule* render_module) { render_ = render_module; }

int Engine::Run(uint64_t max_frames) {
    running_ = true;

    using WallClock = std::chrono::steady_clock;
    using Duration = std::chrono::duration<double, std::milli>;

    const double fixed_dt_ms = SimClock::kFixedDeltaTime * 1000.0;

    Duration accumulator(0.0);
    auto previous = WallClock::now();
    auto last_presentation = previous;

    uint64_t sim_ticks = 0;
    while (running_) {
        const auto now = WallClock::now();
        accumulator += std::chrono::duration_cast<Duration>(now - previous);
        previous = now;

        if (accumulator.count() < 0.0) {
            accumulator = Duration(0.0);
        }
        if (max_frames > 0 && sim_ticks >= max_frames) {
            break;
        }

        bool stepped = false;
        while (running_ && accumulator.count() >= fixed_dt_ms &&
               (max_frames == 0 || sim_ticks < max_frames)) {
            if (context_.clock != nullptr) {
                for (const auto module : modules_) {
                    module->SimTick(*context_.clock);
                }
                // Event fan-out: dispatch all events posted during this tick.
                // Events posted during dispatch go to next_pending_ and will be
                // dispatched in the next tick (F-06: same-tick mutation is not
                // visible until next tick; this is the chosen semantics).
                if (context_.events != nullptr) {
                    context_.events->Dispatch();
                }
                context_.clock->Tick();
            }
            ++sim_ticks;
            accumulator -= Duration(fixed_dt_ms);
            stepped = true;
        }

        // Finite smoke/replay runs still need the fixed-tick wall-time pacing.
        // Without this sleep the loop can render millions of duplicate frames
        // between two sim ticks, consuming effect durations before gameplay
        // can observe them and needlessly inflating CPU usage.
        if (!stepped) {
            const double ahead_ms = fixed_dt_ms - accumulator.count();
            if (ahead_ms > 0.1) {
                const auto wake = WallClock::now() + Duration(ahead_ms);
                std::this_thread::sleep_until(wake);
            }
        }

        // The fixed simulation is the presentation cadence for this small
        // terminal client.  Do not render the same simulation snapshot once
        // before and once after every tick: duplicate presentation calls
        // make render-owned effect timers advance faster than gameplay and
        // produce stale-frame evidence.  A stepped iteration always emits
        // the newest snapshot; the final stepped iteration also covers
        // finite smoke/replay runs.
        if (stepped && render_ != nullptr) {
            bool present = true;
            if (context_.settings != nullptr &&
                context_.settings->frame_rate_cap != 0) {
                const uint8_t capped_hz = static_cast<uint8_t>(std::min<int>(
                    context_.settings->frame_rate_cap, 120));
                const double interval_ms =
                    1000.0 / static_cast<double>(std::max<int>(capped_hz, 1));
                const double since_last_ms =
                    std::chrono::duration_cast<Duration>(
                        WallClock::now() - last_presentation).count();
                present = since_last_ms + 0.1 >= interval_ms;
            }
            if (!present) {
                continue;
            }
            const float alpha = static_cast<float>(
                accumulator.count() / fixed_dt_ms);
            render_->RenderFrame(sim_ticks, alpha < 1.0f ? alpha : 0.0f);
            last_presentation = WallClock::now();
        }
    }

    // Every registered module owns a portion of the live runtime.  Shutdown
    // after both a finite run and a RequestStop path so normal player exit
    // restores platform state just as a bounded smoke run does.
    for (const auto module : modules_) {
        if (module != nullptr) {
            module->Shutdown();
        }
    }
    running_ = false;

    return 0;
}

} // namespace writeover
