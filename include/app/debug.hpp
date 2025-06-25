#pragma once

#if defined(_WIN32)

#include <ostream>
#include <string>
#include <windows.h>

inline void trace(const std::string &msg) { OutputDebugStringA(("TRACE: " + msg + '\n').c_str()); }

#else

#define trace(x) ((void)0)

#endif
