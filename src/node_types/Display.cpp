#include "pch.h"

#include "ine/imgui_node_editor.h"

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
            // TODO: continue here - accept dimensions, move old buffer to graveyard, create new buffer, fetch latest, display via ImGui
            //ImGui::Image()
            break;
    }

    ne::EndNode();
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
    lua_getfield(L, -4, "buf"); // [value, _tag, w, h, buf]
    const void *pbuf = lua_topointer(L, -1); // TODO: need a safer approach - a user could put non-cdata/non-GC values here
    lua_pop(L, 5);
    assert(lua_gettop(L) == entryTop);

    if (pbuf && w > 0 && h > 0) {
        info.dim.width = static_cast<Dimensions::dim_t>(w);
        info.dim.height = static_cast<Dimensions::dim_t>(h);
        info.kind = DataKind::Image;
        info.pixelData = pbuf;
    }

    return info;
}

void Display::Core::onFrame(const RunContext &context)
{
    L = context.L;

    auto vbuf = context.getFirstInput(inPinId_); // may return nullptr
    auto info = getValueInfo(vbuf); // this is safe, as getValueInfo accepts nullptr

    switch (info.kind) {
        using enum Display::DataKind;

    case None:
        // intentional nop/fallthrough to end
        break;

    case String: {
        auto &sbuf          = channel_->stringBuffer;
        sbuf.getWriteSlot() = vbuf->toString(); // TODO: change this to write into the existing string to avoid allocations per frame
        sbuf.commitWrite();
        channel_->dataKind.store(DataKind::String, std::memory_order_release);
        return;
    }

    case Image: {
        acceptLatestImageBuffer();
        bool ready = currentImageBuffer_ && currentImageBuffer_->dim == info.dim;
        if (ready) {
            auto &slot = currentImageBuffer_->images.getWriteSlot();
            std::memcpy(slot.mapped, info.pixelData, static_cast<size_t>(4 * info.dim.width * info.dim.height));
            // the above assumes no row padding, which the UI is currently expected to ensure
            currentImageBuffer_->images.commitWrite();
        } else {
            releaseImageBuffer();
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

} // namespace Mirael::NodeTypes
