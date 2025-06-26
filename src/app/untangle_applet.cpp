#include "app_pch.hpp"

#include "app/untangle_applet.hpp"

inline Color convert(ImVec4 color) { return {color.x, color.y, color.z, color.w}; }

void UntangleApplet::OnRenderBackground(Graphics &g)
{
    if (autoSize_) {
        auto size = GetWindowSize();
        pan_ = {size.x / 2.0f, size.y / 2.0f};
        zoom_ = size.y / 2.0f;
    }

    // TODO: modify Graph to use iterators

    for (const auto &edge : graph_.GetEdges()) {
        DrawEdge(g, edge);
    }

    for (const auto &node : graph_.GetNodes()) {
        DrawNode(g, node);
    }

    if (selectedNodeId_ && selectedNodeId_ != highlightedNodeId_) {
        DrawNode(g, graph_.GetNode(selectedNodeId_));
    }

    if (highlightedNodeId_) {
        DrawNode(g, graph_.GetNode(highlightedNodeId_));
    }
}

void UntangleApplet::DrawNode(Graphics &g, const UndirectedGraph<>::Node &node)
{
    auto pos = ToScreen(node.pos);
    auto style = GetNodeStyle(node.id);
    g.Circle(pos, style.radius * style_.nodeScale, style.lineThickness * style_.nodeScale, convert(style.fillColor),
             convert(style.lineColor));
}

void UntangleApplet::DrawEdge(Graphics &g, const UndirectedGraph<>::Edge &edge)
{
    auto nodeA = graph_.GetNode(edge.nodeIdA);
    auto nodeB = graph_.GetNode(edge.nodeIdB);
    auto posA = ToScreen(nodeA.pos);
    auto posB = ToScreen(nodeB.pos);
    auto style = GetEdgeStyle(edge.id);
    g.Line(posA, posB, style.lineThickness * style_.edgeScale, convert(style.lineColor));
}

