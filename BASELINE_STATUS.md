# BASELINE_STATUS — 2026-09-03

本文件记录交给 Fable 之前的工作树状态，不代表修复后状态。

## 已知静态审计命中（15）

运行：

```powershell
python tools/audit/static_audit.py .
```

当前基线预期会命中以下类型：

1. Test Oracle `WO_CHECK` 不 fail-fast
2. InputMapper first-match global mapping
3. `FlushConsoleInputBuffer`
4. key-up path 缺失/需审
5. Engine 无 EventBus Dispatch
6. `contentc --check` 未使用
7. RoomId 硬编码 1
8. Storylet WorldCommand marker-only
9. Save 7-section 边界错误
10. ANSI per-cell SGR
11. Ray floor-drop 未明确处理
12. DDA corner tie 单轴
13. Controller 无 GroundProbe
14. Lean 无 geometry clamp
15. Storylet definitions/runtime state load 合同冲突

这些都必须由 Fable **独立复现后**决定如何修，不能仅因本文件写了就机械修改。

## 原始 Foundation 证据

见：
- `evidence/FOUNDATION_VALIDATION.md`
- `docs/foundation/FOUNDATION_VERDICT.md`

## 独立复现证据

见：
- `docs/audit/02_INDEPENDENT_REPRODUCTION_EVIDENCE.md`
