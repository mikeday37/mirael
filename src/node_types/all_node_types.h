#pragma once

#include "comment.h"
#include "constant.h"
#include "display.h"
#include "script.h"

namespace Mirael::NodeTypes
{

template <typename F> inline void registerAll(F &&registrar)
{
    registrar.template operator()<
        // === list each Node-dervied class once, order doesn't matter ===
        Comment, Constant, Display, Script
        // ===============================================================
        >();
}

}; // namespace Mirael::NodeTypes