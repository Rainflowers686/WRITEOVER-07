# SETTINGS_SCHEMA — settings.cfg（UTF-8 key=value）

与 `core/settings.h` 的 Settings struct + `SettingsRegistry` 一一对应。

## 文本格式

```ini
# WRITEOVER-07 settings
preset=3
frameratecap=0
fov=90
mousesensitivity=50
gamepadsensitivity=50
aimassist=false
mastervolume=70
narratorvolume=70
sfxvolume=70
difficulty=1
interactionhighlight=true
tacticalfocus=false
subtitles=true
reducecamerashake=false
reduceflicker=false
highcontrast=false
bind.0=0
bind.1=17
...
bind.23=0
```

- `bind.<GameActionIndex>=<PhysicalKeyValue>`：键位表（**数据**，无指针）。
- 未知键忽略（向前兼容）；非法值回退默认（逐键）。
- 范围校验：fov 60–120、音量/灵敏度 0–100、preset 0–3、difficulty 0–2。

## 序列化（存档 gameplay 节）

Settings::Save/Load：16 个 u8 + 绑定表 `kGameActionCount × u16`（小端）。

## 规则

- 设置不改变确定性（固定 settings 快照 + same seed+input → same state）。
- 默认值唯一来源 `Settings::Defaults()`（与 InputMapper::ResetToDefaults 同源）。
- 损坏文件 → fail 回退默认 + 日志（Result），不崩溃。

## 测试锚点

`settings.encode_decode` / `settings.key_value_disk`。