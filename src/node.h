#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <xutility>

#include "data.h"
#include "util.h"

namespace Mirael
{

class Graph;

class Node
{
public:
    virtual ~Node() = default;

protected:
    virtual void onDeserialize(const nlohmann::json &j) {}
    virtual void onInit() = 0;
    virtual void onShow() = 0;
    virtual void onSerialize(nlohmann::json &j) const {}

    virtual void onShowProperties() {}

    NodeId getId() const { return id; }
    void *getIdAsPointer() const { return (void *)(uintptr_t)(uint64_t)(id); } // this is necessary for using ImGui::PushID()
    PinId getPinId(std::string_view pinKey);
    GraphElementId getMaxElementId() const;
    void raiseModified(ChangeImpact impact);

    void setPos(ImVec2 newPos);
    void select() { selectPending = true; }

private:
    bool initialized = false;
    Graph *graph;
    NodeId id;
    std::string typeName;
    std::unordered_map<std::string, PinId, TransparentStringHash, std::equal_to<>> pinKeyToId;

    void init(Graph &owner, NodeId id, std::string_view nodeTypeName);
    void show();

    void serialize(nlohmann::json &j) const;
    static std::unique_ptr<Node> deserialize(Graph &owner, NodeId id, const nlohmann::json &j);
    bool deserializing = false;

    ImVec2 pos;
    std::optional<ImVec2> pendingSetPos{};

    bool selectPending = false;

    friend Graph;
};

}; // namespace Mirael