#pragma once

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

    void init(Graph &owner);
    void show();

    friend Graph;
};

}; // namespace Mirael