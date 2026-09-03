# 08_SAVE_SERIALIZATION_CONTRACT (REPAIR PASS v2)

> 修订（M-007/M-008/M-009 closure）：
> 1) wire 格式**不携带墙钟时间戳**（header 24B 固定：magic4+version4+count4+rsvd4+rsvd8）→
>    确定性 round-trip 可做 byte-for-byte 比较；
> 2) 确定性 replay 测试定义修正（A==C 两路对比，而非 B==C 的错式断言）；
> 3) Profile 元数据独立文件（`profile.wo07p`），与 world save 分离；
> 4) 原子写：tmp + 平台 MoveFileExW provider（app 启动时 InstallPlatformAtomicReplace）。
> 对应实现：`core/save.h`、`common/serialize.h`、`common/io.h`、`core/profile.h`。

## 文件布局（`.wo07`）

```
24B  header   magic "WO07" | version=1 | section_count | rsvd | rsvd
N×12B section header      section_id u32 | data_size u32 | crc32
     section payloads     逐字段小端序列化（无 struct dump / 无 padding 依赖）
4B   footer CRC32(所有 header+payload)
```

保存节枚举：Player / World / Rng / Events / Ai / Narrative / SettingsGameplay。

## 损坏安全

- magic 错 / version 不支持 / 截断 / section CRC 错 / footer CRC 错 → 全部
  fail-closed（`ParseSaveBuffer` 返回 Err；单元测试：`save.corrupt_rejected`、
  `save.rejects_bit_flip`、`save.rejects_truncated`、`save.rejects_garbage`）。

## 原子写

1. 写 `<slot>.wo07.tmp`；
2. `ReplaceFileAtomic(tmp, final)`：默认 remove+rename；Windows 上 app 启动时
   注册 `MoveFileExW(REPLACE_EXISTING|WRITE_THROUGH)` provider（win_file_io.cpp）。

## Deterministic replay（修正定义）

```
path A: seed S + 命令序列 → 运行 1000 ticks → 快照 A
path B: seed S + 命令序列 → 运行 1000 ticks → 快照 B(参考)   // 语义与 A 相同
reload: 从快照 B 恢复 → 再运行 1000 ticks → 快照 C
断言:   A == C（同 tick 同状态），NOT (B == C)   ← 错误写法
```

- 快照必须覆盖：sim RNG 全 128-bit state、事件总线（含 journal）、world 状态、
  FactStore、NPC、叙事 fired 集。
- 断言使用字节级比较（wire 无时间戳，允许 byte-for-byte）。
- 单元测试：`replay.same_seed_identical_bytes`、`save.deterministic_round_trip`。

## Profile（独立于 world save）

`ProfileMeta { death_count, load_count, endings_seen }` → `profile.wo07p`。
旧 world save 的 load 永不回滚 profile 计数；profile 不改变关卡内确定性剧情。

## 报告友好

**STL**: std::vector<uint8_t>、显式小端序列化。
**Core Algorithm**: CRC32 + sectioned container。
**Course Note**: "存档不等于把 struct 直接写盘"——讲义级设计点。