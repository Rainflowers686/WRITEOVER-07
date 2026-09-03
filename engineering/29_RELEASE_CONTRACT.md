# 29_RELEASE_CONTRACT (REPAIR PASS v2)

> 修订：能力探测措辞对齐 12 文档（启发式，无假 ack）；崩溃恢复表述修正为
> "正常退出=保证 + atexit 兜底；SEH 崩溃恢复=release-gate 清单项（未完成前
> 文档不宣称 crash-safe）"。

## 目标

- 双击即玩的单 exe；无安装、无管理员、无网络。
- Release `/MT` 静态运行时（preset 已实现，release build exit 0）。
- 未知机器自动降级：probe → SuggestPreset → 用户可覆写。

## Release 目录（scripts/package.ps1 生成）

```
WRITEOVER-07/  WRITEOVER-07.exe  README.txt  data/  saves/  logs/  ASSET_PROVENANCE.csv
+ SHA256SUMS.txt
```

## 启动探测（与 12 文档同源）

1. Terminal 类型（env 启发式） 2. VT/TrueColor（环境标记，非 ack）
3. 字体宽高比（默认 0.5 + 首启人工校准） 4. 刷新率提示
5. 音频设备（stub-none → 纯字幕） 6. RawInput 可用性（回退 cursor-delta）

## 安全默认（冻结）

Preset=COMPATIBILITY / Volume 70% / Subtitles ON / Mouse 50% / Difficulty normal。

## 终端恢复（准确表述）

- ConsoleGuard RAII：**正常退出恢复 = 保证**；`atexit` 兜底已注册。
- 崩溃（SEH dump + 恢复）：release-gate 清单项，当前状态 NOT_READY（诚实）。
- 回退 exe：上一版保留 `WRITEOVER-07-ROLLBACK.exe`；干净机器失败即切回。

## Clean-machine checklist（release gate）

双击可跑 / 中文与空格路径 / WT+conhost 双端 / 无音频字幕完整 / 60Hz 预设 /
Alt+Tab 恢复 / 退出终端正常 / README 在场 / 录屏备份 / SHA256 记录。
（本机验证子集：smoke exit 0 + 存档落盘；≥3 台未知机器属 9/16 gate，NOT_RUN。）

## 打包

```powershell
pwsh -File scripts/package.ps1     # release build → dist/ zip + SHA256
```

## 报告友好

**Course Note**: 静态链接 + 启发式降级 = 评审机器直接双击 exe 也能玩。