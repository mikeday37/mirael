#include "pch.h"

#include "imgui_node_editor.h"

#include "script.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Script::onInit()
{
    inPinId1 = getNextPinId();
    inPinId2 = getNextPinId();
    outPinId = getNextPinId();
}

void Script::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Script");
    ne::BeginPin(inPinId1, ne::PinKind::Input);
    ImGui::Text("-> In1");
    ne::EndPin();
    ne::BeginPin(inPinId2, ne::PinKind::Input);
    ImGui::Text("-> In2");
    ne::EndPin();
    ne::BeginPin(outPinId, ne::PinKind::Output);
    ImGui::Text("Out ->");
    ne::EndPin();
    ne::EndNode();
}

}; // namespace Mirael::NodeTypes
