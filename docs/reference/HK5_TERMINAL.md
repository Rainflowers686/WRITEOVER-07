# HK-5 Terminal Presenter / Encoder

## 目的

将终端呈现后端加固为确定性能达标的参考实现。

## 所做变更

### 1. Per-cell SGR 修复 (F-24)

`win_terminal.cpp` 中每个 cell 原本输出完整 reset+fg+bg+glyph，每次帧产生数十万字节。

**修复：添加 color-run 编码器**
- 相邻相同颜色的 cell 合并为单个 SGR 序列 + 多个 glyph
- 同一列中仅颜色变化处输出新 SGR
- 全帧不变时跳过提交（fast path）

### 2. Fallback 调色板修复 (F-25)

原本只根据 `bg_r >= 128` 和 `fg_r >= 128` 设置单个 red bit。

**修复：完整 RGB→16 色量化映射**
- 使用标准 ANSI 16 色调色板
- 最近邻 RGB→ANSI 颜色映射
- 亮度优先的映射策略

### 3. 最后 fallback 未初始化 (F-26)

`CreateTerminalBackend` 中若所有 probe 失败，返回未初始化的 `AnsiTrueColorBackend`。

**修复：调用 `Init()` 后再返回**
- 所有 probe 失败时也调用 `Init()` 确保 `width_`/`height_` 初始化

### 4. 编码器

- **Full**: 完整帧重绘（所有 cell）
- **Run**: 等色 run-length 编码
- **Delta**: 仅变化 cell 的增量编码
- 当前使用 run 编码，Luna 优化为 delta

### 5. Benchmark 命名修复 (F-27)

- `p99_ms` 字段改名为 `worst_1pct_avg_ms` 以避免混淆
- 新增 `one_pct_low_fps` 计算字段

## 门控 (G5)

### 通过条件
- [x] full/run/delta 编码器存在
- [x] 实测字节数/帧可测量
- [x] 实测编码时间可测量
- [x] 16 色量化存在
- [x] 无变化帧 fast path
- [x] 最终路径中无逐 cell 冗余 SGR

### 阻塞项
- 无阻塞项

## 给 Luna
- M2 可以使用 run 编码器；Ultra120 交付前必须优化为 delta
- 基准测试数据见 `evidence/FABLE_HARDENING_VALIDATION.md`