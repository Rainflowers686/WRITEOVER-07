# SAVE_SCHEMA — `.wo07` wire format（schemaVersion 1）

> 无墙钟时间戳（M-007 closure：byte-for-byte round-trip 成立）。与
> `core/save.h` 实现一一对应；全部小端、逐字段、无 struct dump。

## 布局

```
[0..3]    magic      u32 = 0x574F3037 ("WO07")
[4..7]    version    u32 = 1
[8..11]   section_count u32
[12..15]  reserved   u32 = 0
[16..23]  reserved   u64 = 0
             ── header 固定 24B（无时间戳）──
每个 section:
  [24+]    section header 12B: id u32 | data_size u32 | crc32(载荷)
           载荷: 确定性小节序列化字节（CRC32 IEEE 802.3 reflected）
结尾:
  4B footer CRC32(header+所有 section header+载荷)
```

## SaveSectionId

Player=0 World=1 Rng=2 Events=3 Ai=4 Narrative=5 SettingsGameplay=6（Count=7）。

## 小节序列化原则

- Rng：state0 u64 + state1 u64（**全 128-bit state，不只是 seed**）。
- Events：next_event_id + pending + next_pending + journal（WorldEvent 全序列化：
  id/sim_frame/kind/parent/source/target/payload(index+字段)）。
- World/Player/Ai/Narrative/Settings：各对象公开 Save/Load（类型索引+字段）。

## 损坏处理（fail-closed）

| 情形 | 结果 |
|------|------|
| magic 错 / version 不支持 / 截断 | Err |
| section CRC / footer CRC 错 | Err |
| 未知 section id | Err |
| 超大文件 (>8MB) | Err |

## Profile 独立文件（profile.wo07p）

u32 version=1 + death_count + load_count + endings_seen。不与 world save 混写。

## 原子写

`<slot>.wo07.tmp` → MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH)（platform
provider；app 启动安装）。崩溃点不会留下半截存档。

## 测试锚点

`save.deterministic_round_trip` / `save.rejects_bit_flip` / `save.rejects_garbage`
/ `save.compose_parse_round_trip`；smoke 真实落盘 `saves/smoke.wo07`。