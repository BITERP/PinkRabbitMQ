#pragma once

#if defined(__ANDROID__)

#define MOBILE_PLATFORM_ANDROID 1

#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)

#define MOBILE_PLATFORM_IOS 1

#elif defined(WINAPI_FAMILY) && ((WINAPI_FAMILY == WINAPI_FAMILY_PC_APP) || (WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP))

#define MOBILE_PLATFORM_WINRT 1

#endif

#include "types.h"

#if defined(MOBILE_PLATFORM_IOS)

extern "C" VOID RegisterLibrary(LPCSTR, LPCVOID, LPCVOID);

#define DECLARE_DLL(name, fnTable) \
namespace { static struct s { s() { RegisterLibrary(name, NULL, fnTable); }} s; }

#endif //MOBILE_PLATFORM_IOS

