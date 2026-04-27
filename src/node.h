#pragma once

#include <nlohmann/json.hpp>

#include "data.h"

namespace Mirael
{

class Graph;

class Node
{
public:
    virtual ~Node() = default;

protected:
    virtual void onInit() = 0;
    virtual void onShow() = 0;

    NodeId getId() const { return id; }
    PinId getNextPinId();

private:
    bool initialized = false;
    Graph *graph;
    NodeId id;
    std::string typeName;

    void init(Graph &owner, std::string_view nodeTypeName);
    void show();

    void serialize(nlohmann::json &j) const;
    static std::unique_ptr<Node> deserialize(Graph &owner, const nlohmann::json &j);

    friend Graph;
};

}; // namespace Mirael