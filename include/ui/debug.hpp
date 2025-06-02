#pragma once

#include <string>

#include <windows.h>
#include <ostream>

inline void trace(const std::string& msg) {
	OutputDebugStringA(("TRACE: " + msg + '\n').c_str());
}
