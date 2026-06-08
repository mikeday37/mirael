#pragma once

#include "Mailbox.h"
#include "Node.h"
#include "TripleBuffer.h"

namespace Mirael::NodeTypes
{

class Display : public Node
{
public:
    static const char *typeName() { return "display"; }

protected:
    void onInit() override;
    void onShow() override;

    struct Channel {
        TripleBuffer<std::string> stringBuffer{};
    };

    class Core : public NodeCore
    {
    public:
        Core(PinId inPinId, std::shared_ptr<Channel> channel) : channel_(std::move(channel)), inPinId_(inPinId) {}

        void onFrame(const RunContext &context) override;

    private:
        PinId inPinId_;
        std::shared_ptr<Channel> channel_;
    };

    std::unique_ptr<NodeCore> createCore() { return std::make_unique<Core>(inPinId_, channel_); }

private:
    PinId inPinId_{};
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    std::string value_{};

    void acceptLatestValue()
    {
        auto result = channel_->stringBuffer.fetchLatestReadSlot();
        if (result.isNew)
            value_ = result.slot;
    }
};

} // namespace Mirael::NodeTypes