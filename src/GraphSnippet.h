#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "data.h"

namespace Mirael
{

/// <summary>
/// Represents a cut or copied portion of the content of a Graph, suitable for pasting
/// in the same or another Graph.
/// </summary>
struct GraphSnippet {
    GraphId sourceGraphId{};
    ImVec2 center;

    struct PinInfo {
        std::string key;
        PinConfig config{};
    };

    struct NodeInfo {
        std::string type;
        std::unordered_map<PinId, PinInfo> pins;
        ImVec2 pos;
        nlohmann::json config;
    };

    std::unordered_map<NodeId, NodeInfo> nodes;

    std::unordered_set<NodeId> incomingNodes; // only a.node in links [a,b] where a is outside the snippet and b is inside

    std::unordered_map<LinkId, Link> links;
};

}; // namespace Mirael
