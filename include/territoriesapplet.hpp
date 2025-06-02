#pragma once

#include "applet.hpp"
#include <string>

class TerritoriesApplet : public Applet {
public:
    TerritoriesApplet(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Territories";};

private:

};
