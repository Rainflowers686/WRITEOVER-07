#include "writeover/platform/platform_api.h"

namespace writeover {

void InstallPlatformAtomicReplace() {
    // common/io.cpp already uses the portable filesystem rename provider.
}

} // namespace writeover
