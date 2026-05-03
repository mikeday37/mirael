#include "pch.h"

#include "ine/imgui_node_editor.h"

#include "script.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Script::onInit()
{
    inPinId1 = getPinId("in1");
    inPinId2 = getPinId("in2");
    outPinId = getPinId("out");
}

void Script::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Script");
    ne::BeginPin(inPinId1, ne::PinKind::Input);
    ImGui::Text("-> in1");
    ne::EndPin();
    ne::BeginPin(inPinId2, ne::PinKind::Input);
    ImGui::Text("-> in2");
    ne::EndPin();
    ne::BeginPin(outPinId, ne::PinKind::Output);
    ImGui::Text("out ->");
    ne::EndPin();
    ne::EndNode();
}

}; // namespace Mirael::NodeTypes
