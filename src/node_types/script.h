#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Script : public Node
{
public:
    static const char *typeName() { return "script"; }
};

}; // namespace Mirael::NodeTypes