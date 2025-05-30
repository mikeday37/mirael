#pragma once
#include <SDL.h>

class Box {
public:
	Box(int x, int y, int w, int h);

	void HandleEvent(const SDL_Event &e);
	void Render(SDL_Renderer *renderer) const;
	bool HitTest(int x, int y) const;
	void ResetPosition();

private:
	SDL_Rect rect_;
	SDL_Color color_;
	bool dragging_ = false;
	int dragOffsetX_ = 0;
	int dragOffsetY_ = 0;
	int initialX_;
	int initialY_;
};
