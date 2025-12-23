#pragma once

#include "CoreMinimal.h"

#ifndef XR_LOG_DEFAULT_VERBOSITY
#if UE_BUILD_SHIPPING
#define XR_LOG_DEFAULT_VERBOSITY Display
#else
#define XR_LOG_DEFAULT_VERBOSITY Log
#endif
#endif

// NOLINTNEXTLINE
DECLARE_LOG_CATEGORY_EXTERN(LogEditorActorTagDisplay, XR_LOG_DEFAULT_VERBOSITY, All);
