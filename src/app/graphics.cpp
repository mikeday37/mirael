#include "app_pch.hpp"

#include "app/color.hpp"
#include "app/graphics.hpp"

Graphics::Graphics(ImDrawList *drawList) : drawList_(drawList) {}

void Graphics::Line(glm::vec2 from, glm::vec2 to, float width, Color color)
{
    drawList_->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y),
                       ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a)), width);
}

void Graphics::Circle(glm::vec2 center, float radius, float lineWidth, Color fillColor, Color lineColor)
{
    if (fillColor.a > 0.0f) {
        drawList_->AddCircleFilled(
            ImVec2(center.x, center.y), radius,
            ImGui::ColorConvertFloat4ToU32(ImVec4(fillColor.r, fillColor.g, fillColor.b, fillColor.a)));
    }

    if (lineWidth > 0.0f && lineColor.a > 0.0f) {
        drawList_->AddCircle(ImVec2(center.x, center.y), radius,
                             ImGui::ColorConvertFloat4ToU32(ImVec4(lineColor.r, lineColor.g, lineColor.b, lineColor.a)),
                             0, lineWidth);
    }
}

void Graphics::Rectangle(glm::vec2 from, glm::vec2 to, float lineWidth, Color fillColor, Color lineColor)
{
    if (fillColor.a > 0.0f) {
        drawList_->AddRectFilled(
            ImVec2(from.x, from.y), ImVec2(to.x, to.y),
            ImGui::ColorConvertFloat4ToU32(ImVec4(fillColor.r, fillColor.g, fillColor.b, fillColor.a)));
    }

    if (lineWidth > 0.0f && lineColor.a > 0.0f) {
        drawList_->AddRect(ImVec2(from.x, from.y), ImVec2(to.x, to.y),
                           ImGui::ColorConvertFloat4ToU32(ImVec4(lineColor.r, lineColor.g, lineColor.b, lineColor.a)),
                           0.0f, 0, lineWidth);
    }
}
