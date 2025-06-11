#include "app_pch.hpp"

#include "app/untangleapplet.hpp"

inline Color convert(ImVec4 color) {
	return {color.x, color.y, color.z, color.w};
}

void UntangleApplet::OnRenderBackground(Graphics &g) {
	if (autoSize_) {
		auto size = GetWindowSize();
		pan_ = {size.x / 2.0f, size.y / 2.0f};
		zoom_ = size.y / 2.0f;
	}

	// TODO: modify Graph to use iterators

	const Graph::Edge *selectedEdge = nullptr;
	const Graph::Edge *highlightedEdge = nullptr;
	for (const auto &edge : graph.GetEdges()) {
		if (edge.id == selectedEdgeId_)
			selectedEdge = &edge;
		if (edge.id == highlightedEdgeId_)
			highlightedEdge = &edge;
		if (edge.id == selectedEdgeId_ || edge.id == highlightedEdgeId_)
			continue;
		DrawEdge(g, edge);
	}
	if (selectedEdge && selectedEdge != highlightedEdge)
		DrawEdge(g, *selectedEdge);
	if (highlightedEdge)
		DrawEdge(g, *highlightedEdge);

	const Graph::Node *selectedNode = nullptr;
	const Graph::Node *highlightedNode = nullptr;
	for (const auto &node : graph.GetNodes()) {
		if (node.id == selectedNodeId_)
			selectedNode = &node;
		if (node.id == highlightedNodeId_)
			highlightedNode = &node;
		if (node.id == selectedNodeId_ || node.id == highlightedNodeId_)
			continue;
		DrawNode(g, node);
	}
	if (selectedNode && selectedNode != highlightedNode)
		DrawNode(g, *selectedNode);
	if (highlightedNode)
		DrawNode(g, *highlightedNode);
}

void UntangleApplet::DrawNode(Graphics &g, const Graph::Node &node)
{
	auto pos = ToScreen(node.pos);
	auto style = GetNodeStyle(node.id);
	g.Circle(pos, style.radius, style.lineThickness, convert(style.fillColor), convert(style.lineColor));
}

void UntangleApplet::DrawEdge(Graphics &g, const Graph::Edge &edge)
{
	auto nodeA = graph.GetNode(edge.nodeIdA);
	auto nodeB = graph.GetNode(edge.nodeIdB);
	auto posA = ToScreen(nodeA.pos);
	auto posB = ToScreen(nodeB.pos);
	auto style = GetEdgeStyle(edge.id);
	g.Line(posA, posB, style.lineThickness, convert(style.lineColor));
}

void UntangleApplet::OnShowControls() {
	if (ImGui::BeginTabBar("##UntangleTabs")) {

		if (ImGui::BeginTabItem("Settings")) {
			ImGui::Checkbox("Auto-Size Window", &autoSize_);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Style")) {
			auto stateStyle = [this](const char *name, const char *suffix, GraphPartStyle &part) -> void {
				if (ImGui::TreeNodeEx(name, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
					if (ImGui::TreeNodeEx(std::format("Node##{}", suffix).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
						ImGui::SliderFloat(std::format("Radius##{}", suffix).c_str(), &part.node.radius, 0.0f, 50.0f, "%.1f");
						ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.node.lineThickness, 0.0f, 20.0f, "%.1f");
						ImGui::ColorEdit4(std::format("Fill Color##{}", suffix).c_str(), (float*)&part.node.fillColor);
						ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float*)&part.node.lineColor);
						ImGui::TreePop();
					}
					if (ImGui::TreeNodeEx(std::format("Edge##{}", suffix).c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_DrawLinesFull)) {
						ImGui::SliderFloat(std::format("Line Thickness##{}", suffix).c_str(), &part.edge.lineThickness, 0.0f, 20.0f, "%.1f");
						ImGui::ColorEdit4(std::format("Line Color##{}", suffix).c_str(), (float*)&part.edge.lineColor);
						ImGui::TreePop();
					}
					ImGui::TreePop();
				}
			};
			stateStyle("Normal", "n", style_.normal);
			stateStyle("Selected", "s", style_.selected);
			stateStyle("Highlight", "h", style_.highlight);
			stateStyle("Highlight Selected", "hs", style_.highlightSelected);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void UntangleApplet::OnEvent(const SDL_Event &e) {
	switch (e.type) {
		case SDL_KEYDOWN:
			switch (e.key.keysym.sym) {
				case SDLK_a:
					OnAdd(mousePos_);
					break;
				case SDLK_d:
					OnDelete(mousePos_);
					break;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (e.button.button == SDL_BUTTON_LEFT) {
				dragging_ = true;
				dragStart_ = {e.button.x, e.button.y};
				OnClick(dragStart_);
			};
			break;

		case SDL_MOUSEMOTION:
			mousePos_ = {e.motion.x, e.motion.y};
			if (dragging_) {
				OnDrag(mousePos_);
			};
			break;

		case SDL_MOUSEBUTTONUP:
			if (e.button.button == SDL_BUTTON_LEFT) {
				dragging_ = false;
			};
			break;
	}
}

glm::vec2 UntangleApplet::ToScreen(glm::vec2 worldPos) {
    return worldPos * zoom_ + pan_;
}

glm::vec2 UntangleApplet::ToWorld(glm::vec2 screenPos) {
    return (screenPos - pan_) / zoom_;
}

UntangleApplet::HitInfo UntangleApplet::HitTest(glm::vec2 screenPos) {
    HitInfo hit;
	hit.worldPos = ToWorld(screenPos);
	hit.nodeId = 0;
	hit.edgeId = 0;
	return hit;
}

void UntangleApplet::OnAdd(glm::vec2 screenPos) {
	auto hit = HitTest(screenPos);
	if (hit.nodeId && selectedNodeId_ && hit.nodeId != selectedNodeId_) {
		graph.AddEdge(hit.nodeId, selectedNodeId_);
	} else if (!hit.nodeId) {
		graph.AddNode(hit.worldPos);
	}
}

void UntangleApplet::OnDelete(glm::vec2 screenPos) {
	auto hit = HitTest(screenPos);
	if (hit.nodeId) {
		graph.RemoveNode(hit.nodeId);
		if (selectedNodeId_ == hit.nodeId) {
			selectedNodeId_ = 0;
		}
	} else if (hit.edgeId) {
		graph.RemoveEdge(hit.edgeId);
	}
}

void UntangleApplet::OnClick(glm::vec2 pos) {
}

void UntangleApplet::OnDrag(glm::vec2 pos) {
}

UntangleApplet::NodeStyle UntangleApplet::GetNodeStyle(int nodeId)
{
    return (nodeId == selectedNodeId_ ?
			(nodeId == highlightedNodeId_ ? style_.highlightSelected : style_.selected) :
			(nodeId == highlightedNodeId_ ? style_.highlight : style_.normal)
		).node;
}

UntangleApplet::EdgeStyle UntangleApplet::GetEdgeStyle(int edgeId)
{
    return (edgeId == selectedEdgeId_ ?
			(edgeId == highlightedEdgeId_ ? style_.highlightSelected : style_.selected) :
			(edgeId == highlightedEdgeId_ ? style_.highlight : style_.normal)
		).edge;
}
