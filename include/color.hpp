#pragma once

struct Color {
	float r, g, b, a;

	Color(float r_, float g_, float b_, float a_ = 1.0f)
		: r(r_), g(g_), b(b_), a(a_) {}

	static const Color Transparent;
	static const Color Black;
	static const Color White;
	static const Color Cyan;
};
