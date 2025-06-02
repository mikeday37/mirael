#pragma once

#include "ui/applet.hpp"

class UntangleApplet : public Applet {
public:
    UntangleApplet(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Untangle";};

	void HiThere();
	void HelloThere();
	void Salutations();

private:

};
