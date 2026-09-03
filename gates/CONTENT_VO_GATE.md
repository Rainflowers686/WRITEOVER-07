# CONTENT_VO_GATE — 内容与 VO 冻结（9/10 18:00）

入口：`scripts/test.ps1 -Gate ContentVo`。

## VO 门槛（Opus）

- [ ] ≥28 条 narrator VO + ≥8 NPC VO 可用、清晰度 ≥7/10
- [ ] ASSET_PROVENANCE.csv 全量登记（来源/许可/checked_by）
- [ ] 100% 字幕路径存在（任何 VO 缺失都能完整叙事）
- FAIL 行动 = 关闭 VO（WO_FEATURE_VO=0 + VoiceAvailable=false → 纯字幕），
  DSP persona 标签保留在字幕层——**不改公共契约**（18 文档）

## 内容门槛（M4/M6）

- [ ] 6 Room + 3 micro-space 内容全量入库且 mapc exit 0
- [ ] storylet/fact/claim 引用无悬垂（编译期 fail-fast）
- [ ] 首次谎言 ≤ 首 60s、快速可反证（内容不变量由校验检查）
- [ ] 内容产物与作者 JSON 同步（CI --check）

## 可执行现状（foundation）

VO 后端 = stub-none（真实 XAudio 属 M2/M6 施工）→ 本 gate 内容项 NOT_READY；
字幕队列/过期/标签路径已有单测（dialog.* 全过）。

## 判定

VO 达标 → PASS；不达标 → 按 FAIL 行动执行纯字幕方案（结构性已支持）。