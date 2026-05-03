#include "pch.h"

#include "ine/utilities/style_editor.h"

#include "app.h"
#include "settings.h"

namespace Mirael
{

void Mirael::Settings::show(bool &open)
{
    auto &style = ImGui::GetStyle();

    ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg |
                                    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable;
    ImGui::SetNextWindowDockID(App::get().getDockspaceId(), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(windowName(), &open)) {
        if (ImGui::CollapsingHeader("Mirael Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Separator();
        }

        if (ImGui::CollapsingHeader("Node Editor Style", ImGuiTreeNodeFlags_DefaultOpen)) {
            ax::NodeEditor::Utilities::ShowStyleEditor();
            ImGui::Separator();
        }

        if (ImGui::CollapsingHeader("ImGui Style", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ShowStyleEditor();
        }
    }
    ImGui::End();
}

}; // namespace Mirael