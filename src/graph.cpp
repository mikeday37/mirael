#include "pch.h"

#include <ranges>

#include "imgui_node_editor.h"

#include "app.h"
#include "graph.h"
#include "registry.h"

namespace ne = ax::NodeEditor;

namespace Mirael
{

void Graph::rename(std::string newName)
{
    name = std::move(newName);
    rebuildWindowName();
    raiseModified(ChangeImpact::Name);
}

GraphData Graph::toData() const { return {name, visible}; }

Graph Graph::fromData(GraphId id, const GraphData &data)
{
    Graph graph(id);
    graph.name = data.name;
    graph.rebuildWindowName();
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

void Graph::bringWindowForward() const
{
    auto windowName = getWindowName();
    ImGui::SetWindowFocus(windowName.c_str());
    auto *window = ImGui::FindWindowByName(windowName.c_str());
    if (window && window->Viewport) {
        auto *viewport = window->Viewport;
        if (viewport->PlatformWindowCreated) {
            ImGui::GetPlatformIO().Platform_SetWindowFocus(viewport);
        }
    }
}

void Graph::activate()
{
    setVisible(true);
    bringWindowForward();
}

void Graph::showView()
{
    if (!visible)
        return;

    if (!context) {
        ne::Config config{};
        settingsFileName        = std::format("node-editor-graph{}.json", id);
        config.SettingsFile     = settingsFileName.c_str();
        config.CanvasSizeMode   = ne::CanvasSizeMode::CenterOnly;
        config.EnableSnapToGrid = false;
        context.reset(ne::CreateEditor(&config));
    }

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(getWindowName().c_str(), &visible)) {
        if (ImGui::IsWindowFocused())
            Project::get().setLastFocusedGraphId(id);
        ne::SetCurrentEditor(&*context);
        ne::Begin("Graph Editor");
        for (const auto &node : nodes | std::views::values)
            node->show();
        ne::End();
        ne::SetCurrentEditor(nullptr);
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

void Graph::userCreateNode(const char *nodeTypeName)
{
    activate();
    auto node = App::get().nodeTypes().createNode(nodeTypeName);
    node->init(*this);
    nodes[node->getId()] = std::move(node);
    raiseModified(ChangeImpact::Other);
}

void Graph::rebuildWindowName() { windowName = std::format("{}###graph{}", name, id); }

void Graph::EditorDeleter::operator()(EditorContext *context) const { ne::DestroyEditor(context); }

}; // namespace Mirael