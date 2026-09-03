# ADR-0001: Height-Span Raycaster 2.5D column contract

- 状态: 接受
- 日期: 2026-09-02
- 决策者: A (core) + B (render) + Opus 复审
- 受影响模块: M2 (render), M4 (world data), M1 (integration)

## 背景
Opus S 节草图使用 3D 视线 (dir.z + eyeZ) 描述 raycast, 与 P0 的 1m×1m
Height-Aware Grid 不兼容, 且 16 区间容量上限与 50m 视距不匹配。

## 决策
每屏幕列只在 XY 平面做 DDA; 每个 cell 边界计算 opening:
- openingBottom = max(floorA, floorB)
- openingTop    = min(ceilA,  ceilB)
- openingTop <= openingBottom => full occlusion
- floor 升起 => 下边界墙 [floorA, floorB]
- ceiling 降低 => 上边界墙 [ceilB, ceilA]
相机 Z 与 pitch 只在投影阶段把世界 Z 区间映射到屏幕 Y。
容量: kMaxSegmentsPerRay = 128, 由 50 边界 × 2 段的最坏情形推导。

## 备选方案
传统等高 raycaster: 拒绝 (无法表达低墙/平台/沟槽窗口)。

## 影响
- 09_HEIGHT_SPAN_RAYCASTER_CONTRACT.md 重写;
- 黄金用例 (RayFlatHitsWall / low wall / full occlusion) 进 CTest。

## 回滚
切回按格中心采样的 3D 视线版本 (S 节草图), 需重开 R0 验证。