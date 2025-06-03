#include "app_pch.hpp"

#include "app/box.hpp"
#include "app/graphics.hpp"
#include "app/color.hpp"

Box::Box(int x, int y, int w, int h) {
	rect_ = { x, y, w, h };
	initialX_ = x;
	initialY_ = y;
}

void Box::OnEvent(const SDL_Event &e) {
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

void Box::OnRender(Graphics &g) const {
	g.Rectangle(
		{static_cast<float>(rect_.x), static_cast<float>(rect_.y)},
		{static_cast<float>(rect_.x + rect_.w), static_cast<float>(rect_.y + rect_.h)},
		4.0f,
		dragging_ ? Color::Black : Color::White,
		Color::Cyan
	);
}

bool Box::HitTest(int x, int y) const {
	bool hit =
		x >= rect_.x &&
		y >= rect_.y &&
		x <= rect_.x + rect_.w &&
		y <= rect_.y + rect_.h;

	return hit;
}

void Box::ResetPosition() {
	rect_.x = initialX_;
	rect_.y = initialY_;
}
