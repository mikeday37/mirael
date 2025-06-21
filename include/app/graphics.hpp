#pragma once

#include "app/color.hpp"
#include "vec2.hpp"

struct ImDrawList;

class Graphics
{
public:
    Graphics(ImDrawList *drawList);

    void Line(glm::vec2 from, glm::vec2 to, float width, Color color);
    void Circle(glm::vec2 center, float radius, float lineWidth, Color fillColor, Color lineColor);
    void Rectangle(glm::vec2 from, glm::vec2 to, float lineWidth, Color fillColor, Color lineColor);

private:
    ImDrawList *drawList_;
};
