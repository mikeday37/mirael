#include "app_pch.hpp"

#include "app/fractal_applet.hpp"

void FractalApplet::DrawNode(Graphics &g, const Graph::Node &node) const
{
    auto pos = ToScreen(node.data);
    const auto &nodeStyle = SelectedNodeId() == node.id ? style_.selectedNode : style_.normalNode;
    g.Circle(pos, nodeStyle.radius * style_.nodeScale, nodeStyle.lineThickness * style_.nodeScale,
             convert(nodeStyle.fillColor), convert(nodeStyle.lineColor));
}

void FractalApplet::DrawEdge(Graphics &g, const Graph::Edge &edge) const
{
    auto nodeA = graph_.GetNode(edge.nodeIdA);
    auto nodeB = graph_.GetNode(edge.nodeIdB);
    auto posA = ToScreen(nodeA.data);
    auto posB = ToScreen(nodeB.data);
    const auto &edgeStyle = style_.edge;
    float lineThickness = edgeStyle.lineThickness * style_.edgeScale;
    if (edge.id == HighlightedEdgeId()) {
        lineThickness += edgeStyle.highlightThicknessPadding;
    }
    g.Line(posA, posB, lineThickness, convert(edgeStyle.primaryLineColor));
    g.LineArrowEnd(posA, posB, lineThickness, convert(edgeStyle.primaryLineColor), edgeStyle.arrowAngle,
                   edgeStyle.arrowLength * style_.edgeScale);
}

void FractalApplet::OnShowSettingsUI() {}

void FractalApplet::OnShowStyleUI()
{
    ImGui::SliderFloat("Node Scale", &style_.nodeScale, 0.0f, 2.0f, "%.3f");
    ImGui::SliderFloat("Node Hit Test Padding", &style_.nodeHitTestPadding, 0.0f, 50.0f, "%.3f");
    ImGui::SliderFloat("Edge Scale", &style_.edgeScale, 0.0f, 10.0f, "%.3f");
    ImGui::SliderFloat("Edge Hit Test Padding", &style_.edgeHitTestPadding, 0.0f, 50.0f, "%.3f");

    if (ImGui::TreeNodeEx("Node", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        auto stateStyle = [](const char *name, const char *suffix, NodeStyle &style) -> void {
            if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
                ImGui::SliderFloat(std::format("Radius##{}", suffix).c_str(), &style.radius, 0.0f, 50.0f, "%.1f");
                ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &style.lineThickness, 0.0f, 20.0f,
                                   "%.1f");
                ImGui::ColorEdit4(std::format("Fill Color##{}", suffix).c_str(), (float *)&style.fillColor,
                                  ImGuiColorEditFlags_AlphaBar);
                ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float *)&style.lineColor,
                                  ImGuiColorEditFlags_AlphaBar);
                ImGui::TreePop();
            }
        };
        stateStyle("Normal", "n", style_.normalNode);
        stateStyle("Selected", "s", style_.selectedNode);
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Edge", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        ImGui::SliderFloat("Line Thickness##{}", &style_.edge.lineThickness, 0.0f, 20.0f, "%.1f");
        ImGui::ColorEdit4("Primary Line Color##{}", (float *)&style_.edge.primaryLineColor,
                          ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Secondary Line Color##{}", (float *)&style_.edge.secondaryLineColor,
                          ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Reversed Line Color##{}", (float *)&style_.edge.reversedLineColor,
                          ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Cosmetic Line Color##{}", (float *)&style_.edge.cosmeticLineColor,
                          ImGuiColorEditFlags_AlphaBar);
        ImGui::SliderFloat("Arrow Angle", &style_.edge.arrowAngle, 0.0f, 90.0f, "%.0f");
        ImGui::SliderFloat("Arrow Length", &style_.edge.arrowLength, 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Highlight Thickness Padding##{}", &style_.edge.highlightThicknessPadding, 0.0f, 20.0f,
                           "%.1f");
        ImGui::TreePop();
    }
}

void FractalApplet::OnEvent(const SDL_Event &e)
{
    Base::OnEvent(e);

    switch (e.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_RIGHT) {
            OnRightClick();
        };
        break;
    }
}

void FractalApplet::OnRightClick()
{
    auto hit = HitTest(GetMousePosition());

    if (hit.nodeId && SelectedNodeId() && hit.nodeId != SelectedNodeId()) {
        CycleEdgeType(SelectedNodeId(), hit.nodeId);
    }
}

void FractalApplet::CycleEdgeType(int fromNodeId, int toNodeId) {}

FractalApplet::Base::HitTestSettings FractalApplet::GetHitTestSettings() const
{
    return {.nodeHitRadiusNormal =
                (style_.normalNode.radius + style_.normalNode.lineThickness / 2.0f) * style_.nodeScale +
                style_.nodeHitTestPadding,
            .nodeHitRadiusSelected =
                (style_.selectedNode.radius + style_.selectedNode.lineThickness / 2.0f) * style_.nodeScale +
                style_.nodeHitTestPadding,
            .edgeHitRadiusNormal = (style_.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding,
            .edgeHitRadiusSelected = (style_.edge.lineThickness / 2.0f) * style_.edgeScale + style_.edgeHitTestPadding};
}