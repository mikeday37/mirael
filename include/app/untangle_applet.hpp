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

struct UntangleAppletGraphInteractionPolicy {

    static constexpr bool canSelectNodes = true;
    static constexpr bool canSelectEdges = true;
    static constexpr bool hoverHighlightsNodes = true;
    static constexpr bool hoverHighlightsEdges = true;
};

class UntangleApplet : public GraphInteractionApplet<UntangleApplet, GraphType::Undirected, glm::vec2, Empty,
                                                     UntangleAppletGraphInteractionPolicy>
{
    friend struct UntangleAppletGraphInteractionPolicy;

public:
    using Base = GraphInteractionApplet<UntangleApplet, GraphType::Undirected, glm::vec2, Empty,
                                        UntangleAppletGraphInteractionPolicy>;

    UntangleApplet(App &app) : GraphInteractionApplet(app) {}

    const char *GetDisplayName() const override { return "Untangle"; }

    void OnShowSettingsUI() override;
    void OnShowStyleUI() override;

    void OnEvent(const SDL_Event &e) override;
    void OnNewFrame() override;

    void DrawNode(Graphics &g, const Graph::Node &node) const;
    void DrawEdge(Graphics &g, const Graph::Edge &edge) const;

    Base::HitTestSettings GetHitTestSettings() const override;

private:
    // graph manipulation
    KnownGraphManipulators graphManipulators_;

    // graph animation
    KnownGraphAnimators graphAnimators_;
    bool playingAnimation_ = false;
    GraphAnimator *currentAnimator_ = nullptr;
    SimulationTimer animationTimer_;
    void SetCurrentAnimator(GraphAnimator *animator);
    void Play();
    void Pause();

    // visual style
    struct NodeStyle {
        float radius;
        float lineThickness;
        ImVec4 fillColor;
        ImVec4 lineColor;
    };

    struct EdgeStyle {
        float lineThickness;
        ImVec4 lineColor;
    };

    struct GraphPartStyle {
        NodeStyle node;
        EdgeStyle edge;
    };

    struct GraphStyle {
        GraphPartStyle normal;
        GraphPartStyle selected;
        GraphPartStyle highlight;
        GraphPartStyle highlightSelected;

        float nodeScale = 0.1f;
        float nodeHitTestPadding = 15.0f;
        float edgeScale = 1.0f;
        float edgeHitTestPadding = 4.0f;

        // only relevant to directed graphs:
        float arrowAngle = 25.0f;
        float arrowLength = 25.0f;
    };

    GraphStyle style_ = {{
                             // ---- normal ----
                             {40.0f, 6.0f, {1, 1, 1, 1}, {0, 0, 0, 1}}, // node
                             {2.0f, {1, 0.635f, 0.161f, 0.761f}}        // edge
                         },
                         {
                             // ---- selected ----
                             {46.0f, 12.0f, {0, 0, 1, 1}, {0, 0, 0.25f, 1}}, // node
                             {4.0f, {0.63f, 0.63f, 1.0, 1}}                  // edge
                         },
                         {
                             // ---- highlight ----
                             {43.0f, 9.0f, {0, 1, 0, 1}, {0, 0.25f, 0, 1}}, // node
                             {3.0f, {0.63f, 1, 0.63f, 1}}                   // edge
                         },
                         {
                             // ---- highlightSelected ----
                             {49.0f, 15.0f, {0, 1, 1, 1}, {0, 0.25f, 0.25f, 1}}, // node
                             {5.0f, {0.63f, 1, 1, 1}}                            // edge
                         }};

    NodeStyle GetNodeStyle(int nodeId) const;
    EdgeStyle GetEdgeStyle(int edgeId) const;
};
