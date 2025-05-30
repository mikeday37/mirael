#pragma once
#include <SDL.h>

class Box {
public:
	Box(int x, int y, int w, int h, SDL_Color color);

	void HandleEvent(const SDL_Event &e);
	void Render(SDL_Renderer *renderer) const;
	bool HitTest(int x, int y) const;

private:
	SDL_Rect rect_;
	SDL_Color color_;
	bool dragging_ = false;
	int dragOffsetX_ = 0;
	int dragOffsetY_ = 0;
};
