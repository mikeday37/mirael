#pragma once

namespace Mirael
{

using GraphId        = uint64_t;
using GraphElementId = uint64_t;
using NodeId         = GraphElementId;
using LinkId         = GraphElementId;
using PinId          = GraphElementId;

struct PinRef {
    NodeId node;
    PinId pin;
};

struct Link {
    PinRef a, b; // a -> b, usually, though in the future we may support undirected or bidirectional links
};

}; // namespace Mirael
