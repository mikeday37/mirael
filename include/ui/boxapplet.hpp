#pragma once

#include "ui/applet.hpp"
#include "ui/box.hpp"

#include <string>

class BoxApplet final : public Applet {
public:
    BoxApplet(App &app);

	std::string_view GetDisplayName() const override {return "Box";}
	void OnRenderBackground(Graphics &g) override;
	void OnShowControls() override;
	void OnEvent(const SDL_Event &e) override;

private:
	Box box_;
};
