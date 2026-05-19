#pragma once

#include "Node.h"

#include "Mailbox.h"

namespace Mirael::NodeTypes
{

class Value : public Node
{
public:
    static const char *typeName() { return "value"; }

protected:
    void onDeserialize(const nlohmann::json &j) override;
    void onInit() override;
    void onShow() override;
    void onSerialize(nlohmann::json &j) const override;

    struct Channel {
        Mailbox<std::string> pendingValue;
    };

    class Core : public NodeCore
    {
    public:
        Core(PinId outPinId, std::shared_ptr<Channel> channel) : outPinId_(outPinId), channel_(channel) {}

    protected:
        void onFrame(const RunContext &context) override;

    private:
        std::shared_ptr<Channel> channel_;
        std::string value_{};
        PinId outPinId_;

        void acceptLatestValue()
        {
            if (auto taken = channel_->pendingValue.tryAcceptLatest())
                value_ = std::move(*taken);
        }
    };

    std::unique_ptr<NodeCore> createCore()
    {
        postValue();
        return std::make_unique<Core>(outPinId_, channel_);
    };

private:
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    std::string value_{};
    PinId outPinId_{};

    void postValue() { channel_->pendingValue.postNew(std::make_unique<std::string>(value_)); }
};

} // namespace Mirael::NodeTypes