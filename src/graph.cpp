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
    name_ = std::move(newName);
    rebuildWindowName();
    raiseModified(ChangeImpact::GraphName);
}

void Graph::serialize(nlohmann::json &j) const
{
    j["uid"]     = uid_;
    j["name"]    = name_;
    j["visible"] = visible_;

    j["x"]    = canvasInfo_.orientation.origin.x;
    j["y"]    = canvasInfo_.orientation.origin.y;
    j["zoom"] = canvasInfo_.orientation.zoom;

    j["nodes"] = json::object();
    for (const auto &[nodeId, node] : nodes_) {
        json nodeJson;
        node->serialize(nodeJson);
        j["nodes"][std::to_string(nodeId)] = nodeJson;
    }

    j["links"] = json::object();
    for (const auto &[linkId, link] : links_) {
        json linkJson;
        serializeLink(linkJson, link);
        j["links"][std::to_string(linkId)] = linkJson;
    }
}

std::unique_ptr<Graph> Graph::deserialize(GraphId id, std::string_view uid, const nlohmann::json &j)
{
    assert(uid == j["uid"].get<std::string>()); // during deserialization, the uid is parsed by the Project in advance

    auto graph   = std::make_unique<Graph>(id, uid);
    graph->name_ = j["name"].get<std::string>();
    graph->rebuildWindowName();
    graph->visible_ = j["visible"].get<bool>();

    graph->canvasInfo_.orientation.origin.x = j["x"].get<float>();
    graph->canvasInfo_.orientation.origin.y = j["y"].get<float>();
    graph->canvasInfo_.orientation.zoom     = j["zoom"].get<float>();

    graph->pendingSetInitialCanvasOrientation_ = graph->canvasInfo_.orientation;

    const auto &nodesObj = j.at("nodes");
    if (!nodesObj.is_object())
        throw std::runtime_error("Graph json parsing error: 'nodes' is not an object.");

    GraphElementId maxElementId = 0;

    for (const auto &[key, value] : nodesObj.items()) {
        NodeId nodeId       = static_cast<NodeId>(std::stoull(key));
        auto [it, inserted] = graph->nodes_.try_emplace(nodeId, Node::deserialize(*graph, nodeId, value));
        if (!inserted)
            throw std::runtime_error(
                std::format("Node Id {} not inserted into Graph Id {} during deserialization.", nodeId, graph->id_));
        auto maxElementIdInNode = it->second->getMaxElementId();
        maxElementId            = std::max(maxElementId, maxElementIdInNode);
    }

    if (j.contains("links")) {
        const auto &linksObj = j.at("links");
        if (!linksObj.is_object())
            throw std::runtime_error("Graph json parsing error: 'links' is not an object.");

        for (const auto &[key, value] : linksObj.items()) {
            LinkId linkId = static_cast<LinkId>(std::stoull(key));
            graph->addLinkWithId(graph->deserializeLink(value), linkId);
            maxElementId = std::max(maxElementId, linkId);
        }
    }

    graph->nextElementId_ = maxElementId + 1;

    return graph;
}

void Graph::serializeLink(nlohmann::json &j, const Link &link) const
{
    j["a"] = link.a.pin;
    j["b"] = link.b.pin;
}

Link Graph::deserializeLink(const nlohmann::json &j)
{
    PinId a = static_cast<PinId>(j["a"].get<uint64_t>());
    PinId b = static_cast<PinId>(j["b"].get<uint64_t>());

    return Link{.a = {.node = pins_.at(a).nodeId, .pin = a}, //
                .b = {.node = pins_.at(b).nodeId, .pin = b}};
}

