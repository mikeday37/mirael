#pragma once

#include <map>
#include <string>

namespace Mirael
{

//
// These types are used only for serialization.
//

using GraphId        = uint32_t;
using GraphElementId = uint32_t;
using NodeId         = GraphElementId;
using LinkId         = GraphElementId;
using PinId          = GraphElementId;

struct GraphData {
    std::string name;
    bool visible;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GraphData, name, visible);

struct ProjectData {
    std::unordered_map<GraphId, GraphData> graphs;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ProjectData, graphs);

}; // namespace Mirael
