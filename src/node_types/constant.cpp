#include "pch.h"

#include "imgui_node_editor.h"

#include "constant.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Constant::onInit() { outPinId = getPinId("out"); }

void Constant::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Constant");
    ne::BeginPin(outPinId, ne::PinKind::Output);
    ImGui::Text("Out ->");
    ne::EndPin();
    ne::EndNode();
}

}; // namespace Mirael::NodeTypes
