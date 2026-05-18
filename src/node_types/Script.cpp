#include "pch.h"

#include "ine/imgui_node_editor.h"

#include "Script.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Script::onInit()
{
    inPinId1_ = addPin("in1", {.direction = PinDirection::Input});
    inPinId2_ = addPin("in2", {.direction = PinDirection::Input});
    outPinId_ = addPin("out", {.direction = PinDirection::Output});
}

void Script::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Script");
    ne::BeginPin(inPinId1_, ne::PinKind::Input);
    ImGui::Text("-> in1");
    ne::EndPin();
    ne::BeginPin(inPinId2_, ne::PinKind::Input);
    ImGui::Text("-> in2");
    ne::EndPin();
    ne::BeginPin(outPinId_, ne::PinKind::Output);
    ImGui::Text("out ->");
    ne::EndPin();
    ne::EndNode();
}

} // namespace Mirael::NodeTypes
