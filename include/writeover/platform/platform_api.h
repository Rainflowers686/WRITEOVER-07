#pragma once
// Platform edge hooks used by the composition root.  Semantic modules only
// see the portable interfaces; platform implementations live under src.

#include "writeover/common/io.h"

namespace writeover {

// Installs the platform's atomic replacement provider.  On platforms where
// the standard filesystem rename is already the available primitive this is
// intentionally a no-op.
void InstallPlatformAtomicReplace();

// Returns the backend actually selected by the platform layer.
const char* PlatformAudioBackendName();

} // namespace writeover
