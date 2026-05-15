#pragma once

#include "Comment.h"
#include "Display.h"
#include "Script.h"
#include "Switch.h"
#include "Value.h"

namespace Mirael::NodeTypes
{

template <typename F> inline void registerAll(F &&registrar)
{
    registrar.template operator()<
        // === list each Node-dervied class once, order doesn't matter ===
        Comment, Display, Script, Switch, Value
        // ===============================================================
        >();
}

}; // namespace Mirael::NodeTypes