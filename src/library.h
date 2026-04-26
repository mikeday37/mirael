#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "node.h"
#include "registry.h"

namespace Mirael
{

class Library
{
public:
    static const char *explorerWindowName() { return "Node Library"; }
    void showExplorer();
};

}; // namespace Mirael
