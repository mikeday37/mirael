#include "pch.h"

#include <cmath>
#include <ranges>

#include "ine/imgui_node_editor.h"

#include "app.h"
#include "data.h"
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
    raiseModified(ChangeImpact::GraphName);
}

void Graph::serialize(nlohmann::json &j) const
{
    j["uid"]     = uid;
    j["name"]    = name;
    j["visible"] = visible;

    j["x"]    = canvasInfo.orientation.origin.x;
    j["y"]    = canvasInfo.orientation.origin.y;
    j["zoom"] = canvasInfo.orientation.zoom;

    j["nodes"] = json::object();
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

    graph->canvasInfo.orientation.origin.x = j["x"].get<float>();
    graph->canvasInfo.orientation.origin.y = j["y"].get<float>();
    graph->canvasInfo.orientation.zoom     = j["zoom"].get<float>();

    graph->pendingSetInitialCanvasOrientation = graph->canvasInfo.orientation;

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
        raiseModified(ChangeImpact::GraphVisibility);
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

        if (pendingSetInitialCanvasOrientation) {
            canvasInfo.orientation.zoom   = pendingSetInitialCanvasOrientation->zoom;
            canvasInfo.orientation.origin = pendingSetInitialCanvasOrientation->origin;
            ne::SetInitialViewOrientation(pendingSetInitialCanvasOrientation->zoom, pendingSetInitialCanvasOrientation->origin);
            pendingSetInitialCanvasOrientation.reset();
        }

        ne::Begin(editorId.c_str());

        if (pendingSetCanvasOrientation) {
            ne::SetViewOrientation(pendingSetCanvasOrientation->zoom, pendingSetCanvasOrientation->origin);
            pendingSetCanvasOrientation.reset();
        }

        if (!ne::IsPendingInitialViewOrientation()) {
            for (const auto &node : nodes | std::views::values) {

                auto priorPos = node->pos;

                bool setPosThisFrame = false;
                if (node->pendingSetPos) {
                    setPosThisFrame = true;
                    ne::SetNodePosition(node->id, *node->pendingSetPos);
                    node->pendingSetPos.reset();
                }

                if (node->selectPending) {
                    node->selectPending = false;
                    if (ne::GetSelectedObjectCount() > 0)
                        ne::ClearSelection();
                    ne::SelectNode(static_cast<ne::NodeId>(node->id));
                }

                node->show();

                node->pos = ne::GetNodePosition(node->id);

                if (!setPosThisFrame && (node->pos.x != priorPos.x || node->pos.y != priorPos.y))
                    raiseModified(ChangeImpact::NodePosition);
            }

            for (const auto &[linkId, link] : links) {
                ne::Link(static_cast<ne::LinkId>(linkId), link.a.pin, link.b.pin);
            }

            processSelectionState();

            if (ne::BeginCreate()) {

                ne::PinId startEditorPinId{}, endEditorPinId{};
                if (ne::QueryNewLink(&startEditorPinId, &endEditorPinId)) {
                    if (startEditorPinId && endEditorPinId && startEditorPinId != endEditorPinId) {
                        PinId startPinId = static_cast<PinId>(startEditorPinId);
                        PinId endPinId   = static_cast<PinId>(endEditorPinId);

                        // for each pin id, we need to know: node id, and pin direction.
                        // to accept, one pin must be output and one pin must be input, and they must be on different nodes

                        auto startPinInfo = getPinInfo(startPinId);
                        auto endPinInfo   = getPinInfo(endPinId);

                        if (startPinInfo.direction == PinDirection::Input) {
                            std::swap(startPinId, endPinId);
                            std::swap(startPinInfo, endPinInfo);
                        }

                        bool valid = startPinInfo.direction == PinDirection::Output //
                                     && endPinInfo.direction == PinDirection::Input //
                                     && startPinInfo.nodeId != endPinInfo.nodeId;

                        // additional constraint: inputs cannot have more than one link
                        if (valid && !pinLinks.at(endPinId).empty())
                            valid = false;

                        if (!valid) {
                            ne::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        } else if (ne::AcceptNewItem()) {
                            addLink({.a = PinRef{.node = startPinInfo.nodeId, .pin = startPinId},
                                     .b = PinRef{.node = endPinInfo.nodeId, .pin = endPinId}});
                        }
                    }
                }

                ne::EndCreate();
            }

            auto lastOrientation   = canvasInfo.orientation;
            canvasInfo.orientation = {.zoom = ne::GetCurrentZoom(), .origin = ne::GetCurrentOrigin()};
            canvasInfo.mousePos    = ImGui::GetMousePos();
            ne::GetCurrentViewRect(&canvasInfo.viewRectMin, &canvasInfo.viewRectMax);

            if (isOrientationChangeSignificant(lastOrientation, canvasInfo.orientation))
                raiseModified(ChangeImpact::GraphPanZoom);
        }
        ne::Suspend();

        if (ne::ShowBackgroundContextMenu()) {
            ImGui::OpenPopup("Background Menu");
        }

        if (ImGui::BeginPopup("Background Menu")) {
            if (ImGui::MenuItem("Fit Content")) {
                ne::NavigateToContent(1.0f);
            }
            ImGui::EndPopup();
        }

        ne::Resume();

        ne::End();
        ne::SetCurrentEditor(nullptr);
    }
    ImGui::End();

    if (!visible)
        raiseModified(ChangeImpact::GraphVisibility);
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
    node->select();
    node->setPos(getCanvasViewCenter());
    nodes.try_emplace(id, std::move(node));
    raiseModified(ChangeImpact::AddNode);
}

