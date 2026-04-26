#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Display : public Node
{
public:
    static const char *typeName() { return "display"; }
};

}; // namespace Mirael::NodeTypes