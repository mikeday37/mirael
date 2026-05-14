#include "pch.h"

#include <stdexcept>
#include <unordered_set>

#include "app.h"
#include "data.h"
#include "graph.h"
#include "node.h"

using json = nlohmann::json;

namespace Mirael
{

PinId Node::addPin(std::string_view pinKey, PinConfig config)
{
    /* this method adds a pin for a node to use, and is intended to be used primiarily within a derived Node's onInit() implementation.
     * during deserialization, the pinKeyToId_ map is pre-populated so that pin references are stable across save/load.  But that
     * deserialization step only populates pinKeyToId_, not the pinConfig map.  Therefore, if the key is found, it has to see if the
     * config is known - if so, it verifies that it hasn't changed.  Otherwise, it establishes that config.  If the key is not found,
     * it only adds the key, getting a next element id, and config, if not deserializing.  We should never add an unknown pin during
     * deserialization.
     */

    auto it = pinKeyToId_.find(pinKey);
    if (it != pinKeyToId_.end()) {
        auto pinId = it->second;
        auto cit   = pinIdToConfig_.find(pinId);
        if (cit == pinIdToConfig_.end()) {
            // key found, but config missing
            pinIdToConfig_.try_emplace(pinId, config);
            graph_->onPinAdded(id_, pinId, config);
        } else {
            assert(false); // there is not yet a situation in which this case is actually expected to happen
            if (config != cit->second)
                throw std::runtime_error(
                    std::format("Node (id={}, type={}) attempted to establish an inconsistent config for pin (key={}, id={}).", id_,
                                typeName_, pinKey, pinId));
        }
        return pinId;
    } else {
        if (deserializing_)
            throw std::runtime_error(std::format(
                "Node (id={}, type={}) attempted to access an unknown pin (key={}) during deserialization.", id_, typeName_, pinKey));
        PinId pinId = static_cast<PinId>(graph_->getNextElementId());
        pinKeyToId_.try_emplace(std::string(pinKey), pinId);
        pinIdToConfig_.try_emplace(pinId, config);
        graph_->onPinAdded(id_, pinId, config);
        return pinId;
    }
}

void Node::removePin(std::string_view key)
{
    auto it = pinKeyToId_.find(key);
    if (it != pinKeyToId_.end()) {
        auto pinId = it->second;
        pinIdToConfig_.erase(pinId);
        pinKeyToId_.erase(it);
        graph_->onPinRemoved(id_, pinId);
    }
}

/*PinId Node::getPinId(std::string_view pinKey)
{
    auto it = pinKeyToId_.find(pinKey);
    if (it == pinKeyToId_.end()) {
        return 0;
    } else {
        return it->second;
    }
}*/

GraphElementId Node::getMaxElementId() const
{
    if (pinKeyToId_.empty())
        return id_;
    else
        return std::max(id_, std::ranges::max(pinKeyToId_ | std::views::values));
}

void Node::raiseModified(ChangeImpact impact) { graph_->raiseModified(impact); }

void Node::setPos(ImVec2 newPos)
{
    pendingSetPos_ = newPos;
    raiseModified(ChangeImpact::NodePosition);
}

void Node::init(Graph &owner, NodeId id, std::string_view nodeTypeName)
{
    if (initialized_)
        throw std::runtime_error("Node already initialized");
    initialized_ = true;

    graph_    = &owner;
    id_       = id;
    typeName_ = nodeTypeName;

    onInit();
}

void Node::show() { onShow(); }

void Node::serialize(nlohmann::json &j) const
{
    j["type"] = typeName_;

    json pinsJson = json::object();
    for (const auto &[key, id] : pinKeyToId_)
        pinsJson[key] = id;
    j["pins"] = pinsJson;

    j["x"] = pos_.x;
    j["y"] = pos_.y;

    json configJson = json::object();
    onSerialize(configJson);
    if (!configJson.empty())
        j["config"] = configJson;
}

std::unique_ptr<Node> Node::deserialize(Graph &owner, NodeId id, const nlohmann::json &j)
{
    auto typeName = j["type"].get<std::string>();
    auto node     = App::get().nodeTypes().createNode(typeName);

    node->deserializing_ = true;

    for (const auto &[key, value] : j["pins"].items())
        node->pinKeyToId_[key] = value.get<uint64_t>();
    auto pinCountDeserialized = node->pinKeyToId_.size();

    node->pos_.x = j["x"].get<float>();
    node->pos_.y = j["y"].get<float>();

    node->pendingSetPos_ = node->pos_;

    if (j.contains("config"))
        node->onDeserialize(j["config"]);
    node->init(owner, id, typeName);

    auto pinCountInitialized = node->pinKeyToId_.size();
    if (pinCountDeserialized != pinCountInitialized)
        throw std::runtime_error(
            std::format("Node (id={}, type={}) deserialized with pin count mismatch (deserialized={}, initialized={}).", id,
                        typeName.c_str(), pinCountDeserialized, pinCountInitialized));

    node->truncatePinKeysToConfigured(); // this is only necessary because some old saved projects kept unused pin keys

    if (node->pinKeyToId_.size() != node->pinIdToConfig_.size())
        throw std::runtime_error(std::format("Node (id={}, type={}) deserialized with pin count mismatch (keyToId={}, idToConfig={}).",
                                             id, typeName.c_str(), node->pinKeyToId_.size(), node->pinIdToConfig_.size()));

    node->deserializing_ = false;

    return node;
}

void Node::truncatePinKeysToConfigured()
{
    std::unordered_set<std::string> danglingPinKeys{};

    for (const auto &[pinKey, nodeId] : pinKeyToId_) {
        if (!pinIdToConfig_.contains(nodeId))
            danglingPinKeys.insert(pinKey);
    }

    for (const auto &pinKey : danglingPinKeys) {
        pinKeyToId_.erase(pinKey);
    }
}

void Node::removeAllPins()
{
    while (!pinKeyToId_.empty())
        removePin(pinKeyToId_.begin()->first);
    assert(pinKeyToId_.empty() && pinIdToConfig_.empty());
}

}; // namespace Mirael