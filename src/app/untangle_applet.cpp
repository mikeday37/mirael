#include "app_pch.hpp"

#include "app/applet_graph_types.hpp"
#include "app/untangle_applet.hpp"

void UntangleApplet::OnShowSettingsUI()
{
    if (ImGui::TreeNodeEx("Graph Manipulators", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        if (ImGui::BeginTable("##GraphManipulators", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
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
    if (ImGui::TreeNodeEx("Graph Animators", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        if (playingAnimation_) {
            if (ImGui::Button("Pause")) {
                Pause();
            }
        } else {
            if (ImGui::Button("Play")) {
                Play();
            }
        }
        if (ImGui::BeginTable("##GraphAnimators", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
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
}

void UntangleApplet::OnEvent(const SDL_Event &e)
{
    switch (e.type) {
    case SDL_KEYDOWN:
        switch (e.key.keysym.sym) {

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

    default:
        // this applet doesn't care about other events
        break;
    }

    Base::OnEvent(e);
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

UntangleApplet::NodeStyle UntangleApplet::GetNodeStyle(int nodeId) const
{
    return (nodeId == SelectedNode() ? (nodeId == HighlightedNode() ? style_.highlightSelected : style_.selected)
                                     : (nodeId == HighlightedNode() ? style_.highlight : style_.normal))
        .node;
}

UntangleApplet::EdgeStyle UntangleApplet::GetEdgeStyle(int edgeId) const
{
    return (edgeId == SelectedEdge() ? (edgeId == HighlightedEdge() ? style_.highlightSelected : style_.selected)
                                     : (edgeId == HighlightedEdge() ? style_.highlight : style_.normal))
        .edge;
}

void UntangleApplet::DrawNode(Graphics &g, const Graph::Node &node) const
{
    auto pos = ToScreen(node.data);
    auto style = GetNodeStyle(node.id);
    g.Circle(pos, style.radius * style_.nodeScale, style.lineThickness * style_.nodeScale, convert(style.fillColor),
             convert(style.lineColor));
}

void UntangleApplet::DrawEdge(Graphics &g, const Graph::Edge &edge) const
{
    auto nodeA = graph_.GetNode(edge.nodeIdA);
    auto nodeB = graph_.GetNode(edge.nodeIdB);
    auto posA = ToScreen(nodeA.data);
    auto posB = ToScreen(nodeB.data);
    auto style = GetEdgeStyle(edge.id);
    g.Line(posA, posB, style.lineThickness * style_.edgeScale, convert(style.lineColor));
}

/*
        if constexpr (TType == GraphType::Directed) {
        g.LineArrowEnd(posA, posB, style.lineThickness * style_.edgeScale, convert(style.lineColor), style_.arrowAngle,
                    style_.arrowLength);
    }


*/

void UntangleApplet::OnShowStyleUI()
{

    ImGui::SliderFloat("Node Scale", &style_.nodeScale, 0.0f, 2.0f, "%.3f");
    ImGui::SliderFloat("Node Hit Test Padding", &style_.nodeHitTestPadding, 0.0f, 50.0f, "%.3f");
    ImGui::SliderFloat("Edge Scale", &style_.edgeScale, 0.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("Edge Hit Test Padding", &style_.edgeHitTestPadding, 0.0f, 50.0f, "%.3f");
    /*if constexpr (TType == GraphType::Directed) {
        ImGui::SliderFloat("Line End Arrow Angle", &style_.arrowAngle, 0.0f, 90.0f, "%.0f");
        ImGui::SliderFloat("Line End Arrow Length", &style_.arrowLength, 0.0f, 100.0f, "%.1f");
    }*/

    auto stateStyle = [](const char *name, const char *suffix, GraphPartStyle &part) -> void {
        if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
            if (ImGui::TreeNodeEx(std::format("Node##{}", suffix).c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                ImGui::SliderFloat(std::format("Radius##{}", suffix).c_str(), &part.node.radius, 0.0f, 50.0f, "%.1f");
                ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.node.lineThickness, 0.0f,
                                   20.0f, "%.1f");
                ImGui::ColorEdit4(std::format("Fill Color##{}", suffix).c_str(), (float *)&part.node.fillColor,
                                  ImGuiColorEditFlags_AlphaBar);
                ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float *)&part.node.lineColor,
                                  ImGuiColorEditFlags_AlphaBar);
                ImGui::TreePop();
            }
            if (ImGui::TreeNodeEx(std::format("Edge##{}", suffix).c_str(),
                                  ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.edge.lineThickness, 0.0f,
                                   20.0f, "%.1f");
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
}

UntangleApplet::Base::HitTestSettings UntangleApplet::GetHitTestSettings() const
{
    return {.nodeHitRadiusNormal =
                (style_.normal.node.radius + style_.normal.node.lineThickness / 2.0f) * style_.nodeScale +
                style_.nodeHitTestPadding,
            .nodeHitRadiusSelected =
                (style_.selected.node.radius + style_.selected.node.lineThickness / 2.0f) * style_.nodeScale +
                style_.nodeHitTestPadding,
            .edgeHitRadiusNormal =
                (style_.normal.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding,
            .edgeHitRadiusSelected =
                (style_.selected.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding};
}