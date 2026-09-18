# WRITEOVER-07 性能测量记录（2026-09-18）

## 结论先行

本次测量显示，Release 构建的内部 Character-Art 渲染和整帧 CPU 管线都有明显余量；但这些指标不等于 Windows Terminal 窗口中的可见帧率。游戏仍保持 **120 Hz 固定步长模拟**，终端写入、宿主终端刷新和字体绘制不在内部 benchmark 的同一计时边界内。

在本机测得的代表性结果是：核心渲染约 **924 FPS 等效吞吐**，内部整帧管线约 **697 FPS 等效吞吐**；另一个以真实程序 smoke 路径、标准输出重定向为边界的固定管线测量约 **113.5 FPS**。最后一个数字是提交循环的近似值，不是可见终端 FPS。

## 测量条件

| 项目 | 值 |
|---|---|
| 机器 | Intel(R) Core(TM) Ultra 9 275HX，24 个逻辑处理器 |
| 系统 | Windows 10 Pro，NT 10.0.26200.0 |
| 构建 | Release，Windows 本地构建 |
| 编译器 | Visual C++ / MSVC |
| 固定模拟 | 120 Hz |
| Benchmark 样本 | 5 次独立运行；每次先完成程序自身预热 |
| Smoke 样本 | 80x30、120x40、160x50 各 5 次，每次 600 帧 |
| 测量工作树 | 602c112 基线上的本轮 C02/C03 兼容性改动；最终提交号以本次交付回执为准 |

Benchmark 命令为：

```text
out\build\release\Release\writeover_bench.exe
out\build\release\Release\writeover_app.exe --smoke --data-dir data --frames 600 --width <W> --height <H>
```

benchmark 证据保存在 `docs/production/evidence/final_release_hardening_performance_20260918/`。该目录是工程证据，不是玩家运行时目录。

## 内部 benchmark 结果

下表保留程序输出的原始指标名。`worst_1pct_avg_ms` 表示“平均最慢 1% 帧时间”，不能改称 p99；这里的 P95/P99 是对 5 次运行结果的统计位置，样本很少，只用于展示波动范围，不是长时间帧延迟分布的替代品。

| Metric | Average | Median | Best | Worst | P95 | P99 | Equivalent FPS | Meaning |
|---|---:|---:|---:|---:|---:|---:|---:|---|
| `PVS_RENDER_TIME_MS` (`worst_1pct_avg_ms`) | 1.082 ms | 1.083 ms | 0.979 ms | 1.267 ms | 1.267 ms | 1.267 ms | 924 FPS | 内部 Character-Art / PVS 渲染时间 |
| `PVS_TOTAL_FRAME_TIME_MS` (`worst_1pct_avg_ms`) | 1.435 ms | 1.394 ms | 1.353 ms | 1.557 ms | 1.557 ms | 1.557 ms | 697 FPS | benchmark 管线代理；平台终端写入不计入 |
| `TERMINAL_FULL_TIME_MS` | 0.226 ms | 0.215 ms | 0.205 ms | 0.274 ms | 0.274 ms | 0.274 ms | 4421 FPS | 完整帧编码器时间，不是终端显示时间 |
| `TERMINAL_DELTA_TIME_MS` | 0.098 ms | 0.094 ms | 0.092 ms | 0.113 ms | 0.113 ms | 0.113 ms | 10163 FPS | 小范围变化的增量编码时间 |
| `TERMINAL_WORSTCASE_TIME_MS` | 0.589 ms | 0.266 ms | 0.219 ms | 1.937 ms | 1.937 ms | 1.937 ms | 1699 FPS | 编码器最坏路径；第 1 次有明显离群值 |

等效 FPS 使用 `1000 / 平均毫秒数` 计算，并且只描述对应计时边界内的 CPU 吞吐。

## 实际程序 smoke 路径

对 `writeover_app --smoke` 的 600 帧运行，标准输出重定向到空设备，以隔离测试日志噪声。三个终端尺寸的固定管线结果相近：

| 尺寸 | 5 次运行的固定管线 FPS 约值 | 解释 |
|---|---:|---|
| 80x30 | 112.9–113.7 | 约 113.5 FPS，非可见窗口刷新率 |
| 120x40 | 113.1–113.9 | 约 113.6 FPS，非可见窗口刷新率 |
| 160x50 | 113.1–113.8 | 约 113.5 FPS，非可见窗口刷新率 |

这组结果更适合标为 `TERMINAL_SUBMIT_FPS` 的近似值，实际测量边界是固定步长程序路径加上重定向输出环境；它没有读取 Windows Terminal 的真实绘制完成时刻。

## 指标边界

```text
UNLIMITED_CORE_RENDER_FPS = approximately 924 FPS equivalent
UNLIMITED_PIPELINE_FPS = approximately 697 FPS equivalent
TERMINAL_SUBMIT_FPS = approximately 113.5 FPS headless fixed-pipeline proxy
VISIBLE_TERMINAL_FPS = NOT_DIRECTLY_MEASURABLE in this setup
SIMULATION_RATE = 120 Hz fixed
```

这些数字不表示：

- 120 Hz 模拟就是 120 FPS 的显示结果；
- 内部渲染吞吐就是玩家在终端窗口中看到的帧率；
- 未变化帧的增量编码成本等于每一帧都有大范围变化时的成本；
- 重定向输出的 600 帧运行等于 Windows Terminal 的可见刷新；
- 5 次样本足以描述所有机器、字体、终端宿主或窗口尺寸的长期尾延迟。

Windows Terminal 的可见刷新还会受到终端宿主、字体、窗口尺寸、输出缓冲和系统调度影响。当前项目没有一个能直接读取“字符已经在终端窗口完成绘制”的跨宿主计时接口，因此这里不虚构 `VISIBLE_TERMINAL_FPS`。

## 工程判断

本轮没有修改 120 Hz 模拟语义，也没有为了得到更大的 FPS 数字而永久关闭产品呈现节拍。现有结果支持把 CPU 渲染和帧生成视为当前主要余量，但不能据此承诺所有终端环境都能稳定显示同样的刷新率。后续若要优化，应优先在目标 Windows Terminal、课程演示字体和实际窗口尺寸下做人工观感与录屏对照，而不是继续放大内部 benchmark 数字。
