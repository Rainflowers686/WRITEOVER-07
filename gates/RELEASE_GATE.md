# RELEASE_GATE — 发布与未知机器验收（9/16–18）

入口：`scripts/test.ps1 -Gate Release`（+ 人工 checklist）。

## 可执行检查（当前已具备）

- [x] Release /MT 构建 exit 0
- [x] smoke（终端初始化/渲染/存档/恢复）exit 0
- [x] package.ps1 产出 dist + SHA256 机制
- [x] 终端恢复 = 正常退出保证（ConsoleGuard RAII + atexit）

## 人工 checklist（≥3 台干净机器，9/16–17）

- [ ] 双击 exe 可玩；中文/空格路径 OK
- [ ] Windows Terminal + conhost 双端 OK
- [ ] 无音频机 = 100% 字幕完整
- [ ] 60Hz 显示 → PRESENTATION_60；≥120Hz → ULTRA120 实测
- [ ] Alt+Tab / 失焦恢复（ClearInputState 防卡键）
- [ ] 崩溃恢复（SEH dump + 终端恢复）—— **release-gate 施工项**，当前 NOT_READY
- [ ] 回退 exe 就绪（WRITEOVER-07-ROLLBACK.exe）+ 录屏备份

## 失败处置（T 节 #27/#28）

性能不达标 → 锁 COMPATIBILITY；干净机器失败 → 立即切回退 exe。

## 演示与提交

9/18 课堂展示（3 个"哇"）+ 交换测试 + 课程报告定稿 + 测试报告 + ARM 编译说明。