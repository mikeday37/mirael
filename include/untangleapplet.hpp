#pragma once

#include "applet.hpp"

class UntangleApplet : public Applet {
public:
    UntangleApplet(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Untangle";};

private:

};
