#include "pch.h"

#include <algorithm>
#include <format>
#include <string_view>

#include "all_node_types.h"
#include "NodeTypeRegistry.h"

namespace Mirael
{

NodeTypeRegistry::NodeTypeRegistry()
{
    auto registrar = [this]<typename... Ts>() { (registerType<Ts>(), ...); };

    NodeTypes::registerAll(registrar);

    namesOrdered_.reserve(entries_.size());
    for (auto &[name, _] : entries_)
        namesOrdered_.push_back(name.c_str());
    std::sort(namesOrdered_.begin(), namesOrdered_.end(), [](const char *a, const char *b) { return std::strcmp(a, b) < 0; });
}

std::unique_ptr<Node> NodeTypeRegistry::createNode(std::string_view typeName) const
{
    auto it = entries_.find(typeName);

    if (it == entries_.end()) {
        throw std::runtime_error(std::format("Attempted to create Node of unknown type: {}", typeName));
    }

    return it->second.factory();
}

}; // namespace Mirael