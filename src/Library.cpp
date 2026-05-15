#include "pch.h"

#include "App.h"
#include "Library.h"

namespace Mirael
{

void Library::show(bool &open)
{
    if (ImGui::Begin(windowName(), &open)) {
        if (ImGui::TreeNodeEx("built-in", ImGuiTreeNodeFlags_DefaultOpen)) {
            const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            for (const char *name : App::get().nodeTypes().names()) {
                ImGui::TreeNodeEx(name, leafFlags);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    Project::get().createNodeInLastFocusedGraphIfVisible(name);
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

}; // namespace Mirael