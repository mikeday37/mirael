#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Constant : public Node
{
public:
    static const char *typeName() { return "constant"; }
};

}; // namespace Mirael::NodeTypes