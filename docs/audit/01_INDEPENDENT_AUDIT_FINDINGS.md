# 独立静态/可执行审计发现

> 基于用户上传的 `OUTPUT_FABLE_ENGINEERING_FOUNDATION_REPAIRED.zip` **真实 201 文件**。
> 本报告只审工程地基，不重开产品范围。
>
> 重要：一些问题并不意味着 Opus Repair 无价值；相反，它已经把工程从“空目录”提升成真实可编译 seed。但目前的 `50 tests, 0 failed`、Raycaster bench 与 smoke 仍不足以证明 Luna 可以安全大规模施工。

# 总裁决

```text
INDEPENDENT_AUDIT_VERDICT = MODIFY_BEFORE_LUNA
PRODUCT_SCOPE_REOPEN      = NO
OPUS_PRODUCTION_PLAN      = KEEP
FABLE_INDEPENDENT_AUDIT   = REQUIRED
FABLE_HARD_KERNEL_WORK    = REQUIRED
START_6_LUNA_AGENTS_NOW   = NO
```

---

# F-01 — 测试 Oracle 会报告“ASSERT FAILED”但仍把测试算 PASS

文件：

```text
repo_seed/tests/test_harness.h
repo_seed/tests/test_player.cpp
```

当前宏：

```cpp
#define WO_CHECK(expr) ::writeover::Check(...)
```

它只返回 bool，不会让 test function 立即失败。

真实现有测试：

```cpp
WO_CHECK(mapper.MapKey(PhysicalKey::E) == GameAction::Interact);
return mapper.MapKey(PhysicalKey::F10) == GameAction::Count;
```

当前程序可以打印：

```text
ASSERT FAILED ... mapper.MapKey(E) == Interact
[PASS] player.input_mapper_rebind
```

也就是说：

> **50 tests, 0 failed 目前不是可信的测试证据。**

独立编译现有代码时已复现这个现象。把断言宏临时改成 fail-fast 后，同一套 50 测试变成：

```text
50 tests, 1 failed
player.input_mapper_rebind = FAIL
```

这是本轮第一优先级。

### Fable 必须做

- 修复 Test Oracle；
- 所有既有 50 tests 用新的 fail-fast 语义重跑；
- 新增一个 meta-test，故意失败时 test executable 必须 exit != 0；
- 不允许继续用“打印失败但函数最后 return true”的模式。

---

# F-02 — InputMapper 真实存在重复绑定冲突

默认：

```text
LeanRight = E
Interact  = F
Weapon1   = 1
Dialog1   = 1
Weapon2   = 2
Dialog2   = 2
...
```

测试再把：

```cpp
Interact = E
```

后 `MapKey(E)` 仍返回第一个匹配的 `LeanRight`。

这不是单一测试错误，而是架构缺口：

> 当前 `PhysicalKey -> 单个 GameAction` 的全局映射不能表达 Gameplay / Dialogue / Menu 输入上下文。

### Fable 必须做

建立 Input Context：

```text
Physical Input
→ Backend
→ Input Context (Gameplay / Dialogue / Menu / Dev)
→ Binding Resolver
→ Action State
```

并修：
- duplicate binding policy；
- rebinding conflict detection；
- Dialog 数字键与 WeaponSlot 数字键共存；
- focus lost 清键；
- key-up；
- mouse delta；
- IME separation。

---

# F-03 — Windows Keyboard backend 会丢 key-up，并 Flush 整个输入缓冲

文件：

`repo_seed/src/platform/windows/win_input.cpp`

现代码只处理：

```cpp
KEY_EVENT && bKeyDown
```

随后：

```cpp
FlushConsoleInputBuffer(input_handle_);
```

结果：
- release event 不会进入系统；
- held state 无可靠来源；
- 一次 Poll 可能丢掉同批其他事件；
- WASD 连续移动无法基于这套事件流可靠维护。

同时 `HasFocus()` 仅检查：

```cpp
GetConsoleWindow() != nullptr
```

这并不等价于真正 foreground focus。

### Fable 必须做

独立 `input_probe.exe` / `input_probe` target，真实验证：
- key down/up；
- 1000Hz mouse；
- Windows Terminal；
- conhost；
- Alt+Tab；
- 中文 IME；
- focus regain；
- cursor fallback；
- keyboard-only fallback。

