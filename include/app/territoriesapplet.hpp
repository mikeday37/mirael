#pragma once

#include "app/applet.hpp"

#include <string>

class TerritoriesApplet : public Applet {
public:
    TerritoriesApplet(App &app) : Applet(app) {}

	std::string_view GetDisplayName() const override {return "Territories";};

	void OnShowControls() override;
	void OnRenderBackground(Graphics &g) override;
	void OnEvent(const SDL_Event &e) override;

	struct HitInfo {
		bool onGrid; // false if the hit missed the grid entirely
		int territoryId; // id of the territory at the hit cell, or 0 if none.
		int mapX, mapY; // map coordinates of the hit cell
	};
	HitInfo HitTest(int x, int y) const;


private:

};
