#include "pch.h"

#include "app.h"
#include "library.h"

namespace Mirael
{

void Library::showExplorer()
{
    if (ImGui::Begin(explorerWindowName())) {
        const ImGuiTreeNodeFlags leafFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        for (const auto &[name, _] : App::get().nodeTypes().all()) {
            ImGui::TreeNodeEx(name, leafFlags);
        }
    }
    ImGui::End();
}

}; // namespace Mirael