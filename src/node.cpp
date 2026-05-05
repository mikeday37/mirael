#include "pch.h"

#include <stdexcept>

#include "app.h"
#include "data.h"
#include "graph.h"
#include "node.h"

using json = nlohmann::json;

namespace Mirael
{

PinId Node::getPinId(std::string_view pinKey)
{
    auto it = pinKeyToId.find(pinKey);
    if (it != pinKeyToId.end()) {
        return it->second;
    } else {
        if (deserializing)
            throw std::runtime_error(std::format(
                "Node (id={}, type={}) attempted to access an unknown pin (key={}) during deserialization.", id, typeName, pinKey));
        PinId newId = static_cast<PinId>(graph->getNextElementId());
        pinKeyToId.emplace(std::string(pinKey), newId);
        return newId;
    }
}

GraphElementId Node::getMaxElementId() const
{
    if (pinKeyToId.empty())
        return id;
    else
        return std::max(id, std::ranges::max(pinKeyToId | std::views::values));
}

void Node::raiseModified(ChangeImpact impact)
{
    graph->raiseModified(impact);
}

void Node::setPos(ImVec2 newPos)
{
    pendingSetPos = newPos;
    raiseModified(ChangeImpact::NodePosition);
}

void Node::init(Graph &owner, NodeId id, std::string_view nodeTypeName)
{
    if (initialized)
        throw std::runtime_error("Node already initialized");
    initialized = true;

    graph    = &owner;
    this->id = id;
    typeName = nodeTypeName;

    onInit();
}

void Node::show() { onShow(); }

void Node::serialize(nlohmann::json &j) const
{
    j["type"]     = typeName;

    json pinsJson = json::object();
    for (const auto &[key, id] : pinKeyToId)
        pinsJson[key] = id;
    j["pins"]       = pinsJson;

    j["x"] = pos.x;
    j["y"] = pos.y;

    json configJson = json::object();
    onSerialize(configJson);
    if (!configJson.empty())
        j["config"] = configJson;
}

std::unique_ptr<Node> Node::deserialize(Graph &owner, NodeId id, const nlohmann::json &j)
{
    auto typeName       = j["type"].get<std::string>();
    auto node           = App::get().nodeTypes().createNode(typeName);

    node->deserializing = true;

    for (const auto &[key, value] : j["pins"].items())
        node->pinKeyToId[key] = value.get<uint64_t>();
    auto pinCountDeserialized = node->pinKeyToId.size();

    node->pos.x = j["x"].get<float>();
    node->pos.y = j["y"].get<float>();

    node->pendingSetPos = node->pos;

    if (j.contains("config"))
        node->onDeserialize(j["config"]);
    node->init(owner, id, typeName);

    auto pinCountInitialized = node->pinKeyToId.size();
    if (pinCountDeserialized != pinCountInitialized)
        throw std::runtime_error(
            std::format("Node (id={}, type={}) deserialized with pin count mismatch (deserialized={}, initialized={}).", id,
                        typeName.c_str(), pinCountDeserialized, pinCountInitialized));

    node->deserializing = false;

    return node;
}

}; // namespace Mirael