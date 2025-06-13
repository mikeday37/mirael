#include "app/graph_animators.hpp"


void JitterGraphAnimator::OnShowControls() {
	ImGui::InputFloat("Jitter Magnitude", &jitterMagnitude_);
	ImGui::SliderFloat("Overall Scale", &overallScale_, 0, 1);
	ImGui::SliderFloat("Horizontal Scale", &horizontalScale_, 0, 1);
	ImGui::SliderFloat("Vertical Scale", &verticalScale_, 0, 1);
}

void JitterGraphAnimator::Animate(Graph &g, seconds worldTime, seconds deltaTime) {

	std::uniform_real_distribution<float> jitter(-jitterMagnitude_ * overallScale_, jitterMagnitude_ * overallScale_);

	for (auto node : g.GetNodes()) {
		g.RepositionNode(node.id, {
			node.pos.x + jitter(rng_) * horizontalScale_,
			node.pos.y + jitter(rng_) * verticalScale_
		});
	}
}

void OrbitGraphAnimator::OnShowControls() {

}

void OrbitGraphAnimator::Animate(Graph &g, seconds worldTime, seconds deltaTime) {
}

void UntanglePhysicsGraphAnimator::OnShowControls() {

}

void UntanglePhysicsGraphAnimator::Animate(Graph &g, seconds worldTime, seconds deltaTime) {
}
