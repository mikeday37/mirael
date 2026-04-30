#include "pch.h"

#include <ranges>

#include "imgui_node_editor.h"

#include "app.h"
#include "graph.h"
#include "imguiex.h"
#include "registry.h"

namespace ne = ax::NodeEditor;
using json   = nlohmann::json;

namespace Mirael
{

void Graph::rename(std::string newName)
{
    name = std::move(newName);
    rebuildWindowName();
    raiseModified(ChangeImpact::Name);
}

void Graph::serialize(nlohmann::json &j) const
{
    j["uid"]     = uid;
    j["name"]    = name;
    j["visible"] = visible;
    j["nodes"]   = json::object();
    for (const auto &[id, node] : nodes) {
        json nodeJson;
        node->serialize(nodeJson);
        j["nodes"][std::to_string(id)] = nodeJson;
    }
}

std::unique_ptr<Graph> Graph::deserialize(GraphId id, std::string_view uid, const nlohmann::json &j)
{
    assert(uid == j["uid"].get<std::string>()); // during deserialization, the uid is parsed by the Project in advance

    auto graph  = std::make_unique<Graph>(id, uid);
    graph->name = j["name"].get<std::string>();
    graph->rebuildWindowName();
    graph->visible = j["visible"].get<bool>();

    const auto &nodesObj = j.at("nodes");
    if (!nodesObj.is_object())
        throw std::runtime_error("Graph json parsing error: 'nodes' is not an object.");

    GraphElementId maxElementId = 0;

    for (const auto &[key, value] : nodesObj.items()) {
        GraphElementId id   = static_cast<GraphElementId>(std::stoull(key));
        auto [it, inserted] = graph->nodes.try_emplace(id, Node::deserialize(*graph, id, value));
        if (!inserted)
            throw std::runtime_error(std::format("Node Id {} not inserted into Graph Id {} during deserialization.", id, graph->id));
        auto maxElementIdInNode = it->second->getMaxElementId();
        maxElementId            = std::max(maxElementId, maxElementIdInNode);
    }

    graph->nextElementId = maxElementId + 1;

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

    if (!context)
        initEditorContext();

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(getWindowName().c_str(), &visible)) {
        if (ImGui::IsWindowFocused())
            Project::get().setLastFocusedGraphId(id);
        ne::SetCurrentEditor(&*context);
        adjustEditorStyle();
        ne::Begin("Graph Editor");

        for (const auto &node : nodes | std::views::values)
            node->show();

        if (ne::BeginCreate()) {

            // TODO: continue link implementation here

            ne::EndCreate();
        }

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
    auto id   = static_cast<NodeId>(getNextElementId());
    node->init(*this, id, nodeTypeName);
    nodes.emplace(id, std::move(node));
    raiseModified(ChangeImpact::Other);
}

void Graph::showDiagnosticRows()
{
    ImGuiEx::DiagnosticLabel("ID");
    ImGui::Text("%u", id);

    ImGuiEx::DiagnosticLabel("Name");
    ImGui::TextUnformatted(name.c_str());

    ImGuiEx::DiagnosticLabel("UID");
    ImGui::TextUnformatted(uid.c_str());

    ImGuiEx::DiagnosticLabel("Nodes");
    ImGui::Text("%zu", nodes.size());

    ImGuiEx::DiagnosticLabel("Links");
    ImGui::Text("%zu", links.size());
}

void Graph::rebuildWindowName() { windowName = std::format("{}###graph-{}", name, uid); }

void Graph::initEditorContext()
{
    ne::Config config{};
    settingsFileName        = std::format("node-editor-graph-{}.json", uid);
    config.SettingsFile     = settingsFileName.c_str();
    config.CanvasSizeMode   = ne::CanvasSizeMode::CenterOnly;
    config.EnableSnapToGrid = false;
    context.reset(ne::CreateEditor(&config));
}

void Graph::adjustEditorStyle()
{
    auto &style                                = ne::GetStyle();
    style.Colors[ne::StyleColor_HovNodeBorder] = ImColor(0, 0, 0, 0); // disable hover highlight
    style.Colors[ne::StyleColor_SelNodeBorder] = ImColor(50, 176, 255, 255);
}

void Graph::EditorDeleter::operator()(EditorContext *context) const { ne::DestroyEditor(context); }

}; // namespace Mirael