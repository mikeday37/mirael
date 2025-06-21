#pragma once

#include "app/applet.hpp"
#include "imgui.h"

class ExampleApplet : public Applet
{
public:
    ExampleApplet(App &app) : Applet(app) {}

    const char *GetDisplayName() const override { return "Example"; }
    void OnShowControls() override;
};