void Graph::setVisible(bool visible)
{
    auto oldValue  = this->visible_;
    this->visible_ = visible;
    if (oldValue != this->visible_)
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
    if (!visible_)
        return;

    if (!context_)
        initEditorContext();

    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(getWindowName().c_str(), &visible_)) {

        if (ImGui::IsWindowFocused())
            Project::get().setLastFocusedGraphId(id_);

        ne::SetCurrentEditor(&*context_);
        adjustEditorStyle();

        if (pendingSetInitialCanvasOrientation_) {
            canvasInfo_.orientation.zoom   = pendingSetInitialCanvasOrientation_->zoom;
            canvasInfo_.orientation.origin = pendingSetInitialCanvasOrientation_->origin;
            ne::SetInitialViewOrientation(pendingSetInitialCanvasOrientation_->zoom, pendingSetInitialCanvasOrientation_->origin);
            pendingSetInitialCanvasOrientation_.reset();
        }

        ne::Begin(editorId_.c_str());

        if (pendingSetCanvasOrientation_) {
            ne::SetViewOrientation(pendingSetCanvasOrientation_->zoom, pendingSetCanvasOrientation_->origin);
            pendingSetCanvasOrientation_.reset();
        }

        if (!ne::IsPendingInitialViewOrientation()) {

            showNodesAndLinks(); // wait until view orientation is stable to avoid a glitchy-looking load

            auto lastOrientation    = canvasInfo_.orientation;
            canvasInfo_.orientation = {.zoom = ne::GetCurrentZoom(), .origin = ne::GetCurrentOrigin()};
            canvasInfo_.mousePos    = ImGui::GetMousePos();
            ne::GetCurrentViewRect(&canvasInfo_.viewRectMin, &canvasInfo_.viewRectMax);

            if (isOrientationChangeSignificant(lastOrientation, canvasInfo_.orientation))
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

    if (!visible_)
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
    nodes_.try_emplace(id, std::move(node));
    raiseModified(ChangeImpact::AddNode);
}

void Graph::showDiagnosticRows()
{
    ImGuiEx::RowLabel("ID");
    ImGui::Text("%llu", id_);

    ImGuiEx::RowLabel("Name");
    ImGui::TextUnformatted(name_.c_str());

    ImGuiEx::RowLabel("UID");
    ImGui::TextUnformatted(uid_.c_str());

    ImGuiEx::RowLabel("Nodes");
    ImGui::Text("%zu", nodes_.size());

    ImGuiEx::RowLabel("Pins");
    ImGui::Text("%zu", pins_.size());

    ImGuiEx::RowLabel("Links");
    ImGui::Text("%zu", links_.size());

    ImGuiEx::RowLabel("Canvas Zoom");
    ImGui::Text("%.6f", canvasInfo_.orientation.zoom);

    ImGuiEx::RowLabel("Canvas Origin");
    ImGui::Text("%.3f, %.3f", canvasInfo_.orientation.origin.x, canvasInfo_.orientation.origin.y);

    ImGuiEx::RowLabel("Canvas View Rect");
    ImGui::Text("(%.3f, %.3f) - (%.3f, %.3f)", //
                canvasInfo_.viewRectMin.x, canvasInfo_.viewRectMin.y, canvasInfo_.viewRectMax.x, canvasInfo_.viewRectMax.y);

    ImGuiEx::RowLabel("Canvas Mouse Pos");
    ImGui::Text("%.3f, %.3f", canvasInfo_.mousePos.x, canvasInfo_.mousePos.y);

    ImGuiEx::RowLabel("Selection Status");
    ImGui::TextUnformatted(to_string(selectionStatus_));

    ImGuiEx::RowLabel("Selected NodeId", "Only populated if a single node is selected.");
    if (selectedNodeId_.has_value())
        ImGui::Text("%llu", *selectedNodeId_);
    else
        ImGui::TextDisabled("n/a");

    ImGuiEx::RowLabel("Selected LinkId", "Only populated if a single link is selected.");
    if (selectedLinkId_.has_value())
        ImGui::Text("%llu", *selectedLinkId_);
    else
        ImGui::TextDisabled("n/a");
}

ImVec2 Graph::getCanvasViewCenter() const
{
    return ImVec2((canvasInfo_.viewRectMin.x + canvasInfo_.viewRectMax.x) / 2.0f, //
                  (canvasInfo_.viewRectMin.y + canvasInfo_.viewRectMax.y) / 2.0f);
}

void Graph::RepositionNodes()
{
    for (auto &[id, node] : nodes_)
        node->pendingSetPos_ = node->pos_;
}

void Graph::Reorient() { pendingSetCanvasOrientation_ = canvasInfo_.orientation; }

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
    if (selectionStatus_ != SelectionStatus::SingleNode)
        return nullptr;

    auto nodeId = *selectedNodeId_;
    auto it     = nodes_.find(nodeId);
    if (it == nodes_.end())
        return nullptr;
    else
        return &*it->second;
}

void Graph::onPinAdded(NodeId nodeId, PinId pinId, PinConfig pinConfig)
{
    auto [it, inserted] = pins_.try_emplace(pinId, PinInfo{.nodeId = nodeId, .direction = pinConfig.direction});
    assert(inserted); // each add should actually insert
    auto [it2, inserted2] = pinLinks_.try_emplace(pinId);
    assert(inserted2); // this should actually insert as well
}

void Graph::onPinRemoved(NodeId nodeId, PinId pinId)
{
    // remove all links involving pin
    auto &linkSet = pinLinks_.at(pinId);
    while (!linkSet.empty())
        removeLink(*linkSet.begin());

    // remove the pin
    auto it = pins_.find(pinId);
    assert(it->second.nodeId == nodeId); // removes should always correspond to the correct node
    pins_.erase(it);

    // remove the pin link set
    pinLinks_.erase(pinId);
}

void Graph::rebuildWindowName() { windowName_ = std::format("{}###graph-{}", name_, uid_); }

