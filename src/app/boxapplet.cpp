#include "app_pch.hpp"

#include "app/boxapplet.hpp"
#include "app/color.hpp"
#include "app/graphics.hpp"
#include "app/box.hpp"

BoxApplet::BoxApplet(App &app) :
	Applet(app),
	box_(100, 100, 200, 150) {
}

void BoxApplet::OnRenderBackground(Graphics &g)
{
    box_.OnRender(g);
}

void BoxApplet::OnShowControls() {
	if (ImGui::Button("Reset Position")) {
		box_.ResetPosition();
	}
}

void BoxApplet::OnEvent(const SDL_Event &e) {
	box_.OnEvent(e);
}
