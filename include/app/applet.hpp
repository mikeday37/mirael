#pragma once

#include "app/graphics.hpp"

#include <string>

#include <SDL.h>

class App;

class Applet {
protected:
    App &app_;

public:
    Applet(App &app) : app_(app) {}
	virtual ~Applet() = default;

	// required overrides
	virtual std::string_view GetDisplayName() const = 0;

	// overrides for applet lifecycle
	virtual void OnStartup() {}
	virtual void OnGainFocus() {}
	virtual void OnLoseFocus() {}
	virtual void OnShutdown() {}

	// overrides for applet UI
	virtual void OnShowMenu() {}
	virtual void OnShowControls() {}
	virtual void OnRenderBackground(Graphics &g) {}
	virtual void OnRenderForeground(Graphics &g) {}
	virtual void OnEvent(const SDL_Event &e) {}
};
