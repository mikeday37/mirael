#pragma once

#include "app/graphics.hpp"

#include <SDL.h>

class Box {
public:
	Box(int x, int y, int w, int h);

	void OnEvent(const SDL_Event &e);
	void OnRender(Graphics &g) const;
	void ResetPosition();

private:
	bool HitTest(int x, int y) const;

	SDL_Rect rect_;
	bool dragging_ = false;
	int dragOffsetX_ = 0;
	int dragOffsetY_ = 0;
	int initialX_;
	int initialY_;
};
