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

void Display::Core::onFrame(const RunContext &context) {
    auto it = context.inputs.find(inPinId_);
    if (it == context.inputs.end())
        return;

    for (const auto *valueBuffer : it->second) {
        // TODO: this is allocating per frame - switch to something like an expandable ring buffer or triple buffer
        channel_->pendingValue.postNew(std::make_unique<std::string>(valueBuffer->getValue()));
        break; // if there's more than one input, we don't care
    }
}

} // namespace Mirael::NodeTypes
