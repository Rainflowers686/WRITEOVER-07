#pragma once
// Small platform audio seam for PVS-01.  Audio is presentation-only: it is
// never part of deterministic simulation state or save bytes.  A platform may
// honestly expose subtitles-only mode when no mixer is available.

#include "writeover/common/ids.h"

#include <cstdint>
#include <memory>

namespace writeover {

class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual bool Init(int mix_rate = 44100) = 0;
    virtual void Shutdown() = 0;
    virtual bool VoiceAvailable() const = 0;
    virtual bool SubtitlesOnly() const = 0;
    virtual void PlaySfx(AudioId id, float volume = 1.0f) = 0;
    virtual void PlayVo(AudioId id, float volume, uint32_t tag) = 0;
    virtual void CancelVoTag(uint32_t tag) = 0;
    virtual void SetVolume(float master, float sfx, float vo) = 0;
    virtual const char* Name() const = 0;
};

// The platform implementation is selected at link time.  It may be a real
// mixer (Windows) or a fail-soft subtitles-only backend (POSIX in this
// repository); the capability flags are truthful in both cases.
std::unique_ptr<IAudioBackend> CreateAudioBackend();

} // namespace writeover
