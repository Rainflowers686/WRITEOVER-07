# NPC_SCHEMA — Authoring JSON (schemaVersion 1)

Authoring 文件示例（编译期校验；NPC 运行时数据模型见 `ai/npc.h`）：

```json
{
  "schemaVersion": 1,
  "npcs": [
    {
      "id": "npc_guard_01",
      "name": "守卫",
      "cognition": "SemiHuman",
      "faction": "Security",
      "role": "Guard",
      "spawn": {"room": "room_01_calibration", "x": 3.5, "y": 2.5, "yaw": 1.57},
      "health": 50,
      "isCritical": false,
      "perception": {"sightRange": 10.0, "sightFovRad": 2.09, "hearingRange": 8.0}
    }
  ]
}
```

## 字段规则

| 字段 | 规则 |
|------|------|
| id | 全部 NPC authoring 文件内唯一；FNV-1a64 碰撞或重复均 fail-fast |
| cognition | `Full` 或 `SemiHuman`；这是认知深度，不是职业 |
| faction | `GeneralStaff` / `Security` / `Medical` / `Research` / `Maintenance` / `Executive` / `Detained` / `Civilian` |
| role | `Guard` / `Cleaner` / `Doctor` / `Researcher` / `Technician` / `Administrator` / `Executive` / `Detained` / `Civilian` / `Other` |
| spawn.room | 必须能解析到已知房间 id（fail-fast）；位置必须为有限数 |
| health | 1..1000 |
| isCritical | bool；main-quest 保护：不可离屏死亡 |
| perception | 有限的 sightRange 0..64、sightFovRad 0..6.2832、hearingRange 0..64 |

## 映射到运行时类型

- id → NpcId（编译期稳定 FNV-1a64）
- name → 文本表 StringId（**不**作为运行时裸字符串内嵌）
- cognition / faction / role → `NPCInstance` 的三个正交字段
- perception 参数是 authored profile；contentc 生成 `data/npcs/npcs.bin`，
  production runtime 从该二进制加载 spawn/health/critical/perception，
  再与 systemic actor identity 合并。

## 编译产物

- `contentc --check` 会确定性重编译 `npcs/npcs.bin`；运行时从该 profile binary
  加载 NPC runtime data，并从 systemic seed 加载 identity，再接入小型
  autonomous adapter。完整 M5 routine/planner 仍不在此 authoring schema 内。

Legacy migration: old `class: Guard` is accepted only for existing content and
maps to `cognition: SemiHuman`, `role: Guard`. New files must use the three
explicit dimensions above; `Medium` and `Light` are not valid cognition tiers.
