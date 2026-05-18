#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include "Node.h"
#include "NodeTypeRegistry.h"

namespace Mirael
{

class Library
{
public:
    static const char *windowName() { return "Node Library"; }
    void show(bool &open);
};

} // namespace Mirael
