#include "pch.h"

#include "ine/imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "Comment.h"
#include "data.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{

void Comment::onDeserialize(const nlohmann::json &j)
{
    if (!j.empty())
        comment_ = j["comment"].get<std::string>();
}

void Comment::onInit() {}

void Comment::onShow()
{
    ne::BeginNode(getId());
    ImGui::PushID(getIdAsPointer());
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Comment:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(comment_.c_str()).x + 2 * ImGui::CalcTextSize("0").x);
    if (ImGui::InputText("###comment", &comment_, ImGuiInputTextFlags_NoHorizontalScroll))
        raiseModified(ChangeImpact::NodeConfig);
    ImGui::PopID();
    ne::EndNode();
}

void Comment::onSerialize(nlohmann::json &j) const
{
    if (!comment_.empty())
        j["comment"] = comment_;
}

} // namespace Mirael::NodeTypes
