# HK-2 Height-Span Renderer Reference

## 目的

将 Height-Span Raycaster 的双向高度边界、角点策略、投影与可见合成加固为参考实现。

## 关键契约

### 双向边界 (F-20 闭合)

每个 cell-boundary 产生 4 类 vertical span：

| 条件 | Segment | Flag |
|---|---|---|
| `far.floor > near.floor` | 低墙（floor rise） | `SegFloorRise = 1` |
| `far.ceiling < near.ceiling` | 高墙（ceiling drop） | `SegCeilingDrop = 2` |
| 无开口（`openingTop <= openingBottom`） | 满墙 | `SegFullWall = 3` |
| `far.floor < near.floor` | 壕沟墙（floor drop） | `SegFloorDrop = 4` |
| `far.ceiling > near.ceiling` | 高顶（ceiling rise） | `SegCeilingRise = 5` |

### 角点 tie 策略 (F-21 闭合)

- DDA 每步比较 `t_max_x` 与 `t_max_y`
- 当 `|t_max_x - t_max_y| < 1e-6`（精确角点）：**两轴同时前进**（supercover），
  直接检查对角 cell，不访问仅角接触的中间 cell
- 避免 spurious intermediate cell

### 容量

- maxDistance=50m / 1m cell → 每 ray 最多 50 边界
- 每边界最多 2 segment → worst case 100
- `kMaxSegmentsPerRay = 128` 有确定性余量；超限 `truncated=true`（绝不静默）

### Projection

- `ProjectWall` 用 eye_z + pitch + focal 将 world-Z 段投影到 screen row
- pitch 由 controller 限制 ±30°

## Golden Scenes (G01-G18)

| ID | 场景 | 断言 |
|---|---|---|
| G01 | 平坦走廊 | 1 full wall，距离正确 |
| G02 | floor rise | 1 SegFloorRise [0,1.0] |
| G03 | floor drop / 壕沟 | 1 SegFloorDrop [-0.8,0] |
| G04 | ceiling drop | 1 SegCeilingDrop [2.5,4] |
| G05 | ceiling rise | 1 SegCeilingRise [4,5] |
| G06 | 地板+天花板同变 | 1 rise + 1 drop |
| G07 | 完全闭合 | 1 SegFullWall |
| G08 | 交替高度 | 3 rise + 3 drop |
| G09 | 对角 corner | 45° ray 穿过，不撞角邻 cell |
| G10 | 精确角点 tie | 两轴前进，无中间 spurious cell |
| G11 | 出界 | 边界 solid，距离正确 |
| G12 | 近零方向 | 无 NaN/Inf |
| G13/G14 | ±30° pitch | 投影方向正确 |
| G15/G16 | 蹲/站低墙 | 蹲：墙顶在 horizon 上；站：在 horizon 下 |
| G17/G18 | 目标在眼上/下 | 投影到 horizon 上/下 |

## 参考渲染器

`reference_renderer.cpp` — 可见字符 3D 输出参考：

- 每列 raycast → segment → ProjectWall → 全块 glyph 栅格化（深度着色）
- floor/ceiling 背景填充（深度衰减）
- marker sprite 投影 + 深度测试遮挡
- 确定性：同输入 → 同输出（`reference_renderer_deterministic` 测试）

## 门控 (G2)

PASS：
- ≥18 golden scenes
- floor rise/drop ✓
- ceiling rise/drop ✓
- corner tie ✓
- pitch ✓
- crouch/stand low-wall ✓
- visible reference raster ✓
- no NaN/OOB ✓
- benchmark at 240 columns ✓

## 给 Luna
- M2 / M4 的 raycast 数学、边界分类、角点策略不可改；只可优化实现。
