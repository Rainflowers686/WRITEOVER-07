# 18_AUDIO_VO_CONTRACT

## Scope

- Pre-generated offline VO (narrator ≤35 lines, key NPCs ≤12 lines)
- 100% subtitle fallback (VO can be fully disabled without affecting gameplay)
- SFX: procedural for UI/menu, licensed/button-recorded for weapons/environment
- Determinism: audio is never part of world determinism; playback is best-effort

## Audio Assets

```
data/
├── vo/
│   ├── narrator_guide_01.wav
│   ├── narrator_director_01.wav
│   └── narrator_corrupted_01.wav
├── sfx/
│   ├── pistol_fire.wav
│   ├── smg_fire.wav
│   ├── stun_zap.wav
│   ├── door_open.wav
│   ├── footstep.wav
│   └── ui_click.wav
└── ASSET_PROVENANCE.csv
```

## ASSET_PROVENANCE.csv

```csv
file,source,license,checked_by,date
sfx/pistol_fire.wav,CC0 freesound.org 124564,CC0,B,2026-09-08
vo/narrator_guide_01.wav,original recording by F,original,F,2026-09-10
```

Provenance is mandatory before any audio file can be committed.

## AudioBackend Interface (Platform Layer)

```cpp
// Lives in platform/windows (outer edge). Audio never blocks the sim.
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;
    virtual bool Init(int mixRate = 44100) = 0;
    virtual void Shutdown() = 0;
    virtual bool VoiceAvailable() const = 0;     // has audio device
    virtual bool SubtitlesOnly() const { return !VoiceAvailable(); }
    virtual void PlaySfx(AudioId id, float volume) = 0;     // fire & forget
    virtual void PlayVo(AudioId id, float volume, uint32_t tag) = 0;
    virtual void CancelVoTag(uint32_t tag) = 0;             // stale VO cancel
    virtual void SetVolume(float master, float sfx, float vo) = 0;
    virtual const char* Name() const = 0;         // "XAudio2" / "None"
};

std::unique_ptr<IAudioBackend> CreateAudioBackend();
// Falls back to a null backend (subtitles only) when no device.
```

## VO Queue Semantics (Narrative side)

```cpp
struct VoRequest {
    AudioId clip;
    uint32_t priority;      // higher first
    uint32_t tag;           // stale cancelling group
    uint32_t maxAgeFrames;  // expire after N frames if not started
};

class VoQueue {
public:
    void Push(const VoRequest& r);
    void Update(uint32_t frame);
    Optional<uint32_t> NextReady();   // returns tag of clip to start
    void CancelTag(uint32_t tag);
    void Clear();
};
```

Rules:
- One VO plays at a time. New higher-priority VO interrupts current (cancels with fade).
- Stale VO (age > maxAge) is dropped — never queued behind the player's back forever.
- Subtitles always display even when VO plays.
- If no device: VO queue drains silently; subtitles remain.

## SFX Budget

| SFX | Trigger | Volume |
|-----|---------|--------|
| pistol_fire | EventWeaponFire (pistol) | 0.8 |
| smg_fire | EventWeaponFire (smg) | 0.9 |
| stun_zap | stunner fire | 0.6 |
| door_open | door event | 0.5 |
| ui_click | menu navigation | 0.4 |
| footstep | movement step | 0.2 |

SFX playback is triggered by narrative/module code via events or direct calls through the platform audio interface — platform layer only.

## Voice Gate (9/10, from Opus)

PASS: ≥28 narrator VO + ≥8 NPC VO usable, clarity ≥7/10
FAIL: disable VO system entirely → 100% subtitles, keep DSP persona labels (Guide/Director/Corrupted) visible in subtitles.

Implementation: `WO_FEATURE_VO` compile flag + runtime `VoiceAvailable()`. Switching requires only a config change, no contract change.

## 报告友好

**STL**: std::unique_ptr, std::deque for queue.
**Design Pattern**: Null Object (NoAudioBackend), Decorator? no.
**Core Algorithm**: Priority queue with timeout.
**Course Note**: VO 是可关闭的——关闭后 100% 字幕兜底，游戏逻辑完全不受影响（R0 已设计该通路）。
---

## REPAIR PASS v2 — CLOSURE NOTE

> 本文档在本轮（FOUNDATION_REPAIR_AND_EVIDENCE_CLOSURE）复查通过：所有涉及公共
> 类型的表述以 epo_seed/include/writeover/** 的冻结头文件为准（强 ID、typed
> variant payload、composition-root 依赖方向、启发式终端探测、无时间戳存档、
> JSON→编译产物管线等均已落实到代码与测试）。若本文与头文件不一致，以头文件 +
> 对应单元测试为准；变更走 ADR（docs/adr/）。
