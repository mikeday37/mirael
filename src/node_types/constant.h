#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Constant : public Node
{
public:
    static const char *typeName() { return "constant"; }

    void onInit() override;
    void onShow() override;

private:
    PinId outPinId{};
};

}; // namespace Mirael::NodeTypes