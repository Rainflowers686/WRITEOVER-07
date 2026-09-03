# 19_SETTINGS_CONFIG_CONTRACT (REPAIR PASS v2)

> 修订（M-013 closure）：`keyBindings` 从无效的
> `uint8_t keyBindings; // (pointer to InputMapper)` 改为
> **可序列化绑定表** `std::array<PhysicalKey, kGameActionCount>`（放 Settings 内，
> 与 InputMapper 同源默认值）。实现：`core/settings.h/.cpp`。

## Settings（14 主项 + 3 无障碍 + dev 页由 DebugMetrics 承载）

```cpp
struct Settings {
    QualityPreset preset; uint8_t frame_rate_cap, fov, mouse_sensitivity,
                  gamepad_sensitivity;
    std::array<PhysicalKey, kGameActionCount> key_bindings;  // ← 数据，不是指针
    bool aim_assist; uint8_t master_volume, narrator_volume, sfx_volume;
    uint8_t difficulty; bool interaction_highlight, tactical_focus, subtitles;
    bool reduce_camera_shake, reduce_flicker, high_contrast;
    static Settings Defaults();      // 绑定表默认值唯一来源
    void Save(Serializer&) const; void Load(Deserializer&);
};
```

- 序列化（gameplay 节/存档）：16 标量 + 绑定表（逐 PhysicalKey u16）。
- 文本持久化 `settings.cfg`：`key=value`；未知键忽略（向前兼容）、坏值回退默认。

## 规则（冻结）

- 设置**不改变确定性**：difficulty 调伤害/生成数但走同一代码路径；
  固定 settings 快照下 same seed+input → same state。
- 加载损坏 settings → 失败即回退默认 + 日志（`Result`），不崩溃。
- 绑定表独立于玩法逻辑：换键只改表，不改 InputMapper 映射语义。

## 测试锚点

`settings.encode_decode` / `settings.key_value_disk`（真实磁盘 round-trip）。

## 报告友好

**Course Note**: 14 项设置都是 struct 字段，读写持久化一套代码，
评审问"键位重绑存哪"答案可指 settings.cfg + 绑定表序列化。