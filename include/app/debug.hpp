#pragma once

#include <ostream>
#include <string>
#include <windows.h>

inline void trace(const std::string &msg) { OutputDebugStringA(("TRACE: " + msg + '\n').c_str()); }
