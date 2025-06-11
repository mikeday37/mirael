#pragma once

#include "app/applet.hpp"
#include "graph.hpp"
#include "vec2.hpp"
#include "imgui.h"

class UntangleApplet : public Applet {
public:
    UntangleApplet(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Untangle";}

	void OnRenderBackground(Graphics &g) override;
	void OnShowControls() override;
	void OnEvent(const SDL_Event &e) override;

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
	bool dragging_;
	glm::vec2 dragStart_;
	glm::vec2 mousePos_;
	Graph graph;

	// coordinate handling
	glm::vec2 ToScreen(glm::vec2 worldPos); // TODO: make inline
	glm::vec2 ToWorld(glm::vec2 screenPos);

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
	void OnAdd(glm::vec2 pos);
	void OnDelete(glm::vec2 pos);
	void OnClick(glm::vec2 pos);
	void OnDrag(glm::vec2 pos);

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
	};

	GraphStyle style_ = {
		{ // ---- normal ----
			{10.0f, 2.0f, {1,1,1,1}, {0,0,0,1}}, // node
			{2.0f, {0,0,0,1}} // edge
		},
		{ // ---- selected ----
			{12.0f, 4.0f, {0,0,1,1}, {0,0,0.25f,1}}, // node
			{4.0f, {0,0,0.6f,1}} // edge
		},
		{ // ---- highlight ----
			{11.0f, 3.0f, {0,1,0,1}, {0,0.25f,0,1}}, // node
			{3.0f, {0,0.6f,0,1}} // edge
		},
		{ // ---- highlightSelected ----
			{13.0f, 5.0f, {0,1,1,1}, {0,0.25f,0.25f,1}}, // node
			{5.0f, {0,0.6f,0.6f,1}} // edge
		}
	};

	NodeStyle GetNodeStyle(int nodeId);
	EdgeStyle GetEdgeStyle(int edgeId);
};
