#include "pch.h"

#include <stdexcept>

#include "app.h"
#include "data.h"
#include "graph.h"
#include "node.h"

namespace Mirael
{

PinId Node::getNextPinId() { return static_cast<PinId>(graph->getNextElementId()); }

void Node::init(Graph &owner, std::string_view nodeTypeName)
{
    if (initialized)
        throw std::runtime_error("Node already initialized");
    initialized = true;

    graph    = &owner;
    this->id = static_cast<NodeId>(graph->getNextElementId());
    typeName = nodeTypeName;

    onInit();
}

void Node::show() { onShow(); }

void Node::serialize(nlohmann::json &j) const { j["type"] = typeName; }

std::unique_ptr<Node> Node::deserialize(Graph &owner, const nlohmann::json &j)
{
    auto typeName = j["type"].get<std::string>();
    auto node     = App::get().nodeTypes().createNode(typeName);
    node->init(owner, typeName);
    return node;
}

}; // namespace Mirael