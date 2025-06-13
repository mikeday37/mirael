#pragma once

#include <string>
#include "app/graph.hpp"

class GraphAnimator {
public:
	virtual ~GraphAnimator() = default;

	virtual std::string_view GetDisplayName() const = 0;
	virtual void OnShowControls() {};

	using seconds = float;
	virtual void Animate(Graph &g, seconds worldTime, seconds deltaTime) = 0;
};

class JitterGraphAnimator : public GraphAnimator {
public:
	std::string_view GetDisplayName() const override {return "Jitter";};
	void OnShowControls() override;

	void Animate(Graph &g, seconds worldTime, seconds deltaTime) override;

private:
	std::mt19937 rng_{std::random_device{}()};
	float jitterMagnitude_ = 0.04f;
	float overallScale_ = 0.1f;
	float horizontalScale_ = 0.5f;
	float verticalScale_ = 0.5f;
};

class OrbitGraphAnimator : public GraphAnimator {
public:
	std::string_view GetDisplayName() const override {return "Orbit";};
	void OnShowControls() override;

	void Animate(Graph &g, seconds worldTime, seconds deltaTime) override;

	using kilograms = float;
	using kilometers = float;

private:
	static constexpr kilograms earthMass = 5.972e24f;
	kilograms massMin_ = 0;
	kilograms massMax_ = 2 * earthMass;
	kilograms centralMass_ = earthMass;

	static constexpr kilometers moonOrbitDistance = 384400;
	kilometers distanceScaleMin_ = 0;
	kilometers distanceScaleMax_ = 2 * moonOrbitDistance;
	kilometers distanceScale_ = moonOrbitDistance;

	static constexpr seconds moonOrbitPeriod = 27.3f * 24 * 60 * 60; // 27.3 days
	static constexpr seconds desiredPeriodAtPerimeter = 60.0f; // one revolution per minute at 1 world unit from origin
	static constexpr seconds defaultTimeScale = desiredPeriodAtPerimeter / moonOrbitPeriod;
	seconds minTimeScale_ = 0;
	seconds maxTimeScale_ = 2 * defaultTimeScale;
	seconds timeScale_ = defaultTimeScale;
};

class UntanglePhysicsGraphAnimator : public GraphAnimator {
public:
	std::string_view GetDisplayName() const override {return "Attempt Untangle via Physics";};
	void OnShowControls() override;

	void Animate(Graph &g, seconds worldTime, seconds deltaTime) override;
};

struct KnownGraphAnimators {
	JitterGraphAnimator jitter;
	OrbitGraphAnimator orbit;
	UntanglePhysicsGraphAnimator untanglePhysics;

	std::vector<GraphAnimator *> GetAll() {return {
		&jitter,
		&orbit,
		&untanglePhysics,
	};}
};
