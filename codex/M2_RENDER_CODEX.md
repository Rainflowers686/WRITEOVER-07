# M2_RENDER_CODEX — Render / Terminal / Performance

Human Owner: B。角色：raycaster、terminal backends、HUD、bench。

## Owned / Forbidden

```
Owned:   src/render/, include/writeover/render/**,
         src/platform/windows/win_terminal.cpp
Forbidden: world mutation 路径；include/writeover/{world,player} 只读使用
```

## 关键契约（读这些头）

- `render/raycaster.h`（2.5D 列边界开口规则；容量不变量 128）
- `render/terminal_backend.h`（CharCell/启发式 probe/工厂）
- `render/benchmark.h`（1% low = slowest-1% 平均）
- `render/hud.h`、`common/types.h`（GridCell/材质）

## 不变量

- ray 阶段不用 camera Z/pitch；投影单独（`ProjectWall`）。
- 3D 层只单宽字形；CJK 只进 compositor 层。
- 无逐字符 cout；Submit 整帧一次刷新；不滚屏。
- Render 只读 IWorldQuery/快照，永不 mutate world。
- benchmark 输出 = 真采样 CSV，禁止 hardcode。

## 施工建议（R0 gate 相关）

1. ray 黄金用例扩展（低墙+平台混合）；2. Win32 WriteConsoleOutputW 精化；
3. floor/ceiling 光栅策略落地；4. bench 基线入库。

## 模块 DoD

黄金 ray 测试全过、utf 单宽/CJK 测试、BUDGET=PASS（参考：Debug p99 0.26ms/240列）、
present/restore 路径 smoke 验证。

## 报告素材

DDA 算法、Strategy（ANSI/Win32）、1% low 统计。UML：Raycaster→Backend→HUD。