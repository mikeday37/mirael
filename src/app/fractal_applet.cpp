#include "app_pch.hpp"

#include "app/fractal_applet.hpp"
#include <cmath>
#include <optional>

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
    ImVec4 imColor;
    switch (graph_.EdgeData(edge.id)) {
    case FractalDefinition::LineType::Primary:
        imColor = style_.edge.primaryLineColor;
        break;
    case FractalDefinition::LineType::Secondary:
        imColor = style_.edge.secondaryLineColor;
        break;
    case FractalDefinition::LineType::Reversed:
        imColor = style_.edge.reversedLineColor;
        break;
    case FractalDefinition::LineType::Cosmetic:
        imColor = style_.edge.cosmeticLineColor;
        break;
    }
    Color color = convert(imColor);
    g.Line(posA, posB, lineThickness, color);
    g.LineArrowEnd(posA, posB, lineThickness, color, edgeStyle.arrowAngle, edgeStyle.arrowLength * style_.edgeScale);
}

void FractalApplet::OnRenderBackground(Graphics &g)
{
    if (auto result = TryGetFractalDefinition(); result)
        RenderFractal(g, *result);

    Base::OnRenderBackground(g);
}

void FractalApplet::OnShowSettingsUI()
{
    ImGui::SliderInt("Max Depth", &maxDepth_, 1, 20);
    ImGui::SliderInt("Max Rendered Lines", &maxLines_, 1, 100000);
}

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
        ImGui::SliderFloat("Line Thickness", &style_.edge.lineThickness, 0.0f, 20.0f, "%.1f");
        ImGui::ColorEdit4("Primary Line Color", (float *)&style_.edge.primaryLineColor, ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Secondary Line Color", (float *)&style_.edge.secondaryLineColor,
                          ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Reversed Line Color", (float *)&style_.edge.reversedLineColor, ImGuiColorEditFlags_AlphaBar);
        ImGui::ColorEdit4("Cosmetic Line Color", (float *)&style_.edge.cosmeticLineColor, ImGuiColorEditFlags_AlphaBar);
        ImGui::SliderFloat("Arrow Angle", &style_.edge.arrowAngle, 0.0f, 90.0f, "%.0f");
        ImGui::SliderFloat("Arrow Length", &style_.edge.arrowLength, 0.0f, 100.0f, "%.1f");
        ImGui::SliderFloat("Highlight Thickness Padding", &style_.edge.highlightThicknessPadding, 0.0f, 20.0f, "%.1f");
        ImGui::TreePop();
    }
    if (ImGui::TreeNodeEx("Fractal", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
        ImGui::SliderFloat("Line Thickness", &style_.fractal.lineThickness, 0.0f, 20.0f, "%.1f");
        ImGui::ColorEdit4("Line Color", (float *)&style_.fractal.lineColor, ImGuiColorEditFlags_AlphaBar);
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

void FractalApplet::OnRightClick()
{
    auto hit = HitTest(GetMousePosition());

    if (hit.nodeId && SelectedNodeId() && hit.nodeId != SelectedNodeId()) {
        CycleEdgeType(SelectedNodeId(), hit.nodeId);
    }
}

void FractalApplet::CycleEdgeType(int fromNodeId, int toNodeId)
{
    if (!graph_.HasEdges()) {
        graph_.AddEdge(fromNodeId, toNodeId, FractalDefinition::LineType::Primary);
    } else if (!graph_.ContainsEdge(fromNodeId, toNodeId)) {
        graph_.AddEdge(fromNodeId, toNodeId, FractalDefinition::LineType::Secondary);
    } else {
        auto edge = graph_.GetEdge(fromNodeId, toNodeId);
        auto &lineType = graph_.EdgeData(edge.id);
        switch (lineType) {
        case FractalDefinition::LineType::Secondary:
            lineType = FractalDefinition::LineType::Reversed;
            break;
        case FractalDefinition::LineType::Reversed:
            lineType = FractalDefinition::LineType::Cosmetic;
            break;
        case FractalDefinition::LineType::Cosmetic:
            lineType = FractalDefinition::LineType::Primary;
            break;
        case FractalDefinition::LineType::Primary:
            graph_.RemoveEdge(edge.id);
            break;
        }
    }
}

std::optional<FractalDefinition> FractalApplet::TryGetFractalDefinition() const
{
    int primaryCount = 0, controlCount = 0, cosmeticCount = 0;

    for (const auto &edge : graph_.Edges()) {
        switch (edge.data) {
        case FractalDefinition::LineType::Secondary:
            [[fallthrough]];
        case FractalDefinition::LineType::Reversed:
            controlCount++;
            break;
        case FractalDefinition::LineType::Primary:
            primaryCount++;
            [[fallthrough]]; // primary counts as cosmetic as well
        case FractalDefinition::LineType::Cosmetic:
            cosmeticCount++;
            break;
        }
    }

    // there must be exactly 1 primary and at least one control
    const bool isValid = primaryCount == 1 && controlCount >= 1;
    if (!isValid)
        return std::nullopt;

    FractalDefinition def;
    def.controls.reserve(controlCount);
    def.cosmetics.reserve(cosmeticCount);

    for (const auto &edge : graph_.Edges()) {
        auto from = ToScreen(graph_.NodeData(edge.nodeIdA));
        auto to = ToScreen(graph_.NodeData(edge.nodeIdB));
        switch (edge.data) {
        case FractalDefinition::LineType::Primary:
            def.primary = {.from = from, .to = to};
            [[fallthrough]];
        case FractalDefinition::LineType::Cosmetic:
            def.cosmetics.push_back({.from = from, .to = to});
            break;
        case FractalDefinition::LineType::Secondary:
            def.controls.push_back({.from = from, .to = to, .reversed = false});
            break;
        case FractalDefinition::LineType::Reversed:
            def.controls.push_back({.from = from, .to = to, .reversed = true});
            break;
        }
    }

    assert(def.controls.size() == controlCount);
    assert(def.cosmetics.size() == cosmeticCount);

    return def;
}

void FractalApplet::RenderFractal(Graphics &g, const FractalDefinition &def)
{
    // prepare drawing helper
    auto width = style_.fractal.lineThickness;
    auto color = convert(style_.fractal.lineColor);
    auto drawLine = [this, &g, width, color](glm::vec2 from, glm::vec2 to) { g.Line(from, to, width, color); };

    // calculate one transform per control
    std::vector<glm::mat3> transforms;
    transforms.reserve(def.controls.size());
    for (const auto &control : def.controls) {
        transforms.emplace_back(
            CalculateMappingTransform(def.primary.from, def.primary.to, control.from, control.to, control.reversed));
    }

    // establish a safety exit (user controllable within reason)
    // TODO: these are temporary measures - the real fix is rendering in a background thread with cancel token - TBD
    int linesDrawn = 0;
    const int lineHardLimit = 500000; // just in case the user inputs something enormous
    const int lineLimit = std::min(maxLines_, lineHardLimit);

    // recursively draw
    auto step = [&](auto &self, int depth, glm::mat3 curTransform) {
        for (const auto &cosmetic : def.cosmetics) {
            auto from = glm::vec2(curTransform * glm::vec3(cosmetic.from, 1.0f));
            auto to = glm::vec2(curTransform * glm::vec3(cosmetic.to, 1.0f));
            drawLine(from, to);
            linesDrawn++;
        }

        if (depth >= maxDepth_ || linesDrawn > lineLimit) {
            return;
        }

        for (const auto &transform : transforms) {
            self(self, depth + 1, transform * curTransform);
        };
    };

    step(step, 0, glm::mat3(1.0f));
}