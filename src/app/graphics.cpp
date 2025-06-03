#include "app_pch.hpp"

#include "app/graphics.hpp"
#include "app/vector.hpp"
#include "app/color.hpp"

Graphics::Graphics(ImDrawList *drawList) : drawList_(drawList)
{
}

void Graphics::Line(Vector from, Vector to, float width, Color color)
{
	drawList_->AddLine(
		ImVec2(from.x, from.y),
		ImVec2(to.x, to.y),
		ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a)),
		width
	);
}

void Graphics::Circle(Vector center, float radius, float lineWidth, Color fillColor, Color lineColor)
{
	if (fillColor.a > 0.0f) {
		drawList_->AddCircleFilled(
			ImVec2(center.x, center.y),
			radius,
			ImGui::ColorConvertFloat4ToU32(ImVec4(fillColor.r, fillColor.g, fillColor.b, fillColor.a))
		);
	}

	if (lineWidth > 0.0f && lineColor.a > 0.0f) {
		drawList_->AddCircle(
			ImVec2(center.x, center.y),
			radius,
			ImGui::ColorConvertFloat4ToU32(ImVec4(lineColor.r, lineColor.g, lineColor.b, lineColor.a))
		);
	}
}

void Graphics::Rectangle(Vector from, Vector to, float lineWidth, Color fillColor, Color lineColor)
{
	if (fillColor.a > 0.0f) {
		drawList_->AddRectFilled(
			ImVec2(from.x, from.y),
			ImVec2(to.x, to.y),
			ImGui::ColorConvertFloat4ToU32(ImVec4(fillColor.r, fillColor.g, fillColor.b, fillColor.a))
		);
	}

	if (lineWidth > 0.0f && lineColor.a > 0.0f) {
		drawList_->AddRect(
			ImVec2(from.x, from.y),
			ImVec2(to.x, to.y),
			ImGui::ColorConvertFloat4ToU32(ImVec4(lineColor.r, lineColor.g, lineColor.b, lineColor.a)),
			0.0f,
			0,
			lineWidth
		);
	}
}