void Graph::showDiagnosticRows()
{
    ImGuiEx::RowLabel("ID");
    ImGui::Text("%llu", id);

    ImGuiEx::RowLabel("Name");
    ImGui::TextUnformatted(name.c_str());

    ImGuiEx::RowLabel("UID");
    ImGui::TextUnformatted(uid.c_str());

    ImGuiEx::RowLabel("Nodes");
    ImGui::Text("%zu", nodes.size());

    ImGuiEx::RowLabel("Pins");
    ImGui::Text("%zu", pins.size());

    ImGuiEx::RowLabel("Links");
    ImGui::Text("%zu", links.size());

    ImGuiEx::RowLabel("Canvas Zoom");
    ImGui::Text("%.6f", canvasInfo.orientation.zoom);

    ImGuiEx::RowLabel("Canvas Origin");
    ImGui::Text("%.3f, %.3f", canvasInfo.orientation.origin.x, canvasInfo.orientation.origin.y);

    ImGuiEx::RowLabel("Canvas View Rect");
    ImGui::Text("(%.3f, %.3f) - (%.3f, %.3f)", //
                canvasInfo.viewRectMin.x, canvasInfo.viewRectMin.y, canvasInfo.viewRectMax.x, canvasInfo.viewRectMax.y);

    ImGuiEx::RowLabel("Canvas Mouse Pos");
    ImGui::Text("%.3f, %.3f", canvasInfo.mousePos.x, canvasInfo.mousePos.y);

    ImGuiEx::RowLabel("Selection Status");
    ImGui::TextUnformatted(to_string(selectionStatus));

    ImGuiEx::RowLabel("Selected NodeId", "Only populated if a single node is selected.");
    if (selectedNodeId.has_value())
        ImGui::Text("%llu", *selectedNodeId);
    else
        ImGui::TextDisabled("n/a");

    ImGuiEx::RowLabel("Selected LinkId", "Only populated if a single link is selected.");
    if (selectedLinkId.has_value())
        ImGui::Text("%llu", *selectedLinkId);
    else
        ImGui::TextDisabled("n/a");
}

ImVec2 Graph::getCanvasViewCenter() const
{
    return ImVec2((canvasInfo.viewRectMin.x + canvasInfo.viewRectMax.x) / 2.0f, //
                  (canvasInfo.viewRectMin.y + canvasInfo.viewRectMax.y) / 2.0f);
}

void Graph::RepositionNodes()
{
    for (auto &[id, node] : nodes)
        node->pendingSetPos = node->pos;
}

void Graph::Reorient() { pendingSetCanvasOrientation = canvasInfo.orientation; }

void Graph::showProperties()
{
    Node *node = getSingleSelectedNode();
    if (node)
        node->onShowProperties();
}

const char *Graph::to_string(SelectionStatus status)
{
    switch (status) {
    case SelectionStatus::None:
        return "None";
    case SelectionStatus::SingleNode:
        return "Single Node";
    case SelectionStatus::SingleLink:
        return "Single Link";
    case SelectionStatus::Multiple:
        return "Multiple";
    default:
        return "(unknown)";
    }
}

