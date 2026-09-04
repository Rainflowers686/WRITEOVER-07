// Windows presentation audio.  Clips are generated as short PCM cues at
// runtime, so the PVS has an actual audible path without adding a middleware
// dependency or pretending that an unavailable device is usable.

#include "writeover/audio/audio_backend.h"
#include "writeover/platform/platform_api.h"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <vector>

namespace writeover {

namespace {

constexpr double kPi = 3.14159265358979323846;

void PutU16(std::vector<uint8_t>& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFu));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFu));
}

void PutU32(std::vector<uint8_t>& out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
    }
}

void PutTag(std::vector<uint8_t>& out, const char (&tag)[5]) {
    out.insert(out.end(), tag, tag + 4);
}

std::vector<uint8_t> MakeClip(AudioId id, int sample_rate, float volume) {
    const uint64_t key = id.GetValue();
    const float duration = key == 4 ? 0.34f : (key == 8 ? 0.28f :
                           (key == 5 ? 0.22f : (key == 2 ? 0.16f : 0.09f)));
    const double base_hz = key == 1 ? 420.0 : (key == 2 ? 150.0 :
                          (key == 3 ? 720.0 : (key == 4 ? 90.0 :
                          (key == 5 ? 260.0 : (key == 6 ? 180.0 :
                          (key == 7 ? 980.0 : 55.0))))));
    const size_t sample_count = static_cast<size_t>(
        std::max(1.0f, duration) * static_cast<float>(sample_rate));
    const float gain = std::clamp(volume, 0.0f, 1.0f) * 0.42f;

    std::vector<uint8_t> wav;
    wav.reserve(44 + sample_count * 2);
    PutTag(wav, "RIFF");
    PutU32(wav, static_cast<uint32_t>(36 + sample_count * 2));
    PutTag(wav, "WAVE");
    PutTag(wav, "fmt ");
    PutU32(wav, 16);
    PutU16(wav, 1);
    PutU16(wav, 1);
    PutU32(wav, static_cast<uint32_t>(sample_rate));
    PutU32(wav, static_cast<uint32_t>(sample_rate * 2));
    PutU16(wav, 2);
    PutU16(wav, 16);
    PutTag(wav, "data");
    PutU32(wav, static_cast<uint32_t>(sample_count * 2));

    uint32_t noise = static_cast<uint32_t>(key * 0x9E3779B9u + 17u);
    for (size_t i = 0; i < sample_count; ++i) {
        const double t = static_cast<double>(i) / sample_rate;
        const float envelope = std::exp(-static_cast<float>(t) /
                                        std::max(0.025f, duration * 0.55f));
        const float tone = static_cast<float>(std::sin(2.0 * kPi * base_hz * t));
        noise ^= noise << 13;
        noise ^= noise >> 17;
        noise ^= noise << 5;
        const float hiss = static_cast<float>(static_cast<int32_t>(noise & 0xFFFFu)) /
                           32768.0f;
        const float mix = key == 8 ? (tone * 0.35f + hiss * 0.65f) :
                          (key == 4 ? tone * 0.65f + hiss * 0.35f : tone);
        const int sample = static_cast<int>(std::clamp(
            mix * envelope * gain, -1.0f, 1.0f) * 32767.0f);
        PutU16(wav, static_cast<uint16_t>(static_cast<int16_t>(sample)));
    }
    return wav;
}

} // namespace

class WinMmAudioBackend final : public IAudioBackend {
public:
    bool Init(int mix_rate) override {
        mix_rate_ = std::clamp(mix_rate, 8000, 96000);
        initialized_ = true;
        return true;
    }

    void Shutdown() override {
        PlaySoundA(nullptr, nullptr, 0);
        clips_.clear();
        active_vo_tag_ = 0;
        initialized_ = false;
    }

    bool VoiceAvailable() const override {
        return initialized_ && waveOutGetNumDevs() > 0;
    }

    bool SubtitlesOnly() const override { return !VoiceAvailable(); }

    void PlaySfx(AudioId id, float volume) override {
        PlayClip(id, volume * master_volume_ * sfx_volume_);
    }

    void PlayVo(AudioId id, float volume, uint32_t tag) override {
        active_vo_tag_ = tag;
        PlayClip(id, volume * master_volume_ * vo_volume_);
    }

    void CancelVoTag(uint32_t tag) override {
        if (tag != 0 && tag == active_vo_tag_) {
            PlaySoundA(nullptr, nullptr, 0);
            active_vo_tag_ = 0;
        }
    }

    void SetVolume(float master, float sfx, float vo) override {
        master_volume_ = std::clamp(master, 0.0f, 1.0f);
        sfx_volume_ = std::clamp(sfx, 0.0f, 1.0f);
        vo_volume_ = std::clamp(vo, 0.0f, 1.0f);
    }

    const char* Name() const override {
        return VoiceAvailable() ? "winmm-procedural" : "winmm-no-device";
    }

private:
    void PlayClip(AudioId id, float volume) {
        if (!initialized_ || !VoiceAvailable()) return;
        // PlaySound requires the memory to remain valid until asynchronous
        // playback completes.  Retain a small bounded clip ring; on overflow
        // stop the old sound before releasing its backing memory.
        if (clips_.size() >= 32) {
            PlaySoundA(nullptr, nullptr, 0);
            clips_.clear();
        }
        clips_.push_back(MakeClip(id, mix_rate_, volume));
        (void)PlaySoundA(reinterpret_cast<LPCSTR>(clips_.back().data()),
                         nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    }

    bool initialized_ = false;
    int mix_rate_ = 44100;
    float master_volume_ = 1.0f;
    float sfx_volume_ = 1.0f;
    float vo_volume_ = 1.0f;
    uint32_t active_vo_tag_ = 0;
    std::deque<std::vector<uint8_t>> clips_;
};

std::unique_ptr<IAudioBackend> CreateAudioBackend() {
    return std::make_unique<WinMmAudioBackend>();
}

const char* PlatformAudioBackendName() { return "winmm-procedural"; }

} // namespace writeover

#endif // _WIN32