---

# F-04 — CursorDeltaBackend 的 mouse delta 实际无法通过 IInputBackend 取出来

`CursorDeltaBackend` 自己有：

```cpp
ConsumeDx()
ConsumeDy()
```

但它们不是 `IInputBackend` 接口成员。

`Poll()` 对鼠标移动只返回：

```text
key = Unknown
analog = 0
```

composition root 也没有 downcast/接口来读取累积 dx/dy。

注释声称：

> composition root reads accumulated deltas

但实际代码没有这条路径。

### Fable 必须做

不要补一个脆弱 dynamic_cast。
重新冻结 backend frame/event contract，使 mouse delta 是正式 typed input data。

---

# F-05 — Engine 根本没有调用 EventBus::Dispatch()

文件：

`repo_seed/src/core/engine.cpp`

主循环：

```text
for modules: SimTick
clock.Tick
render
```

没有：

```cpp
events.Dispatch();
```

因此真实 App 中模块 Post 的事件不会进入 consumer fan-out / journal 的正常阶段。

同时当前没有完整 Event Phase Integration。

### Fable 必须做

亲自实现一个最小 deterministic mini-world，冻结 tick phases：

```text
INPUT SAMPLE
→ ACTION/COMMAND BUILD
→ AUTHORITATIVE MUTATION
→ EVENT EMISSION
→ EVENT OBSERVATION/FANOUT
→ NEXT-TICK COMMAND QUEUE
→ SNAPSHOT FINALIZE
→ RENDER
```

再用测试证明顺序固定。

---

# F-06 — EventBus::Post 目前所有事件都进入 next_pending_

当前：

```cpp
Post(...) {
    next_pending_.push_back(...)
}
```

`Dispatch()` 先 dispatch `pending_`，最后：

```cpp
pending_.swap(next_pending_);
```

这使得即便 Engine 将来“在 tick 末调用一次 Dispatch”，mutation phase 刚产生的 event 也会再延迟一 tick 才观察到。

这可以是一个合法设计，但必须**明确并一致**。

当前注释同时暗示“end-of-tick fan-out”，实现却是 unconditional next-tick。

### Fable 必须裁决

二选一并测试：
- mutation 同 tick 末观察；
- 或所有观察固定下一 tick。

不要让 Luna 自己猜。

---

# F-07 — NarrativeModule 的因果账本目前是伪造的

`composition_root.cpp` 每个 tick 都写：

```cpp
EventId::New(frame)
parent = EventId::New(frame-1)
```

与真实 `EventBus` 无关。

这会让 F3 因果面板看起来“有因果”，但实际只是 frame 链。

### Fable 必须做

删除这类 fake causality。
Causality 只能来自：
- real EventId；
- real parent_event_id；
- real command→event relationship。

Foundation 允许“无数据”，不允许假数据。

---

# F-08 — Storylet 编译产物按当前代码会被 LoadBinary 判 corrupted

这是已通过独立 probe 复现的真实 bug。

`contentc.py` 生成 storylets.bin 后，在最后一个 storylet action 结束即 EOF。

但：

```cpp
StoryletEngine::Load(...)
```

在读取所有 definitions 后还继续：

```cpp
const uint32_t fired_count = d.ReadU32();
```

`LoadBinary()` 最后检查：

```cpp
if (d.HasError()) return corrupted;
```

独立调用：

```text
StoryletEngine::LoadBinary(data/storylets/storylets.bin)
```

真实结果：

```text
storylet_load_ok=0
count=2
err=storylet content corrupted
```

因此当前 `NarrativeModule::Init` 会把 engine reset 成空 StoryletEngine。

Smoke 仍然可以 PASS，因为 smoke 没要求叙事内容真的加载成功。

### Fable 必须做

分离：
- immutable content definitions；
- runtime Storylet state（fired/flags/cooldown）。

不要让 `StoryletEngine::Save` 序列化整份 definitions。

---

# F-09 — WorldCommandAction 在内容编译与 Storylet Save 中会丢失 payload

当前 contentc：

```python
worldcommand -> write marker 0
```

C++ Load：

```cpp
case command:
    Read marker
    不生成 action
```

