#include "pch.h"

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
        }
        if (ImGui::CollapsingHeader("ImGui Style", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ShowStyleEditor();
        }
    }
    ImGui::End();
}

}; // namespace Mirael