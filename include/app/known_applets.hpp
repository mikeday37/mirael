#pragma once

#include "app.hpp"
#include "applet.hpp"
#include <vector>

#include "app/example_applet.hpp"
#include "app/untangle_applet.hpp"

struct KnownApplets {
    KnownApplets(App &app) : example(app), untangle(app) {}

    ExampleApplet example;
    UntangleApplet untangle;

    std::vector<Applet *> GetAll() { return std::vector<Applet *>{&example, &untangle}; }

    Applet *GetDefault() { return &untangle; }
};
