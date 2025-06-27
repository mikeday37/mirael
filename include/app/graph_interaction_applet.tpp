#pragma once

#include "app/applet_graph_types.hpp"
#include "app/graph.hpp"
#include "app/graph_interaction_applet.hpp"
#include "imgui.h"
#include "vec2.hpp"
#include <format>

inline Color convert(ImVec4 color) { return {color.x, color.y, color.z, color.w}; }

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnRenderBackground(Graphics &g)
{
    DrawGraph(g);
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::DrawGraph(Graphics &g)
{
    if (autoSize_) {
        auto size = GetWindowSize();
        pan_ = {size.x / 2.0f, size.y / 2.0f};
        zoom_ = size.y / 2.0f;
    }

    // TODO: modify Graph to use iterators

    for (const auto &edge : graph_.GetEdges()) {
        static_cast<TDerived *>(this)->DrawEdge(g, edge);
    }

    for (const auto &node : graph_.GetNodes()) {
        static_cast<TDerived *>(this)->DrawNode(g, node);
    }

    if constexpr (TInteractionPolicy::hoverHighlightsNodes) {
        if (selectedNodeId_ && selectedNodeId_ != highlightedNodeId_) {
            static_cast<TDerived *>(this)->DrawNode(g, graph_.GetNode(selectedNodeId_));
        }
    }

    if constexpr (TInteractionPolicy::canSelectNodes) {
        if (highlightedNodeId_) {
            static_cast<TDerived *>(this)->DrawNode(g, graph_.GetNode(highlightedNodeId_));
        }
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnShowControls()
{
    if (ImGui::BeginTabBar("##UntangleTabs")) {

        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Checkbox("Auto-Size Window", &autoSize_);
            ImGui::PushID(this);
            OnShowSettingsUI();
            ImGui::PopID();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Style")) {
            OnShowStyleUI();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Debug")) {
            if (ImGui::BeginTable("##varInspector", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Variable", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                auto showVar = [](const char *name, const char *value) -> void {
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", name);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", value);
                };
                showVar("mousePos_.x", std::format("{:.1f}", mousePos_.x).c_str());
                showVar("mousePos_.y", std::format("{:.1f}", mousePos_.y).c_str());

                auto hit = HitTest(mousePos_);
                showVar("hit.worldPos.x", std::format("{:.4f}", hit.worldPos.x).c_str());
                showVar("hit.worldPos.y", std::format("{:.4f}", hit.worldPos.y).c_str());

                showVar("hit.nodeId", std::format("{:d}", hit.nodeId).c_str());
                showVar("hit.edgeId", std::format("{:d}", hit.edgeId).c_str());

                showVar("selectedNodeId_", std::format("{:d}", selectedNodeId_).c_str());
                showVar("highlightedNodeId_", std::format("{:d}", highlightedNodeId_).c_str());
                showVar("selectedEdgeId_", std::format("{:d}", selectedEdgeId_).c_str());
                showVar("highlightedEdgeId_", std::format("{:d}", highlightedEdgeId_).c_str());

                showVar("dragging_", dragging_ ? "true" : "false");
                showVar("dragOffset_.x", std::format("{:.4f}", dragOffset_.x).c_str());
                showVar("dragOffset_.y", std::format("{:.4f}", dragOffset_.y).c_str());

                showVar("Node count", std::format("{:d}", graph_.GetNodeCount()).c_str());
                showVar("Edge count", std::format("{:d}", graph_.GetEdgeCount()).c_str());

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnEvent(const SDL_Event &e)
{
    switch (e.type) {
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {
        case SDLK_a:
        case SDLK_INSERT:
            OnAdd();
            break;
        case SDLK_d:
        case SDLK_DELETE:
            OnDelete();
            break;

        default:
            // this applet doesn't care about other keys
            break;
        }

        break;

    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT) {
            auto oldMousePos = mousePos_;
            mousePos_ = {e.motion.x, e.motion.y};
            if (oldMousePos != mousePos_) {
                OnMove();
            }
            OnClick();
        };
        break;

    case SDL_MOUSEMOTION:
        mousePos_ = {e.motion.x, e.motion.y};
        OnMove();
        break;

    case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT) {
            OnEndClick();
        }
        break;

    default:
        // this applet doesn't care about other events
        break;
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::ClearGraphIndicators()
{
    selectedNodeId_ = 0;
    selectedEdgeId_ = 0;
    highlightedNodeId_ = 0;
    highlightedEdgeId_ = 0;
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
glm::vec2
GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::ToScreen(glm::vec2 worldPos) const
{
    return worldPos * zoom_ + pan_;
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
glm::vec2
GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::ToWorld(glm::vec2 screenPos) const
{
    return (screenPos - pan_) / zoom_;
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::HitInfo
GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::HitTest(glm::vec2 screenPos) const
{
    HitInfo hit;
    hit.worldPos = ToWorld(screenPos);
    hit.nodeId = 0;
    hit.edgeId = 0;

    auto hitTestSettings = GetHitTestSettings();

    float closestPotentialNodeDist = 0;
    for (auto node : graph_.GetNodes()) {
        auto dist = glm::distance(ToScreen(node.data), screenPos);
        auto hitRadius =
            node.id == selectedNodeId_ ? hitTestSettings.nodeHitRadiusSelected : hitTestSettings.nodeHitRadiusNormal;
        if (dist <= hitRadius && (!hit.nodeId || dist < closestPotentialNodeDist)) {
            hit.nodeId = node.id;
            closestPotentialNodeDist = dist;
        }
    }

    float closestPotentialEdgeDist = 0;
    for (auto edge : graph_.GetEdges()) {
        auto dist = PointDistanceToLineSegment(screenPos, ToScreen(graph_.GetNode(edge.nodeIdA).data),
                                               ToScreen(graph_.GetNode(edge.nodeIdB).data));
        auto hitRadius =
            edge.id == selectedEdgeId_ ? hitTestSettings.edgeHitRadiusSelected : hitTestSettings.edgeHitRadiusNormal;
        if (dist <= hitRadius && (!hit.edgeId || dist < closestPotentialEdgeDist)) {
            hit.edgeId = edge.id;
            closestPotentialEdgeDist = dist;
        }
    }

    return hit;
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnAdd()
{
    if (dragging_)
        return;
    auto hit = HitTest(mousePos_);
    if (hit.nodeId && selectedNodeId_ && hit.nodeId != selectedNodeId_) {
        graph_.AddEdge(hit.nodeId, selectedNodeId_);
    } else if (!hit.nodeId) {
        graph_.AddNode(hit.worldPos);
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnDelete()
{
    if (dragging_)
        return;
    auto hit = HitTest(mousePos_);
    if (hit.nodeId) {
        graph_.RemoveNode(hit.nodeId);
        if (selectedNodeId_ == hit.nodeId) {
            selectedNodeId_ = 0;
        }
        if (highlightedNodeId_ == hit.nodeId) {
            highlightedNodeId_ = 0;
        }
    } else if (hit.edgeId) {
        graph_.RemoveEdge(hit.edgeId);
        if (selectedEdgeId_ == hit.edgeId) {
            selectedEdgeId_ = 0;
        }
        if (highlightedEdgeId_ == hit.edgeId) {
            highlightedEdgeId_ = 0;
        }
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnClick()
{
    auto hit = HitTest(mousePos_);

    if (hit.nodeId) {
        selectedNodeId_ = hit.nodeId;
    } else {
        selectedNodeId_ = 0;
        if (hit.edgeId) {
            selectedEdgeId_ = hit.edgeId;
        } else {
            selectedEdgeId_ = 0;
        }
    }

    if (selectedNodeId_) {
        dragging_ = true;
        auto node = graph_.GetNode(hit.nodeId);
        dragOffset_ = ToWorld(mousePos_) - node.data;
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnMove()
{
    if (dragging_) {
        assert(selectedNodeId_);

        graph_.RepositionNode(selectedNodeId_, ToWorld(mousePos_) - dragOffset_);
    } else {
        auto hit = HitTest(mousePos_);

        if (hit.nodeId) {
            highlightedNodeId_ = hit.nodeId;
            highlightedEdgeId_ = 0;
        } else {
            highlightedNodeId_ = 0;
            if (hit.edgeId) {
                highlightedEdgeId_ = hit.edgeId;
            } else {
                highlightedEdgeId_ = 0;
            }
        }
    }
}

template <typename TDerived, GraphType TType, typename TNodeData, typename TEdgeData, typename TInteractionPolicy>
void GraphInteractionApplet<TDerived, TType, TNodeData, TEdgeData, TInteractionPolicy>::OnEndClick()
{
    dragging_ = false;
}
