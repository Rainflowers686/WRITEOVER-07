# 14_WORLD_CONTENT_SCHEMA (REPAIR PASS v2)

> 修订（M-015 closure）：**authoring JSON → contentc.py 编译 → 运行时只读编译产物**
> （.woc / .bin），运行时零 JSON 解析、零第三方依赖。编译器 stdlib-only。
> 实现：`tools/contentc/contentc.py`、`world/room.h/.cpp`（WOC 编解码）、
> `narrative/storylet.cpp::LoadBinary`。产物已提交于 `repo_seed/data/`。

## 内容管线（冻结）

```
data/rooms/*.json  ─┐
data/facts/*.json   ├─ python tools/contentc/contentc.py（确定性、stdlib-only）
data/storylets/*.json┘        │
                 ┌────────────┴────────────┐
                 ▼                          ▼
 data/rooms/*.woc (room 二进制)      storylets.bin / facts.bin
                 │                          │
    mapc 校验(.woc, exit 0/1)       C++ LoadBinary（运行时唯一入口）
```

- `--check`：CI 里重编译并 diff，防"提交 JSON 忘提交产物"。
- 确定性：稳定 id 按文件内排序分配（1..N）；文件扫描排序；输出字节稳定。
- 运行时**不**解析 JSON（release 不打包 .json 也可运行）。

## 编译 Room 二进制（WOC1 v1）——与 C++ codec 对齐

```
<II magic/version> <Q roomId> <utf8 displayName> <ii w h> <ffff spawn+x/y/z/yaw>
<I cellCount=w*h> 然后 per cell: <ffBBB floor ceiling material light flags>
```

缺失格子 = 默认开阔格（floor0/ceil4/空 flags）。mapc 会校验：
spawn 在界内、站立净空、格子 floor<ceiling、实心墙存在性等。

## Fact / Storylet 编译产物

- `facts.bin`：facts 清单（id/初始值）——编译器校验与"事实注册表"的编译时快照；
  运行时 FactStore 播种与运行期演进见 15 文档（foundation 阶段只以测试驱动）。
- `storylets.bin`：header(<II>) + `StoryletEngine::Load` 的 typed 格式——
  condition/action 以**类型索引 + 字段**序列化（对应 typed StoryletCondition/
  StoryletAction 的公共类型，无 a/b/f magic fields）。
- 内容引用（fact/npc/room）在编译期做 duplicate/unknown 检查（缺引用 fail-fast）。

## Authoring 格式（作者友好，runtime 不读）

- 每文件 `schemaVersion:1`；房间/事实/故事节 JSON 见 `data/` 实例与
  `schemas/`（ROOM_SCHEMA/NPC_SCHEMA/STORYLET_SCHEMA/FACT_CLAIM_SCHEMA）。

## 测试锚点

`room.codec_round_trip`（C++ 编解码）、`map_validator.spawn_fits`、
`mapc` 对真实 .woc 的 exit 0 实测、`storylet` 系列（typed 条件求值）。

## 报告友好

**Course Note**: JSON 是给 D/F（内容 Owner）写的；机器编译成确定性二进制，
Codex 与 6 人并行写内容不破坏运行时。