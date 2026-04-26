#include "pch.h"

#include <stdexcept>

#include "data.h"
#include "graph.h"
#include "node.h"

namespace Mirael
{

PinId Node::getNextPinId() { return static_cast<PinId>(graph->getNextElementId()); }

void Node::init(Graph &owner)
{
    if (initialized)
        throw std::runtime_error("Node already initialized");
    initialized = true;

    graph    = &owner;
    this->id = static_cast<NodeId>(graph->getNextElementId());

    onInit();
}

void Node::show() { onShow(); }

}; // namespace Mirael