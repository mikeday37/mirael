#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "data.h"
#include "value.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Value::onDeserialize(const nlohmann::json &j)
{
    if (!j.empty())
        value_ = j["value"].get<std::string>();
}

void Value::onInit() { outPinId_ = addPin("out", {.direction = PinDirection::Output}); }

void Value::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::Text("Value:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(24.0f * ImGui::CalcTextSize("0").x);
    if (ImGui::InputText("###value", &value_))
        raiseModified(ChangeImpact::NodeConfig);
    ImGui::SameLine();
    ne::BeginPin(outPinId_, ne::PinKind::Output);
    ImGui::Text("out ->");
    ne::EndPin();
    ImGui::PopID();
    ne::EndNode();
}

void Value::onSerialize(nlohmann::json &j) const
{
    if (!value_.empty())
        j["value"] = value_;
}

}; // namespace Mirael::NodeTypes
