#pragma once

#include "app/applet.hpp"
#include "app/graph.hpp"
#include "applet_graph_types.hpp"
#include "imgui.h"
#include "vec2.hpp"

struct DefaultGraphInteractionPolicy {
    // Note: if a graph aspect (nodes or edges) supports neither selection nor hover highlighting, then it won't
    // be involved in hit-testing, which can greatly improve performance.
    static constexpr bool canSelectNodes = true;
    static constexpr bool canSelectEdges = true;
    static constexpr bool hoverHighlightsNodes = true;
    static constexpr bool hoverHighlightsEdges = true;
};

// TODO: add concepts and static asserts to verify DrawNode() and DrawEdge() have been defined by the derived.

// This class uses the CRTP and Policy patterns for zero-cost extensibility and configurability.
template <typename TDerived, GraphType TGraphType, typename TNodeData, typename TEdgeData,
          typename TInteractionPolicy = DefaultGraphInteractionPolicy>
class GraphInteractionApplet : public Applet
{
    static_assert(std::is_same_v<TNodeData, glm::vec2>, "TNodeData must currently be glm::vec2.");

public:
    GraphInteractionApplet(App &app) : Applet(app) {}

    using Graph = Graph<TGraphType, TNodeData, TEdgeData>;

    void OnRenderBackground(Graphics &g) override;
    void OnShowControls() override;
    void OnEvent(const SDL_Event &e) override;

protected:
    // graph
    Graph graph_;

    // derived UI handling
    virtual void OnShowSettingsUI() {}
    virtual void OnShowStyleUI() {}
    void ClearGraphIndicators();

    // coordinate handling
    glm::vec2 ToScreen(glm::vec2 worldPos) const;
    glm::vec2 ToWorld(glm::vec2 screenPos) const;

    int &SelectedNodeId() { return selectedNodeId_; }
    int &HighlightedNodeId() { return highlightedNodeId_; }
    int &SelectedEdgeId() { return selectedEdgeId_; }
    int &HighlightedEdgeId() { return highlightedEdgeId_; }

    const int &SelectedNodeId() const { return selectedNodeId_; }
    const int &HighlightedNodeId() const { return highlightedNodeId_; }
    const int &SelectedEdgeId() const { return selectedEdgeId_; }
    const int &HighlightedEdgeId() const { return highlightedEdgeId_; }

    glm::vec2 GetMousePosition() const { return mousePos_; }

    struct HitTestSettings {
        float nodeHitRadiusNormal;
        float nodeHitRadiusSelected;
        float edgeHitRadiusNormal;
        float edgeHitRadiusSelected;
    };
    virtual HitTestSettings GetHitTestSettings() const = 0;

    // hit testing
    struct HitInfo {
        glm::vec2 worldPos;
        int nodeId;
        int edgeId;
    };
    HitInfo HitTest(glm::vec2 screenPos) const;

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

    // drawing
    void DrawGraph(Graphics &g);

    // ui handling
    void OnAdd();
    void OnDelete();
    void OnClick();
    void OnMove();
    void OnEndClick();
};

#include "app/graph_interaction_applet.tpp"
