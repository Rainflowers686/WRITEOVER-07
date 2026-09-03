# 21_THREADING_POLICY

## Default Thread Model

- **World Simulation**: single thread, always
- **Render**: single thread, R0
- **Present**: single thread, R0
- **Audio**: separate backend thread permitted (platform), playback only
- **File I/O**: synchronous, infrequent

## Policy Statement

> Profile first, parallelize second.

## Safe Acceleration Gates

1. If profiler proves Present blocks >1ms consistently → add a present thread with latest-frame mailbox.
2. If profiler proves raycaster >4ms consistently → consider per-column render partitioning.
3. If profiler proves audio callbacks cause glitches → move only audio mixing to backend thread.
4. World logic NEVER multi-threads.

## Latest-Frame Mailbox (optional future)

```cpp
template<typename T>
class LatestMailbox {
public:
    void Publish(T value);    // overwrites old frame
    bool Consume(T& out);     // gets latest; never blocks
};
```

Used only if `WO_FEATURE_PRESENT_THREAD` is enabled after profiling proof.

## Why Not More Threads

- Determinism gets harder
- Debugging becomes harder
- Freshmen and Codex can reason about single-threaded data flow much more easily
- Current scope does not need it

## Synchronization Rules

- No shared mutable world state across threads
- If a thread exists, it may only exchange immutable snapshots or mailbox values
- No lock-free heroics
- If a mutex is needed, it must be for logging/audio backend only (not game state)

## Timing / Sleep Rules

- `sleep_for` may be used only for pacing/waiting the render loop, not for gameplay correctness.
- Prefer `sleep_until(nextFrameTime)` or equivalent waitable timer for steady pacing.
- Simulation correctness is always driven by the fixed-step accumulator, never by sleeping.

## 报告友好

**STL**: std::thread, std::mutex only in platform layer if needed.
**Design Pattern**: Producer/Consumer mailbox (future), but not yet enabled.
**Course Note**: 先用 profiler 证明瓶颈，再加线程，避免过早并发。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
