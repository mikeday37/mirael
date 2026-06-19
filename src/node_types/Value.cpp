#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "data.h"
#include "NodeEditorEx.h"
#include "Value.h"

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
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Value:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(value_.c_str()).x + 2 * ImGui::CalcTextSize("0").x);
    if (ImGui::InputText("###value", &value_, ImGuiInputTextFlags_NoHorizontalScroll)) {
        raiseModified(ChangeImpact::NodeConfig);
        postValue();
    }
    ImGui::SameLine();
    ne::BeginPin(outPinId_, ne::PinKind::Output);
    NodeEditorEx::DrawPinIcon(true);
    ne::EndPin();
    ImGui::PopID();
    ne::EndNode();
}

void Value::onSerialize(nlohmann::json &j) const
{
    if (!value_.empty())
        j["value"] = value_;
}

void Value::Core::onFrame(const RunContext &context)
{
    acceptLatestValue();

    if (auto buf = context.getOutput(outPinId_))
        buf->setValue(value_);
}

} // namespace Mirael::NodeTypes
