#include "pch.h"

#include "imgui_node_editor.h"
#include "misc/cpp/imgui_stdlib.h"

#include "comment.h"

namespace ne = ax::NodeEditor;

namespace Mirael::NodeTypes
{
void Comment::onDeserialize(const nlohmann::json &j)
{
    if (!j.empty())
        comment = j["comment"];
}

void Comment::onInit()
{
    label = std::format("###comment{}", getId());
}

void Comment::onShow()
{
    ne::BeginNode(getId());
    ImGui::Text("Comment:");
    ImGui::SameLine();
    ImGui::InputText(label.c_str(), &comment);
    ne::EndNode();
}

void Comment::onSerialize(nlohmann::json &j) const
{
    if (!comment.empty())
        j["comment"] = comment;
}

}; // namespace Mirael::NodeTypes
