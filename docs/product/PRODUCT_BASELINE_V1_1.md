# PRODUCT BASELINE v1.1 — WRITEOVER-07

> **Superseded by PRODUCT_BASELINE_V1_2.** This document is preserved as
> history and is not the active product baseline.

> 状态：冻结（2026-09-03）。本文件是产品规格，不是策划长书。
> 取代此前任何"课程作业可交"表述。六模块施工以本文件为唯一产品输入。

## 1. Product Goal

目标不再是"课程作业可交"。目标：**high-quality short-form indie game / Steam-demo-grade experience**。

优先级：
1. 真正好玩
2. 真正独特
3. 真正稳定
4. 商业级第一印象
5. 课程第一

目标首周目：**30–45 min**（不是 2 小时）。

## 2. 世界规模

- 正式：**6 Signature Rooms**
- 建议：**4–6 Micro / Secret Spaces**
- 不恢复 14–18 主房间
- 每个 Signature Room 必须有明确 gameplay identity
- 保持 Height-Span Grid
- 不恢复 Sector/Portal、BSP、triangle mesh world、arbitrary free-form geometry

## 3. Persistent Social Memory（新核心 Pillar）

不再允许只有 1–2 个重要 NPC 有记忆、普通 NPC 一次性木偶。

- 大约 **18–24 个 identity-bearing NPC**
- 分层：2–3 Anchor NPC、4–6 Secondary NPC、8–12 Witness/Ambient NPC、6–10 Guard/Security（类别允许重叠）
- 不是 24 个 Full GOAP
- 所有拥有 identity 的 NPC 必须具备：
  - Persistent Identity
  - Persistent Memory Capsule
  - Relationship
  - Belief
  - Knowledge Provenance
  - Salience

## 4. Memory Capsule

普通 NPC 建议支持 4–8 个长期记忆槽。语义字段建议概念：
- event type
- actor
- target
- room
- frame/timeline
- salience
- valence
- confidence
- source/provenance
- tags

本轮不实现全部，本文件只冻结产品/数据语义。

## 5. Knowledge Provenance

NPC 只能知道：自己看到、自己听到、别人告诉、系统广播、旁白声明、环境推理、残留记忆。

产品语义至少支持：
- DIRECT_WITNESS
- HEARD_SOUND
- HEARSAY
- SYSTEM_FEED
- NARRATOR_CLAIM
- ENVIRONMENTAL_INFERENCE
- RESIDUAL_MEMORY

禁止 NPC impossible knowledge。

## 6. Memory Callback 核心价值

支持：玩家已经忘记的小行为在 10–30 分钟后被一个不起眼 NPC 重新提到并产生真实影响。

不是只有杀人/救人/结局选择。允许小事件：
- PLAYER_LEFT_DOOR_OPEN
- PLAYER_POINTED_WEAPON
- PLAYER_GAVE_MEDKIT
- PLAYER_IGNORED_GREETING
- PLAYER_DISABLED_CAMERA
- PLAYER_USED_VENT
- PLAYER_RETURNED_CARD
- PLAYER_LOOKED_AT_PHOTO

不能每件事都回调；通过 Salience 选择值得长期保留的事件。

## 7. Rumor / Belief Propagation

v1.1 正式允许 NPC 把信息告诉另一个 NPC。

- Fact、Claim、Belief 必须继续分开
- NPC 可以相信错误信息
- 特别允许 Narrator 向 NPC 提供错误 Claim → 被污染的社会现实
- 不实现完整 gossip simulator，只冻结能力和 ownership

## 8. Residual Memory

正式恢复/批准：部分 NPC 可以跨 Death / Reload / Forced Rewind / Narrative Reset 保留少量模糊记忆。

- 例如：时间线回滚后 NPC 说"……我们是不是见过？"
- 不是所有 NPC
- 不做随机大规模 meta-memory
- 由 Storylet + explicit flags 控制

## 9. Narrative Sovereignty

旁白 = **Narrative Authority / Timeline Administrator**。

正式权限层级：
- Presentation Authority
- Infrastructure Authority
- Actor/Organization Authority
- Temporal Authority
- Save/Load Authority

### 9.1 Presentation Authority
允许旁白修改 Objective、HUD label、Room name、Enemy/friend marker、Map presentation、Broadcast、Subtitle、Warning、Mission status。不得永久摧毁核心 accessibility 设置。

### 9.2 Infrastructure Authority
允许 lock/unlock door、power off/on、camera、alarm、elevator、security clearance、routing。必须是真实世界状态改变。

### 9.3 Actor/Organization Authority
旁白可通过设施系统下令守卫、改变 patrol assignment、发布/撤销通缉、隔离/放行 NPC、调用增援、向 NPC 发布 Claim。NPC 是否相信由 Belief / provenance / relation 决定（不做魔法式思想直写）。

