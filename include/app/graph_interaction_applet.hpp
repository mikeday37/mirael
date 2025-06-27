#pragma once

#include "app/applet.hpp"
#include "app/graph.hpp"
#include "imgui.h"
#include "vec2.hpp"

template <GraphType TType, typename TNodeData, typename TEdgeData> class GraphInteractionApplet : public Applet
{
    static_assert(std::is_same_v<TNodeData, glm::vec2>, "TNodeData must currently be glm::vec2.");

public:
    GraphInteractionApplet(App &app) : Applet(app) {}

    using Graph = Graph<TType, TNodeData, TEdgeData>;

    void OnRenderBackground(Graphics &g) override;
    void OnShowControls() override;
    void OnEvent(const SDL_Event &e) override;

protected:
    // graph
    Graph graph_;

    // derived UI handling
    virtual void OnShowDerivedAppletControls() {}
    void ClearGraphIndicators();

    // coordinate handling
    glm::vec2 ToScreen(glm::vec2 worldPos);
    glm::vec2 ToWorld(glm::vec2 screenPos);

private:
    // controls
    bool autoSize_ = true;
    glm::vec2 pan_ = {0, 0};
    float zoom_ = 1.0f;

    // ui state
    int selectedNodeId_ = 0;
    int highlightedNodeId_ = 0;
    int selectedEdgeId_ = 0;
    int highlightedEdgeId_ = 0;
    bool dragging_ = false;
    glm::vec2 dragOffset_;
    glm::vec2 mousePos_ = {-1, -1};

    // drawing helpers
    void DrawNode(Graphics &g, const Graph::Node &node);
    void DrawEdge(Graphics &g, const Graph::Edge &edge);

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

    NodeStyle GetNodeStyle(int nodeId);
    EdgeStyle GetEdgeStyle(int edgeId);
};

#include "app/graph_interaction_applet.tpp"
