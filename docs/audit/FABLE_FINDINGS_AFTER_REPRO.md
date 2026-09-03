# FABLE FINDINGS AFTER REPRODUCTION

## 独立复现后对 01_INDEPENDENT_AUDIT_FINDINGS.md 的判断

| Finding | 独立复现 | 严重性 | 状态 |
|---------|---------|--------|------|
| F-01 Test Oracle 不 fail-fast | CONFIRMED | CRITICAL | **已修复** |
| F-02 InputMapper 重复绑定冲突 | CONFIRMED | HIGH | **已修复**（replace 冲突策略） |
| F-03 Windows Keyboard 丢 key-up | CONFIRMED | HIGH | **已修复**（ReadConsoleInput + key-up 处理） |
| F-04 CursorDelta 不可通过接口获取 | CONFIRMED | MEDIUM | 已记录，Luna 改善 IInputBackend |
| F-05 Engine 无 EventBus Dispatch | CONFIRMED | HIGH | **已修复** |
| F-06 EventBus next_pending 语义 | CONFIRMED | LOW | **已记录为语义**（end-of-tick fan-out） |
| F-07 Narrative 因果账本伪造 | CONFIRMED | HIGH | **已修复**（移除假因果链） |
| F-08 Storylet 加载破坏 | CONFIRMED | HIGH | **已修复**（有条件的 runtime 状态读） |
| F-09 WorldCommandAction marker-only | CONFIRMED | LOW | **已修复**（typed WorldCommand 序列化 + round-trip 测试） |
| F-10 contentc --check 未实现 | CONFIRMED | MEDIUM | **已修复**（temp recompile byte-compare） |
| F-11 RoomId 硬编码 1 | CONFIRMED | MEDIUM | **已修复**（编译器赋 1..N） |
| F-12 Room refs 为空 | CONFIRMED | LOW | 数据层阶段由 M4 填充 |
| F-13 Save 7-section 被拒绝 | CONFIRMED | HIGH | **已修复**（`>=` → `>`） |
| F-14 无 Save→Load→Resume 测试 | CONFIRMED | HIGH | **已修复**（2000 == 1000+save+load+1000） |
| F-15 Smoke save 只存 RNG+Events | CONFIRMED | MEDIUM | **已修复**（扩展为 World + Player + Narrative） |
| F-16 控制器 idle 后 grounded=false | CONFIRMED | HIGH | **已修复**（GroundProbe + velocity 守卫） |
| F-17 Step-Up 未实现 | CONFIRMED | HIGH | **已修复**（IsStepRiseOnly + floor snap） |
| F-18 Lean 无几何约束 | CONFIRMED | MEDIUM | **已修复**（LeanClamp 二分搜索） |
| F-19 Vault/Mantle/Ladder stub | CONFIRMED | LOW | 已记录，Luna 实现 |
| F-20 Height-Span 只处理单向 | CONFIRMED | HIGH | **已修复**（双向：floor drop + ceiling rise） |
| F-21 DDA corner tie 只走 X | CONFIRMED | MEDIUM | **已修复**（两轴同时前进） |
| F-22 Render smoke 没真正画到屏幕 | CONFIRMED | HIGH | **已修复**（reference_renderer 可见 3D 输出） |
| F-23 Player view 只设置一次 | CONFIRMED | HIGH | **已修复**（RenderModule 每帧更新） |
| F-24 ANSI 逐 cell 完整 SGR | CONFIRMED | MEDIUM | **已修复**（color-run 压缩 + delta 编码） |
| F-25 Win32 fallback 不完整 | CONFIRMED | LOW | **已修复**（full RGB→16 色量化） |
| F-26 最后 fallback 未初始化 | CONFIRMED | MEDIUM | **已修复**（fallback 调用 Init()） |
| F-27 Benchmark 命名混淆 | CONFIRMED | LOW | **已修复**（p99_ms → worst_1pct_avg_ms） |
| F-28 Content schema 漂移 | CONFIRMED | MEDIUM | **已修复**（--check 保证字节级一致） |
| F-29 MapValidator 不可能触发检查 | CONFIRMED | LOW | **已修复**（改为 zero-light 警告） |
| F-30 Windows 可移植性边界 | CONFIRMED | LOW | 已记录，不影响 R0 |

## 汇总

| 类别 | 总数 | 已修复 | 未修复（已记录） |
|------|------|--------|------------------|
| CRITICAL | 1 | 1 | 0 |
| HIGH | 12 | 12 | 0 |
| MEDIUM | 11 | 9 | 2 |
| LOW | 6 | 4 | 2 |
| **总计** | **30** | **26** | **4** |