void Graph::initEditorContext()
{
    editorId_ = std::format("Graph {} Editor", id_);
    ne::Config config{};
    config.SettingsFile      = nullptr;
    config.CanvasSizeMode    = ne::CanvasSizeMode::CenterOnly;
    config.EnableSnapToGrid  = false;
    config.EnablePersistence = false;
    context_.reset(ne::CreateEditor(&config));
}

void Graph::adjustEditorStyle()
{
    auto &style                                = ne::GetStyle();
    style.Colors[ne::StyleColor_HovNodeBorder] = ImColor(0, 0, 0, 0); // disable hover highlight
    style.Colors[ne::StyleColor_SelNodeBorder] = ImColor(50, 176, 255, 255);
}

void Graph::addLink(Link &&link) { addLinkWithId(std::move(link), getNextElementId()); }

void Graph::addLinkWithId(Link &&link, LinkId linkId)
{
    PinId pinA = link.a.pin, pinB = link.b.pin;
    const auto &[it, inserted] = links_.try_emplace(linkId, std::move(link));
    assert(inserted);
    pinLinks_.at(pinA).insert(linkId);
    pinLinks_.at(pinB).insert(linkId);
}

void Graph::removeLink(LinkId linkId)
{
    auto it = links_.find(linkId);
    if (it == links_.end())
        return;
    const auto &link = it->second;
    pinLinks_.at(link.a.pin).erase(linkId);
    pinLinks_.at(link.b.pin).erase(linkId);
    links_.erase(it);
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
        selectionStatus_ = SelectionStatus::SingleNode;
        selectedNodeId_  = static_cast<NodeId>(rawNodeId);
        selectedLinkId_.reset();
    } else if (linkSelCount > 0 && totalSelCount == 1) {
        selectionStatus_ = SelectionStatus::SingleLink;
        selectedLinkId_  = static_cast<LinkId>(rawLinkId);
        selectedNodeId_.reset();
    } else if (totalSelCount == 0) {
        selectionStatus_ = SelectionStatus::None;
        selectedNodeId_.reset();
        selectedLinkId_.reset();
    } else {
        assert(totalSelCount > 1);
        selectionStatus_ = SelectionStatus::Multiple;
        selectedNodeId_.reset();
        selectedLinkId_.reset();
    }
}

void Graph::showNodesAndLinks()
{
    // this is called exclusively within the ne::Begin()/::End() region of ::show()

    for (const auto &node : nodes_ | std::views::values) {

        auto priorPos = node->pos_;

        bool setPosThisFrame = false;
        if (node->pendingSetPos_) {
            setPosThisFrame = true;
            ne::SetNodePosition(node->id_, *node->pendingSetPos_);
            node->pendingSetPos_.reset();
        }

        if (node->selectPending_) {
            node->selectPending_ = false;
            if (ne::GetSelectedObjectCount() > 0)
                ne::ClearSelection();
            ne::SelectNode(static_cast<ne::NodeId>(node->id_));
        }

        node->show();

        node->pos_ = ne::GetNodePosition(node->id_);

        if (!setPosThisFrame && (node->pos_.x != priorPos.x || node->pos_.y != priorPos.y))
            raiseModified(ChangeImpact::NodePosition);
    }

    for (const auto &[linkId, link] : links_) {
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
                if (valid && !pinLinks_.at(endPinId).empty())
                    valid = false;

                if (!valid) {
                    ne::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                } else if (ne::AcceptNewItem()) {
                    addLink({.a = PinRef{.node = startPinInfo.nodeId, .pin = startPinId},
                             .b = PinRef{.node = endPinInfo.nodeId, .pin = endPinId}});
                    raiseModified(ChangeImpact::AddLink);
                }
            }
        }

        ne::EndCreate();
    }

    if (ne::BeginDelete()) {

        ne::NodeId editorNodeId = 0;
        while (ne::QueryDeletedNode(&editorNodeId)) {
            if (ne::AcceptDeletedItem()) {
                NodeId nodeId = static_cast<NodeId>(editorNodeId);
                removeNode(nodeId);
                raiseModified(ChangeImpact::RemoveNode);
            }
        }

        ne::LinkId editorLinkId = 0;
        while (ne::QueryDeletedLink(&editorLinkId)) {
            if (ne::AcceptDeletedItem()) {
                LinkId linkId = static_cast<LinkId>(editorLinkId);
                removeLink(linkId);
                raiseModified(ChangeImpact::RemoveLink);
            }
        }

        ne::EndDelete();
    }
}

void Graph::removeNode(NodeId nodeId)
{
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end())
        return;

    it->second->removeAllPins();
    nodes_.erase(it);
}

void Graph::EditorDeleter::operator()(EditorContext *context) const { ne::DestroyEditor(context); }

}; // namespace Mirael