StoryletEngine::Save 同样只写 marker。

所以未来需要的：

```text
旁白锁门
旁白断电
Storylet 触发基础设施命令
```

不能由数据驱动 storylet 表达。

### Fable 必须做

定义有限、强类型、可验证的 content command union，例如：

```text
SetDoorOpen
SetDoorLocked
SetPower
SetFlag
QueueNarratorLine
EndGame
```

authoring JSON → contentc → runtime variant 全链路一致。

---

# F-10 — Content compiler 的 `--check` 实际没有实现 check

`argparse` 有：

```python
--check
```

但后续代码从未使用 `args.check` 做：
- temp recompile；
- byte compare；
- no-diff verification。

CI 中：

```text
python contentc.py ... --check
```

因此目前并没有验证“确定性编译产物未漂移”。

### Fable 必须做

真正实现：
1. compile to temp;
2. compare expected committed/runtime outputs;
3. mismatch → nonzero exit；
或明确改名，不能继续做假 check。

---

# F-11 — 所有 Room 编译时 RoomId 都硬编码为 1

`contentc.py`：

```python
body += struct.pack("<Q", 1)
```

现在只有一个测试房所以没爆。

但正式 P0 有 6 Signature Rooms。

### Fable 必须做

建立跨文件稳定 registry：
- RoomId
- NpcId/archetype
- StoryletId
- FactId
- Resource/String IDs

并增加 duplicate / unknown reference validation。

---

# F-12 — Room compiler 目前把 NPC/storylet refs 永远写成空

contentc 明确：

```python
# NPC / storylet refs (empty ... foundation)
body += 0
body += 0
```

正式 Room 数据管线因此还没有做到“内容 schema 已冻结”。

### Fable 必须提前完成

至少让一个 synthetic reference room：
- 引用 NPC；
- 引用 Storylet；
- 引用基础设施节点；
- mapc 能检查引用；
- Runtime load 后能 resolve。

不需要写正式 Room 2–6。

---

# F-13 — Save parser 不能接受全部 7 个合法 Section

`SaveSectionId::Count = 7`。

Parser：

```cpp
if (section_count >= Count) reject;
```

若保存 0..6 共 7 个 section：

```text
save_7_sections_ok = 0
error = save section count out of range
```

这是独立 probe 已复现的真实 bug。

正确至少应允许：

```text
section_count <= Count
```

并且还应验证：
- duplicate section IDs；
- required sections；
- unknown future sections policy；
- payload size overflow；
- truncation/fuzz。

---

# F-14 — 当前“deterministic replay”测试并没有测试 Save→Load→Resume

现有：

`replay.same_seed_identical_bytes`

只证明：
- 两次同 seed / 同 tick 的 RNG+EventBus bytes 相同；
- 300 tick 与 600 tick 不相同。

它没有证明：

```text
uninterrupted 2000 ticks
==
1000 ticks → Save → new process/state → Load → 1000 ticks
```

当前 Foundation Verdict 中关于 replay 的信心仍然超前于测试。

### Fable 必须亲自实现 reference deterministic mini-world + real replay test。

---

# F-15 — 当前 smoke save 只存 RNG + EventBus，不是完整游戏状态

`composition_root.cpp` smoke：

```text
SaveSection::Rng
SaveSection::Events
```

没有：
- Player；
- World；
- AI；
- Narrative；
- Storylet runtime；
- Infrastructure；
- pending commands。

所以：

> “smoke → save → restore”不能证明课程要求的“读档回到原场景/状态”。

Foundation 阶段允许 incomplete，但 Fable Hard Kernel 应至少做 synthetic complete mini-world save。

---

# F-16 — Controller 在静止地面上会自己变成 `grounded=false`

独立 probe：

```text
spawn z=0
contact.grounded=true
IntegrateLocomotion(no input, one tick)
```

结果：

```text
ground_after_idle=0
z=0
vz=0
Traversal=Grounded
```

形成内部矛盾：
- traversal = Grounded
- contact.grounded = false

下一 tick 才开始重力。

原因：
`AabbBlocked` 只判断 penetration，没有 ground contact query；
controller 在 box_z 不 blocked 时直接把 grounded=false。

### Fable 必须做

