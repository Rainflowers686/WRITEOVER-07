# ROOM_SCHEMA — Authoring JSON (schemaVersion 1)

编译目标：`data/rooms/<id>.woc`（contentc.py；运行时只读 .woc，见 14 文档）。

```json
{
  "schemaVersion": 1,
  "id": "room_01_calibration",
  "displayName": "校准靶场",
  "gridWidth": 16,
  "gridHeight": 12,
  "cellSize": 1.0,
  "spawnPoint": {"x": 1.5, "y": 9.5, "z": 0.0, "yaw": 0.0},
  "cells": [
    {"col": 0, "row": 0, "floor": 0.0, "ceiling": 4.0,
     "material": "metal", "light": 255, "flags": ["solid"]}
  ]
}
```

## 字段规则

| 字段 | 类型 | 规则 |
|------|------|------|
| schemaVersion | int | 必须 1 |
| id | string | 同 kind 内唯一；编译期去重检查 |
| displayName | string | UTF-8 显示名 |
| gridWidth/gridHeight | int | >0 且 ≤256；乘积 ≤64×64 预算警告 |
| cellSize | number | 冻结 1.0 |
| spawnPoint | {x,y,z,yaw} | 必须在界内；mapc 校验站立净空 |
| cells[].floor/ceiling | number | ceiling-floor ≥0.05 |
| cells[].material | enum | wall/metal/glass/dirt/concrete/wood/grate/hazard |
| cells[].light | int | 0..255 |
| cells[].flags | string[] | solid/door/breakable/special |

## 语义约定

- 缺失格子 = 开阔格（floor 0, ceiling 4, 无 flag）。
- floor 抬升 → 低墙（下段墙）；ceiling 降低 → 上段墙（见 09 文档边界开口规则）。
- 未列出字段编译器忽略（向前兼容），未知 flag 报错。

## 编译产物二进制（WOC1 v1，供 mapc/运行时）

```
<II magic,version> <Q roomId> <utf8 displayName> <ii w h>
<ffff spawn x,y,z,yaw> <I cellCount=w*h>
per cell: <ffBBB floor,ceiling,material,light,flags>
```

## 校验（mapc exit 0/1）

id 唯一、尺寸合法、spawn 在界内 + 站立可容、格子 floor<ceiling、存在实心墙
（无实心 → warning）、npc/storylet 引用非空。