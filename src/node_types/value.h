#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Value : public Node
{
public:
    static const char *typeName() { return "value"; }

    virtual void onDeserialize(const nlohmann::json &j);
    void onInit() override;
    void onShow() override;
    virtual void onSerialize(nlohmann::json &j) const;

private:
    std::string value;
    PinId outPinId{};
};

}; // namespace Mirael::NodeTypes