建立明确：
- GroundProbe / floor support；
- snap-to-ground epsilon；
- rising/falling ceiling collision；
- floor contact；
- step-up。

---

# F-17 — 自动 Step-Up 目前没有实现

P0 要有约 0.35m 自动上阶。

当前水平 move 只把 candidate AABB 放到高 floor cell：
- floor > current feet z → `AabbBlocked=true`
- X/Y move 被取消
- 没有 step candidate。

独立 probe 给 0.2m 高台、持续向前，玩家不能上台。

### Fable Hard Kernel

亲自实现 reference：
- sweep；
- step up；
- ceiling clearance；
- step down；
- ledge/fall；
- stable grounding。

---

# F-18 — Lean 目前只是设置 enum，没有碰撞/相机约束

`SetLean()`：

```cpp
ls.lean = Left/Right
```

没有：
- lean camera offset；
- wall clamp；
- exposed head/shoulder sample；
- weapon near-wall；
- renderer view update。

R0 的“探头真实成立”目前仍未证明。

---

# F-19 — Vault / Mantle / Ladder 只是 enum/feature vocabulary，没有 kernel

Foundation 允许 stub，但这正是 Luna 容易写歪的高风险动作。

Fable 不必做最终动画，但应实现：
- eligibility query；
- deterministic traversal state transition；
- collision-safe finish；
- cancel/fallback；
- tests。

若时间不足：
按照 Opus 原 gate，Prone/Vault 可延迟，但 Stand/Crouch/Jump/Lean 必须硬通过。

---

# F-20 — Height-Span 当前只生成“进入更窄空间”的边界面

`CastColumnRay()`只做：

```cpp
far.floor > near.floor   → floor rise segment
far.ceiling < near.ceil  → ceiling drop segment
```

因此：
- 从高平台看向低地面（floor drop）；
- 从低天花看向更高房间（ceiling rise）

没有对应反向 boundary face。

这会导致 trench / platform lip / opening backside 视觉不完整。

### Fable 必须裁决并黄金测试

不能只测：
- low wall floor rise。

至少测双向：
- low→high floor；
- high→low floor；
- high→low ceiling；
- low→high ceiling；
- mixed opening；
- corner crossing；
- diagonal tie；
- out-of-bounds；
- near-zero direction；
- player above/below target。

---

# F-21 — DDA corner tie 目前只走 X，可能产生 spurious intermediate cell

当前：

```cpp
if (t_max_x <= t_max_y) X
else Y
```

精确穿过格角时，只先跨 X，再下一次跨 Y，可能“看到”一个实际上只在角点接触的邻格。

Fable 应定义：
- tie epsilon；
- supercover / two-axis advance；
- corner visibility policy；
并做 golden tests。

---

# F-22 — 当前 Render smoke 没真正把 raycaster geometry 画到屏幕

`RenderModule` 确实调用了 CastColumnRay。

但随后：
- 只对 40 列设置颜色；
- `code_point = U' '`;
- foreground 改色；
- background 基本固定。

也没有：
- `ProjectWall`；
- wall span rasterization；
- floor/ceiling；
- depth；
- player position每帧同步。

所以现有 Smoke 证明：
> CastColumnRay 被调用 + Terminal Submit 被调用。

它**没有证明**：
> 真实字符 3D 画面已经成立。

Fable 的 Hard Kernel 必须做真正 visible reference renderer。

---

# F-23 — Renderer 的 player view 在 composition root 只设置一次

当前：

```cpp
render->SetPlayerView(spawn, 0)
```

之后没有从 PlayerModule 更新。

所以即使输入/移动实现，画面也不会跟随玩家。

Hard Kernel 应改为：
- immutable RenderSnapshot；
或
- 每 frame 从只读 player snapshot 获取。

---

# F-24 — ANSI backend 是逐 cell 重复完整颜色 SGR，不是 color-run / delta

每个 cell 都输出：

```text
reset
38;2;r;g;b
48;2;r;g;b
glyph
```

即使 1000 个连续 cell 完全同色，也重复发颜色码。

240×67 = 16080 cell。
这条路径典型会产生数十万字节/帧，120Hz 是真正风险。

