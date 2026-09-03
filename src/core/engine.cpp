#include "writeover/core/engine.h"

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
        while (accumulator.count() >= fixed_dt_ms) {
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

        if (!stepped && max_frames == 0) {
            const double ahead_ms = fixed_dt_ms - accumulator.count();
            if (ahead_ms > 0.1) {
                const auto wake = WallClock::now() + Duration(ahead_ms);
                std::this_thread::sleep_until(wake);
            }
        }

        const float alpha = static_cast<float>(
            accumulator.count() / fixed_dt_ms);
        if (render_ != nullptr) {
            render_->RenderFrame(sim_ticks, alpha < 1.0f ? alpha : 0.0f);
        }
    }

    return 0;
}

} // namespace writeover