void UntangleApplet::OnShowControls()
{
    if (ImGui::BeginTabBar("##UntangleTabs")) {

        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Checkbox("Auto-Size Window", &autoSize_);
            if (ImGui::TreeNodeEx("Graph Manipulators",
                                  ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                if (ImGui::BeginTable("##GraphManipulators", 2,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                    for (auto manipulator : graphManipulators_.GetAll()) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if (ImGui::Button(manipulator->GetDisplayName())) {
                            ClearGraphIndicators();
                            manipulator->Manipulate(graph_);
                        }
                        ImGui::TableNextColumn();
                        ImGui::PushID(manipulator);
                        manipulator->OnShowControls();
                        ImGui::PopID();
                    }
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    if (ImGui::Button("Clear")) {
                        ClearGraphIndicators();
                        graph_.Clear();
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx("Graph Animators",
                                  ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                if (playingAnimation_) {
                    if (ImGui::Button("Pause")) {
                        Pause();
                    }
                } else {
                    if (ImGui::Button("Play")) {
                        Play();
                    }
                }
                if (ImGui::BeginTable("##GraphAnimators", 2,
                                      ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
                    for (auto animator : graphAnimators_.GetAll()) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if (ImGui::Button(animator->GetDisplayName())) {
                            SetCurrentAnimator(animator);
                        }
                        ImGui::TableNextColumn();
                        ImGui::PushID(animator);
                        animator->OnShowControls();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Style")) {
            ImGui::SliderFloat("Node Scale", &style_.nodeScale, 0.0f, 2.0f, "%.3f");
            ImGui::SliderFloat("Node Hit Test Padding", &style_.nodeHitTestPadding, 0.0f, 50.0f, "%.3f");
            ImGui::SliderFloat("Edge Scale", &style_.edgeScale, 0.0f, 10.0f, "%.3f");
            ImGui::SliderFloat("Edge Hit Test Padding", &style_.edgeHitTestPadding, 0.0f, 50.0f, "%.3f");

            auto stateStyle = [](const char *name, const char *suffix, GraphPartStyle &part) -> void {
                if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                    if (ImGui::TreeNodeEx(std::format("Node##{}", suffix).c_str(),
                                          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                        ImGui::SliderFloat(std::format("Radius##{}", suffix).c_str(), &part.node.radius, 0.0f, 50.0f,
                                           "%.1f");
                        ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.node.lineThickness,
                                           0.0f, 20.0f, "%.1f");
                        ImGui::ColorEdit4(std::format("Fill Color##{}", suffix).c_str(), (float *)&part.node.fillColor,
                                          ImGuiColorEditFlags_AlphaBar);
                        ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float *)&part.node.lineColor,
                                          ImGuiColorEditFlags_AlphaBar);
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNodeEx(std::format("Edge##{}", suffix).c_str(),
                                          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                        ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.edge.lineThickness,
                                           0.0f, 20.0f, "%.1f");
                        ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float *)&part.edge.lineColor,
                                          ImGuiColorEditFlags_AlphaBar);
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
            };
            stateStyle("Normal", "n", style_.normal);
            stateStyle("Selected", "s", style_.selected);
            stateStyle("Highlight", "h", style_.highlight);
            stateStyle("Highlight Selected", "hs", style_.highlightSelected);
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

void UntangleApplet::OnEvent(const SDL_Event &e)
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

        case SDLK_SPACE:
            if (!currentAnimator_) {
                break;
            };

            if (playingAnimation_) {
                Pause();
            } else {
                Play();
            }
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

void UntangleApplet::OnNewFrame()
{
    if (!currentAnimator_ || !playingAnimation_) {
        return;
    }

    auto [worldTime, deltaTime] = animationTimer_.Tick();
    currentAnimator_->Animate(graph_, worldTime, deltaTime);
}

void UntangleApplet::SetCurrentAnimator(GraphAnimator *animator)
{
    ClearGraphIndicators();
    currentAnimator_ = animator;
    playingAnimation_ = true;
    animationTimer_.Reset();
}

void UntangleApplet::Play()
{
    animationTimer_.Resume();
    ClearGraphIndicators();
    playingAnimation_ = true;
}

void UntangleApplet::Pause()
{
    animationTimer_.Pause();
    playingAnimation_ = false;
}

glm::vec2 UntangleApplet::ToScreen(glm::vec2 worldPos) { return worldPos * zoom_ + pan_; }

glm::vec2 UntangleApplet::ToWorld(glm::vec2 screenPos) { return (screenPos - pan_) / zoom_; }

UntangleApplet::HitInfo UntangleApplet::HitTest(glm::vec2 screenPos)
{
    HitInfo hit;
    hit.worldPos = ToWorld(screenPos);
    hit.nodeId = 0;
    hit.edgeId = 0;

    auto nodeHitRadiusNormal =
        (style_.normal.node.radius + style_.normal.node.lineThickness / 2.0f) * style_.nodeScale +
        style_.nodeHitTestPadding;
    auto nodeHitRadiusSelected =
        (style_.selected.node.radius + style_.selected.node.lineThickness / 2.0f) * style_.nodeScale +
        style_.nodeHitTestPadding;

    float closestPotentialNodeDist = 0;
    for (auto node : graph_.GetNodes()) {
        auto dist = glm::distance(ToScreen(node.pos), screenPos);
        auto hitRadius = node.id == selectedNodeId_ ? nodeHitRadiusSelected : nodeHitRadiusNormal;
        if (dist <= hitRadius && (!hit.nodeId || dist < closestPotentialNodeDist)) {
            hit.nodeId = node.id;
            closestPotentialNodeDist = dist;
        }
    }

    auto edgeHitRadiusNormal = (style_.normal.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding;
    auto edgeHitRadiusSelected =
        (style_.selected.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding;

    float closestPotentialEdgeDist = 0;
    for (auto edge : graph_.GetEdges()) {
        auto dist = PointDistanceToLineSegment(screenPos, ToScreen(graph_.GetNode(edge.nodeIdA).pos),
                                               ToScreen(graph_.GetNode(edge.nodeIdB).pos));
        auto hitRadius = edge.id == selectedEdgeId_ ? edgeHitRadiusSelected : edgeHitRadiusNormal;
        if (dist <= hitRadius && (!hit.edgeId || dist < closestPotentialEdgeDist)) {
            hit.edgeId = edge.id;
            closestPotentialEdgeDist = dist;
        }
    }

    return hit;
}

void UntangleApplet::OnAdd()
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

void UntangleApplet::OnDelete()
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

void UntangleApplet::OnClick()
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
        dragOffset_ = ToWorld(mousePos_) - node.pos;
    }
}

void UntangleApplet::OnMove()
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

void UntangleApplet::OnEndClick() { dragging_ = false; }

UntangleApplet::NodeStyle UntangleApplet::GetNodeStyle(int nodeId)
{
    return (nodeId == selectedNodeId_ ? (nodeId == highlightedNodeId_ ? style_.highlightSelected : style_.selected)
                                      : (nodeId == highlightedNodeId_ ? style_.highlight : style_.normal))
        .node;
}

UntangleApplet::EdgeStyle UntangleApplet::GetEdgeStyle(int edgeId)
{
    return (edgeId == selectedEdgeId_ ? (edgeId == highlightedEdgeId_ ? style_.highlightSelected : style_.selected)
                                      : (edgeId == highlightedEdgeId_ ? style_.highlight : style_.normal))
        .edge;
}

void UntangleApplet::ClearGraphIndicators()
{
    selectedNodeId_ = 0;
    selectedEdgeId_ = 0;
    highlightedNodeId_ = 0;
    highlightedEdgeId_ = 0;
}
