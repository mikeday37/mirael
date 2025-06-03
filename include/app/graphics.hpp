#pragma once

#include "app/color.hpp"
#include "app/vector.hpp"

struct ImDrawList;

class Graphics {
public:
	Graphics(ImDrawList *drawList);

	void Line(Vector from, Vector to, float width, Color color);
	void Circle(Vector center, float radius, float lineWidth, Color fillColor, Color lineColor);
	void Rectangle(Vector from, Vector to, float lineWidth, Color fillColor, Color lineColor);

private:
	ImDrawList *drawList_;
};