### 9.4 Temporal Authority
正式允许：kill player、force respawn、prevent death、force checkpoint、delay checkpoint、rewind 10s/30s/room、restart room、partial timeline reset、loop segment、load alternate checkpoint、preserve selected state across rewind、restore selected dead NPC、keep selected NPC dead。

### 9.5 Save/Load Authority
正式允许：refuse save、force save、rename visible save、hide visible save、temporarily lock save slot、pretend visible save corrupted、refuse requested load、force another load、load different checkpoint、rewrite visible slot metadata、relabel save as experiment record、force rewind、alter visible timeline history。

唯一硬禁止：**PERMANENTLY DESTROY LAST RECOVERABLE PLAYER PROGRESS**。

## 10. Protected Recovery State

- VISIBLE NARRATIVE SAVE STATE 与 PROTECTED RECOVERY STATE 分离
- 旁白拥有 Visible Narrative State 几乎完全权限
- 旁白没有 DeleteProtectedRecovery 能力
- Protected Recovery 平时不展示给普通玩家；是软件可靠性保险
- 保留 OS-level emergency exit、Recovery Mode / safe fallback

## 11. Endings

- 恢复 **4 Macro Endings**
- 新增 **1 Hidden Loop / Residual Memory Meta Ending**
- 四个宏观结局不要求四套终局地图，依靠 truth/control/relationships/memory/narrator dominance 状态组合决定
- 隐藏 Meta Ending 与 death/reload/rewind/residual memory/narrative experiment 相关

## 12. 恢复的旧废案（RESTORE_NOW）

- 4 macro endings
- hidden loop/meta ending
- deeper narrator director
- more identity-bearing NPCs
- Persistent Social Memory
- Residual Memory
- 30–45 min route
- more selective VO
- Identity Deception / social stealth
- small systemic destruction
- 1 systemic tool
- deeper power/security systems
- 4–6 micro spaces

## 13. Identity Deception

正式恢复：badge、identity credential、uniform/tag、access identity。影响 doors/cameras/guards/NPC reactions。普通 NPC 可认出伪装、选择举报或沉默，受 past memory / relationship 影响。本轮不实现。

## 14. Systemic Tool

正式规划：3 primary weapons + 1 systemic tool（优先于第四把普通枪）。可能用于：disable camera、overload door、stun electronics、reroute power、expose false marker、interfere with narrator control。具体玩法由 M3/M4 决定。

## 15. Small Systemic Destruction

正式允许：lights、glass、camera、terminal、fuse box、alarm、small lock devices。
不恢复：destructible walls、voxel destruction、large-scale physics destruction。

## 16. Selective VO

允许增加 VO。优先 Narrator、Anchor NPC、重要 Secondary、关键 Witness callback。普通 NPC 关键远距离 callback 有 VO 的优先级非常高。不做全 NPC 全语音。

## 17. RESTORE_IF_GREEN（仅 backlog，本轮不实现）

- A. Limited Ramp Cell（不是 arbitrary slope）
- B. Limited semantic dialogue input（finite intent / keyword questions；不是 runtime LLM、自由生成 NPC）

## 18. PERMANENTLY OUT

继续禁止（除非未来 Human 明确重新批准）：
- Shop / economy system、traditional loot economy
- 14–18 signature rooms
- Sector/Portal、BSP、triangle world、arbitrary slopes
- LAN multiplayer、server、spectator interference
- runtime LLM、free-form generative NPC
- 10+ Full GOAP NPC、all-NPC full VO
- 2+ hour campaign、large-scale destruction
- wallrun、double jump、grapple、online services

## 19. 六模块 ownership（为 v1.1 准备）

| 模块 | Owned |
|------|-------|
| M1 Core | timeline、save/load authority execution、Protected Recovery、checkpoint、timeline rollback、integration |
| M2 Render | presentation manipulation、HUD、glitch、visual deception、terminal performance（不拥有世界真相） |
| M3 Player | movement、combat、systemic tool、interaction input |
| M4 World | room、infrastructure、identity credentials、destructible devices、world facts |
| M5 AI | NPC identity、memory、belief、knowledge provenance、relationship、perception、planning |
| M6 Narrative | Narrator authority policy、Storylet、Claim generation、timeline/narrative decision、ending logic |

M6 不能直接破坏 protected recovery；只能 REQUEST authority action。M1 validation + execute。

## 20. 不提前写公共 API

本基线主要冻结产品语义、ownership、安全边界、扩展方向。禁止为未来可能需要的 MemoryCapsule/NarrativeSovereignty/TemporalCommand 提前创造 20 个 public interfaces、抽象 factory、泛型 message bus、plugin system。

> YOU ARE ALLOWED TO DOCUMENT FUTURE CONTRACTS WITHOUT IMPLEMENTING SPECULATIVE TYPES.
