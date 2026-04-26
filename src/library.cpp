#include "pch.h"

#include "app.h"
#include "library.h"

namespace Mirael
{

void Library::showExplorer()
{
    if (ImGui::Begin(explorerWindowName())) {
        const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        for (const char *name : App::get().nodeTypes().names()) {
            ImGui::TreeNodeEx(name, leafFlags);
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                Project::get().createNodeInLastFocusedGraphIfVisible(name);
        }
    }
    ImGui::End();
}

}; // namespace Mirael