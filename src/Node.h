#pragma once

#include <nlohmann/json.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <xutility>

#include "data.h"
#include "NodeCore.h"
#include "util.h"

namespace Mirael
{

class Graph;

class Node
{
public:
    virtual ~Node() = default;

    NodeId getId() const { return id_; }
    void *getIdAsPointer() const { return (void *)(uintptr_t)(uint64_t)(id_); } // this is necessary for using ImGui::PushID()

    std::span<PinId> getPinOrder() { return pinOrder_; } // value is undefined unless the derived set it in onOrderPins()
    size_t getPinCount() const { return pinIdToConfig_.size(); }

    bool isDeserializing() const { return deserializing_; }

protected:
    virtual void onDeserialize(const nlohmann::json &j) {}
    virtual void onInit() = 0;
    virtual void onOrderPins(std::vector<PinId> &pinOrder) {} // called if pins have changed since last call to onShow
    virtual void onShow() = 0;
    virtual void onSerialize(nlohmann::json &j) const {}

    virtual void onShowProperties() {}

    virtual std::unique_ptr<NodeCore> createCore() { return nullptr; }

    PinId addPin(std::string_view key, PinConfig config);
    void removePin(std::string_view key);

    GraphElementId getMaxElementId() const;
    void raiseModified(ChangeImpact impact);

    void setPos(ImVec2 newPos);
    void select() { selectPending_ = true; }

private:
    bool initialized_ = false;
    Graph *graph_;
    NodeId id_;
    std::string typeName_;
    std::unordered_map<std::string, PinId, TransparentStringHash, std::equal_to<>> pinKeyToId_;
    std::unordered_map<PinId, PinConfig> pinIdToConfig_;
    std::vector<PinId> pinOrder_;
    bool pinOrderDirty_ = true;

    void init(Graph &owner, NodeId id, std::string_view nodeTypeName);
    void show();

    void serialize(nlohmann::json &j) const;
    static std::unique_ptr<Node> deserialize(Graph &owner, NodeId id, const nlohmann::json &j);
    bool deserializing_ = false;

    ImVec2 pos_;
    std::optional<ImVec2> pendingSetPos_{};

    bool selectPending_ = false;

    void truncatePinKeysToConfigured();
    void removeAllPins();

    friend Graph;
};

} // namespace Mirael