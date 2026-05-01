#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Switch : public Node
{
public:
    static const char *typeName() { return "switch"; }

    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    void onShowProperties() override;

private:
    bool enabled     = true;
    bool dynamic     = true;
    int inputCount   = 2;
    int manualChoice = 0;
    PinId choicePinId{}, outPinId{};
    struct InputPin {
        int n;
        PinId id;
        std::string label;
    };
    std::vector<InputPin> inputs;
    void expandInputs();
    void addPin(int pinNumber); // 1-indexed
};

}; // namespace Mirael::NodeTypes