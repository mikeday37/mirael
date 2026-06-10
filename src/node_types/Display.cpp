#include "pch.h"

#include "ine/imgui_node_editor.h"

#include "App.h"
#include "Display.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Display::onInit() { inPinId_ = addPin("in", {.direction = PinDirection::Input}); }

void Display::onShow()
{
    ne::BeginNode(getId());
    ne::BeginPin(inPinId_, ne::PinKind::Input);
    ImGui::TextUnformatted("->");
    ne::EndPin();
    ImGui::SameLine();

    switch (getKind()) {

    case DataKind::String:
        fetchLatestStringValue();
        ImGui::Text("Display: %s", stringValue_.c_str());
        break;

    case DataKind::Image:
        ImGui::TextUnformatted("Display:");
        displayLatestImage();
        break;

    default:
        ImGui::TextUnformatted("Display");
        break;
    }

    ne::EndNode();
}

void Display::displayLatestImage()
{
    // get latest dimensions, do nothing if we haven't received a request yet
    acceptLatestRequestedDimensions();
    if (!requestedDimensions_)
        return;

    // verify the dimensions are valid
    auto &io = ImGui::GetPlatformIO();
    // TODO: handle zero/excessive dimensions better than just asserting
    assert(requestedDimensions_->width > 0 && requestedDimensions_->height > 0);
    assert(io.Renderer_TextureMaxWidth == 0 ||
           requestedDimensions_->width <= static_cast<Dimensions::dim_t>(io.Renderer_TextureMaxWidth));
    assert(io.Renderer_TextureMaxHeight == 0 ||
           requestedDimensions_->height <= static_cast<Dimensions::dim_t>(io.Renderer_TextureMaxHeight));

    // if dimensions have changed, release the current image buffer
    if (requestedDimensions_ && currentImageBuffer_ && *requestedDimensions_ != currentImageBuffer_->dim) {
        currentImageBuffer_ = nullptr;
    }

    // if we need to create a new image buffer, do so, sharing it with the app's graveyard and posting a carrier of it to the core
    if (!currentImageBuffer_) {
        assert(requestedDimensions_);
        createShareAndPostImageBuffer();
        assert(currentImageBuffer_);
    }

    // now we should have requested dimensions and an image buffer with these dimensions
    assert(currentImageBuffer_ && requestedDimensions_ && *requestedDimensions_ == currentImageBuffer_->dim);

    // draw the latest image
    // TODO: avoid drawing initial image the core hasn't written to
    auto result = currentImageBuffer_->images.fetchLatestReadSlot();
    ImVec2 size{static_cast<float>(currentImageBuffer_->dim.width), static_cast<float>(currentImageBuffer_->dim.height)};
    auto *dl = ImGui::GetWindowDrawList();
    dl->AddCallback(io.DrawCallback_SetSamplerNearest, nullptr); // TODO: may want to use linear if zoomed out
    ImGui::Image(reinterpret_cast<ImTextureID>(result.slot.descriptor), size);
    dl->AddCallback(io.DrawCallback_ResetRenderState, nullptr);
}

void Display::createShareAndPostImageBuffer()
{
    currentImageBuffer_      = std::make_shared<ImageBuffer>();
    currentImageBuffer_->dim = *requestedDimensions_;
    auto &slots              = currentImageBuffer_->images.initialGetAll();
    for (auto &slot : slots)
        App::get().initializeDisplayImage(*currentImageBuffer_,
                                          slot); // TODO: not RAII - replace initialGetAll() with factory-ctor pattern
    auto carrier    = std::make_unique<BufferCarrier>();
    carrier->buffer = currentImageBuffer_.get();                    // carrier destruction will mark buffer dead
    channel_->pendingBufferCarrier.postNew(std::move(carrier));     // buffer made available to core via carrier
    App::get().acceptNewImageBuffer(currentImageBuffer_); // buffer can now outlive node

    // the above sequence ensures that the ImageBuffer is destroyed by the UI, but not before Core is either done with it
    // or never receives it (in the case where Carrier was destroyed by being overwritten in the mailbox).  This is essential, because
    // a core can always outlive a node.
}

void Display::Core::onFrame(const RunContext &context)
{
    L = context.L;

    auto vbuf = context.getFirstInput(inPinId_); // may return nullptr
    auto info = getValueInfo(vbuf);              // this is safe, as getValueInfo accepts nullptr

    switch (info.kind) {
        using enum Display::DataKind;

    case None:
        // intentional nop/fallthrough to end
        break;

    case String: {
        assert(vbuf); // info.Kind should not be String unless vbuf is not null
        auto &sbuf          = channel_->stringBuffer;
        sbuf.getWriteSlot() = vbuf->toString(); // TODO: change this to write into the existing string to avoid allocations per frame
        sbuf.commitWrite();
        channel_->dataKind.store(DataKind::String, std::memory_order_release);
        return;
    }

    case Image: {
        acceptLatestBufferCarrier();
        auto buffer = currentBufferCarrier_ ? currentBufferCarrier_->buffer : nullptr;
        bool ready  = buffer && buffer->dim == info.dim;
        if (ready) {
            auto &slot = buffer->images.getWriteSlot();
            std::memcpy(slot.mapped, info.pixelData, static_cast<size_t>(4 * info.dim.width * info.dim.height));
            // the above assumes no row padding, which the UI is currently expected to ensure
            buffer->images.commitWrite();
        } else {
            currentBufferCarrier_ = nullptr;
            channel_->pendingDimensions.postNew(std::make_unique<Dimensions>(info.dim));
            // TODO: make a slimmer method of requesting new dimensions - this will spam the channel until it gets what it wants
            // (acceptable for now because dimension changes are intended to be rare)
        }
        channel_->dataKind.store(DataKind::Image, std::memory_order_release);
        return;
    }

    default:
        assert(false); // should have handled all possibilities above
        break;
    }

    // set kind = none if we didn't set otherwise and return above
    channel_->dataKind.store(DataKind::None, std::memory_order_release);
}

Display::Core::ValueInfo Display::Core::getValueInfo(const ValueBuffer *vbuf)
{
    ValueInfo info = {.kind = DataKind::None};

    if (!vbuf)
        return info;

    info.kind = DataKind::String; // we display everything but image tables as a string

    if (!vbuf->isTable())
        return info;

    auto entryTop = lua_gettop(L);

    vbuf->pushValueToLuaStack(); // [value]

    lua_getfield(L, -1, "_tag"); // [value, _tag]
    auto tag = lua_tostring(L, -1);
    if (!tag || std::strcmp(tag, "image")) { // TODO: can the strcmp be done faster since lua interns strings?
        lua_pop(L, 2);
        assert(lua_gettop(L) == entryTop);
        return info;
    }

    lua_getfield(L, -2, "w"); // [value, _tag, w]
    int w = static_cast<int>(lua_tointeger(L, -1));
    lua_getfield(L, -3, "h"); // [value, _tag, w, h]
    int h = static_cast<int>(lua_tointeger(L, -1));
    lua_getfield(L, -4, "buf");              // [value, _tag, w, h, buf]
    const void *pbuf = lua_topointer(L, -1); // TODO: need a safer approach - a user could put non-cdata/non-GC values here
    lua_pop(L, 5);
    assert(lua_gettop(L) == entryTop);

    if (pbuf && w > 0 && h > 0) {
        info.dim.width  = static_cast<Dimensions::dim_t>(w);
        info.dim.height = static_cast<Dimensions::dim_t>(h);
        info.kind       = DataKind::Image;
        info.pixelData  = pbuf;
    }

    return info;
}

} // namespace Mirael::NodeTypes
