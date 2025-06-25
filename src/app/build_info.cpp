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

#define TOSTRING_INNER(x) #x
#define TOSTRING(x) TOSTRING_INNER(x)

#if defined(__clang__)
#define COMPILER_NAME "Clang"
#define COMPILER_VERSION TOSTRING(__clang_major__) "." TOSTRING(__clang_minor__) "." TOSTRING(__clang_patchlevel__)
#elif defined(__GNUC__)
#define COMPILER_NAME "GCC"
#define COMPILER_VERSION TOSTRING(__GNUC__) "." TOSTRING(__GNUC_MINOR__)
#elif defined(_MSC_VER)
#define COMPILER_NAME "MSVC"
#define COMPILER_VERSION TOSTRING(_MSC_VER)
#else
#define COMPILER_NAME "Unknown"
#define COMPILER_VERSION "n/a"
#endif

#if defined(_WIN32)
#define OPERATING_SYSTEM_NAME "Windows"
#elif defined(__linux__)
#define OPERATING_SYSTEM_NAME "Linux"
#elif defined(__APPLE__)
#define OPERATING_SYSTEM_NAME return "macOS"
#else
#define OPERATING_SYSTEM_NAME return "unknown"
#endif

std::vector<const char *> GetBuildInfo()
{
    return {"CMake Configured: " MIRAEL_BUILD_CONFIGURED_TIMESTAMP,
            "Build Preset:     " MIRAEL_BUILD_PRESET,
            "Build Type:       " MIRAEL_BUILD_TYPE,
            "NDEBUG Defined:   " MIRAEL_BUILD_NDEBUG_DEFINED,
            "Compiler:         " COMPILER_NAME,
            "Compiler Version: " COMPILER_VERSION,
            "Built for OS:     " OPERATING_SYSTEM_NAME};
}