# Luna Handoff Rules

Fable Hard Kernel 完成后，GPT-5.6 Luna 的角色是“施工”，不是重新当架构师。

## 全局

Luna 每个任务必须先读：
1. root AGENTS.md
2. Hard Kernel reference contract
3. 自己模块 prompt
4. 相关 golden/property tests

不得：
- 删除/弱化 Fable golden tests；
- 重写 hard kernel semantic；
- 为了让测试过而改 expected output；
- 新建第二套 event/input/math/ID；
- 顺手重构其他模块；
- 加线程/依赖；
- 改 save/content schema；
- 扩产品范围。

## M1 Luna

可以：
- 正式菜单；
- settings persistence；
- full snapshot assembly；
- release integration；
- logs/debug。

不得：
- 改 tick phase；
- 改 event ordering；
- 改 save wire contract。

## M2 Luna

可以：
- 正式 wall/floor/ceiling compositor；
- texture-ish glyph/material；
- lighting；
- HUD integration；
- terminal optimizations。

不得：
- 改 height boundary semantics；
- 改 DDA corner policy；
- 用只测 ray 的 bench 冒充 present bench。

## M3 Luna

可以：
- 手感；
- acceleration；
- recoil；
- reload；
- weapon feedback；
- aim assist。

不得：
- 改 ground/step/clearance invariant；
- 改 input contexts；
- 绕开 InputBackend。

## M4 Luna

可以：
- 六房 graybox/content；
- infrastructure；
- puzzles；
- mapc 规则。

不得：
- 改 compiled format；
- 手写特殊 case 绕过 schema；
- 给每个 room 写专用代码类。

## M5 Luna

可以：
- perception；
- memory；
- Utility；
- GOAP-lite；
- guard FSM。

不得：
- 改 event phases；
- 让 AI 直接 mutate authoritative world；
- 依赖 unordered iteration 决策。

## M6 Luna

可以：
- 正式 Storylets；
- narrator lines；
- dialogue wheel；
- causality presentation；
- Judge content。

不得：
- fake causality；
- 直接改 FactStore；
- storylet 直接 bypass typed WorldCommand。

## 每次输出

必须：

```text
CURRENT_FACTS
REFERENCE_KERNELS_USED
OWNED_FILES
INVARIANTS
PLAN
IMPLEMENTED_CHANGES
BUILD_COMMAND
BUILD_RESULT
TEST_COMMAND
TEST_RESULT
HARD_KERNEL_GATE_RESULT
INTEGRATION_RESULT
BENCHMARK_RESULT
DIFF_SUMMARY
PUBLIC_CONTRACT_CHANGED = NO
KNOWN_LIMITATIONS
STOP_REASON
```
