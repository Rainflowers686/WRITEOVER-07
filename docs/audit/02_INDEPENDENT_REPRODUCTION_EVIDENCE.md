# Independent Reproduction Evidence

本文件记录本次独立验收中真正执行过的补充验证。

## 1. Archive inventory

上传包实际：
- 201 files
- `repo_seed` 非空
- engineering 00–30 均存在
- schemas / codex / gates / evidence 非空

上一轮“空目录”Fatal 确实已经修复。

---

## 2. GNU 独立编译用途说明

为了独立验证**平台无关逻辑**，在 Linux/g++ 上尝试编译非 Win32 semantic source。

这不是 Windows Release Gate，也不替代 MSVC 证据。

初始严格编译暴露：
- `localtime_s` Windows-only；
- `MapValidator` 中 `uint8_t light > 255` 永假，在 GCC `-Wtype-limits` 下告警；
- `test_common.cpp` 未显式 include `<cmath>`。

为了继续运行平台无关测试，审计时临时：
- alias `localtime_s → localtime_r`
- preinclude `<cmath>`
- 关闭 type-limits warning

没有修改用户源代码。

---

## 3. 原 Test Harness：真实出现 ASSERT FAILED 但最终仍 0 failed

原测试运行输出关键片段：

```text
[PASS] player.jump_then_land
  ASSERT FAILED tests/test_player.cpp:88:
  mapper.MapKey(PhysicalKey::E) == GameAction::Interact
[PASS] player.input_mapper_rebind
...
50 tests, 0 failed
```

因此当前 Test Oracle 存在 false-positive。

---

## 4. 临时把 WO_CHECK 改为 fail-fast 后

仅在审计副本临时把：

```cpp
WO_CHECK(...)
```

改成失败立即 `return false`。

结果：

```text
[FAIL] player.input_mapper_rebind
50 tests, 1 failed
```

证明 InputMapper duplicate-binding bug 是真实存在的，并被旧 harness 隐藏。

---

## 5. Targeted probe

### Storylet compiled content

```text
storylet_load_ok=0
count=2
err=storylet content corrupted:
.../data/storylets/storylets.bin
```

### Save with all 7 legal sections

```text
save_7_sections_ok=0
err=save section count out of range
```

### Ground support

一帧 idle 后：

```text
ground_after_idle=0
z=0.0000
vz=0.0000
traversal=Grounded
```

说明 ContactState 与 Traversal 已出现矛盾。

### Step-up

构造相邻 0.2m 高台并持续前进：

```text
stepup_x=2.6478
z=0
floor=0
```

玩家没有进入高台 cell，自动 step-up 尚未实现。

### Input duplicate

```text
map_E=9
Interact=10
LeanRight=9
```

即 E 返回 LeanRight，而不是重绑后的 Interact。

---

## 6. Static execution-flow evidence

### Event dispatch
`Engine::Run()` 未调用 `EventBus::Dispatch()`。

### Narrative content
`NarrativeModule::Init()` 在 LoadBinary 失败后直接 reset 空 StoryletEngine；
因此 Storylet binary bug 不会让 smoke fail，只会让叙事静默消失。

### Ray smoke
Render smoke 只调用 CastColumnRay，然后填 CharCell 空格与色值；
没有真正 ProjectWall/raster span，因此不是可见 3D renderer 验收。

### Terminal bench
现有 `writeover_bench` 仅计：
`240 × CastColumnRay`
没有计 terminal encoding / fwrite / visible Present。

---

## 7. 本轮要求 Fable 的独立复现

Fable 不应直接相信本报告。

它应在 Windows / 当前 Foundation 上独立：
1. 修 test oracle；
2. 重跑原 50 tests；
3. 添加上述 probes 为正式 regression tests；
4. 复现/否定每一项；
5. 只有证据支持才改 contract / implementation。
