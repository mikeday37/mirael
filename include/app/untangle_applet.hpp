#pragma once

#include "app/applet.hpp"
#include "app/graph.hpp"
#include "app/graph_animators.hpp"
#include "app/graph_manipulators.hpp"
#include "app/simulation_timer.hpp"
#include "imgui.h"
#include "vec2.hpp"

class UntangleApplet : public Applet
{
public:
    UntangleApplet(App &app) : Applet(app) {}

    const char *GetDisplayName() const override { return "Untangle"; }

    void OnRenderBackground(Graphics &g) override;
    void OnShowControls() override;
    void OnEvent(const SDL_Event &e) override;
    void OnNewFrame() override;

private:
    // controls
    bool autoSize_ = true;
    glm::vec2 pan_ = {0, 0};
    float zoom_ = 1.0f;

    // graph
    UndirectedGraph<> graph_;
    KnownGraphManipulators graphManipulators_;

    // animation
    KnownGraphAnimators graphAnimators_;
    bool playingAnimation_ = false;
    GraphAnimator *currentAnimator_ = nullptr;
    SimulationTimer animationTimer_;
    void SetCurrentAnimator(GraphAnimator *animator);
    void Play();
    void Pause();

    // ui state
    int selectedNodeId_ = 0;
    int highlightedNodeId_ = 0;
    int selectedEdgeId_ = 0;
    int highlightedEdgeId_ = 0;
    bool dragging_ = false;
    glm::vec2 dragOffset_;
    glm::vec2 mousePos_ = {-1, -1};

    // coordinate handling
    glm::vec2 ToScreen(glm::vec2 worldPos); // TODO: make inline
    glm::vec2 ToWorld(glm::vec2 screenPos);

    // drawing helpers
    void DrawNode(Graphics &g, const UndirectedGraph<>::Node &node);
    void DrawEdge(Graphics &g, const UndirectedGraph<>::Edge &edge);

    // hit testing
    struct HitInfo {
        glm::vec2 worldPos;
        int nodeId;
        int edgeId;
    };
    HitInfo HitTest(glm::vec2 screenPos);

    // ui handling
    void OnAdd();
    void OnDelete();
    void OnClick();
    void OnMove();
    void OnEndClick();

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

    NodeStyle GetNodeStyle(int nodeId);
    EdgeStyle GetEdgeStyle(int edgeId);
    void ClearGraphIndicators();
};
