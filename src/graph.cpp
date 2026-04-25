#include "pch.h"

#include "imgui_node_editor.h"

#include "app.h"
#include "graph.h"

namespace ed = ax::NodeEditor;

namespace Mirael
{

void Graph::rename(std::string newName)
{
    name = std::move(newName);
    raiseModified(ChangeImpact::Name);
}

GraphData Graph::toData() const { return {name, visible}; }

Graph Graph::fromData(GraphId id, const GraphData &data)
{
    Graph graph(id);
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

std::string Graph::getWindowName() const
{
    return std::format("{}###graph{}", name, id);
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

void Graph::showView(GraphId id)
{
    if (!visible)
        return;

    if (!context) {
        //ed::Config config;
        context.reset(ed::CreateEditor(/*&config*/));
    }

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(getWindowName().c_str(), &visible)) {
        ed::SetCurrentEditor(&*context);
        ed::Begin("Graph Editor");
        int uniqueId = 1;
        // Start drawing nodes.
        ed::BeginNode(uniqueId++);
            ImGui::Text("Node A");
            ed::BeginPin(uniqueId++, ed::PinKind::Input);
                ImGui::Text("-> In");
            ed::EndPin();
            ImGui::SameLine();
            ed::BeginPin(uniqueId++, ed::PinKind::Output);
                ImGui::Text("Out ->");
            ed::EndPin();
        ed::EndNode();
        ed::End();
        ed::SetCurrentEditor(nullptr);
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

void Graph::EditorDeleter::operator()(EditorContext *context) const
{
    ed::DestroyEditor(context);
}

}; // namespace Mirael