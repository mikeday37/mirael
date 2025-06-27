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

class UntangleApplet : public GraphInteractionApplet<GraphType::Undirected, glm::vec2, Empty>
{
public:
    using Base = GraphInteractionApplet<GraphType::Undirected, glm::vec2, Empty>;

    UntangleApplet(App &app) : GraphInteractionApplet(app) {}

    const char *GetDisplayName() const override { return "Untangle"; }

    void OnShowDerivedAppletControls() override;

    void OnEvent(const SDL_Event &e) override;
    void OnNewFrame() override;

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
};
