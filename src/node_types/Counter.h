#pragma once

#include <atomic>
#include <limits>

#include "Node.h"
#include "Mailbox.h"

namespace Mirael::NodeTypes
{

class Counter : public Node
{
public:
    using value_t = int;

    static const char *typeName() { return "counter"; }

protected:
    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    void onShowProperties() override;

    struct Config {
        value_t step     = 1;
        value_t minValue = std::numeric_limits<value_t>::min(), maxValue = std::numeric_limits<value_t>::max();
        bool clipMin = false, clipMax = false, wrap = true;
    };

    struct Channel {
        std::atomic<value_t> value;
        Mailbox<Config> pendingConfig{};
    };

    class Core : public NodeCore
    {
    public:
        Core(PinId outPinId, std::shared_ptr<Channel> channel) : outPinId_(outPinId), channel_(std::move(channel)) {}

        void onFrame(const RunContext &context) override;

    private:
        PinId outPinId_;
        std::shared_ptr<Channel> channel_;
        value_t value_ = 0;
        Config config_;

        inline void acceptLatestConfig()
        {
            if (auto taken = channel_->pendingConfig.tryAcceptLatest())
                config_ = *taken;
        }
        inline void putValue() { channel_->value.store(value_, std::memory_order_relaxed); }
    };

    std::unique_ptr<NodeCore> createCore()
    {
        postConfig();
        return std::make_unique<Core>(outPinId_, channel_);
    };

private:
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    Config config_{};
    PinId outPinId_{};

    inline void postConfig() {
        channel_->pendingConfig.postNew(std::make_unique<Config>(config_));
    }
    inline value_t getValue() { return channel_->value.load(std::memory_order_relaxed); }
};

} // namespace Mirael::NodeTypes
