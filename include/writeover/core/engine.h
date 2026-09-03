#pragma once
// Engine: composition-scheduler. Core depends ONLY on common. The app
// (composition root) builds the real module graph and registers modules here
// (M-002 closure: writeover_core never links world/player/ai/narrative/render).

#include "writeover/common/clock.h"
#include "writeover/common/debug.h"
#include "writeover/common/logging.h"
#include "writeover/common/rng.h"
#include "writeover/common/world_event.h"
#include "writeover/core/settings.h"

#include <cstdint>
#include <string>
#include <vector>

namespace writeover {

struct EngineContext {
    SimClock* clock = nullptr;
    EventBus* events = nullptr;
    DeterministicRNG* sim_rng = nullptr;
    Settings* settings = nullptr;
    DebugMetrics* metrics = nullptr;
    Logger* logger = nullptr;
    std::string data_dir;  // content directory (compiled .woc files)
};

class IEngineModule {
public:
    virtual ~IEngineModule() = default;
    virtual void Init(const EngineContext& ctx) = 0;
    virtual void Shutdown() = 0;
    // Fixed 120Hz sim tick. Never takes a float dt (variable timestep is a
    // forbidden pattern).
    virtual void SimTick(const SimClock& clock) = 0;
    virtual const char* Name() const = 0;
};

class IRenderModule {
public:
    virtual ~IRenderModule() = default;
    // Called once per presented frame; alpha in [0,1) is render interpolation
    // between the previous and current sim snapshot (visual only).
    virtual void RenderFrame(uint64_t frame_index, float alpha) = 0;
    virtual const char* Name() const = 0;
};

class Engine {
public:
    Engine() = default;

    void SetContext(const EngineContext& ctx) { context_ = ctx; }
    const EngineContext& Context() const { return context_; }

    void RegisterModule(IEngineModule* module);
    void SetRenderModule(IRenderModule* render_module);

    // Runs the fixed-step accumulator loop. max_frames > 0 caps the number of
    // SIM ticks (smoke mode); 0 runs until RequestStop.
    int Run(uint64_t max_frames = 0);

    void RequestStop() { running_ = false; }

private:
    EngineContext context_;
    std::vector<IEngineModule*> modules_;
    IRenderModule* render_ = nullptr;
    bool running_ = false;
};

} // namespace writeover