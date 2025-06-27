#include "app_pch.hpp"

#include "app/color.hpp"
#include "app/graphics.hpp"
#include <numbers>

Graphics::Graphics(ImDrawList *drawList) : drawList_(drawList) {}

void Graphics::Line(glm::vec2 from, glm::vec2 to, float width, Color color)
{
    drawList_->AddLine(ImVec2(from.x, from.y), ImVec2(to.x, to.y),
                       ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a)), width);
}

void Graphics::LineArrowEnd(glm::vec2 from, glm::vec2 to, float width, Color color, degrees arrowAngle,
                            float arrowLength)
{
    glm::vec2 dir = glm::normalize(from - to);

    float arrowAngleRadians = arrowAngle * std::numbers::pi_v<float> / 180.0f;
    float cosA = std::cos(arrowAngleRadians);
    float sinA = std::sin(arrowAngleRadians);

    glm::vec2 left = {dir.x * cosA - dir.y * sinA, dir.x * sinA + dir.y * cosA};
    glm::vec2 right = {dir.x * cosA + dir.y * sinA, -dir.x * sinA + dir.y * cosA};

    left *= arrowLength;
    right *= arrowLength;

    ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
    drawList_->AddLine(ImVec2(to.x, to.y), ImVec2(to.x + left.x, to.y + left.y), col, width);
    drawList_->AddLine(ImVec2(to.x, to.y), ImVec2(to.x + right.x, to.y + right.y), col, width);
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
