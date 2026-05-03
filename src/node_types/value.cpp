#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "value.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Value::onDeserialize(const nlohmann::json &j)
{
    if (!j.empty())
        value = j["value"].get<std::string>();
}

void Value::onInit() {
    outPinId = getPinId("out");
}

void Value::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::Text("Value:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(24.0f * ImGui::CalcTextSize("0").x);
    if (ImGui::InputText("###value", &value))
        raiseModified();
    ImGui::SameLine();
    ne::BeginPin(outPinId, ne::PinKind::Output);
    ImGui::Text("out ->");
    ne::EndPin();
    ImGui::PopID();
    ne::EndNode();
}

void Value::onSerialize(nlohmann::json &j) const
{
    if (!value.empty())
        j["value"] = value;
}

}; // namespace Mirael::NodeTypes
