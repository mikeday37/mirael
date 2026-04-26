#pragma once

#include "node.h"

#include <concepts>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace Mirael
{

template <typename T>
concept NodeType = std::derived_from<T, Node> && requires {
    T();
    { T::typeName() } -> std::convertible_to<const char *>;
};

class NodeTypeRegistry
{
public:
    NodeTypeRegistry();

    struct Entry {
        const char *name;
        std::function<std::unique_ptr<Node>()> factory;
    };

    std::span<const Entry> all() const { return entries; }

private:
    std::vector<Entry> entries;

    template <NodeType T> void registerType()
    {
        entries.push_back({.name = T::typeName(), .factory = []() { return std::make_unique<T>(); }});
    }
};

}; // namespace Mirael