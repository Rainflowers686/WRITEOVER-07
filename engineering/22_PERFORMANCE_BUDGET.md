# 22_PERFORMANCE_BUDGET (REPAIR PASS v2)

> 修订：**1% low 定义修正**——旧文写 "1st percentile frame time"（错误方向）；
> 现定义为 **slowest 1% 帧的平均帧时间**（benchmark.h FrameStats::p99_ms 注释
> 与实现一致；测试 `bench.p99_is_one_percent_low` 锚定）。budget 表不变。

## Frozen Budgets

| Metric | Budget | Preset |
|--------|--------|--------|
| Sim tick | <4ms | all |
| Raycast render frame | <4/5/6/8ms | ULTRA/HIGH/PRES60/COMP |
| Present submit | <2ms (ULTRA/HIGH), <8ms (PRES60/COMP) | per preset |
| Input poll→gameplay | <1ms | all |
| GOAP replan | <2ms | all |
| Storylet evaluate | <1ms | all |
| Room load | <1s | all |

## 1% low（修正定义）

```text
1% low = 最慢 1% 帧的平均帧时间
实现：sorted 样本取后 max(1, N/100) 个求均值  → p99_ms
基准输出：scenario,count,avg_ms,p99_ms,min_ms,max_ms
```

## 实测锚点（Debug，本机）

```text
raycast_column_sweep,240,0.185,0.260,0.167,0.274  BUDGET=PASS
```

## Presets（Sim 恒 120Hz）

ULTRA120 240×67 / HIGH_REFRESH 192×54 / PRESENTATION_60 200×60 /
COMPATIBILITY 160×45 —— 只改 render/present 分辨率与目标帧率，Sim 不动。

## 分析纪律

- 9/8 前不预优化；9/8–10 profiler 驱动；9/11–14 全量优化；9/15 冻结只修 bug。
- 回归阻断：1% low 降 >15%、平均降 >10%、Sim >4ms、滚动/闪烁。
- Present thread / render workers 默认关闭（`WO_FEATURE_PRESENT_THREAD=0`），
  只有 profiler 证明才开（21 文档）。

## 报告友好

**Core Algorithm**: 百分位统计（slowest-1% 平均）。
**Course Note**: 性能结论只认 bench CSV——不是感受。