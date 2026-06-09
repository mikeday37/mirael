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

    enum class DataKind {
        None,
        String,
        Image
    };

    struct Dimensions {
        using dim_t = uint32_t;
        dim_t width, height;
        bool operator==(const Dimensions &) const = default;
    };

    struct Image {
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;
        void *mapped = nullptr;
        vk::raii::ImageView view = nullptr;
        vk::raii::Sampler sampler = nullptr;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
    };

    struct ImageBuffer {
        Dimensions dim; // immutable upon creation by UI
        std::atomic<bool> live = true; // core -> node, set to false when the core no longer wants it (wrong dimensions)
        std::atomic<bool> writing = false; // core -> node, true only while core needs to write to image slot
        TripleBuffer<Image> images{}; // all images in the buffer have the same dimensions
    };

    struct Channel {
        std::atomic<DataKind> dataKind = DataKind::None; // core -> node

        // string channels
        TripleBuffer<std::string> stringBuffer{}; // shared

        // image channels
        Mailbox<Dimensions> pendingDimensions{}; // core -> node - signals need for new ImageBuffer
        Mailbox<std::shared_ptr<ImageBuffer>> pendingImageBuffer{}; // node -> core - passes back a shared_ptr to the latest ImageBuffer
    };

    class Core : public NodeCore
    {
    public:
        Core(PinId inPinId, std::shared_ptr<Channel> channel) : channel_(std::move(channel)), inPinId_(inPinId) {}
        ~Core() {releaseImageBuffer();}

        struct ValueInfo {
            DataKind kind;
            Dimensions dim;
            const void *pixelData;
        };

        ValueInfo getValueInfo(const ValueBuffer *vbuf);
        void onFrame(const RunContext &context) override;

    private:
        PinId inPinId_;
        std::shared_ptr<Channel> channel_;
        lua_State *L   = nullptr;
        std::shared_ptr<ImageBuffer> currentImageBuffer_ = nullptr;

        void releaseImageBuffer() {
            if (currentImageBuffer_)
                currentImageBuffer_->live = false;
            currentImageBuffer_ = nullptr;
        }

        void acceptLatestImageBuffer() {
            if (auto latest = channel_->pendingImageBuffer.tryAcceptLatest())
            {
                releaseImageBuffer();
                currentImageBuffer_ = std::move(*latest);
            }
        }
    };

    std::unique_ptr<NodeCore> createCore() { return std::make_unique<Core>(inPinId_, channel_); }

private:
    PinId inPinId_{};
    std::shared_ptr<Channel> channel_ = std::make_shared<Channel>();
    std::string stringValue_{};

    void fetchLatestStringValue()
    {
        auto result = channel_->stringBuffer.fetchLatestReadSlot();
        if (result.isNew)
            stringValue_ = result.slot;
    }

    DataKind getKind() const {return channel_->dataKind.load(std::memory_order_acquire);}

    std::unique_ptr<Dimensions> acceptLatestRequestedDimensions() {
        return channel_->pendingDimensions.tryAcceptLatest();
    }
};

} // namespace Mirael::NodeTypes