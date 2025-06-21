#include "app_pch.hpp"

#include "app/app.hpp"
#include "app/applet.hpp"
#include <SDL.h>

glm::vec2 Applet::GetWindowSize()
{
    auto window = app_.GetWindow();
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return {w, h};
}
