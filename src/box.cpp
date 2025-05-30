#include <SDL.h>
#include "box.hpp"

Box::Box(int x, int y, int w, int h, SDL_Color color) {
	rect_ = { x, y, w, h };
	color_ = color;
}

void Box::HandleEvent(const SDL_Event &e) {
	// start a drag if not dragging and left button down
	if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && !dragging_) {
		if (HitTest(e.button.x, e.button.y)) {
			dragging_ = true;
			dragOffsetX_ = e.button.x - rect_.x;
			dragOffsetY_ = e.button.y - rect_.y;
		}
	}
	else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
		dragging_ = false;
	}
	else if (e.type == SDL_MOUSEMOTION && dragging_) {
		rect_.x = e.motion.x - dragOffsetX_;
		rect_.y = e.motion.y - dragOffsetY_;
	}
}

void Box::Render(SDL_Renderer *renderer) const {
	SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
	SDL_RenderDrawRect(renderer, &rect_);
}

bool Box::HitTest(int x, int y) const {
	bool hit =
		x >= rect_.x &&
		y >= rect_.y &&
		x <= rect_.x + rect_.w &&
		y <= rect_.y + rect_.h;

	return hit;
}
