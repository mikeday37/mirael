#include "pch.h"

#include "ine/imgui_node_editor.h"

#include "display.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Display::onInit() { inPinId = addPin("in", {.direction = PinDirection::Input}); }

void Display::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Display");
    ne::BeginPin(inPinId, ne::PinKind::Input);
    ImGui::Text("-> in");
    ne::EndPin();
    ne::EndNode();
}

}; // namespace Mirael::NodeTypes
