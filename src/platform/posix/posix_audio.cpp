// POSIX fail-soft audio boundary.  The game remains playable with subtitles
// when no portable mixer is bundled; the capability is reported honestly.

#include "writeover/audio/audio_backend.h"
#include "writeover/platform/platform_api.h"

namespace writeover {

class PosixSubtitleAudio final : public IAudioBackend {
public:
    bool Init(int) override { initialized_ = true; return true; }
    void Shutdown() override { initialized_ = false; }
    bool VoiceAvailable() const override { return false; }
    bool SubtitlesOnly() const override { return true; }
    void PlaySfx(AudioId, float) override {}
    void PlayVo(AudioId, float, uint32_t) override {}
    void CancelVoTag(uint32_t) override {}
    void SetVolume(float, float, float) override {}
    const char* Name() const override { return "posix-subtitles-only"; }

private:
    bool initialized_ = false;
};

std::unique_ptr<IAudioBackend> CreateAudioBackend() {
    return std::make_unique<PosixSubtitleAudio>();
}

const char* PlatformAudioBackendName() { return "posix-subtitles-only"; }

} // namespace writeover
