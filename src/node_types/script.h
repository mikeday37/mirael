#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Script : public Node
{
public:
    static const char *typeName() { return "script"; }

    void onInit() override;
    void onShow() override;

private:
    PinId inPinId1{}, inPinId2{}, outPinId{};
};

}; // namespace Mirael::NodeTypes