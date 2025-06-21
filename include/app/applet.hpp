#pragma once

#include "app/graphics.hpp"
#include "app/utility.hpp"
#include "vec2.hpp"
#include <SDL.h>
#include <string>

class App;

class Applet
{
protected:
    App &app_;

public:
    Applet(App &app) : app_(app) {}
    virtual ~Applet() = default;

    // required overrides
    virtual const char *GetDisplayName() const = 0;

    // overrides for applet lifecycle
    virtual void OnStartup() {}
    virtual void OnGainFocus() {}
    virtual void OnNewFrame() {}
    virtual void OnEndFrame() {}
    virtual void OnLoseFocus() {}
    virtual void OnShutdown() {}

    // overrides for applet UI
    virtual void OnShowMenu() {}
    virtual void OnShowControls() {}
    virtual void OnRenderBackground(Graphics &g) { unused(g); }
    virtual void OnRenderForeground(Graphics &g) { unused(g); }
    virtual void OnEvent(const SDL_Event &e) { unused(e); }

    // utility
    glm::vec2 GetWindowSize();
};
