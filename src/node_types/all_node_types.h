#pragma once

#include "constant.h"
#include "display.h"
#include "script.h"

namespace Mirael::NodeTypes
{

template <typename F> inline void registerAll(F &&registrar) { registrar.template operator()<Constant, Display, Script>(); }

}; // namespace Mirael::NodeTypes