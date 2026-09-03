# REPAIR_CHANGELOG — FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE

> 本包 = 对上一轮 `OUTPUT_FABLE_ENGINEERING_FOUNDATION` 的增量修复。裁决背景：
> `FOUNDATION_ACCEPTANCE_VERDICT = REJECT_AND_REPAIR`（F-001 致命：声称 YES 但
> 目录为空 + M-001..M-015 合同矛盾）。产品范围与 Opus 制作计划未重开。

## 1. F-001：交付物与裁决脱节 → 已关闭

| 验收项 | 上一轮 | 本轮 |
|--------|--------|------|
| repo_seed 真实代码 | 空目录 | 真实 CMake 工程（8 库+4 可执行+50 测试） |
| schemas/ | 空 | 7 个 schema（含 COMPILED_CONTENT_FORMAT） |
| codex/ | 空 | 8 个文件（global+template+M1..M6） |
| gates/ | 空 | 8 个 gate（R0..RELEASE） |
| evidence/ | 空 | FOUNDATION_VALIDATION + 真实命令日志 |
| engineering/11_INPUT_CONTRACT.md | 缺 | 新建（M-013 输入合同） |

## 2. 合同矛盾修复（M 项映射）

| 项 | 修复 | 证据 |
|----|------|------|
| M-001 build 文档 | presets/testPresets/CI 与代码一致；CMake_MSVC_RUNTIME_LIBRARY；去 /arch:AVX2 锁定 | 03 文档 + configure/build exit 0 |
| M-002 无组合根 | writeover_app 组合根 + core 只依赖 common | 05 文档 + check_deps + CMake |
| M-003 Event 含 Command/Query | WorldEvent 只留 Mutation/Notification；四类分离 | 07 文档 + world_event.h |
| M-004 事件语义/级联 | tick phases 8 步 + 事件天然下移 tick；删 kMaxCascadeDepth 伪安全阀 | 07 文档 + EventBus 单测 |
| M-005 ray 3D 视线 | 2.5D 列边界开口 contract；容量不变量 128 | 09 文档 + ray 黄金测试 |
| M-006 控制器重复状态 | 单 LocomotionState（去 onGround/velocityZ/jumpVelocity 重复） | 10 文档 + 单测 |
| M-007 存档时间戳 | wire 去墙钟（header 24B 固定）→ byte round-trip | 08 文档 + SAVE_SCHEMA + 测试 |
| M-008 replay 断言错式 | A==C 两路定义 + 测试 | 23 文档 + replay 测试 |
| M-009 profile 混淆 | profile.wo07p 独立文件/独立原子写 | 08 文档 + profile 测试 |
| M-010 FactValue 无法装 EntityId | typed variant(bool/int32/EntityId/RoomId/uint8) | 15 文档 + fact round-trip |
| M-011 旁白只能改文案 | NarratorCapabilitySet → typed WorldCommand（world 校验应用） | 17 文档 + narrator 测试 |
| M-012 a/b/f magic | typed condition/action variant + 显式字段 | 17 文档 + storylet 测试 |
| M-013 输入合同缺失/键位指针 | 新建 11 文档；Settings 存可序列化绑定表 | 19 文档 + settings 测试 |
| M-014 终端假探测/崩溃承诺 | 启发式探测 + 正常退出保证、SEH=release gate NOT_READY | 12 文档 |
| M-015 runtime JSON | authoring JSON → contentc.py 编译 → runtime 只读二进制 | 14 文档 + contentc/mapc 实测 |
| 17.x 强 ID/文档细节 | combat/ai 片段同步冻结头（EntityId/ResourceId/StringId） | 13/16 文档补丁 |

## 3. 新增真实代码规模

- 公共头 50+（common 11 / core 5 / world 6 / player 5 / ai 4 / narrative 5 / render 4）
- 实现 ~55 TU；tests 10 文件 50 用例；tools 4 组；scripts 7 个；CI 1 个
- 内容样例：1 Room JSON→.woc、4 facts、2 storylets（typed 编译产物）

## 4. 验证摘要（详见 evidence/）

configure/build(Debug+Release)/ctest/smoke/bench/contract_check/mapc/contentc 全部
exit 0；`50 tests, 0 failed`；bench p99=0.26ms/240 列。

## 5. 诚实边界（KNOWN_LIMITATIONS）

- Raw Input 主鼠标后端、XAudio2 VO、Win32 WriteConsoleOutputW 精化、SEH 崩溃
  恢复、≥3 台干净机器、GitHub Actions 实跑 = NOT_RUN/NOT_READY + 已排期；
- AI/内容集成模块为"空实现+单测覆盖逻辑"（非 fake，见各 gate）。
- 本机证据在 Debug 构建下采集；Release 基线于 R0 复采。

## 6. 交付

`OUTPUT_FABLE_ENGINEERING_FOUNDATION_REPAIRED.zip`。修复完成即止；
不再建议重开规划红队。