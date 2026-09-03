# COMPILED_CONTENT_FORMAT — WOC/.bin 字节格式与内容管线

> runtime 只读编译产物（M-015 closure）。编译器 = `tools/contentc/contentc.py`
> （stdlib-only、确定性）。本文件是 `mapc`/C++ codec/编译器三方的唯一真源。

## 通用头（所有编译产物）

```
<II magic u32, version u32>
room:    magic=0x574F4331 ("WOC1") version=1
facts:   magic=0x574F4331 version=1
storylets: magic=0x574F4331 version=1
```

## room .woc（与 world/room.cpp DeserializeRoom 对齐）

```
<II> <Q roomId> <utf8 displayName> <ii w,h>
<ffff spawn.x,spawn.y,spawn.z,spawn.yaw>
<I cellCount = w*h>
per cell <ffBBB floor, ceiling, material, light, flags>
（尾部可选扩展字段，reader 不读——向前兼容）
```

utf8 字段 = `<I len>` + UTF-8 字节。material/light/flags 定义见 ROOM_SCHEMA。

## storylets.bin（与 StoryletEngine::Load 对齐）

```
<II>                              (magic/version)
<I storyletCount>
per storylet:
   <Q id> <utf8 textId> <H priority> <B once>
   <I condCount>
   per condition: <B typeIndex> + fields：
     0 fact:      <Q factId> <B equals>
     1 room:      <Q roomId>
     2 npcstate:  <Q npcId> <B state>
     3 frame:     <Q minFrame> <Q maxFrame>
     4 difficulty:<B minLevel>
     5 flag:      <utf8 flag>
   <I actionCount>
   per action: <B typeIndex> + fields：
     0 narrator:  <utf8 textId> <B persona>
     1 dialog:    <utf8 textId>
     2 command:   <B marker=0>   （运行时构造，不持久化）
     3 endgame:   <B ending>
<（fired 集由运行时 Save/Load 处理，不属内容文件）>
```

## facts.bin（validation artifact）

```
<II> <I count> per fact: <utf8 id> <B predicate> <B initial> <I subject>
```

## 确定性

- 文件扫描排序（rooms/*.json 等按文件名）。
- 稳定 id：文件内按 id 排序分配 1..N（storylet/fact/room 各自命名空间）。
- 同一输入集 → 字节级相同产物（CI `--check` 校验）。

## 管线命令

```powershell
python tools/contentc/contentc.py --data-dir data --out-dir data   # 作者 JSON → 产物
python tools/contentc/contentc.py --data-dir data --out-dir data --check  # CI 复现校验
mapc.exe data/rooms/room_01_calibration.woc                        # 产物校验
```

## 已提交产物（repo_seed/data/）

rooms/room_01_calibration.woc（2180B）· storylets/storylets.bin · facts/facts.bin