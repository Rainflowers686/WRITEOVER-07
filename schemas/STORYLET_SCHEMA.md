# STORYLET_SCHEMA — Authoring JSON (schemaVersion 1)

编译目标：`data/storylets/storylets.bin`（typed 格式，与 `StoryletEngine::Load`
同构）。**没有 magic a/b/f 字段**（M-012 closure）。运行时条件/动作类型见
`narrative/storylet.h`。

```json
{
  "schemaVersion": 1,
  "storylets": [
    {
      "id": "storylet_r1_01",
      "textId": "text_r1_01",
      "priority": 100,
      "once": true,
      "conditions": [
        {"type": "fact",      "fact": "fact_player_has_gun", "equals": true},
        {"type": "frame",     "minFrame": 60, "maxFrame": 7200},
        {"type": "difficulty","minLevel": 1}
      ],
      "actions": [
        {"type": "narrator", "textId": "text_r1_narr_01", "persona": 1},
        {"type": "endgame",  "ending": 0}
      ]
    }
  ]
}
```

## Condition types（→ StoryletCondition）

| type | 字段 | 运行时类型 |
|------|------|-----------|
| fact | fact(编译期解析为 FactId), equals | FactEqualsCondition |
| room | roomId | RoomVisitedCondition |
| npcstate | npcId, state | NpcStateCondition |
| frame | minFrame, maxFrame | FrameRangeCondition |
| difficulty | minLevel | DifficultyCondition |
| flag | flag | FlagCondition |

## Action types（→ StoryletAction）

| type | 字段 | 运行时类型 |
|------|------|-----------|
| narrator | textId, persona | NarratorLineAction |
| dialog | textId | DialogAction |
| command | —（运行时构造） | WorldCommandAction |
| endgame | ending | EndGameCommand |

## 编译期校验

- schemaVersion=1；id 唯一；condition/action type 未知 → 报错；
- fact/room/npc 引用必须解析（fail-fast）；persona/ending 范围检查。

## 运行时选择确定性

priority desc → storylet id asc；once fired 集有序。条件求值顺序 = 数组顺序
（全真才触发）。数据实例见 `repo_seed/data/storylets/storylets.json`。