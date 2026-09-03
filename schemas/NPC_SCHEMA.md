# NPC_SCHEMA — Authoring JSON (schemaVersion 1)

Authoring 文件示例（编译期校验；NPC 运行时数据模型见 `ai/npc.h`）：

```json
{
  "schemaVersion": 1,
  "npcs": [
    {
      "id": "npc_guard_01",
      "name": "守卫",
      "class": "light",
      "faction": 0,
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
| id | 同 kind 唯一（编译期去重） |
| class | full/medium/light/guard（预算：2/2/3/6–10） |
| faction | 0 guard / 1 staff / 2 civilian / 3 player |
| spawn.room | 必须能解析到房间 id（fail-fast） |
| isCritical | main-quest 保护：不可离屏死亡（测试不变量） |

## 映射到运行时类型

- id → NpcId（编译期 1..N 稳定分配）
- name → 文本表 StringId（**不**作为运行时裸字符串内嵌）
- perception 参数只影响 PerceptionSystem 配置常量选择

## 编译产物

- contentc 生成校验清单；NPC 运行时注册表由 M5 施工项按 `npc.h` 类型填充
  （foundation 阶段 NPC 集成模块为空实现 + 单元测试覆盖系统逻辑，非 fake）。