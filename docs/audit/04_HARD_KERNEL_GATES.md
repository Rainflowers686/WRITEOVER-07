# Hard Kernel Gates

Fable 只有通过以下 Gate，才允许把对应模块正式交给 Luna。

## G0 — Test Oracle Gate

PASS:
- assertion failure 可靠令 test fail；
- 原有全部 tests 重跑；
- `player.input_mapper_rebind` 被真正修好而不是删除测试；
- intentional-fail harness self-test 证明 runner 非 false-positive。

Blocking:
- 全部模块。

---

## G1 — Sim/Event/Save Gate

PASS:
- Engine 有正式 tick phase；
- EventBus fan-out 行为明确；
- real causality parent；
- 2000 uninterrupted == 1000 save/load 1000 semantic hash；
- pending reaction exactly-once；
- legal 7-section save PASS；
- corruption fuzz PASS。

Blocks:
- M1 / M5 / M6。

---

## G2 — Height-Span Gate

PASS:
- ≥18 golden scenes；
- floor rise/drop；
- ceiling rise/drop；
- corner tie；
- pitch；
- crouch/stand low-wall；
- visible reference raster；
- no NaN/OOB；
- benchmark at 240 columns。

Blocks:
- M2 / M4。

---

## G3 — Controller Gate

PASS:
- idle grounded stable；
- 0.35m step；
- >step max rejected；
- jump/land stable；
- head hit；
- posture clearance；
- lean wall clamp；
- no penetration property fuzz。

Blocks:
- M3 / M4。

---

## G4 — Input Gate

PASS:
- key down/up；
- contexts；
- Raw Input or explicit UNVERIFIED；
- cursor fallback；
- keyboard fallback；
- focus loss clears state；
- IME isolation；
- no duplicate-context ambiguity。

If Raw Input cannot be tested:
- M3 may proceed only against frozen fake backend;
- `LUNA_M3_READY = CONDITIONAL`, not YES.

---

## G5 — Terminal Gate

PASS:
- full/run/delta encoders；
- actual bytes/frame；
- actual encode time；
- Windows Terminal submit measurement if environment available；
- 16-color quantization；
- unchanged frame fast path；
- no per-cell redundant SGR baseline in final path。

Blocks:
- M2 Ultra120 claim。

---

## G6 — Content/Storylet Gate

PASS:
- storylets.bin runtime load succeeds；
- runtime state separated from definitions；
- typed world commands survive compile/load；
- global stable IDs；
- two-room synthetic refs；
- `--check` actually detects drift；
- invalid refs fail；
- compiled schema == code == docs。

Blocks:
- M4 / M6 content production。

---

# Parallel readiness

`SIX_LUNA_PARALLEL_READY = YES` 需要：

```text
G0 PASS
G1 PASS
G2 PASS
G3 PASS
G6 PASS
G4 PASS or formally accepted CONDITIONAL fallback
G5 minimum functional PASS
OPEN_FATAL=0
```

不要求：
- 最终 XAudio2；
- 正式 VO；
- 六房内容；
- 最终 HUD；
- 最终美术。
