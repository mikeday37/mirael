#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "comment.h"
#include "data.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Comment::onDeserialize(const nlohmann::json &j)
{
    if (!j.empty())
        comment = j["comment"].get<std::string>();
}

void Comment::onInit() {}

void Comment::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Comment:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(50.0f * ImGui::CalcTextSize("W").x);
    if (ImGui::InputText("###comment", &comment))
        raiseModified(ChangeImpact::NodeConfig);
    ImGui::PopID();
    ne::EndNode();
}

void Comment::onSerialize(nlohmann::json &j) const
{
    if (!comment.empty())
        j["comment"] = comment;
}

}; // namespace Mirael::NodeTypes
