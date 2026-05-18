#pragma once

#include "Node.h"

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
    bool enabled_     = true;
    bool dynamic_     = true;
    int inputCount_   = 2;
    int manualChoice_ = 0;
    PinId choicePinId_{}, outPinId_{};
    struct InputPin {
        int n;
        PinId id;
        std::string label;
    };
    std::vector<InputPin> inputs_;
    void expandInputs();
    void reduceInputs();
    void addSwitchInputPin(int pinNumber);    // 1-indexed
    void removeSwitchInputPin(int pinNumber); // 1-indexed
    void handleToggleDynamic();
};

} // namespace Mirael::NodeTypes