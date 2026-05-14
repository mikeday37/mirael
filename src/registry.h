#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>

#include "node.h"
#include "util.h"

namespace Mirael
{

class Graph;

template <typename T>
concept NodeType = std::derived_from<T, Node> && requires {
    T();
    { T::typeName() } -> std::convertible_to<std::string>;
};

class NodeTypeRegistry
{
public:
    NodeTypeRegistry();

    const std::vector<const char *> &names() const { return namesOrdered_; }

    std::unique_ptr<Node> createNode(std::string_view typeName) const;

private:
    struct Entry {
        const char *name = nullptr;
        std::function<std::unique_ptr<Node>()> factory;
    };

    std::unordered_map<std::string, Entry, TransparentStringHash, std::equal_to<>> entries_;
    std::vector<const char *> namesOrdered_;

    template <NodeType T> void registerType()
    {
        auto name      = T::typeName();
        entries_[name] = {.name = name, .factory = []() { return std::make_unique<T>(); }};
    }
};

}; // namespace Mirael