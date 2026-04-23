#include "pch.h"

#include "app.h"
#include "graph.h"

namespace Mirael
{

void Graph::rename(std::string newName)
{
    name = std::move(newName);
    raiseModified(ChangeImpact::Name);
}

GraphData Graph::toData() const { return {name, visible}; }

Graph Graph::fromData(const GraphData &data)
{
    Graph graph;
    graph.name    = data.name;
    graph.visible = data.visible;
    return graph;
}

void Graph::setVisible(bool visible)
{
    auto oldValue = this->visible;
    this->visible = visible;
    if (oldValue != this->visible)
        raiseModified(ChangeImpact::Other);
}

void Graph::showView(GraphId id)
{
    if (!visible)
        return;

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(std::format("{}###graph{}", name, id).c_str(), &visible)) {
    }
    ImGui::End();

    if (!visible)
        raiseModified(ChangeImpact::Other);
}

void Graph::raiseModified(ChangeImpact impact) const
{
    if (onModified)
        onModified(impact);
}

}; // namespace Mirael