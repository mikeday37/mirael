#pragma once

#include "ui/applet.hpp"

#include <string>

class TestApplet2 final : public Applet {
public:
    TestApplet2(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Test Two";}
	void OnRenderBackground(Graphics &g) override;
	void OnShowControls() override;
};
