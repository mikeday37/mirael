#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Display : public Node
{
public:
    static const char *typeName() { return "display"; }

    void onInit() override;
    void onShow() override;

private:
    PinId inPinId_{};
};

}; // namespace Mirael::NodeTypes