当前 `writeover_bench` **只测 CastColumnRay**：
- 不测 CharCell composition；
- 不测 ANSI encode；
- 不测 fwrite/terminal submit；
- 不测 bytes/frame；
- 不测 full vs delta；
- 不测 backlog。

因此：

> `BUDGET=PASS` 不能作为 Ultra120 Terminal Throughput 证据。

### Fable 必须亲自做 terminal kernel + bench

至少：
- uniform frame；
- highly changing frame；
- 10% changed；
- 50% changed；
- full encoder；
- run/color state encoder；
- delta encoder；
- bytes/frame；
- encode ms；
- submit ms；
- effective Hz；
- fallback Win32 path。

---

# F-25 — Win32 fallback 目前不是“完整 16 色映射”

现代码只根据：

```text
bg_r >= 128
fg_r >= 128
```

设置少量 red bits。

作为 foundation stub 可以接受，但不能作为最终 COMPATIBILITY fallback。

Fable 可只冻结正确 RGB→16 color palette quantization reference；
Luna 再美化。

---

# F-26 — `CreateTerminalBackend` 的最后 fallback 返回未 Init 的 ANSI backend

若：
- probe 不支持 VT；
- Win32 Init 失败；

函数直接：

```cpp
return std::make_unique<AnsiTrueColorBackend>();
```

没有 `Init()`。

这是小而真实的 correctness bug。

---

# F-27 — Benchmark 指标命名仍混淆

结构字段叫：

```text
p99_ms
```

实现却是：

> slowest 1% frames 的平均 frame time。

它不是严格意义上的 P99 percentile。

建议改名：
- `worst_1pct_avg_ms`
- `one_pct_low_fps = 1000 / worst_1pct_avg_ms`

并单独提供真正 percentile 如需要。

---

# F-28 — `contentc` / schema 还有内容格式不一致

`COMPILED_CONTENT_FORMAT.md` 的 facts.bin 描述与 Python 实际输出字段不完全一致。

Room:
- RoomId 全硬编码 1；
- refs 空。

Storylet:
- compiled definitions 与 runtime fired state 混用一个 Load。

Fable 应把 content compiler 作为一个 Hard Kernel，而不是交 Luna 自由扩展。

---

# F-29 — MapValidator 有一个永远不可能触发的检查

`GridCell.light` 是 `uint8_t`。

但 validator 检查：

```cpp
if (cell.light > 255)
```

永远 false。

这本身不严重，但说明：
> 目前“validator 很严格”的印象不能代替 adversarial content tests。

需要 content fuzz / boundary tests。

---

# F-30 — Windows-only 代码仍有明显可移植性边界，ARM 报告应诚实

独立 GNU 编译时还发现：
- `localtime_s` 是 Windows-specific；
- `test_common.cpp` 缺显式 `<cmath>`，靠 MSVC include chain 可编过；
- `/W4 /WX` 在 MSVC 下没暴露 `uint8_t >255` 这类问题。

课程不要求真的 ARM 部署，所以无需为了可移植性重写。
但 Fable 应继续把 Win32 泄漏限制在 platform layer，并在 ARM 说明中诚实描述。

---

# 推荐 Fable 亲自拿下的 7 个 Hard Kernels

## HK-0 Test Oracle / Contract Trust
先修测试框架，否则后面所有“PASS”都不可信。

## HK-1 Deterministic Sim + Command/Event/Save/Replay
Mini-world reference，真实 save→load→resume。

## HK-2 Height-Span Renderer Reference
双向高度边界 + corner policy + projection + visible compositor + golden scenes。

## HK-3 Character Controller Geometry
ground/step/jump/crouch/prone/lean + vault eligibility + property tests。

## HK-4 Windows Input Probe
Raw Input + focus + key up/down + context mapping + fallback。

## HK-5 Terminal Presenter / Encoder
run-length/color-state + delta + full adaptive + actual throughput bench。

## HK-6 Content / Storylet Compiler Runtime Contract
stable registries + typed commands + definition/runtime-state split + deterministic --check.

这些完成后，Luna 再做：
- 6 Room 内容；
- NPC 具体行为；
- 完整 GOAP/Utility；
- HUD 美化；
- 武器手感；
- 设置 UI；
- Voice integration；
- Judge 内容；
- 迭代和大量 bugfix。
