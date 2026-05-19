#pragma once

#include "Node.h"

namespace Mirael::NodeTypes
{

class Value : public Node
{
public:
    static const char *typeName() { return "value"; }

    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

private:
    std::string value_;
    PinId outPinId_{};
};

} // namespace Mirael::NodeTypes