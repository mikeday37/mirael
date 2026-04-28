#pragma once

#include "node.h"

namespace Mirael::NodeTypes
{

class Switch : public Node
{
public:
    static const char *typeName() { return "switch"; }

    virtual void onDeserialize(const nlohmann::json &j);
    void onInit() override;
    void onShow() override;
    virtual void onSerialize(nlohmann::json &j) const;

private:
    bool enabled = true;
    bool dynamic = true;
    int inputCount = 2;
    int manualChoice = 1;
    PinId choicePinId{}, outPinId{};
    struct InputPin {
        int n;
        PinId id;
        std::string label;
    };
    std::vector<InputPin> inputs;
};

}; // namespace Mirael::NodeTypes