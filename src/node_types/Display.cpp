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
    acceptLatestValue();
    ImGui::Text("Display: %s", value_.c_str());
    ne::EndNode();
}

void Display::Core::onFrame(const RunContext &context)
{
    if (auto buf = context.getFirstInput(inPinId_))
    {
        auto &sbuf = channel_->stringBuffer;
        sbuf.getWriteSlot() = buf->toString(); // TODO: change this to write into an existing string to avoid allocations per frame
        sbuf.commitWrite();
    }
}

} // namespace Mirael::NodeTypes
