#pragma once

#include "app/graph.hpp"

#include <string>
#include <vector>

// Each of the following GraphManipulations is a singleton.
// The KnownGraphManipulations type is meant to be a direct memeber variable of the UntangleApplet.
// These classes should not appear elsewhere.

class GraphManipulation {
public:
	virtual std::string_view GetDisplayName() const = 0;

	virtual void OnShowControls() {};
	virtual void ManipulateGraph(Graph &g) = 0;
};

class GenerateRandomGraphManipulation : public GraphManipulation {
public:
	std::string_view GetDisplayName() const override {return "Generate Random";}

	void OnShowControls() override;
	void ManipulateGraph(Graph &g) override;

private:
	int nodeCount_ = 10;
	int edgeCount_ = 25;
	int maxEdgeTries_ = 15;
	float scale_ = 0.9f;
};

class GenerateGridGraphManipulation : public GraphManipulation {
public:
	std::string_view GetDisplayName() const override {return "Generate Grid";}

	void OnShowControls() override;
	void ManipulateGraph(Graph &g) override;

private:
	int width_ = 10;
	int height_ = 10;
	float scale_ = 0.9f;
	bool includeEdges_ = true;
};

class TangleGraphManipulation : public GraphManipulation {
public:
	std::string_view GetDisplayName() const override {return "Tangle";};

	void OnShowControls() override;
	void ManipulateGraph(Graph &g) override;

private:
	enum class Shape {
		Random = 0,
		Circle = 1
	};
	inline static constexpr const char* ShapeNames[] = {
		"Random",
		"Circle"
	};
	Shape shape_ = Shape::Circle;
	float scale_ = 0.9f;
};

class RemoveEdgesGraphManipulation : public GraphManipulation {
public:
	std::string_view GetDisplayName() const override {return "Remove Edges";}

	void OnShowControls() override;
	void ManipulateGraph(Graph &g) override;

private:
	float percentage_ = 0.1f;
};

struct KnownGraphManipulations {
	GenerateRandomGraphManipulation random;
	GenerateGridGraphManipulation grid;
	TangleGraphManipulation tangle;
	RemoveEdgesGraphManipulation removeEdges;

	std::vector<GraphManipulation *> GetAll() {return {
		&random,
		&grid,
		&tangle,
		&removeEdges,
	};}
};
