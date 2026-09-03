# M4_WORLD_CODEX — World / Map / Puzzle / Data

Human Owner: D。角色：grid/room/infrastructure/FactStore、内容管线、mapc。

## Owned / Forbidden

```
Owned:   src/world/, include/writeover/world/**,
         data/**（作者 JSON 与编译产物）, tools/mapc/, tools/contentc/
Forbidden: 其它模块 src/；改公共类型需 ADR
```

## 关键契约（读这些头）

- `world/grid.h`（Grid + IWorldQuery，网格是唯一几何真相）
- `world/room.h`（WOC codec，与 contentc.py 对齐）
- `world/infrastructure.h` / `world/puzzle.h`（命令应用目标）
- `world/fact_belief.h`（FactValue typed variant、FactStore 单写者）
- `world/map_validator.h`（mapc 校验）

## 不变量

- 世界 mutation 单写者：命令 → world 校验应用 → WorldEvent。
- 运行时零 JSON；作者 JSON 必须经 contentc 编译，产物与 C++ codec 字节对齐。
- FactStore 迭代确定性（std::map）；Room 网格内容确定性加载。
- 新增房间/谜题数据不得破坏既有 .woc 字节（CI --check）。

## 施工建议

1. 6 Room + 3 micro-space 内容按 Opus H 节顺序产出 + mapc 全绿；
2. puzzle/infrastructure 数据驱动的命令接入；
3. FactStore 运行时播种自 facts.bin（schema 已备）。

## 模块 DoD

room codec round-trip、grid 碰撞、fact/belief round-trip、mapc 对全部 .woc
exit 0、内容 ref 无悬垂。

## 报告素材

DDA 清除、确定性加载排序、CRC。UML：Grid→Room→Infrastructure→FactStore。