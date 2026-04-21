#include "pch.h"

#include "graph.h"

namespace Mirael
{

void Graph::rename(std::string newName)
{
    name = std::move(newName);
    if (onModified)
        onModified(ChangeImpact::Name);
}

GraphData Graph::toData() const
{
    return {name, visible};
}

Graph Graph::fromData(const GraphData& data)
{
    Graph graph;
    graph.name = data.name;
    graph.visible = data.visible;
    return graph;
}

}; // namespace Mirael