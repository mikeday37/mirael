#pragma once

#include "app/applet.hpp"
#include "app/applet_graph_types.hpp"
#include "app/graph.hpp"
#include "app/graph_animators.hpp"
#include "app/graph_interaction_applet.hpp"
#include "app/graph_manipulators.hpp"
#include "app/simulation_timer.hpp"
#include "imgui.h"
#include "vec2.hpp"

struct FractalAppletGraphInteractionPolicy {
    static constexpr bool canSelectNodes = true;
    static constexpr bool canSelectEdges = false;
    static constexpr bool hoverHighlightsNodes = true;
    static constexpr bool hoverHighlightsEdges = true;
};

class FractalApplet : public GraphInteractionApplet<FractalApplet, GraphType::Directed, glm::vec2, Empty,
                                                    FractalAppletGraphInteractionPolicy>
{
public:
    using Base = GraphInteractionApplet<FractalApplet, GraphType::Directed, glm::vec2, Empty,
                                        FractalAppletGraphInteractionPolicy>;

    FractalApplet(App &app) : GraphInteractionApplet(app) {}

    const char *GetDisplayName() const override { return "Fractal"; }

    void OnShowSettingsUI() override;
    void OnShowStyleUI() override;

    void OnEvent(const SDL_Event &e) override;

    void DrawNode(Graphics &g, const Graph::Node &node) const;
    void DrawEdge(Graphics &g, const Graph::Edge &edge) const;

    Base::HitTestSettings GetHitTestSettings() const override;

private:
    void OnRightClick();
    void CycleEdgeType(int fromNodeId, int toNodeId);

    struct NodeStyle {
        float radius;
        float lineThickness;
        ImVec4 fillColor;
        ImVec4 lineColor;
    };

    struct EdgeStyle {
        float lineThickness;
        ImVec4 primaryLineColor;
        ImVec4 secondaryLineColor;
        ImVec4 reversedLineColor;
        ImVec4 cosmeticLineColor;
        float arrowAngle = 25.0f;
        float arrowLength = 25.0f;
        float highlightThicknessPadding = 4.0f;
    };

    struct Style {

        NodeStyle normalNode;
        NodeStyle selectedNode;
        EdgeStyle edge;

        float nodeScale = 0.35f;
        float nodeHitTestPadding = 15.0f;
        float edgeScale = 1.0f;
        float edgeHitTestPadding = 4.0f;
    };

    Style style_ = {.normalNode = {40.0f, 6.0f, {1, 1, 1, 1}, {0, 0, 0, 1}},
                    .selectedNode = {44.0f, 9.0f, {0, 0.5f, 1, 1}, {0, 0, 0, 1}},
                    .edge = {
                        2.0f,
                        {0, 0, 1, 1},         // primary
                        {0, 1, 0, 1},         // secondary
                        {1, 0, 0, 1},         // reversed
                        {0.7f, 0.7f, 0.7f, 1} // cosmetic
                    }};
};
