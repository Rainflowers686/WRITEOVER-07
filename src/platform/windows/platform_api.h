#pragma once
// Platform-visible API surface that the composition root wires into the app.
// SEMANTIC MODULES MUST NOT INCLUDE THIS FILE (platform stays at the edge;
// the app/composition-root only).

#ifndef WO_PLATFORM_API_H
#define WO_PLATFORM_API_H

#include "writeover/common/io.h"

namespace writeover {

// Installs the MoveFileExW atomic-replace provider (called once at startup).
void InstallPlatformAtomicReplace();

// Audio is a platform stub during the foundation stage; always returns the
// backend name actually in use, "stub-none" until real XAudio is wired.
const char* PlatformAudioBackendName();

} // namespace writeover

#endif // WO_PLATFORM_API_H