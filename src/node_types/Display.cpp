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
    ImGui::Text("Display");
    ne::BeginPin(inPinId_, ne::PinKind::Input);
    ImGui::Text("-> in");
    ne::EndPin();
    ne::EndNode();
}

}; // namespace Mirael::NodeTypes
