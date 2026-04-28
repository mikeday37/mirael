#pragma once

#include "comment.h"
#include "display.h"
#include "script.h"
#include "switch.h"
#include "value.h"

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