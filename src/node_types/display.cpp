#include "pch.h"

#include "imgui_node_editor.h"

#include "display.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Display::onInit() { inPinId = getPinId("in"); }

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
