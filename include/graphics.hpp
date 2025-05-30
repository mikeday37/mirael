#pragma once
#include "imgui.h"
#include "color.hpp"
#include "vector.hpp"

class Graphics {
public:
	Graphics();

	void Line(Vector from, Vector to, float width, Color color);
	void Circle(Vector center, float radius, float lineWidth, Color fillColor, Color lineColor);
	void Rectangle(Vector from, Vector to, float lineWidth, Color fillColor, Color lineColor);

private:
	ImDrawList *drawList_;
};



