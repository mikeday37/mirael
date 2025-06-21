#include "app_pch.hpp"

#include "app/build_info.hpp"

#ifndef MIRAEL_BUILD_CONFIGURED_TIMESTAMP
#define MIRAEL_BUILD_CONFIGURED_TIMESTAMP "unknown"
#endif
#ifndef MIRAEL_BUILD_PRESET
#define MIRAEL_BUILD_PRESET "unknown"
#endif
#ifndef MIRAEL_BUILD_TYPE
#define MIRAEL_BUILD_TYPE "unknown"
#endif

#ifdef NDEBUG
#define MIRAEL_BUILD_NDEBUG_DEFINED "true"
#else
#define MIRAEL_BUILD_NDEBUG_DEFINED "false"
#endif

std::vector<const char *> GetBuildInfo()
{
    return {
        "CMake Configured: " MIRAEL_BUILD_CONFIGURED_TIMESTAMP,
        "Build Preset:     " MIRAEL_BUILD_PRESET,
        "Build Type:       " MIRAEL_BUILD_TYPE,
        "NDEBUG Defined:   " MIRAEL_BUILD_NDEBUG_DEFINED,
    };
}