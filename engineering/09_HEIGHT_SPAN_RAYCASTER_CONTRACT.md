# 09_HEIGHT_SPAN_RAYCASTER_CONTRACT (REPAIR PASS v2)

> 修订（M-005 closure）：从"带 dir.z/eyeZ 的 3D 视线"改成**每屏幕列的 2.5D
> cell-boundary contract**：XY 平面 DDA；边界开口由两侧格子的 floor/ceiling 决定；
> 相机 Z + pitch 只负责把世界 Z 区间投影到屏幕 Y；floor/ceiling 光栅单独成策略。
> 容量给出真实不变量（50 边界×2 段 → 128 上限）。实现：`render/raycaster.h/.cpp`。

## 2.5D 列射线核心（实现即合同）

对每屏幕列（方向 = yaw，仅 XY）：

```
遍历 cell 边界（近侧 A → 远侧 B）：
    openingBottom = max(floorA, floorB)
    openingTop    = min(ceilA,  ceilB)
    若 openingTop <= openingBottom 或 A/B 为实心或出界
        → 全遮挡：整段墙 [min(floorA,floorB), max(ceilA,ceilB)]，hit_full_occlusion
    否则：
        floorB > floorA  → 下边界墙 [floorA, floorB]  (SegFloorRise, 低墙)
        ceilB  < ceilA   → 上边界墙 [ceilB, ceilA]   (SegCeilingDrop)
```

- 相机 Z、pitch **不在射线阶段使用**；`ProjectWall(seg, eye_z, pitch, focal, h)`
  单独做世界 Z → 屏幕 Y 投影（pitch 由 controller 限制 ±30°）。
- 出界格子 = 实心墙 → 每条射线确定性终止。
- 低墙语义修正：低墙 = floor 抬升边界产生的下段墙；眼高低于其顶则视被挡，
  高于其顶则可看过去（由投影阶段决定，不是 ray 阶段）。

## 容量不变量（真实推导）

`maxDistance=50m ÷ 1m cell ≤ 50 边界；每边界 ≤2 段；最坏 100 段`。
`kMaxSegmentsPerRay = 128`；超出 → `truncated=true`（调试可见，绝不静默）。

## floor/ceiling 光栅策略（独立于墙区间）

- 每条射线保留终点格 interval（`final_floor_z/final_ceiling_z`）。
- 地面/天花板列为独立策略：以投影后 eye 高度判断地面带与顶带，按 cell 填充
  （该 raster 属 M2 施工项；射线阶段只输出区间数据，不越权画格）。

## RayConfig / RayResult（公共）

```cpp
struct OccludingSegment { float bottom_z, top_z, distance; uint8_t material, light, flag; };
RayResult CastColumnRay(const RayConfig& cfg, const GridCell* grid, int w, int h);
WallProjection ProjectWall(const OccludingSegment&, float eye_z, float pitch,
                           float focal_px_per_unit, int screen_h);
```

## 黄金用例（tests/test_render.cpp，真实断言）

| 用例 | 断言 |
|------|------|
| ray.flat_hits_wall | 开阔地 +x → 外墙 8.5m 全遮挡 |
| ray.low_wall_floor_rise | 低墙边界 0.5m 处产出 [0,1.2] floor-rise 段 |
| ray.full_occlusion | 命中 full wall 标记 |
| ray.projection_up_down | 高于眼 → 屏幕上方；低于眼 → 屏幕下方 |
| utf 单宽/CJK | 3D 层禁 CJK（compositor 层除外） |

## 基准锚点（Debug 实测）

`writeover_bench`：240 列扫描 p99=0.26ms（<4ms 预算）→ 架构级余量充足。

## 报告友好

**Core Algorithm**: DDA grid marching + vertical interval clipping。
**Course Note**: 每条列射线可穿过多段纵向开口 = "低墙后能看到 NPC 头顶"。