# 12_TERMINAL_RENDER_CONTRACT (REPAIR PASS v2)

> 修订（M-014 closure）：终端不支持 SGR 回执——能力探测是**启发式**
> （WT_SESSION/ConEmuANSI/TERM 环境标记），不是"发色序列再检测接受"；
> 字符宽高比默认 0.5 + 首启人工校准（不承诺自动测量）；崩溃恢复措辞修正：
> 保证正常退出恢复，异常崩溃恢复为 best-effort（SEH 属 release gate）。
> 实现：`render/terminal_backend.h/.cpp`、`render/utf.cpp`、
> `src/platform/windows/win_terminal.cpp`（ANSI TrueColor 为默认活动路径；
> Win32 WriteConsoleOutputW 已实现为回退，M2 施工再精化）。

## CharCell / glyph 政策

```cpp
struct CharCell { char32_t code_point; uint8_t fg_r/g/b, bg_r/g/b; uint8_t flags; };
```

- 内部字形 = code point（char32_t，无代理对问题）。
- 3D 层只允许单宽字形：ASCII U+0020–U+007E、块元素 U+2580–U+259F；
  CJK（U+2E80–U+9FFF 等）**禁止进 3D 层**（`IsSingleWidthGlyph` 测试锚点）。
- CJK 只出现在 HUD/字幕/菜单合成层；UTF-8 内部存储，边界转 UTF-16。

## 能力探测（启发式，无假"真彩 ack"）

`ProbeTerminalEnv()`：
- WT_SESSION → windows-terminal；ConEmuANSI → conemu；TERM 含 xterm/vt → vt。
- 输出 `TerminalProbe`（is_windows_terminal/is_conemu/vt_enabled/w/h）；
- `SuggestPreset(probe, refresh_hint)` → ULTRA120/HIGH_REFRESH/PRESENTATION_60/COMPATIBILITY。

## Backend 接口与工厂

```cpp
class ITerminalBackend { Init/Shutdown/Submit(Restore)/GetCaps/Name; };
std::unique_ptr<ITerminalBackend> CreateTerminalBackend(w, h, probe);
// vt_enabled → ANSI TrueColor；否则 Win32 WriteConsoleOutputW 回退
```

- ANSI 后端：整帧构建（home + 逐 cell SGR + 一次 fwrite+flush），**无逐字符 cout**。
- Submit 是全量刷新，不滚屏（帧内无 `\n` 之外的滚动副作用在渲染层可控）。

## 终端恢复

- `ConsoleGuard`（RAII，src/platform/windows/win_console.cpp）：保存 console mode，
  析构/显式 Restore 恢复 mode + 光标可见。
- 正常退出恢复 = **保证**；`atexit` 兜底（构造函数注册）。
- 崩溃（SEH dump/restore）属 release-gate 清单项——文档不谎称已 crash-safe。
- 禁止 `system("cls")`；退出统一走 `Restore()`。

## 预置（与 P0 冻结一致）

| Preset | CharGrid | Sim | Render | Present |
|--------|----------|-----|--------|---------|
| ULTRA120 | 240×67 | 120Hz | ≥120fps | ≥120Hz |
| HIGH_REFRESH | 192×54 | 120Hz | ≥120fps | ≥120Hz |
| PRESENTATION_60 | 200×60 | 120Hz | ≥90fps | ≥60Hz |
| COMPATIBILITY | 160×45 | 120Hz | ≥60fps | ≥60Hz |

## 测试锚点

`utf.ascii_single_width` / `utf.cjk_blocked_in_3d`（3D 层拒 CJK）。

## 报告友好

**Design Pattern**: Strategy（ANSI vs Win32 backend）。
**Course Note**: 终端恢复（RAII+atexit+人工校准）是演示"退出后终端正常"的保障，
但 SEH 崩溃恢复要到 release gate 才算完成——文档不提前邀功。