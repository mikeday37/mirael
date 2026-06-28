#include "pch.h"

#include <cmath>
#include <ranges>

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "App.h"
#include "data.h"
#include "Graph.h"
#include "ImGuiEx.h"
#include "NodeTypeRegistry.h"

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

    j["ratemode"] = to_string(runRate_.rateMode);
    j["fps"]      = runRate_.desiredFramesPerSecond;

    if (!luaEnvInitScript_.empty())
        j["initlua"] = luaEnvInitScript_;

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

    if (j.contains("ratemode")) {
        auto rateModeString = j["ratemode"].get<std::string>();
        if (!try_parse(rateModeString, graph->runRate_.rateMode))
            throw std::runtime_error(std::format("Graph json parsing error: unknown ratemode string: {}", rateModeString));
    }

    if (j.contains("fps")) {
        graph->runRate_.desiredFramesPerSecond = j["fps"].get<float>();
    }

    if (j.contains("initlua")) {
        auto s                   = j["initlua"].get<std::string>();
        graph->luaEnvInitScript_ = s;
        graph->sendInitScript();
    }

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
        graph->onNodeAdded(it->second.get());
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
    auto oldValue = visible_;
    visible_      = visible;
    if (oldValue != visible_)
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
    if (!visible_) {
        updateExecutionPlan();
        return;
    }

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

    updateExecutionPlan();
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
    auto node_ptr = node.get();
    nodes_.try_emplace(id, std::move(node));
    onNodeAdded(node_ptr);
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

    ImGuiEx::RowLabel("Cycle Detected");
    ImGui::TextUnformatted(cycleDetected_ ? "True" : "False");

    ImGuiEx::RowLabel("Execution Plan Version");
    ImGui::Text("%llu", currentPlanVersion_);
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
    if (node) {
        auto typeName = node->typeName_;
        if (ImGui::CollapsingHeader("Node", ImGuiTreeNodeFlags_DefaultOpen)) {
            node->onShowProperties();
        }
    } else if (ImGui::CollapsingHeader("Graph", ImGuiTreeNodeFlags_DefaultOpen)) {

        RunRateMode priorMode                = runRate_.rateMode;
        static constexpr RunRateMode modes[] = {RunRateMode::Disabled, RunRateMode::SetRate,
                                                RunRateMode::Unlimited}; // don't support UIRate yet
        if (ImGui::BeginCombo("Run Rate Mode", to_display_string(runRate_.rateMode), ImGuiComboFlags_WidthFitPreview)) {
            for (auto mode : modes) {
                bool selected = mode == runRate_.rateMode;
                if (ImGui::Selectable(to_display_string(mode), selected)) {
                    runRate_.rateMode = mode;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (runRate_.rateMode != priorMode) {
            raiseModified(ChangeImpact::GraphRunRate);
            runner_.adjustRunRate(runRate_);
        }

        const float priorFrameRateSetting = runRate_.desiredFramesPerSecond;
        ImGui::InputFloat("Desired FPS", &runRate_.desiredFramesPerSecond, 0.0f, 0.0f, "%.7g");
        runRate_.desiredFramesPerSecond = std::clamp(runRate_.desiredFramesPerSecond, 0.0f, 1e6f);
        if (fabs(priorFrameRateSetting - runRate_.desiredFramesPerSecond) > 1e-9f && RunRateMode::SetRate == runRate_.rateMode) {
            raiseModified(ChangeImpact::GraphRunRate);
            runner_.adjustRunRate(runRate_);
        }
        ImGui::SameLine();
        ImGuiEx::ToolTipHint("Only used if Run Rate Mode = Set Rate.");

        ImGui::SeparatorText("Lua Environment");
        if (ImGui::Button("Reset"))
            sendInitScript();
        if (auto r = runner_.tryAcceptInitScriptResult())
            initScriptResult_ = *r;
        if (!initScriptResult_.empty()) {
            ImGui::SameLine();
            ImGui::Text("Init Result: %s", initScriptResult_.c_str());
        }
        ImGui::InputTextMultiline("###init-script", &luaEnvInitScript_, ImGui::GetContentRegionAvail(),
                                  ImGuiInputTextFlags_AllowTabInput);
    }
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

    if (pinConfig.direction == PinDirection::Output) {
        establishDelta();
        pendingDelta_->addedOutputs.push_back(pinId);
    }
}

void Graph::onPinRemoved(NodeId nodeId, PinId pinId)
{
    // remove all links involving pin
    auto &linkSet = pinLinks_.at(pinId);
    while (!linkSet.empty())
        removeLink(*linkSet.begin());

    // remove the pin
    auto it        = pins_.find(pinId);
    bool wasOutput = it->second.direction == PinDirection::Output;
    assert(it->second.nodeId == nodeId); // removes should always correspond to the correct node
    pins_.erase(it);

    // remove the pin link set
    pinLinks_.erase(pinId);

    // add to delta if it was an output
    if (wasOutput) {
        establishDelta();
        pendingDelta_->deletedOutputs.push_back(pinId);
    }
}

const char *Graph::to_display_string(RunRateMode mode)
{
    switch (mode) {
        using enum RunRateMode;
    case Disabled:
        return "Disabled";
    case SetRate:
        return "Set Rate";
    case UIRate:
        return "UI Rate";
    case Unlimited:
        return "Unlimited";
    default:
        return "(unknown)";
    }
}

const char *Graph::to_string(RunRateMode mode)
{
    switch (mode) {
        using enum RunRateMode;
    case Disabled:
        return "disabled";
    case SetRate:
        return "setrate";
    case UIRate:
        return "uirate";
    case Unlimited:
        return "unlimited";
    default:
        throw std::runtime_error(std::format("Unknown Graph::RunRateMode enum value: {}", static_cast<int>(mode)));
    }
}

bool Graph::try_parse(std::string_view s, RunRateMode &out)
{
    if (s == "disabled") {
        out = RunRateMode::Disabled;
        return true;
    } else if (s == "setrate") {
        out = RunRateMode::SetRate;
        return true;
    } else if (s == "uirate") {
        out = RunRateMode::UIRate;
        return true;
    } else if (s == "unlimited") {
        out = RunRateMode::Unlimited;
        return true;
    } else
        return false;
}

void Graph::initRunner()
{
    updateExecutionPlan();
    runner_.run(runRate_);
}

void Graph::sendInitScript()
{
    establishDelta();
    pendingDelta_->luaEnvInitScript = luaEnvInitScript_;
    planDirty_                      = true;
}

void Graph::establishDelta()
{
    if (!pendingDelta_) {
        pendingDelta_          = std::make_unique<ResourceDelta>();
        pendingDelta_->version = nextPlanVersion_++;
    }
}

std::vector<NodeId> Graph::toposort(bool &cycleDetected)
{
    std::vector<NodeId> result;
    std::unordered_map<NodeId, int> inDegree;
    std::unordered_map<NodeId, std::vector<NodeId>> downstream;
    std::vector<NodeId> queue;

    const auto nodeCount = nodes_.size();
    result.reserve(nodeCount);
    inDegree.reserve(nodeCount);
    downstream.reserve(nodeCount);
    queue.reserve(nodeCount);

    for (auto &[id, node] : nodes_) {
        inDegree.try_emplace(id, 0);
        downstream.try_emplace(id);
    }

    for (auto &[id, link] : links_) {
        inDegree.at(link.b.node)++;
        downstream.at(link.a.node).push_back(link.b.node);
    }

    for (auto &[id, degree] : inDegree)
        if (degree == 0)
            queue.push_back(id);

    while (!queue.empty()) {
        auto id = queue.back();
        queue.pop_back();
        result.push_back(id);
        for (auto next : downstream.at(id))
            if (!--inDegree.at(next))
                queue.push_back(next);
    }

    cycleDetected = result.size() != nodes_.size();
    return result;
}

void Graph::updateExecutionPlan()
{
    if (!planDirty_)
        return;
    planDirty_ = false;

    auto sortedNodes = toposort(cycleDetected_);

    // TODO: if cycle detected, flag newly added links as potentially cyclic
    // TODO: if no cycle detected, clear all such flags

    if (cycleDetected_)
        return;

    auto plan     = std::make_unique<ExecutionPlan>();
    plan->version = pendingDelta_ ? pendingDelta_->version : nextPlanVersion_++;

    if (pendingDelta_)
        runner_.queueDelta(std::move(pendingDelta_));
    assert(!pendingDelta_); // the move should clear this ptr

    plan->nodeExecutionOrder = std::move(sortedNodes);

    auto &valueLinks = plan->valueLinks;
    for (auto &[id, link] : links_)
        valueLinks.push_back(ExecutionPlan::Link{.output = link.a.pin, .input = link.b.pin});

    currentPlanVersion_ = plan->version;
    runner_.postPlan(std::move(plan));
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
    planDirty_ = true;
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
    planDirty_ = true;
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

    if (pendingNodeSelection_) {
        ne::ClearSelection();
        for (auto nodeId : *pendingNodeSelection_)
            ne::SelectNode(static_cast<ne::NodeId>(nodeId), true);
        pendingNodeSelection_.reset();
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

    if (ne::BeginShortcut()) {
        bool cut  = ne::AcceptCut();
        bool copy = !cut && ne::AcceptCopy();

        if (cut || copy) {
            std::vector<ne::NodeId> rawNodeIds(ne::GetActionContextNodes(nullptr, 0));
            ne::GetActionContextNodes(rawNodeIds.data(), static_cast<int>(rawNodeIds.size()));
            std::vector<NodeId> nodeIds;
            nodeIds.reserve(rawNodeIds.size());
            for (auto id : rawNodeIds)
                nodeIds.push_back(static_cast<NodeId>(id));
            auto snippet = buildSnippet(nodeIds);
            ImGui::SetClipboardText(""); // TODO: adding a string representation would enable copying across Mirael instances
            App::get().setGraphSnippet(snippet);
            if (cut && snippet) {
                ne::ClearSelection();
                for (const auto &[nodeId, nodeInfo] : snippet->nodes)
                    removeNode(nodeId);
                if (!snippet->nodes.empty())
                    raiseModified(ChangeImpact::RemoveNode);
            }
        } else if (ne::AcceptPaste()) {
            auto snippet = App::get().getGraphSnippet();
            if (snippet)
                pasteSnippet(*snippet);
        }

        ne::EndShortcut();
    }
}

void Graph::removeNode(NodeId nodeId)
{
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end())
        return;

    it->second->removeAllPins();
    nodes_.erase(it);

    establishDelta();
    pendingDelta_->deletedCores.push_back(nodeId);
    planDirty_ = true;
}

void Graph::onNodeAdded(Node *node)
{
    auto core = node->createCore();
    if (!core)
        return;
    establishDelta();
    auto [it, inserted] = pendingDelta_->addedCores.try_emplace(node->id_, std::move(core));
    assert(inserted);
    planDirty_ = true;
}

std::shared_ptr<GraphSnippet> Graph::buildSnippet(std::span<NodeId> nodeIds) const
{
    if (nodeIds.empty())
        return nullptr;

    auto snippet = std::make_shared<GraphSnippet>();

    snippet->sourceGraphId = id_;

    std::optional<ImVec2> minCorner, maxCorner;

    // build node and pin info
    for (auto nodeId : nodeIds) {
        const auto &node = nodes_.at(nodeId);
        auto &nodeInfo   = snippet->nodes[nodeId];
        nodeInfo.type    = node->typeName_;
        nodeInfo.pos     = node->pos_;
        node->onSerialize(nodeInfo.config);
        auto &pins = nodeInfo.pins;
        for (const auto &[pinKey, pinId] : node->pinKeyToId_) {
            auto &pinInfo  = pins[pinId];
            pinInfo.key    = pinKey;
            pinInfo.config = node->pinIdToConfig_.at(pinId);
        }

        ImVec2 size = ne::GetNodeSize(static_cast<ne::NodeId>(nodeId));
        ImVec2 lr   = {node->pos_.x + size.x, node->pos_.y + size.y};

        if (minCorner)
            minCorner = {std::min(minCorner->x, node->pos_.x), std::min(minCorner->y, node->pos_.y)};
        else
            minCorner = node->pos_;

        if (maxCorner)
            maxCorner = {std::max(maxCorner->x, lr.x), std::max(maxCorner->y, lr.y)};
        else
            maxCorner = lr;
    }

    assert(minCorner && maxCorner);
    snippet->center = {(minCorner->x + maxCorner->x) / 2.0f, (minCorner->y + maxCorner->y) / 2.0f};

    // build links and incomingNodes
    for (const auto &[nodeId, nodeInfo] : snippet->nodes)
        for (const auto &[pinId, pinInfo] : nodeInfo.pins)
            if (pinInfo.config.direction == PinDirection::Input)
                for (auto linkId : pinLinks_.at(pinId)) {
                    const auto &link = links_.at(linkId);
                    assert(link.b.node == nodeId && link.b.pin == pinId);
                    snippet->links.try_emplace(linkId, link);
                    if (!snippet->nodes.contains(link.a.node))
                        snippet->incomingNodes.insert(link.a.node);
                }

    return snippet;
}

void Graph::pasteSnippet(const GraphSnippet &snippet)
{
    if (snippet.nodes.empty())
        return;

    ImVec2 at     = ImGui::GetMousePos();
    ImVec2 offset = {at.x - snippet.center.x, at.y - snippet.center.y};

    std::unordered_map<NodeId, NodeId> oldNodeToNew;
    std::unordered_map<PinId, PinId> oldPinToNew;

    // first create all the nodes
    for (const auto &[oldNodeId, oldNodeInfo] : snippet.nodes) {
        auto node = Node::createNewFromSnippet(*this, oldNodeInfo, offset);
        for (const auto &[oldPinId, oldPinInfo] : oldNodeInfo.pins) {
            const auto &pinKey    = oldPinInfo.key;
            oldPinToNew[oldPinId] = node->pinKeyToId_.at(pinKey);
            // NOTE: the above relies on the expected invariant that a derived node's custom config completely determines its pins
            // TODO: add doc: AddingNodeTypes.md, describing how to correctly derive Node Types, including the above required invariant
        }
        auto newNodeId      = node->getId();
        auto [it, inserted] = nodes_.try_emplace(newNodeId, std::move(node));
        assert(inserted);
        oldNodeToNew[oldNodeId] = newNodeId;
        onNodeAdded(it->second.get());
    }

    auto mapLink = [&](const PinRef &old) -> PinRef {
        return PinRef{.node = oldNodeToNew.at(old.node), .pin = oldPinToNew.at(old.pin)};
    };

    // then create the links
    bool addedLink = false;
    for (const auto &[oldLinkId, oldLink] : snippet.links) {
        Link newLink{};
        if (snippet.incomingNodes.contains(oldLink.a.node)) {
            // links to "incoming" nodes (outside the snippet) must only be added in the same graph if the node and pin still exist
            if (snippet.sourceGraphId == id_ && nodes_.contains(oldLink.a.node) && pins_.contains(oldLink.a.pin))
                newLink.a = oldLink.a;
            else
                continue;
        } else
            newLink.a = mapLink(oldLink.a);
        newLink.b = mapLink(oldLink.b);
        addLink(std::move(newLink));
        addedLink = true;
    }

    raiseModified(ChangeImpact::AddNode);
    if (addedLink)
        raiseModified(ChangeImpact::AddLink);

    // select what we pasted -- can't do this immediately, have to queue it for next frame
    ne::ClearSelection();
    std::vector<NodeId> newNodes;
    newNodes.reserve(oldNodeToNew.size());
    for (const auto &[oldNodeId, newNodeId] : oldNodeToNew)
        newNodes.emplace_back(newNodeId);
    pendingNodeSelection_ = std::move(newNodes);
}

void Graph::EditorDeleter::operator()(EditorContext *context) const { ne::DestroyEditor(context); }

} // namespace Mirael