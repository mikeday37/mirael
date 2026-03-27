#pragma once

#include "vec2.hpp"
#include <vector>

namespace IteratedFractalSystem
{

struct Definition {
    enum class LineType { Primary, Secondary, Reversed, Cosmetic };

    struct Line {
        glm::vec2 from, to;
    };

    struct ControlLine {
        glm::vec2 from, to;
        bool reversed;
    };

    Line primary;
    std::vector<ControlLine> controls;
    std::vector<Line> cosmetics;
};

}; // namespace IteratedFractalSystem