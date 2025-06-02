#pragma once

#include "ui/applet.hpp"

#include <string>

class TestApplet1 final : public Applet {
public:
    TestApplet1(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Test One";}
	void OnRenderBackground(Graphics &g) override;
	void OnShowMenu() override;
};
