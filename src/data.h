#pragma once

namespace Mirael
{

using GraphId        = uint64_t;
using GraphElementId = uint64_t;
using NodeId         = GraphElementId;
using LinkId         = GraphElementId;
using PinId          = GraphElementId;

enum class PinDirection {
    Unknown, // value only used during deserialization, before DerivedNode::onInit()
    Input,
    Output
};

struct PinConfig {
    PinDirection direction;
    bool operator==(const PinConfig &) const = default;
};

struct PinRef {
    NodeId node;
    PinId pin;
};

struct Link {
    PinRef a, b; // a -> b, usually, though in the future we may support undirected or bidirectional links
};

enum class ChangeImpact {
    GraphName,
    GraphVisibility,
    GraphPanZoom,
    AddNode,
    RemoveNode,
    AddLink,
    RemoveLink,
    NodePosition,
    NodeConfig,
    GraphRunRate,
};

}; // namespace Mirael
