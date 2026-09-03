// Audio: platform stub during the foundation stage.
// Real XAudio2 support is a release-gate task; the wiring contract (an
// IAudioBackend in platform <=> Narrative VoQueue in M6) is fixed in the
// engineering documents, but the backend is intentionally absent so the
// foundation never fakes audio capability (VoiceAvailable=false by default).

#include "src/platform/windows/platform_api.h"

namespace writeover {

const char* PlatformAudioBackendName() { return "stub-none"; }

} // namespace writeover