Node *Graph::getSingleSelectedNode()
{
    if (selectionStatus != SelectionStatus::SingleNode)
        return nullptr;

    auto nodeId = *selectedNodeId;
    auto it     = nodes.find(nodeId);
    if (it == nodes.end())
        return nullptr;
    else
        return &*it->second;
}

void Graph::onPinAdded(NodeId nodeId, PinId pinId, PinConfig pinConfig)
{
    auto [it, inserted] = pins.try_emplace(pinId, PinInfo{.nodeId = nodeId, .direction = pinConfig.direction});
    assert(inserted); // each add should actually insert
    auto [it2, inserted2] = pinLinks.try_emplace(pinId);
    assert(inserted2); // this should actually insert as well
}

void Graph::onPinRemoved(NodeId nodeId, PinId pinId)
{
    // remove all links involving pin
    auto &linkSet = pinLinks.at(pinId);
    while (!linkSet.empty())
        removeLink(*linkSet.begin());

    // remove the pin
    auto it = pins.find(pinId);
    assert(it->second.nodeId == nodeId); // removes should always correspond to the correct node
    pins.erase(it);

    // remove the pin link set
    pinLinks.erase(pinId);
}

void Graph::rebuildWindowName() { windowName = std::format("{}###graph-{}", name, uid); }

void Graph::initEditorContext()
{
    editorId = std::format("Graph {} Editor", id);
    ne::Config config{};
    config.SettingsFile      = nullptr;
    config.CanvasSizeMode    = ne::CanvasSizeMode::CenterOnly;
    config.EnableSnapToGrid  = false;
    config.EnablePersistence = false;
    context.reset(ne::CreateEditor(&config));
}

void Graph::adjustEditorStyle()
{
    auto &style                                = ne::GetStyle();
    style.Colors[ne::StyleColor_HovNodeBorder] = ImColor(0, 0, 0, 0); // disable hover highlight
    style.Colors[ne::StyleColor_SelNodeBorder] = ImColor(50, 176, 255, 255);
}

void Graph::addLink(Link &&link)
{
    PinId pinA = link.a.pin, pinB = link.b.pin;
    LinkId linkId = static_cast<LinkId>(getNextElementId());
    links.try_emplace(linkId, std::move(link));
    pinLinks.at(pinA).insert(linkId);
    pinLinks.at(pinB).insert(linkId);
}

void Graph::removeLink(LinkId linkId)
{
    auto it = links.find(linkId);
    if (it == links.end())
        return;
    const auto &link = it->second;
    pinLinks.at(link.a.pin).erase(linkId);
    pinLinks.at(link.b.pin).erase(linkId);
    links.erase(it);
}

bool Graph::isOrientationChangeSignificant(CanvasOrientation a, CanvasOrientation b)
{
    const float zoomEpsilon   = 1e-4f;
    const float scrollEpsilon = 1e-2f;

    if (fabs(a.zoom - b.zoom) > zoomEpsilon)
        return true;

    if (fabs(a.origin.x - b.origin.x) > scrollEpsilon || fabs(a.origin.y - b.origin.y) > scrollEpsilon)
        return true;

    return false;
}

void Graph::processSelectionState()
{
    ne::NodeId rawNodeId{};
    ne::LinkId rawLinkId{};
    int totalSelCount = ne::GetSelectedObjectCount();
    int nodeSelCount  = ne::GetSelectedNodes(&rawNodeId, 1);
    int linkSelCount  = ne::GetSelectedLinks(&rawLinkId, 1);
    if (nodeSelCount > 0 && totalSelCount == 1) {
        selectionStatus = SelectionStatus::SingleNode;
        selectedNodeId  = static_cast<NodeId>(rawNodeId);
        selectedLinkId.reset();
    } else if (linkSelCount > 0 && totalSelCount == 1) {
        selectionStatus = SelectionStatus::SingleLink;
        selectedLinkId  = static_cast<LinkId>(rawLinkId);
        selectedNodeId.reset();
    } else if (totalSelCount == 0) {
        selectionStatus = SelectionStatus::None;
        selectedNodeId.reset();
        selectedLinkId.reset();
    } else {
        assert(totalSelCount > 1);
        selectionStatus = SelectionStatus::Multiple;
        selectedNodeId.reset();
        selectedLinkId.reset();
    }
}

void Graph::EditorDeleter::operator()(EditorContext *context) const { ne::DestroyEditor(context); }

}; // namespace Mirael