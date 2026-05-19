#pragma once

#include "Node.h"

namespace Mirael::NodeTypes
{

class Switch : public Node
{
public:
    static const char *typeName() { return "switch"; }

protected:
    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    void onShowProperties() override;

    struct Config {
        std::vector<PinId> inPins;
        PinId choicePin, outPin;
        int manualChoice;
        bool enabled, dynamic;
    };

    struct Channel {
        Mailbox<Config> pendingConfig;
    };

    class Core : public NodeCore
    {
    public:
        Core(Config &&initialConfig, std::shared_ptr<Channel> channel) : config_(std::move(initialConfig)), channel_(channel) {}

    protected:
        void onFrame(const RunContext &context) override;

    private:
        Config config_;
        std::shared_ptr<Channel> channel_;

        void acceptLatestConfig()
        {
            if (auto taken = channel_->pendingConfig.tryAcceptLatest())
                config_ = std::move(*taken);
        }
    };

    virtual std::unique_ptr<NodeCore> createCore() { return std::make_unique<Core>(buildConfig(), channel_); }

private:
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    Config buildConfig() const
    {
        std::vector<PinId> inPins;
        inPins.reserve(inputs_.size());
        for (auto &s : inputs_)
            inPins.push_back(s.id);
        return Config{.inPins       = std::move(inPins),
                      .choicePin    = choicePinId_,
                      .outPin       = outPinId_,
                      .manualChoice = manualChoice_,
                      .enabled      = enabled_,
                      .dynamic      = dynamic_};
    }
    void postConfig() { channel_->pendingConfig.postNew(std::make_unique<Config>(buildConfig())); }

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