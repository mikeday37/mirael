#pragma once

#include <windows.h>
#include <string>
#include <ostream>

inline void trace(const std::string& msg) {
	OutputDebugStringA(("TRACE: " + msg + '\n').c_str());
}
