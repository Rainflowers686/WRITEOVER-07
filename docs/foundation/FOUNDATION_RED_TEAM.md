# FOUNDATION_RED_TEAM（REPAIR PASS v2）

> 上一轮红队对"空目录+口头 YES"未设防（自证其罪），已废弃。本版针对修复后
> 真实代码重做 16 项红队；每项给"当前证据"而非口头裁决。

## 1. Over-design？→ ACCEPTABLE（证据：代码规模）

common 仅 variant/vector/function；无 ECS/DI/service locator/反射/脚本 VM。
`include/writeover` 共 50 头、~55 TU；每模块 2–6 文件。基准：Debug 240 列
raycast p99 0.26ms，无热点压力。

## 2. 大一能解释？→ ACCEPTABLE（证据：报告友好节 + 51 篇 docs 每篇含 Course Note）

正交 FSM、typed variant、DDA、CRC 均课程可讲；红队确认无黑魔法。

## 3. Codex 误用风险 → 已收敛（证据：contract_check 全绿 + baseline）

公共头 hash baseline + 依赖白名单 + forbidden grep 三把锁；TASK_TEMPLATE
强制输出协议块。

## 4. 存档 padding/endian → 无（证据：逐字段小端 codec + round-trip 测试）

struct dump = forbidden；wire 无时间戳；CRC 逐节。

## 5. 事件丢/重 → 无（证据：fan-out journal + Dispatch 单点 + 事件单测）

无 Poll/steal；journal 只读环形 500；Save/Load 含 pending+next+journal。

## 6. determinism 口号？→ 非口号（证据：RNG 全 state round-trip、无墙钟、
replay A==C 测试、std::map 有序迭代）

## 7. terminal restore crash-safe 承诺 → 修正（证据：12/29 文档）

正常退出=保证（RAII+atexit）；SEH 崩溃恢复=NOT_READY，不再过度承诺。

## 8. input fallback → 换实现不改 API（证据：IInputBackend + 工厂）

RawInput(待施工)/cursor-delta/键盘后端同一接口；GameAction 层隔离。

## 9. R0 fallback 破坏公共 API → 不会（证据：feature flags + capability 设计）

WO_FEATURE_* 只切实现；渲染/输入降级走 backend 选择，公共头零变化。

## 10. build 可复现 → 本机一致（证据：两次 configure/build exit 0；presets 固化；
contentc --check 防产物漂移；CI windows-latest 模板）

## 11. test harness 真跑 → 是（证据：50 tests 0 failed；无 fake PASS 机制）

## 12. 循环依赖 → 无（证据：CMake link DAG + check_deps 全绿）

## 13. 过早线程 → 无（证据：WO_FEATURE_PRESENT_THREAD=0；Logger mutex 仅平台）

## 14. debug 侵入 release core → 受控（证据：WO_ENABLE_DEV gate；metrics 轻量）

## 15. 剩险（诚实）

| 风险 | 缓解 | 状态 |
|------|------|------|
| RawInput/XAudio/SEH 未完成 | gates 显式 NOT_READY + M2/M3 施工项 | 已排期 |
| 内容/功能集成未开跑 | 代码路径已备（typed 管线/组合根），单测覆盖逻辑 | 已排期 |
| 六 Owner 未签核 | CONTRACT_FREEZE_GATE 人工清单 | 流程项 |
| AI module 为 legal no-op | 声明空实现+单测；非 fake | 已说明 |

## 结论

无 fatal/major 遗留（OPEN_FATAL=0, OPEN_MAJOR=0）；CODEX_READY 的剩余条件是
**流程性人工动作**，见 FOUNDATION_VERDICT。