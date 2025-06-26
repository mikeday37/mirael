#include "app_pch.hpp"

#include "app/graph_animators.hpp"
#include <cmath>

void JitterGraphAnimator::OnShowControls()
{
    ImGui::InputFloat("Jitter Magnitude", &jitterMagnitude_);
    ImGui::SliderFloat("Overall Scale", &overallScale_, 0, 1);
    ImGui::SliderFloat("Horizontal Scale", &horizontalScale_, 0, 1);
    ImGui::SliderFloat("Vertical Scale", &verticalScale_, 0, 1);
}

void JitterGraphAnimator::Animate(UntangleAppletGraph &g, seconds worldTime, seconds deltaTime)
{
    unused(worldTime, deltaTime);

    std::uniform_real_distribution<float> jitter(-jitterMagnitude_ * overallScale_, jitterMagnitude_ * overallScale_);

    for (auto node : g.GetNodes()) {
        g.RepositionNode(node.id,
                         {node.data.x + jitter(rng_) * horizontalScale_, node.data.y + jitter(rng_) * verticalScale_});
    }
}

void OrbitGraphAnimator::OnShowControls()
{
    ImGui::Checkbox("Rotate Clockwise", &rotateClockwise_);

    ImGui::Spacing();

    ImGui::Text("Central Mass (kg)");
    ImGui::InputFloat("Min##m", &massMin_, 0, 0, "%.3e");
    ImGui::InputFloat("Max##m", &massMax_, 0, 0, "%.3e");
    ImGui::SliderFloat("Central Mass", &centralMass_, massMin_, massMax_, "%.3e");

    ImGui::Spacing();

    ImGui::Text("Distance Scale (km per world unit)");
    ImGui::InputFloat("Min##v", &distanceScaleMin_, 0, 0, "%.1f");
    ImGui::InputFloat("Max##v", &distanceScaleMax_, 0, 0, "%.1f");
    ImGui::SliderFloat("Distance Scale", &distanceScale_, distanceScaleMin_, distanceScaleMax_, "%.1f");

    ImGui::Spacing();

    ImGui::Text("Time Scale (simulated orbit seconds per real second)");
    ImGui::InputFloat("Min##t", &timeScaleMin_, 0, 0, "%.3f");
    ImGui::InputFloat("Max##t", &timeScaleMax_, 0, 0, "%.3f");
    ImGui::SliderFloat("Time Scale", &timeScale_, timeScaleMin_, timeScaleMax_, "%.3f");
}

void OrbitGraphAnimator::Animate(UntangleAppletGraph &g, seconds worldTime, seconds deltaTime)
{
    unused(worldTime);

    if (timeScale_ == 0.0f) {
        return;
    }

    constexpr float G = 6.67430e-20f; // km^3/kg/s^2

    for (auto &node : g.GetNodes()) {
        glm::vec2 pos = node.data * distanceScale_;
        float r = glm::length(pos);
        if (r == 0.0f) {
            continue;
        }

        // circular orbital velocity
        float v = std::sqrt(G * centralMass_ / r);

        // angular velocity
        float omega = rotateClockwise_ ? -v / r : v / r;

        // angle to rotate in this frame
        float theta = omega * deltaTime * timeScale_;

        // rotate position around origin accordingly
        glm::mat2 rotation{
            std::cos(theta),
            -std::sin(theta),
            std::sin(theta),
            std::cos(theta),
        };
        g.RepositionNode(node.id, rotation * node.data);
    }
}
