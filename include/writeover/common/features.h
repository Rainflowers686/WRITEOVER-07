#pragma once
// Common feature flags. Fallbacks change implementation/flags only,
// never public API. (Requirement 5.2, feature capability policy.)

#ifndef WO_FEATURE_VO
#define WO_FEATURE_VO 1
#endif
#ifndef WO_FEATURE_GOAP
#define WO_FEATURE_GOAP 1
#endif
#ifndef WO_FEATURE_PRONE
#define WO_FEATURE_PRONE 1
#endif
#ifndef WO_FEATURE_VAULT
#define WO_FEATURE_VAULT 1
#endif
#ifndef WO_FEATURE_MANTLE
#define WO_FEATURE_MANTLE 1
#endif
#ifndef WO_FEATURE_GAMEPAD
#define WO_FEATURE_GAMEPAD 1
#endif
// Present thread stays OFF until profiler proof (policy: profile, then parallelize).
#ifndef WO_FEATURE_PRESENT_THREAD
#define WO_FEATURE_PRESENT_THREAD 0
#endif