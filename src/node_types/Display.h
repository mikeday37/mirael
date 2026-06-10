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

    struct Dimensions {
        using dim_t = uint32_t;
        dim_t width, height;
        bool operator==(const Dimensions &) const = default;
    };

    struct Image {
        ~Image()
        {
            if (cleanup)
                cleanup();
            cleanup = nullptr;
            mapped = nullptr;
            descriptor = VK_NULL_HANDLE;
        }
        VkImage image                 = VK_NULL_HANDLE;
        VmaAllocation allocation      = VK_NULL_HANDLE;
        void *mapped                  = nullptr;
        vk::raii::ImageView view      = nullptr;
        VkDescriptorSet descriptor    = VK_NULL_HANDLE;
        std::function<void()> cleanup = nullptr; // TODO: this doesn't feel right - reconsider how this cleans up
    };

    struct ImageBuffer {               // shared by App and Node, can outlive Node, App won't destroy until !live
        Dimensions dim;                // immutable upon creation by UI
        std::atomic<bool> live = true; // core -> node, set to false when the core no longer needs it (wrong dimensions or overwritten)
        TripleBuffer<Image> images{};  // all images in the buffer have the same dimensions
    };

protected:
    void onInit() override;
    void onShow() override;

    enum class DataKind { None, String, Image };

    struct BufferCarrier {
        // this is the means by which Node passes a non-owning pointer back to the Core, safely marking dead if overwritten in the
        // mailbox
        ImageBuffer *buffer = nullptr;
        ~BufferCarrier()
        {
            if (buffer)
                buffer->live.store(false, std::memory_order_release);
            buffer = nullptr;
        }
    };

    struct Channel {
        std::atomic<DataKind> dataKind = DataKind::None; // core -> node

        // string channels
        TripleBuffer<std::string> stringBuffer{}; // shared

        // image channels
        Mailbox<Dimensions> pendingDimensions{}; // core -> node - signals need for new ImageBuffer
        Mailbox<BufferCarrier>
            pendingBufferCarrier{}; // node -> core - gives the core the latest ImageBuffer, sets dead on destruction
    };

    class Core : public NodeCore
    {
    public:
        Core(PinId inPinId, std::shared_ptr<Channel> channel) : channel_(std::move(channel)), inPinId_(inPinId) {}

        struct ValueInfo {
            DataKind kind;
            Dimensions dim;
            const void *pixelData;
        };

        void onFrame(const RunContext &context) override;

    private:
        PinId inPinId_;
        std::shared_ptr<Channel> channel_;
        lua_State *L                                         = nullptr;
        std::unique_ptr<BufferCarrier> currentBufferCarrier_ = nullptr;

        ValueInfo getValueInfo(const ValueBuffer *vbuf);

        void acceptLatestBufferCarrier()
        {
            if (auto latest = channel_->pendingBufferCarrier.tryAcceptLatest())
                currentBufferCarrier_ = std::move(latest);
        }
    };

    std::unique_ptr<NodeCore> createCore() { return std::make_unique<Core>(inPinId_, channel_); }

private:
    PinId inPinId_{};
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    std::string stringValue_{};
    std::optional<Dimensions> requestedDimensions_{};
    std::shared_ptr<ImageBuffer> currentImageBuffer_ = nullptr;

    void fetchLatestStringValue()
    {
        auto result = channel_->stringBuffer.fetchLatestReadSlot();
        if (result.isNew)
            stringValue_ = result.slot;
    }

    DataKind getKind() const { return channel_->dataKind.load(std::memory_order_acquire); }

    void acceptLatestRequestedDimensions()
    {
        if (auto latest = channel_->pendingDimensions.tryAcceptLatest())
            requestedDimensions_ = *latest;
    }

    void displayLatestImage();
    void createShareAndPostImageBuffer();
};

} // namespace Mirael::NodeTypes